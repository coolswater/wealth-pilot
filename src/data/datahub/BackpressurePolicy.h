/**
 * @file BackpressurePolicy.h
 * @brief DataHub 背压策略 - 处理高频数据流
 *
 * @details 功能：
 * - 消息队列缓冲：缓冲高频消息，避免丢失
 * - 丢弃策略：队列满时丢弃旧数据或低优先级数据
 * - 采样策略：高频数据按时间/数量采样
 * - 流量控制：动态调整生产者速率
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef BACKPRESSUREPOLICY_H
#define BACKPRESSUREPOLICY_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QDateTime>
#include <functional>

namespace WealthPilot {
namespace DataHub {

/**
 * @brief 背压策略类型
 */
enum class BackpressureType {
    None,           ///< 无背压（直通模式）
    Buffer,         ///< 缓冲模式（队列缓冲）
    DropOldest,     ///< 丢弃最旧数据
    DropNewest,     ///< 丢弃最新数据
    Sample,         ///< 采样模式（按时间/数量采样）
    Throttle        ///< 限流模式（控制生产速率）
};

/**
 * @brief 背压配置
 */
struct BackpressureConfig {
    BackpressureType type = BackpressureType::Buffer;
    int maxQueueSize = 100;             ///< 最大队列大小
    int sampleIntervalMs = 100;         ///< 采样间隔（毫秒）
    int throttleRatePerSecond = 10;     ///< 限流速率（每秒）
    bool enableBurstProtection = true;  ///< 突发保护
    int burstThreshold = 50;            ///< 突发阈值
};

/**
 * @brief 消息项
 */
struct MessageItem {
    QString topic;
    QVariant value;
    qint64 timestamp;       ///< 接收时间戳
    int priority = 0;       ///< 优先级（越高越重要）
};

/**
 * @brief 背压控制器
 * 
 * 处理高频数据流的背压控制，防止消费者被淹没
 */
class BackpressureController : public QObject {
    Q_OBJECT

public:
    explicit BackpressureController(const BackpressureConfig& config, QObject* parent = nullptr);
    ~BackpressureController() override;

    /**
     * @brief 处理消息
     * @return true 表示消息被接受，false 表示被丢弃
     */
    bool processMessage(const QString& topic, const QVariant& value, int priority = 0);
    
    /**
     * @brief 设置配置
     */
    void setConfig(const BackpressureConfig& config);
    
    /**
     * @brief 获取配置
     */
    BackpressureConfig config() const;
    
    /**
     * @brief 获取队列大小
     */
    int queueSize() const;
    
    /**
     * @brief 清空队列
     */
    void clear();
    
    /**
     * @brief 获取统计信息
     */
    struct Statistics {
        qint64 totalReceived = 0;       ///< 总接收数
        qint64 totalDropped = 0;        ///< 总丢弃数
        qint64 totalDelivered = 0;      ///< 总投递数
        int currentQueueSize = 0;       ///< 当前队列大小
        int maxQueueSizeReached = 0;    ///< 最大队列大小
        double dropRate = 0.0;          ///< 丢弃率
    };
    Statistics getStatistics() const;

signals:
    /**
     * @brief 消息就绪信号
     */
    void messageReady(const QString& topic, const QVariant& value);
    
    /**
     * @brief 消息丢弃信号
     */
    void messageDropped(const QString& topic);
    
    /**
     * @brief 队列溢出警告
     */
    void queueOverflow(const QString& topic, int queueSize);

private:
    /**
     * @brief 缓冲模式处理
     */
    bool handleBufferMode(const MessageItem& item);
    
    /**
     * @brief 丢弃模式处理
     */
    bool handleDropMode(const MessageItem& item);
    
    /**
     * @brief 采样模式处理
     */
    bool handleSampleMode(const QString& topic);
    
    /**
     * @brief 限流模式处理
     */
    bool handleThrottleMode(const QString& topic);
    
    /**
     * @brief 投递消息
     */
    void deliverMessage(const MessageItem& item);

private:
    BackpressureConfig m_config;
    QQueue<MessageItem> m_queue;
    mutable QMutex m_mutex;
    
    // 采样相关
    QHash<QString, qint64> m_lastSampleTime;     ///< 每个topic上次采样时间
    QHash<QString, int> m_sampleCounter;          ///< 采样计数器
    
    // 限流相关
    QHash<QString, qint64> m_lastDeliverTime;     ///< 上次投递时间
    QHash<QString, int> m_deliverCount;           ///< 投递计数
    
    // 统计
    Statistics m_stats;
    
    // 突发检测
    QHash<QString, qint64> m_burstStartTime;      ///< 突发开始时间
    QHash<QString, int> m_burstCount;             ///< 突发计数
};

/**
 * @brief DataHub 的背压策略管理器
 * 
 * 为不同 topic 配置不同的背压策略
 */
class BackpressureManager : public QObject {
    Q_OBJECT

public:
    static BackpressureManager& instance();
    
    /**
     * @brief 设置 topic 的背压策略
     */
    void setPolicy(const QString& topic, const BackpressureConfig& config);
    
    /**
     * @brief 设置模式背压策略（支持通配符）
     */
    void setPatternPolicy(const QString& pattern, const BackpressureConfig& config);
    
    /**
     * @brief 处理消息
     */
    bool processMessage(const QString& topic, const QVariant& value, int priority = 0);
    
    /**
     * @brief 获取 topic 的背压控制器
     */
    BackpressureController* getController(const QString& topic);

private:
    BackpressureManager();
    ~BackpressureManager() override;
    
    QHash<QString, std::unique_ptr<BackpressureController>> m_controllers;
    QHash<QString, BackpressureConfig> m_patternPolicies;
    mutable QMutex m_mutex;
    
    // 默认配置
    BackpressureConfig m_defaultConfig;
};

} // namespace DataHub
} // namespace WealthPilot

#endif // BACKPRESSUREPOLICY_H
