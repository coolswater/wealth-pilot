/**
 * @file PerformanceMonitor.cpp
 * @brief 性能监控器实现
 */

#include "PerformanceMonitor.h"
#include "shared/utils/Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QMutexLocker>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace WealthPilot {

PerformanceMonitor& PerformanceMonitor::instance()
{
    static PerformanceMonitor instance;
    return instance;
}

PerformanceMonitor::PerformanceMonitor()
    : m_timer(new QTimer(this))
{
    m_uptimeTimer.start();

    connect(m_timer, &QTimer::timeout, this, [this]() {
        collectMetrics();
    });
}

PerformanceMonitor::~PerformanceMonitor()
{
    stop();
}

void PerformanceMonitor::start(int intervalMs)
{
    if (m_monitoring) return;

    m_monitoring = true;
    m_timer->start(intervalMs);
    LOG_INFO(QString("[PerformanceMonitor] Started with interval %1ms").arg(intervalMs));
}

void PerformanceMonitor::stop()
{
    if (!m_monitoring) return;

    m_monitoring = false;
    m_timer->stop();
    LOG_INFO("[PerformanceMonitor] Stopped");
}

bool PerformanceMonitor::isMonitoring() const
{
    return m_monitoring;
}

PerformanceMetrics PerformanceMonitor::currentMetrics() const
{
    QMutexLocker locker(&m_metricsMutex);
    return m_currentMetrics;
}

QVector<PerformanceMetrics> PerformanceMonitor::history(int count) const
{
    QMutexLocker locker(&m_metricsMutex);

    if (count <= 0 || count >= m_history.size()) {
        return m_history;
    }

    return m_history.mid(m_history.size() - count, count);
}

qint64 PerformanceMonitor::beginFunction(const QString& functionName)
{
    QMutexLocker locker(&m_functionMutex);

    qint64 timerId = m_nextTimerId++;
    QElapsedTimer timer;
    timer.start();

    m_activeTimers[timerId] = { functionName, timer };

    return timerId;
}

void PerformanceMonitor::endFunction(qint64 timerId)
{
    QMutexLocker locker(&m_functionMutex);

    auto it = m_activeTimers.find(timerId);
    if (it == m_activeTimers.end()) {
        return;
    }

    QString functionName = it.value().first;
    qint64 elapsedNs = it.value().second.nsecsElapsed();

    // 更新统计
    auto& stats = m_functionStats[functionName];
    stats.name = functionName;
    stats.update(elapsedNs);

    // 检查告警
    if (m_alertConfig.enabled && elapsedNs > m_alertConfig.functionTimeThreshold) {
        emit functionPerformanceAlert(functionName, elapsedNs);
        LOG_WARNING(QString("[PerformanceMonitor] Function %1 took %2ns (> threshold %3ns)")
                    .arg(functionName).arg(elapsedNs).arg(m_alertConfig.functionTimeThreshold));
    }

    m_activeTimers.erase(it);
}

FunctionStats PerformanceMonitor::getFunctionStats(const QString& functionName) const
{
    QMutexLocker locker(&m_functionMutex);
    return m_functionStats.value(functionName);
}

QVector<FunctionStats> PerformanceMonitor::getAllFunctionStats() const
{
    QMutexLocker locker(&m_functionMutex);
    return m_functionStats.values();
}

void PerformanceMonitor::clearFunctionStats()
{
    QMutexLocker locker(&m_functionMutex);
    m_functionStats.clear();
}

void PerformanceMonitor::setAlertConfig(const PerformanceAlertConfig& config)
{
    m_alertConfig = config;
}

PerformanceAlertConfig PerformanceMonitor::alertConfig() const
{
    return m_alertConfig;
}

QString PerformanceMonitor::generateReport() const
{
    QString report;

    report += QStringLiteral("=== Performance Report ===\n\n");

    // 当前指标
    auto metrics = currentMetrics();
    report += QStringLiteral("Current Metrics:\n");
    report += QString("  CPU Usage: %1%\n").arg(metrics.cpuUsage, 0, 'f', 1);
    report += QString("  Memory Usage: %1 MB (%2%)\n").arg(metrics.memoryUsage, 0, 'f', 1).arg(metrics.memoryUsagePercent, 0, 'f', 1);
    report += QString("  FPS: %1\n").arg(metrics.fps);
    report += QString("  Uptime: %1 seconds\n\n").arg(metrics.uptime / 1000);

    // 函数统计
    auto stats = getAllFunctionStats();
    if (!stats.isEmpty()) {
        report += QStringLiteral("Function Performance:\n");

        // 按总耗时排序
        std::sort(stats.begin(), stats.end(), [](const FunctionStats& a, const FunctionStats& b) {
            return a.totalTime > b.totalTime;
        });

        for (const auto& s : stats) {
            report += QString("  %1:\n").arg(s.name);
            report += QString("    Calls: %1\n").arg(s.totalCalls);
            report += QString("    Total: %1 ns\n").arg(s.totalTime);
            report += QString("    Avg: %1 ns\n").arg(s.avgTime);
            report += QString("    Min: %1 ns\n").arg(s.minTime);
            report += QString("    Max: %1 ns\n").arg(s.maxTime);
        }
    }

    return report;
}

QString PerformanceMonitor::exportToJson() const
{
    QJsonObject root;

    // 当前指标
    auto metrics = currentMetrics();
    QJsonObject metricsObj;
    metricsObj["cpuUsage"] = metrics.cpuUsage;
    metricsObj["memoryUsage"] = metrics.memoryUsage;
    metricsObj["memoryUsagePercent"] = metrics.memoryUsagePercent;
    metricsObj["fps"] = metrics.fps;
    metricsObj["uptime"] = metrics.uptime;
    metricsObj["timestamp"] = metrics.timestamp;
    root["metrics"] = metricsObj;

    // 函数统计
    auto stats = getAllFunctionStats();
    QJsonArray statsArray;
    for (const auto& s : stats) {
        QJsonObject statObj;
        statObj["name"] = s.name;
        statObj["totalCalls"] = s.totalCalls;
        statObj["totalTime"] = s.totalTime;
        statObj["avgTime"] = s.avgTime;
        statObj["minTime"] = s.minTime;
        statObj["maxTime"] = s.maxTime;
        statsArray.append(statObj);
    }
    root["functionStats"] = statsArray;

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

void PerformanceMonitor::collectMetrics()
{
    PerformanceMetrics metrics;
    metrics.timestamp = QDateTime::currentMSecsSinceEpoch();
    metrics.uptime = m_uptimeTimer.elapsed();

#ifdef Q_OS_WIN
    // CPU 使用率
    static qint64 lastTotalTime = 0;
    static qint64 lastIdleTime = 0;

    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        qint64 idle = (static_cast<qint64>(idleTime.dwHighDateTime) << 32) | idleTime.dwLowDateTime;
        qint64 kernel = (static_cast<qint64>(kernelTime.dwHighDateTime) << 32) | kernelTime.dwLowDateTime;
        qint64 user = (static_cast<qint64>(userTime.dwHighDateTime) << 32) | userTime.dwLowDateTime;

        qint64 total = kernel + user;
        qint64 totalDiff = total - lastTotalTime;
        qint64 idleDiff = idle - lastIdleTime;

        if (totalDiff > 0) {
            metrics.cpuUsage = 100.0 * (1.0 - static_cast<double>(idleDiff) / totalDiff);
        }

        lastTotalTime = total;
        lastIdleTime = idle;
    }

    // 内存使用
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        metrics.memoryUsagePercent = memStatus.dwMemoryLoad;
        metrics.memoryUsage = static_cast<double>(memStatus.ullTotalPhys - memStatus.ullAvailPhys) / (1024 * 1024);
    }

    // 进程内存
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        metrics.memoryUsage = static_cast<double>(pmc.WorkingSetSize) / (1024 * 1024);
    }
#else
    // 其他平台的实现
    metrics.cpuUsage = 0.0;
    metrics.memoryUsage = 0.0;
    metrics.memoryUsagePercent = 0.0;
#endif

    // FPS（简化实现）
    metrics.fps = 60; // 默认值，实际应从渲染循环获取

    // 更新当前指标
    {
        QMutexLocker locker(&m_metricsMutex);
        m_currentMetrics = metrics;
        m_history.append(metrics);

        // 限制历史记录数量
        if (m_history.size() > 3600) {
            m_history.removeFirst();
        }
    }

    // 检查告警
    checkAlerts(metrics);

    // 发送信号
    emit metricsUpdated(metrics);
}

void PerformanceMonitor::checkAlerts(const PerformanceMetrics& metrics)
{
    if (!m_alertConfig.enabled) return;

    if (metrics.cpuUsage > m_alertConfig.cpuThreshold) {
        emit cpuAlert(metrics.cpuUsage);
        LOG_WARNING(QString("[PerformanceMonitor] CPU alert: %1% > threshold %2%")
                    .arg(metrics.cpuUsage, 0, 'f', 1).arg(m_alertConfig.cpuThreshold));
    }

    if (metrics.memoryUsagePercent > m_alertConfig.memoryThreshold) {
        emit memoryAlert(metrics.memoryUsagePercent);
        LOG_WARNING(QString("[PerformanceMonitor] Memory alert: %1% > threshold %2%")
                    .arg(metrics.memoryUsagePercent, 0, 'f', 1).arg(m_alertConfig.memoryThreshold));
    }

    if (metrics.fps < m_alertConfig.fpsThreshold) {
        emit fpsAlert(metrics.fps);
        LOG_WARNING(QString("[PerformanceMonitor] FPS alert: %1 < threshold %2")
                    .arg(metrics.fps).arg(m_alertConfig.fpsThreshold));
    }
}

} // namespace WealthPilot