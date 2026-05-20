/**
 * @file PerformanceMonitor.h
 * @brief 性能监控器 - 实时监控应用性能指标
 *
 * @details 功能：
 * - CPU/内存使用率监控
 * - 函数执行时间统计
 * - 帧率监控
 * - 性能告警
 * - 性能报告生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QVector>
#include <QMutex>
#include <QElapsedTimer>
#include <chrono>

namespace WealthPilot {

/**
 * @brief 性能指标
 */
struct PerformanceMetrics {
    double cpuUsage = 0.0;              ///< CPU使用率 (0-100)
    double memoryUsage = 0.0;           ///< 内存使用 (MB)
    double memoryUsagePercent = 0.0;    ///< 内存使用率 (0-100)
    int fps = 0;                        ///< 帧率
    qint64 uptime = 0;                  ///< 运行时间 (ms)
    qint64 timestamp = 0;               ///< 时间戳
};

/**
 * @brief 函数性能统计
 */
struct FunctionStats {
    QString name;                       ///< 函数名
    qint64 totalCalls = 0;              ///< 总调用次数
    qint64 totalTime = 0;               ///< 总耗时 (ns)
    qint64 minTime = LLONG_MAX;         ///< 最小耗时
    qint64 maxTime = 0;                 ///< 最大耗时
    qint64 avgTime = 0;                 ///< 平均耗时
    qint64 lastTime = 0;                ///< 最近一次耗时

    /**
     * @brief 更新统计
     */
    void update(qint64 elapsedNs) {
        totalCalls++;
        totalTime += elapsedNs;
        minTime = qMin(minTime, elapsedNs);
        maxTime = qMax(maxTime, elapsedNs);
        avgTime = totalTime / totalCalls;
        lastTime = elapsedNs;
    }
};

/**
 * @brief 性能告警配置
 */
struct PerformanceAlertConfig {
    double cpuThreshold = 80.0;         ///< CPU告警阈值
    double memoryThreshold = 80.0;      ///< 内存告警阈值
    int fpsThreshold = 30;              ///< FPS告警阈值
    qint64 functionTimeThreshold = 100000000; ///< 函数耗时阈值 (ns, 默认100ms)
    bool enabled = true;                ///< 是否启用告警
};

/**
 * @brief 性能监控器
 */
class PerformanceMonitor : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例
     */
    static PerformanceMonitor& instance();

    /**
     * @brief 启动监控
     * @param intervalMs 监控间隔 (ms)
     */
    void start(int intervalMs = 1000);

    /**
     * @brief 停止监控
     */
    void stop();

    /**
     * @brief 是否正在监控
     */
    bool isMonitoring() const;

    /**
     * @brief 获取当前性能指标
     */
    PerformanceMetrics currentMetrics() const;

    /**
     * @brief 获取性能历史
     * @param count 历史记录数量
     */
    QVector<PerformanceMetrics> history(int count = 60) const;

    // ========== 函数性能统计 ==========

    /**
     * @brief 开始函数计时
     * @param functionName 函数名
     * @return 计时器ID
     */
    qint64 beginFunction(const QString& functionName);

    /**
     * @brief 结束函数计时
     * @param timerId 计时器ID
     */
    void endFunction(qint64 timerId);

    /**
     * @brief 获取函数统计
     */
    FunctionStats getFunctionStats(const QString& functionName) const;

    /**
     * @brief 获取所有函数统计
     */
    QVector<FunctionStats> getAllFunctionStats() const;

    /**
     * @brief 清除函数统计
     */
    void clearFunctionStats();

    // ========== 性能告警 ==========

    /**
     * @brief 设置告警配置
     */
    void setAlertConfig(const PerformanceAlertConfig& config);

    /**
     * @brief 获取告警配置
     */
    PerformanceAlertConfig alertConfig() const;

    // ========== 性能报告 ==========

    /**
     * @brief 生成性能报告
     */
    QString generateReport() const;

    /**
     * @brief 导出性能数据 (JSON)
     */
    QString exportToJson() const;

signals:
    /**
     * @brief 性能指标更新信号
     */
    void metricsUpdated(const PerformanceMetrics& metrics);

    /**
     * @brief CPU告警信号
     */
    void cpuAlert(double usage);

    /**
     * @brief 内存告警信号
     */
    void memoryAlert(double usage);

    /**
     * @brief FPS告警信号
     */
    void fpsAlert(int fps);

    /**
     * @brief 函数性能告警信号
     */
    void functionPerformanceAlert(const QString& functionName, qint64 elapsedNs);

private:
    PerformanceMonitor();
    ~PerformanceMonitor();
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    void collectMetrics();
    void checkAlerts(const PerformanceMetrics& metrics);

    // 监控定时器
    QTimer* m_timer = nullptr;
    bool m_monitoring = false;

    // 性能指标
    PerformanceMetrics m_currentMetrics;
    QVector<PerformanceMetrics> m_history;
    mutable QMutex m_metricsMutex;

    // 函数统计
    QHash<QString, FunctionStats> m_functionStats;
    QHash<qint64, QPair<QString, QElapsedTimer>> m_activeTimers;
    qint64 m_nextTimerId = 1;
    mutable QMutex m_functionMutex;

    // 告警配置
    PerformanceAlertConfig m_alertConfig;

    // 启动时间
    QElapsedTimer m_uptimeTimer;
};

// ============================================================================
// 性能计时辅助类
// ============================================================================

/**
 * @brief 自动性能计时器
 *
 * @details RAII 风格的性能计时，构造时开始计时，析构时自动结束
 *
 * @example
 * @code
 * void myFunction() {
 *     PERF_TIMER("myFunction");
 *     // ... 函数逻辑
 * }
 * @endcode
 */
class ScopedPerfTimer
{
public:
    explicit ScopedPerfTimer(const QString& functionName)
        : m_functionName(functionName)
    {
        m_timerId = PerformanceMonitor::instance().beginFunction(functionName);
    }

    ~ScopedPerfTimer()
    {
        PerformanceMonitor::instance().endFunction(m_timerId);
    }

private:
    QString m_functionName;
    qint64 m_timerId;
};

// 性能计时宏
#define PERF_TIMER(name) ScopedPerfTimer _perfTimer(name)
#define PERF_TIMER_FUNC() ScopedPerfTimer _perfTimer(__FUNCTION__)

} // namespace WealthPilot

#endif // PERFORMANCEMONITOR_H