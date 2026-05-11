/**
 * @file PerformanceManager.cpp
 * @brief 性能管理器实现
 */

#include "PerformanceManager.h"
#include <QThread>
#include <QCoreApplication>
#include <algorithm>

#include "utils/Logger.h"

namespace WealthPilot {

PerformanceManager* PerformanceManager::instance()
{
    static PerformanceManager* inst = new PerformanceManager();
    return inst;
}

PerformanceManager::PerformanceManager(QObject* parent)
    : QObject(parent)
{
    // 启动性能监控定时器（每30秒检查一次）
    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &PerformanceManager::checkPerformance);
    m_monitorTimer->start(30000);
}

PerformanceManager::~PerformanceManager()
{
    // 清理内存池
    for (void* block : m_memoryPool) {
        ::operator delete(block);
    }
    m_memoryPool.clear();
}

// ========== 性能监控 ==========

void PerformanceManager::beginTimer(const QString& name)
{
    QMutexLocker locker(&m_metricsMutex);
    m_timers[name].start();
}

qint64 PerformanceManager::endTimer(const QString& name)
{
    QMutexLocker locker(&m_metricsMutex);

    auto it = m_timers.find(name);
    if (it == m_timers.end()) {
        return -1;
    }

    qint64 elapsed = it.value().elapsed();
    m_timers.remove(name);

    // 更新统计
    m_metrics[name].update(elapsed);

    // 检查是否需要警告
    if (elapsed > 1000) { // 超过1秒
        emit performanceWarning(QString("Operation '%1' took %2ms").arg(name).arg(elapsed));
    }

    return elapsed;
}

PerformanceMetric PerformanceManager::getMetric(const QString& name) const
{
    QMutexLocker locker(&m_metricsMutex);
    return m_metrics.value(name);
}

QHash<QString, PerformanceMetric> PerformanceManager::getAllMetrics() const
{
    QMutexLocker locker(&m_metricsMutex);
    return m_metrics;
}

void PerformanceManager::resetMetrics()
{
    QMutexLocker locker(&m_metricsMutex);
    m_metrics.clear();
    m_timers.clear();
}

QString PerformanceManager::generateReport() const
{
    QMutexLocker locker(&m_metricsMutex);

    QString report;
    report += QStringLiteral("========== 性能分析报告 ==========\n\n");

    if (m_metrics.isEmpty()) {
        report += QStringLiteral("暂无性能数据\n");
        return report;
    }

    // 按总耗时排序
    QVector<QPair<QString, PerformanceMetric>> sortedMetrics;
    for (auto it = m_metrics.begin(); it != m_metrics.end(); ++it) {
        sortedMetrics.append({it.key(), it.value()});
    }
    std::sort(sortedMetrics.begin(), sortedMetrics.end(),
              [](const auto& a, const auto& b) {
                  return a.second.totalTime > b.second.totalTime;
              });

    report += QStringLiteral("指标名称                调用次数    总耗时(ms)   平均耗时(ms)   最大耗时(ms)\n");
    report += QStringLiteral("------------------------------------------------------------------------\n");

    for (const auto& item : sortedMetrics) {
        const QString& name = item.first;
        const PerformanceMetric& m = item.second;

        report += QString("%1  %2    %3      %4       %5\n")
            .arg(name.left(20).leftJustified(20))
            .arg(m.callCount, 8)
            .arg(m.totalTime, 10)
            .arg(m.avgTime, 10, 'f', 2)
            .arg(m.maxTime, 10);
    }

    report += QStringLiteral("\n========== 内存统计 ==========\n");
    auto memStats = getMemoryStats();
    report += QString("当前使用: %1 KB\n").arg(memStats.currentUsage / 1024);
    report += QString("峰值使用: %1 KB\n").arg(memStats.peakUsage / 1024);
    report += QString("池大小: %1 块\n").arg(memStats.poolSize);

    return report;
}

// ========== 内存优化 ==========

void PerformanceManager::configureMemoryPool(const MemoryPoolConfig& config)
{
    QMutexLocker locker(&m_poolMutex);
    m_poolConfig = config;

    // 预分配内存块
    m_memoryPool.reserve(config.maxBlocks);
    for (size_t i = m_memoryPool.size(); i < config.maxBlocks / 2; ++i) {
        void* block = ::operator new(config.blockSize);
        m_memoryPool.append(block);
    }

    LOG_INFO(QString("Memory pool configured: blockSize=%1, maxBlocks=%2")
        .arg(config.blockSize).arg(config.maxBlocks));
}

void* PerformanceManager::allocateBlock()
{
    QMutexLocker locker(&m_poolMutex);

    void* block = nullptr;

    if (!m_memoryPool.isEmpty() && m_poolConfig.enableReuse) {
        // 从池中获取
        block = m_memoryPool.takeLast();
    } else {
        // 新分配
        block = ::operator new(m_poolConfig.blockSize);
    }

    // 更新统计
    m_memoryStats.totalAllocated += m_poolConfig.blockSize;
    m_memoryStats.currentUsage += m_poolConfig.blockSize;
    m_memoryStats.peakUsage = qMax(m_memoryStats.peakUsage, m_memoryStats.currentUsage);

    return block;
}

void PerformanceManager::deallocateBlock(void* block)
{
    if (!block) return;

    QMutexLocker locker(&m_poolMutex);

    if (m_memoryPool.size() < static_cast<int>(m_poolConfig.maxBlocks) && m_poolConfig.enableReuse) {
        // 放回池中
        m_memoryPool.append(block);
    } else {
        // 释放
        ::operator delete(block);
    }

    // 更新统计
    m_memoryStats.totalFreed += m_poolConfig.blockSize;
    m_memoryStats.currentUsage -= m_poolConfig.blockSize;
}

PerformanceManager::MemoryStats PerformanceManager::getMemoryStats() const
{
    QMutexLocker locker(&m_poolMutex);
    MemoryStats stats = m_memoryStats;
    stats.poolSize = m_memoryPool.size();
    return stats;
}

// ========== 延迟加载 ==========

void PerformanceManager::registerLazyLoader(const QString& id,
                                            std::function<void()> loader,
                                            int priority)
{
    LazyLoader item;
    item.id = id;
    item.loader = std::move(loader);
    item.priority = priority;
    m_lazyLoaders.append(item);
}

void PerformanceManager::executeLazyLoaders()
{
    // 按优先级排序
    std::sort(m_lazyLoaders.begin(), m_lazyLoaders.end(),
              [](const LazyLoader& a, const LazyLoader& b) {
                  return a.priority > b.priority;
              });

    LOG_INFO(QString("Executing %1 lazy loaders...").arg(m_lazyLoaders.size()));

    for (const auto& loader : m_lazyLoaders) {
        PERF_TIMER(lazy_loader);
        loader.loader();
        LOG_DEBUG(QString("Lazy loader executed: %1").arg(loader.id));
    }

    m_lazyLoaders.clear();
}

// ========== 性能分析 ==========

QStringList PerformanceManager::analyzeBottlenecks() const
{
    QStringList suggestions;

    QMutexLocker locker(&m_metricsMutex);

    for (auto it = m_metrics.begin(); it != m_metrics.end(); ++it) {
        const PerformanceMetric& m = it.value();

        // 平均耗时过长
        if (m.avgTime > 100) {
            suggestions.append(QString("'%1' 平均耗时 %2ms，建议优化")
                .arg(it.key()).arg(m.avgTime, 0, 'f', 2));
        }

        // 最大耗时异常
        if (m.maxTime > m.avgTime * 10 && m.callCount > 10) {
            suggestions.append(QString("'%1' 存在性能波动，最大耗时 %2ms")
                .arg(it.key()).arg(m.maxTime));
        }

        // 调用频率过高
        if (m.callCount > 1000 && m.avgTime > 10) {
            suggestions.append(QString("'%1' 调用频繁(%2次)，考虑缓存结果")
                .arg(it.key()).arg(m.callCount));
        }
    }

    // 内存建议
    auto memStats = getMemoryStats();
    if (memStats.currentUsage > 500 * 1024 * 1024) { // 500MB
        suggestions.append(QString("内存使用较高(%1MB)，建议检查内存泄漏")
            .arg(memStats.currentUsage / 1024 / 1024));
    }

    return suggestions;
}

void PerformanceManager::checkPerformance()
{
    auto bottlenecks = analyzeBottlenecks();
    for (const QString& suggestion : bottlenecks) {
        LOG_WARNING(QString("Performance: %1").arg(suggestion));
        emit performanceWarning(suggestion);
    }
}

void PerformanceManager::checkMemory()
{
    auto stats = getMemoryStats();
    if (stats.currentUsage > 1024 * 1024 * 1024) { // 1GB
        emit memoryWarning(stats.currentUsage);
    }
}

} // namespace WealthPilot
