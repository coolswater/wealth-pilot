/**
 * @file StressTest.cpp
 * @brief 压力测试工具实现
 */

#include "StressTest.h"
#include "Logger.h"
#include <QtConcurrent>
#include <algorithm>

namespace WealthPilot {

StressTest& StressTest::instance()
{
    static StressTest inst;
    return inst;
}

void StressTest::run(const StressTestConfig& config,
                     std::function<bool()> testFunc)
{
    if (m_running) {
        emit error("Stress test already running");
        return;
    }

    m_config = config;
    m_running = true;
    m_stopRequested = false;
    m_result = StressTestResult();
    m_latencies.clear();

    LOG_INFO(QString("Stress test started: %1 concurrent users, %2 requests per user")
            .arg(config.concurrentUsers).arg(config.requestsPerUser));

    QElapsedTimer totalTimer;
    totalTimer.start();

    // 预热
    if (config.warmupEnabled) {
        LOG_INFO("Warmup phase started");
        for (int i = 0; i < config.warmupRequests && !m_stopRequested; ++i) {
            testFunc();
        }
        LOG_INFO("Warmup phase completed");
    }

    // 主测试
    int totalRequests = config.concurrentUsers * config.requestsPerUser;
    QVector<QFuture<void>> futures;

    for (int user = 0; user < config.concurrentUsers && !m_stopRequested; ++user) {
        futures.append(QtConcurrent::run([&, user]() {
            for (int req = 0; req < config.requestsPerUser && !m_stopRequested; ++req) {
                QElapsedTimer requestTimer;
                requestTimer.start();

                bool success = testFunc();

                qint64 latency = requestTimer.elapsed();
                m_latencies.append(latency);

                m_result.totalRequests++;
                if (success) {
                    m_result.successCount++;
                } else {
                    if (latency > config.timeoutMs) {
                        m_result.timeoutCount++;
                    }
                    m_result.failureCount++;
                }

                // 进度通知
                if (m_result.totalRequests % 100 == 0) {
                    emit progress(m_result.totalRequests, totalRequests, m_result);
                }

                // 请求间隔
                if (config.requestIntervalMs > 0) {
                    QThread::msleep(config.requestIntervalMs);
                }
            }
        }));
    }

    // 等待所有任务完成
    for (auto& future : futures) {
        future.waitForFinished();
    }

    m_result.totalTime = totalTimer.elapsed();

    // 计算统计数据
    calculateLatencyPercentiles();

    m_result.throughput = m_result.totalRequests * 1000.0 / m_result.totalTime;
    m_result.successRate = m_result.successCount * 100.0 / m_result.totalRequests;
    m_result.errorRate = m_result.failureCount * 100.0 / m_result.totalRequests;

    m_running = false;

    LOG_INFO(QString("Stress test completed: %1 requests, %2 success, %3 failures, %4ms total, %5 req/s")
            .arg(m_result.totalRequests)
            .arg(m_result.successCount)
            .arg(m_result.failureCount)
            .arg(m_result.totalTime)
            .arg(m_result.throughput));

    emit completed(m_result);
}

void StressTest::stop()
{
    m_stopRequested = true;
    LOG_INFO("Stress test stop requested");
}

void StressTest::calculateLatencyPercentiles()
{
    if (m_latencies.isEmpty()) {
        return;
    }

    std::sort(m_latencies.begin(), m_latencies.end());

    m_result.minLatency = m_latencies.first();
    m_result.maxLatency = m_latencies.last();
    m_result.avgLatency = 0;

    for (auto latency : m_latencies) {
        m_result.avgLatency += latency;
    }
    m_result.avgLatency /= m_latencies.size();

    // 计算百分位数
    int p50Index = m_latencies.size() * 50 / 100;
    int p95Index = m_latencies.size() * 95 / 100;
    int p99Index = m_latencies.size() * 99 / 100;

    m_result.p50Latency = m_latencies[p50Index];
    m_result.p95Latency = m_latencies[p95Index];
    m_result.p99Latency = m_latencies[p99Index];
}

QJsonObject StressTest::generateReport() const
{
    QJsonObject report;

    report["totalRequests"] = m_result.totalRequests;
    report["successCount"] = m_result.successCount;
    report["failureCount"] = m_result.failureCount;
    report["timeoutCount"] = m_result.timeoutCount;
    report["totalTimeMs"] = m_result.totalTime;
    report["avgLatencyMs"] = m_result.avgLatency;
    report["minLatencyMs"] = m_result.minLatency == LLONG_MAX ? 0 : m_result.minLatency;
    report["maxLatencyMs"] = m_result.maxLatency;
    report["p50LatencyMs"] = m_result.p50Latency;
    report["p95LatencyMs"] = m_result.p95Latency;
    report["p99LatencyMs"] = m_result.p99Latency;
    report["throughput"] = m_result.throughput;
    report["successRate"] = m_result.successRate;
    report["errorRate"] = m_result.errorRate;

    // 配置信息
    QJsonObject configObj;
    configObj["concurrentUsers"] = m_config.concurrentUsers;
    configObj["requestsPerUser"] = m_config.requestsPerUser;
    configObj["requestIntervalMs"] = m_config.requestIntervalMs;
    configObj["timeoutMs"] = m_config.timeoutMs;
    report["config"] = configObj;

    return report;
}

} // namespace WealthPilot