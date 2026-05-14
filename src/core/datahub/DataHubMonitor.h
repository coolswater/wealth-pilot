/**
 * @file DataHubMonitor.h
 * @brief DataHub 性能监控器
 *
 * @details 功能：
 * - 订阅数量统计
 * - 发布频率监控
 * - 内存使用监控
 * - 回调延迟统计
 * - 性能报告生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAHUBMONITOR_H
#define DATAHUBMONITOR_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QHash>
#include <QMutex>
#include <atomic>
#include <memory>

/**
 * @brief 性能统计数据结构
 */
struct PerformanceStats {
    qint64 totalSubscriptions = 0;      ///< 总订阅数
    qint64 activeSubscriptions = 0;     ///< 活跃订阅数
    qint64 totalPublishes = 0;          ///< 总发布数
    qint64 publishesPerSecond = 0;      ///< 每秒发布数
    qint64 avgCallbackLatencyUs = 0;    ///< 平均回调延迟（微秒）
    qint64 maxCallbackLatencyUs = 0;    ///< 最大回调延迟（微秒）
    qint64 cacheHits = 0;                ///< 缓存命中数
    qint64 cacheMisses = 0;              ///< 缓存未命中数
    double memoryUsageMB = 0.0;         ///< 内存使用（MB）
    qint64 timestamp = 0;               ///< 时间戳
};

/**
 * @brief DataHub 性能监控器
 *
 * @details 使用方式：
 * @code
 * // 启动监控
 * DataHubMonitor::instance().start(1000); // 每秒更新一次
 *
 * // 获取统计
 * auto stats = DataHubMonitor::instance().getStats();
 * qDebug() << "Subscriptions:" << stats.activeSubscriptions;
 *
 * // 停止监控
 * DataHubMonitor::instance().stop();
 * @endcode
 */
class DataHubMonitor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static DataHubMonitor& instance();

    /**
     * @brief 启动监控
     * @param updateIntervalMs 更新间隔（毫秒）
     */
    void start(int updateIntervalMs = 1000);

    /**
     * @brief 停止监控
     */
    void stop();

    /**
     * @brief 获取统计数据
     */
    PerformanceStats getStats() const;

    /**
     * @brief 重置统计
     */
    void reset();

    /**
     * @brief 记录订阅
     */
    void recordSubscription();

    /**
     * @brief 记录取消订阅
     */
    void recordUnsubscription();

    /**
     * @brief 记录发布
     * @param latencyUs 发布延迟（微秒）
     */
    void recordPublish(qint64 latencyUs = 0);

    /**
     * @brief 记录缓存命中
     */
    void recordCacheHit();

    /**
     * @brief 记录缓存未命中
     */
    void recordCacheMiss();

signals:
    /**
     * @brief 统计更新信号
     */
    void statsUpdated(const PerformanceStats& stats);

private slots:
    void onTimerTick();

private:
    DataHubMonitor(QObject* parent = nullptr);
    ~DataHubMonitor() override;
    Q_DISABLE_COPY(DataHubMonitor)

    QTimer* m_timer = nullptr;
    QElapsedTimer m_elapsedTimer;

    std::atomic<qint64> m_totalSubscriptions{0};
    std::atomic<qint64> m_activeSubscriptions{0};
    std::atomic<qint64> m_totalPublishes{0};
    std::atomic<qint64> m_publishesInInterval{0};
    std::atomic<qint64> m_callbackLatencySum{0};
    std::atomic<qint64> m_callbackLatencyCount{0};
    std::atomic<qint64> m_maxCallbackLatency{0};
    std::atomic<qint64> m_cacheHits{0};
    std::atomic<qint64> m_cacheMisses{0};

    mutable QMutex m_mutex;
};

#endif // DATAHUBMONITOR_H