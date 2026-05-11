/**
 * @file PerformanceMonitor.h
 * @brief 性能监控器 - 监控应用性能指标
 *
 * @details 功能：
 * - 帧率监控
 * - 内存使用监控
 * - 方法执行时间测量
 * - 性能日志记录
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QDebug>

#include "core/base/Singleton.h"

/**
 * @brief 性能监控器
 *
 * @details 提供性能监控功能：
 * - 方法执行时间测量
 * - 帧率计算
 * - 内存使用估算
 * - 性能报告生成
 *
 * @example
 * @code
 * // 测量方法执行时间
 * PERF_START("loadData");
 * loadData();
 * PERF_END("loadData");
 *
 * // 获取性能报告
 * qDebug() << PerformanceMonitor::instance()->report();
 * @endcode
 */
class PerformanceMonitor : public QObject, public Singleton<PerformanceMonitor>
{
    Q_OBJECT
    friend class Singleton<PerformanceMonitor>;

public:
    // ========== 计时器管理 ==========

    /**
     * @brief 开始计时
     * @param name 计时器名称
     */
    void startTimer(const QString& name);

    /**
     * @brief 结束计时
     * @param name 计时器名称
     * @return 执行时间（毫秒）
     */
    qint64 endTimer(const QString& name);

    /**
     * @brief 获取计时器当前值
     * @param name 计时器名称
     * @return 已经过的时间（毫秒），如果计时器不存在返回-1
     */
    qint64 elapsed(const QString& name) const;

    // ========== 统计数据 ==========

    /**
     * @brief 记录统计值
     * @param name 统计名称
     * @param value 值
     */
    void recordValue(const QString& name, double value);

    /**
     * @brief 获取平均值
     * @param name 统计名称
     * @return 平均值
     */
    double average(const QString& name) const;

    /**
     * @brief 获取最大值
     * @param name 统计名称
     * @return 最大值
     */
    double maximum(const QString& name) const;

    /**
     * @brief 获取最小值
     * @param name 统计名称
     * @return 最小值
     */
    double minimum(const QString& name) const;

    /**
     * @brief 获取计数
     * @param name 统计名称
     * @return 记录次数
     */
    int count(const QString& name) const;

    // ========== 帧率监控 ==========

    /**
     * @brief 记录一帧
     */
    void recordFrame();

    /**
     * @brief 获取当前帧率
     * @return 帧率（FPS）
     */
    double fps() const;

    // ========== 报告生成 ==========

    /**
     * @brief 生成性能报告
     * @return 报告字符串
     */
    QString report() const;

    /**
     * @brief 清空所有数据
     */
    void clear();

    /**
     * @brief 启用/禁用监控
     * @param enabled 是否启用
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief 是否启用
     */
    bool isEnabled() const { return m_enabled; }

signals:
    /**
     * @brief 性能警告信号
     * @param name 名称
     * @param value 值
     * @param threshold 阈值
     */
    void performanceWarning(const QString& name, double value, double threshold);

private:
    PerformanceMonitor();
    ~PerformanceMonitor() = default;

    // 计时器
    QHash<QString, QElapsedTimer> m_timers;

    // 统计数据
    struct Stats {
        double sum = 0;
        double max = 0;
        double min = std::numeric_limits<double>::max();
        int count = 0;
    };
    QHash<QString, Stats> m_stats;

    // 帧率计算
    QElapsedTimer m_frameTimer;
    int m_frameCount = 0;
    double m_fps = 0;

    // 是否启用
    bool m_enabled = true;

    // 性能阈值
    QHash<QString, double> m_thresholds;
};

// ============================================================================
// 便捷宏定义
// ============================================================================

/**
 * @brief 开始性能计时
 */
#define PERF_START(name) \
    PerformanceMonitor::instance()->startTimer(name)

/**
 * @brief 结束性能计时
 */
#define PERF_MONITOR_END(name) \
    PerformanceMonitor::instance()->endTimer(name)

/**
 * @brief 自动计时作用域
 */
#define PERF_SCOPE(name) \
    PerformanceScope _perfScope_##name(#name)

/**
 * @brief 性能作用域类
 */
class PerformanceScope
{
public:
    explicit PerformanceScope(const QString& name)
        : m_name(name)
    {
        PerformanceMonitor::instance()->startTimer(m_name);
    }

    ~PerformanceScope()
    {
        PerformanceMonitor::instance()->endTimer(m_name);
    }

private:
    QString m_name;
};

#endif // PERFORMANCEMONITOR_H
