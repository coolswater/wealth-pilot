/**
 * @file DataHubMonitor.cpp
 * @brief DataHub 性能监控器实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "DataHubMonitor.h"
#include <QCoreApplication>
#include <QDebug>

DataHubMonitor& DataHubMonitor::instance()
{
    static DataHubMonitor instance;
    return instance;
}

DataHubMonitor::DataHubMonitor(QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    connect(m_timer, &QTimer::timeout, this, &DataHubMonitor::onTimerTick);
    m_elapsedTimer.start();
}

DataHubMonitor::~DataHubMonitor()
{
    stop();
}

void DataHubMonitor::start(int updateIntervalMs)
{
    if (!m_timer->isActive()) {
        m_timer->start(updateIntervalMs);
        qDebug() << "[DataHubMonitor] Started with interval" << updateIntervalMs << "ms";
    }
}

void DataHubMonitor::stop()
{
    if (m_timer->isActive()) {
        m_timer->stop();
        qDebug() << "[DataHubMonitor] Stopped";
    }
}

PerformanceStats DataHubMonitor::getStats() const
{
    QMutexLocker locker(&m_mutex);

    PerformanceStats stats;
    stats.totalSubscriptions = m_totalSubscriptions.load();
    stats.activeSubscriptions = m_activeSubscriptions.load();
    stats.totalPublishes = m_totalPublishes.load();
    stats.cacheHits = m_cacheHits.load();
    stats.cacheMisses = m_cacheMisses.load();
    stats.timestamp = m_elapsedTimer.elapsed();

    // 计算平均延迟
    qint64 latencyCount = m_callbackLatencyCount.load();
    if (latencyCount > 0) {
        stats.avgCallbackLatencyUs = m_callbackLatencySum.load() / latencyCount;
    }
    stats.maxCallbackLatencyUs = m_maxCallbackLatency.load();

    // 计算每秒发布数（需要根据间隔计算）
    // 这里简化处理，实际应该根据时间间隔计算
    stats.publishesPerSecond = m_publishesInInterval.load();

    return stats;
}

void DataHubMonitor::reset()
{
    QMutexLocker locker(&m_mutex);

    m_totalSubscriptions.store(0);
    m_activeSubscriptions.store(0);
    m_totalPublishes.store(0);
    m_publishesInInterval.store(0);
    m_callbackLatencySum.store(0);
    m_callbackLatencyCount.store(0);
    m_maxCallbackLatency.store(0);
    m_cacheHits.store(0);
    m_cacheMisses.store(0);

    qDebug() << "[DataHubMonitor] Stats reset";
}

void DataHubMonitor::recordSubscription()
{
    m_totalSubscriptions++;
    m_activeSubscriptions++;
}

void DataHubMonitor::recordUnsubscription()
{
    if (m_activeSubscriptions > 0) {
        m_activeSubscriptions--;
    }
}

void DataHubMonitor::recordPublish(qint64 latencyUs)
{
    m_totalPublishes++;
    m_publishesInInterval++;

    if (latencyUs > 0) {
        m_callbackLatencySum += latencyUs;
        m_callbackLatencyCount++;

        // 更新最大延迟
        qint64 currentMax = m_maxCallbackLatency.load();
        while (latencyUs > currentMax) {
            if (m_maxCallbackLatency.compare_exchange_weak(currentMax, latencyUs)) {
                break;
            }
        }
    }
}

void DataHubMonitor::recordCacheHit()
{
    m_cacheHits++;
}

void DataHubMonitor::recordCacheMiss()
{
    m_cacheMisses++;
}

void DataHubMonitor::onTimerTick()
{
    // 计算每秒发布数
    qint64 publishesInInterval = m_publishesInInterval.exchange(0);
    // 假设间隔是1秒，直接使用
    // 如果间隔不同，需要除以实际秒数

    // 发送统计更新信号
    emit statsUpdated(getStats());

    // 输出日志（可选）
    auto stats = getStats();
    qDebug() << "[DataHubMonitor]"
             << "Subscriptions:" << stats.activeSubscriptions
             << "Publishes/s:" << publishesInInterval
             << "AvgLatency:" << stats.avgCallbackLatencyUs << "us"
             << "CacheHitRate:" << (stats.cacheHits + stats.cacheMisses > 0
                                    ? (double)stats.cacheHits / (stats.cacheHits + stats.cacheMisses) * 100
                                    : 0) << "%";
}