/**
 * @file StressTest.h
 * @brief 压力测试工具
 *
 * @details 功能：
 * - 模拟高并发数据请求
 * - 测试系统稳定性
 * - 生成压力测试报告
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef STRESSTEST_H
#define STRESSTEST_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QThreadPool>
#include <QFuture>
#include <QElapsedTimer>
#include <QJsonObject>

namespace WealthPilot {

/**
 * @brief 压力测试配置
 */
struct StressTestConfig {
    int concurrentUsers = 10;       ///< 并发用户数
    int requestsPerUser = 100;      ///< 每用户请求数
    int requestIntervalMs = 100;    ///< 请求间隔 (ms)
    int timeoutMs = 30000;          ///< 超时时间 (ms)
    bool warmupEnabled = true;      ///< 是否预热
    int warmupRequests = 10;        ///< 预热请求数
};

/**
 * @brief 压力测试结果
 */
struct StressTestResult {
    qint64 totalRequests = 0;       ///< 总请求数
    qint64 successCount = 0;        ///< 成功数
    qint64 failureCount = 0;        ///< 失败数
    qint64 timeoutCount = 0;        ///< 超时数
    qint64 totalTime = 0;           ///< 总耗时 (ms)
    qint64 avgLatency = 0;          ///< 平均延迟 (ms)
    qint64 minLatency = LLONG_MAX;  ///< 最小延迟 (ms)
    qint64 maxLatency = 0;          ///< 最大延迟 (ms)
    qint64 p50Latency = 0;          ///< P50 延迟 (ms)
    qint64 p95Latency = 0;          ///< P95 延迟 (ms)
    qint64 p99Latency = 0;          ///< P99 延迟 (ms)
    double throughput = 0.0;        ///< 吞吐量 (req/s)
    double successRate = 0.0;       ///< 成功率 (%)
    double errorRate = 0.0;         ///< 错误率 (%)
};

/**
 * @brief 压力测试器
 */
class StressTest : public QObject
{
    Q_OBJECT

public:
    static StressTest& instance();

    /**
     * @brief 运行压力测试
     * @param config 测试配置
     * @param testFunc 测试函数
     */
    void run(const StressTestConfig& config,
             std::function<bool()> testFunc);

    /**
     * @brief 停止测试
     */
    void stop();

    /**
     * @brief 获取测试结果
     */
    StressTestResult getResult() const { return m_result; }

    /**
     * @brief 是否正在运行
     */
    bool isRunning() const { return m_running; }

    /**
     * @brief 生成测试报告
     */
    QJsonObject generateReport() const;

signals:
    /**
     * @brief 测试进度
     */
    void progress(int current, int total, const StressTestResult& intermediateResult);

    /**
     * @brief 测试完成
     */
    void completed(const StressTestResult& result);

    /**
     * @brief 测试错误
     */
    void error(const QString& message);

private:
    StressTest() = default;
    ~StressTest() = default;
    StressTest(const StressTest&) = delete;
    StressTest& operator=(const StressTest&) = delete;

    void calculateLatencyPercentiles();

    StressTestConfig m_config;
    StressTestResult m_result;
    QVector<qint64> m_latencies;
    bool m_running = false;
    bool m_stopRequested = false;
};

} // namespace WealthPilot

#endif // STRESSTEST_H