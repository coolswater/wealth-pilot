/**
 * @file PerformanceMonitor.cpp
 * @brief 性能监控器实现
 */

#include "PerformanceMonitor.h"
#include <QDateTime>

// ============================================================================
// 构造函数
// ============================================================================

PerformanceMonitor::PerformanceMonitor()
    : QObject(nullptr)
{
    m_frameTimer.start();

    // 设置默认阈值
    m_thresholds["frameTime"] = 16.67;  // 60 FPS
    m_thresholds["methodTime"] = 100.0;  // 100ms
}

// ============================================================================
// 计时器管理
// ============================================================================

void PerformanceMonitor::startTimer(const QString& name)
{
    if (!m_enabled) return;

    m_timers[name].start();
}

qint64 PerformanceMonitor::endTimer(const QString& name)
{
    if (!m_enabled) return 0;

    if (!m_timers.contains(name)) {
        return -1;
    }

    qint64 elapsed = m_timers[name].elapsed();
    m_timers.remove(name);

    // 记录统计
    recordValue(name, static_cast<double>(elapsed));

    // 检查阈值
    if (m_thresholds.contains(name)) {
        double threshold = m_thresholds[name];
        if (elapsed > threshold) {
            emit performanceWarning(name, static_cast<double>(elapsed), threshold);
        }
    }

    return elapsed;
}

qint64 PerformanceMonitor::elapsed(const QString& name) const
{
    if (!m_timers.contains(name)) {
        return -1;
    }
    return m_timers[name].elapsed();
}

// ============================================================================
// 统计数据
// ============================================================================

void PerformanceMonitor::recordValue(const QString& name, double value)
{
    if (!m_enabled) return;

    Stats& stats = m_stats[name];
    stats.sum += value;
    stats.max = qMax(stats.max, value);
    stats.min = qMin(stats.min, value);
    stats.count++;
}

double PerformanceMonitor::average(const QString& name) const
{
    if (!m_stats.contains(name)) return 0;
    const Stats& stats = m_stats[name];
    return stats.count > 0 ? stats.sum / stats.count : 0;
}

double PerformanceMonitor::maximum(const QString& name) const
{
    if (!m_stats.contains(name)) return 0;
    return m_stats[name].max;
}

double PerformanceMonitor::minimum(const QString& name) const
{
    if (!m_stats.contains(name)) return 0;
    return m_stats[name].min;
}

int PerformanceMonitor::count(const QString& name) const
{
    if (!m_stats.contains(name)) return 0;
    return m_stats[name].count;
}

// ============================================================================
// 帧率监控
// ============================================================================

void PerformanceMonitor::recordFrame()
{
    if (!m_enabled) return;

    m_frameCount++;

    // 每秒更新一次帧率
    qint64 elapsed = m_frameTimer.elapsed();
    if (elapsed >= 1000) {
        m_fps = m_frameCount * 1000.0 / elapsed;
        m_frameCount = 0;
        m_frameTimer.restart();
    }
}

double PerformanceMonitor::fps() const
{
    return m_fps;
}

// ============================================================================
// 报告生成
// ============================================================================

QString PerformanceMonitor::report() const
{
    QString result;
    result += "=== Performance Report ===\n";
    result += QString("FPS: %1\n\n").arg(m_fps, 0, 'f', 1);

    result += "Method Statistics:\n";
    for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
        const Stats& stats = it.value();
        if (stats.count > 0) {
            result += QString("  %1:\n").arg(it.key());
            result += QString("    Count: %1\n").arg(stats.count);
            result += QString("    Avg: %1 ms\n").arg(stats.sum / stats.count, 0, 'f', 2);
            result += QString("    Max: %1 ms\n").arg(stats.max, 0, 'f', 2);
            result += QString("    Min: %1 ms\n").arg(stats.min, 0, 'f', 2);
        }
    }

    return result;
}

void PerformanceMonitor::clear()
{
    m_timers.clear();
    m_stats.clear();
    m_frameCount = 0;
    m_fps = 0;
    m_frameTimer.restart();
}
