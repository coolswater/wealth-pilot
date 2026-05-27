/**
 * @file DataHub.h
 * @brief 数据中心 - 统一数据分发和调度
 *
 * @details DataHub 是 WealthPilot 的核心数据中枢，负责：
 * - 发布/订阅模式的数据分发
 * - 数据生命周期管理（TTL、缓存）
 * - 统一的定时刷新调度
 * - 数据源注册和发现
 *
 * @details 架构设计：
 * - 单例模式，全局唯一实例
 * - 生产者-消费者模型
 * - 支持通配符订阅（如 "market:quote:*"）
 * - 自动去重和合并高频更新
 *
 * @details 使用示例：
 * @code
 * // 订阅数据
 * DataHub::instance()->subscribe("market:quote:sh600519", [](const QVariant& data) {
 *     StockQuote quote = data.value<StockQuote>();
 *     // 处理行情数据
 * });
 *
 * // 发布数据
 * DataHub::instance()->publish("market:quote:sh600519", QVariant::fromValue(quote));
 *
 * // 注册数据生产者
 * stockDataSource->registerToDataHub(DataHub::instance(), 5000);
 * @endcode
 *
 * @details 与传统 QTimer 方式对比：
 * - 传统：每个数据源独立定时器，资源浪费，刷新时间分散
 * - DataHub：统一调度，合并请求，智能缓存，性能更优
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef WEALTHPILOT_DATAHUB_H
#define WEALTHPILOT_DATAHUB_H

#include <QObject>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QDateTime>
#include <functional>
#include <QMetaObject>

namespace WealthPilot {
namespace DataHub {

/**
 * @brief Topic刷新策略配置
 */
struct TopicPolicy {
    int ttlMs = 30000;           // 缓存有效期（毫秒）
    int minIntervalMs = 5000;    // 最小刷新间隔
    bool pushOnly = false;       // true: WebSocket推送模式，不主动刷新
    int priority = 0;            // 优先级，数值越高越先刷新
    int coalesceWithinMs = 0;    // 合并发布窗口（用于渐进式发布）
};

/**
 * @brief Topic统计信息（调试用）
 */
struct TopicStats {
    QString topic;
    int subscriberCount = 0;
    qint64 lastPublishMs = 0;
    qint64 lastRefreshRequestMs = 0;
    int totalPublishes = 0;
    bool inFlight = false;
};

/**
 * @brief 数据生产者接口
 * 
 * 所有数据服务需要实现此接口，以便DataHub统一调度
 */
class IDataProducer : public QObject {
    Q_OBJECT
public:
    explicit IDataProducer(QObject* parent = nullptr) : QObject(parent) {}
    
    /**
     * @brief 返回此Producer负责的topic模式列表
     * @return 例如 {"market:quote:*", "market:history:*"}
     */
    virtual QStringList topicPatterns() const = 0;
    
    /**
     * @brief 刷新指定的topics数据
     * @param topics 需要刷新的topic列表
     * Producer异步获取数据后调用DataHub::publish()
     */
    virtual void refresh(const QStringList& topics) = 0;
    
    /**
     * @brief 最大请求速率限制（每秒）
     * @return 0表示无限制，用于API速率控制
     */
    virtual int maxRequestsPerSecond() const { return 0; }
    
    /**
     * @brief 当topic无订阅者时调用
     * @param topic 空闲的topic
     * 用于释放资源（关闭WebSocket等）
     */
    virtual void onTopicIdle(const QString& topic) { Q_UNUSED(topic); }
    
    /**
     * @brief 当topic首次有订阅者时调用
     * @param topic 激活的topic
     */
    virtual void onTopicActive(const QString& topic) { Q_UNUSED(topic); }
};

/**
 * @brief 数据中心 - 统一的发布/订阅数据层
 * 
 * 核心功能:
 * 1. Topic-based订阅: 所有数据通过topic订阅
 * 2. 自动去重: 同一topic只请求一次
 * 3. 生命周期绑定: QObject销毁时自动取消订阅
 * 4. 统一刷新策略: 消除每个Widget独立QTimer的问题
 * 
 * Topic格式: domain:subdomain:id[:modifier]
 * 示例:
 *   market:quote:AAPL        - 股票行情
 *   market:history:AAPL:1y   - 历史数据
 *   news:symbol:NVDA         - 新闻
 *   econ:fred:GDP            - 经济数据
 *   ws:kraken:BTC-USD        - WebSocket推送
 */
class DataHub : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief 获取单例实例
     */
    static DataHub& instance();
    
    /**
     * @brief 显式关闭（在 QApplication 退出前调用）
     * 
     * @details 必须在 QApplication::aboutToQuit 信号中调用此方法，
     *          而不是依赖析构函数。因为静态单例析构晚于 QApplication，
     *          此时 QTimer 已无法正常工作。
     */
    void shutdown();
    
    /**
     * @brief 订阅topic
     * @param owner 订阅者对象，销毁时自动取消订阅
     * @param topic 要订阅的topic
     * @param slot 数据回调函数
     * @return Qt连接，可用于手动断开
     * 
     * 订阅时:
     * - 如果缓存数据有效，立即回调
     * - 如果topic之前空闲，触发Producer激活
     */
    QMetaObject::Connection subscribe(
        QObject* owner,
        const QString& topic,
        std::function<void(const QVariant&)> slot);
    
    /**
     * @brief 泛型订阅（自动类型转换）
     * @tparam T 数据类型，需注册Q_DECLARE_METATYPE
     */
    template<typename T>
    QMetaObject::Connection subscribe(
        QObject* owner,
        const QString& topic,
        std::function<void(const T&)> slot);
    
    /**
     * @brief 模式订阅（支持通配符）
     * @param pattern 例如 "market:quote:*"
     * @param slot 接收(topic, value)的回调
     */
    QMetaObject::Connection subscribePattern(
        QObject* owner,
        const QString& pattern,
        std::function<void(const QString&, const QVariant&)> slot);
    
    /**
     * @brief 取消订阅
     * @param owner 取消该对象的所有订阅
     */
    void unsubscribe(QObject* owner);
    
    /**
     * @brief 取消特定topic订阅
     */
    void unsubscribe(QObject* owner, const QString& topic);
    
    /**
     * @brief 发布数据
     * @param topic 目标topic
     * @param value 数据值
     * 
     * 存入CacheManager并广播给所有订阅者
     */
    void publish(const QString& topic, const QVariant& value);
    
    /**
     * @brief 发布数据（自定义TTL）
     */
    void publish(const QString& topic, const QVariant& value, 
                 std::chrono::milliseconds ttl);
    
    /**
     * @brief 注册Producer
     */
    void registerProducer(IDataProducer* producer);
    
    /**
     * @brief 注销Producer
     */
    void unregisterProducer(IDataProducer* producer);
    
    /**
     * @brief 设置topic策略
     */
    void setPolicy(const QString& topic, const TopicPolicy& policy);
    
    /**
     * @brief 设置模式策略
     */
    void setPolicyPattern(const QString& pattern, const TopicPolicy& policy);
    
    /**
     * @brief 读取当前缓存值（不订阅）
     * @return 无效QVariant表示未知topic
     */
    QVariant peek(const QString& topic) const;
    
    /**
     * @brief 请求刷新
     * @param force true: 忽略minInterval限制
     */
    void request(const QString& topic, bool force = false);
    
    /**
     * @brief 批量请求刷新
     */
    void request(const QStringList& topics, bool force = false);
    
    /**
     * @brief 获取统计信息（调试用）
     */
    QVector<TopicStats> stats() const;
    
signals:
    /**
     * @brief Topic更新信号（低级别，用于调试）
     */
    void topicUpdated(const QString& topic, const QVariant& value);
    
    /**
     * @brief Topic变为空闲（无订阅者）
     */
    void topicIdle(const QString& topic);
    
    /**
     * @brief Topic变为活跃（有订阅者）
     */
    void topicActive(const QString& topic);
    
private:
    // 单例模式
    DataHub();
    ~DataHub();
    DataHub(const DataHub&) = delete;
    DataHub& operator=(const DataHub&) = delete;
    
    // 内部结构
    struct Subscription {
        QObject* owner;
        QString topic;
        std::function<void(const QVariant&)> slot;
        bool isPattern;
        QString pattern;
    };
    
    struct TopicState {
        QVariant cachedValue;
        qint64 lastPublishMs = 0;
        qint64 lastRefreshRequestMs = 0;
        int totalPublishes = 0;
        bool inFlight = false;
        TopicPolicy policy;
        QSet<QObject*> subscribers;
        IDataProducer* producer = nullptr;
    };
    
    // 数据成员
    QHash<QString, TopicState> m_topics;
    QHash<QObject*, QVector<Subscription>> m_ownerSubscriptions;
    QHash<QString, QSet<QObject*>> m_patternSubscriptions;
    QVector<IDataProducer*> m_producers;
    QTimer* m_schedulerTimer;
    
    // 内部方法
    void onOwnerDestroyed(QObject* owner);
    IDataProducer* findProducerForTopic(const QString& topic) const;
    bool matchesPattern(const QString& pattern, const QString& topic) const;
    bool isTopicExpired(const QString& topic) const;
    void scheduleRefresh();
    void processScheduledRefresh();
};

// ========== 模板实现 ==========

template<typename T>
QMetaObject::Connection DataHub::subscribe(
    QObject* owner,
    const QString& topic,
    std::function<void(const T&)> slot)
{
    static_assert(QMetaTypeId2<T>::Defined,
                  "T must be registered with Q_DECLARE_METATYPE");
    
    return subscribe(owner, topic, [slot](const QVariant& v) {
        if (v.canConvert<T>()) {
            slot(v.value<T>());
        }
    });
}

} // namespace DataHub
} // namespace WealthPilot

// ========== 注册常用类型 ==========

Q_DECLARE_METATYPE(WealthPilot::DataHub::TopicPolicy)
Q_DECLARE_METATYPE(WealthPilot::DataHub::TopicStats)

#endif // WEALTHPILOT_DATAHUB_H