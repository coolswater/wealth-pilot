/**
 * @file DataHubBenchmark.h
 * @brief DataHub 性能基准测试
 *
 * @details 测试内容：
 * - 订阅性能测试
 * - 发布性能测试
 * - 模式匹配性能测试
 * - 内存使用测试
 * - 并发性能测试
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAHUBBENCHMARK_H
#define DATAHUBBENCHMARK_H

#include <QObject>
#include <QtTest/QtTest>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>

/**
 * @brief 性能测试结果结构
 */
struct BenchmarkResult {
    QString testName;           ///< 测试名称
    qint64 totalTimeMs = 0;     ///< 总耗时（毫秒）
    qint64 avgTimeUs = 0;       ///< 平均耗时（微秒）
    qint64 minTimeUs = 0;       ///< 最小耗时（微秒）
    qint64 maxTimeUs = 0;       ///< 最大耗时（微秒）
    int iterations = 0;         ///< 迭代次数
    int operationsPerSecond = 0;///< 每秒操作数
    double memoryMB = 0.0;      ///< 内存使用（MB）
    bool passed = false;        ///< 是否通过
    QString threshold;          ///< 阈值描述
};

/**
 * @brief DataHub 性能基准测试类
 *
 * @details 使用方式：
 * @code
 * DataHubBenchmark benchmark;
 * benchmark.runAllBenchmarks();
 * benchmark.generateReport("benchmark_report.md");
 * @endcode
 */
class DataHubBenchmark : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     */
    explicit DataHubBenchmark(QObject* parent = nullptr);

    /**
     * @brief 运行所有基准测试
     */
    void runAllBenchmarks();

    /**
     * @brief 生成测试报告
     * @param filePath 报告文件路径
     */
    void generateReport(const QString& filePath);

    /**
     * @brief 获取测试结果
     */
    QVector<BenchmarkResult> getResults() const { return m_results; }

private slots:
    // ========== 订阅性能测试 ==========

    /**
     * @brief 测试单次订阅性能
     */
    void benchmarkSingleSubscribe();

    /**
     * @brief 测试批量订阅性能（1000次）
     */
    void benchmarkBatchSubscribe();

    /**
     * @brief 测试大量订阅性能（10000次）
     */
    void benchmarkMassSubscribe();

    // ========== 发布性能测试 ==========

    /**
     * @brief 测试单次发布性能
     */
    void benchmarkSinglePublish();

    /**
     * @brief 测试批量发布性能（1000次）
     */
    void benchmarkBatchPublish();

    /**
     * @brief 测试高频发布性能（10000次）
     */
    void benchmarkHighFrequencyPublish();

    // ========== 模式匹配性能测试 ==========

    /**
     * @brief 测试模式订阅性能
     */
    void benchmarkPatternSubscribe();

    /**
     * @brief 测试模式匹配性能
     */
    void benchmarkPatternMatching();

    // ========== 回调性能测试 ==========

    /**
     * @brief 测试回调延迟
     */
    void benchmarkCallbackLatency();

    /**
     * @brief 测试回调吞吐量
     */
    void benchmarkCallbackThroughput();

    // ========== 内存测试 ==========

    /**
     * @brief 测试订阅内存占用
     */
    void benchmarkSubscribeMemory();

    /**
     * @brief 测试缓存内存占用
     */
    void benchmarkCacheMemory();

    // ========== 并发测试 ==========

    /**
     * @brief 测试并发订阅
     */
    void benchmarkConcurrentSubscribe();

    /**
     * @brief 测试并发发布
     */
    void benchmarkConcurrentPublish();

private:
    /**
     * @brief 运行单个基准测试
     */
    BenchmarkResult runBenchmark(const QString& name,
                                  std::function<void()> setup,
                                  std::function<void()> operation,
                                  std::function<void()> cleanup,
                                  int iterations,
                                  qint64 thresholdMs);

    /**
     * @brief 获取当前内存使用
     */
    double getMemoryUsageMB();

    /**
     * @brief 记录测试结果
     */
    void recordResult(const BenchmarkResult& result);

    QVector<BenchmarkResult> m_results;
    QElapsedTimer m_totalTimer;
};

#endif // DATAHUBBENCHMARK_H