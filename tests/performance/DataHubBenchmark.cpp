/**
 * @file DataHubBenchmark.cpp
 * @brief DataHub 性能基准测试实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "DataHubBenchmark.h"
#include "core/datahub/DataHub.h"
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>
#include <QCoreApplication>

DataHubBenchmark::DataHubBenchmark(QObject* parent)
    : QObject(parent)
{
    m_totalTimer.start();
}

void DataHubBenchmark::runAllBenchmarks()
{
    qDebug() << "========================================";
    qDebug() << "DataHub Performance Benchmark Suite";
    qDebug() << "========================================";
    qDebug() << "";

    // 订阅性能测试
    qDebug() << "--- Subscription Performance ---";
    benchmarkSingleSubscribe();
    benchmarkBatchSubscribe();
    benchmarkMassSubscribe();

    // 发布性能测试
    qDebug() << "\n--- Publish Performance ---";
    benchmarkSinglePublish();
    benchmarkBatchPublish();
    benchmarkHighFrequencyPublish();

    // 模式匹配性能测试
    qDebug() << "\n--- Pattern Matching Performance ---";
    benchmarkPatternSubscribe();
    benchmarkPatternMatching();

    // 回调性能测试
    qDebug() << "\n--- Callback Performance ---";
    benchmarkCallbackLatency();
    benchmarkCallbackThroughput();

    // 内存测试
    qDebug() << "\n--- Memory Performance ---";
    benchmarkSubscribeMemory();
    benchmarkCacheMemory();

    // 并发测试
    qDebug() << "\n--- Concurrency Performance ---";
    benchmarkConcurrentSubscribe();
    benchmarkConcurrentPublish();

    qDebug() << "\n========================================";
    qDebug() << "All benchmarks completed!";
    qDebug() << "Total time:" << m_totalTimer.elapsed() << "ms";
    qDebug() << "========================================";
}

void DataHubBenchmark::benchmarkSingleSubscribe()
{
    auto result = runBenchmark(
        "Single Subscribe",
        []() {},
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            hub.subscribe(&owner, "test:benchmark:single", [](const QString&, const QVariant&) {});
            hub.unsubscribeAll(&owner);
        },
        []() {},
        1000,
        100  // 阈值：100ms
    );
    recordResult(result);
}

void DataHubBenchmark::benchmarkBatchSubscribe()
{
    auto result = runBenchmark(
        "Batch Subscribe (1000)",
        []() {},
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            for (int i = 0; i < 1000; ++i) {
                hub.subscribe(&owner, QString("test:batch:%1").arg(i),
                    [](const QString&, const QVariant&) {});
            }
            hub.unsubscribeAll(&owner);
        },
        []() {},
        10,
        500  // 阈值：500ms
    );
    recordResult(result);
}

void DataHubBenchmark::benchmarkMassSubscribe()
{
    auto result = runBenchmark(
        "Mass Subscribe (10000)",
        []() {},
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            for (int i = 0; i < 10000; ++i) {
                hub.subscribe(&owner, QString("test:mass:%1").arg(i),
                    [](const QString&, const QVariant&) {});
            }
            hub.unsubscribeAll(&owner);
        },
        []() {},
        3,
        1000  // 阈值：1000ms
    );
    recordResult(result);
}

void DataHubBenchmark::benchmarkSinglePublish()
{
    auto result = runBenchmark(
        "Single Publish",
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            hub.subscribe(&owner, "test:publish:single", [](const QString&, const QVariant&) {});
        },
        []() {
            DataHub& hub = DataHub::instance();
            hub.publish("test:publish:single", "data");
        },
        []() {
            DataHub& hub = DataHub::instance();
        },
        1000,
        100  // 阈值：100ms
    );
    recordResult(result);
}

void DataHubBenchmark::benchmarkBatchPublish()
{
    auto result = runBenchmark(
        "Batch Publish (1000)",
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            hub.subscribe(&owner, "test:publish:batch", [](const QString&, const QVariant&) {});
        },
        []() {
            DataHub& hub = DataHub::instance();
            for (int i = 0; i < 1000; ++i) {
                hub.publish("test:publish:batch", i);
            }
        },
        []() {},
        10,
        500  // 阈值：500ms
    );
    recordResult(result);
}

void DataHubBenchmark::benchmarkHighFrequencyPublish()
{
    auto result = runBenchmark(
        "High Frequency Publish (10000)",
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            hub.subscribe(&owner, "test:publish:highfreq", [](const QString&, const QVariant&) {});
        },
        []() {
            DataHub& hub = DataHub::instance();
            for (int i = 0; i < 10000; ++i) {
                hub.publish("test:publish:highfreq", i);
            }
        },
        []() {},
        5,
        1000  // 阈值：1000ms
    );
    recordResult(result);
}

void DataHubBenchmark::benchmarkPatternSubscribe()
{
    auto result = runBenchmark(
        "Pattern Subscribe",
        []() {},
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            hub.subscribePattern(&owner, "test:pattern:*", [](const QString&, const QVariant&) {});
            hub.unsubscribeAll(&owner);
        },
        []() {},
        100,
        200  // 阈值：200ms
    );
    recordResult(result);
}

void DataHubBenchmark::benchmarkPatternMatching()
{
    auto result = runBenchmark(
        "Pattern Matching (1000)",
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            hub.subscribePattern(&owner, "test:match:*", [](const QString&, const QVariant&) {});
        },
        []() {
            DataHub& hub = DataHub::instance();
            for (int i = 0; i < 1000; ++i) {
                hub.publish(QString("test:match:%1").arg(i), "data");
            }
        },
        []() {},
        10,
        500  // 阈值：500ms
    );
    recordResult(result);
}

void DataHubBenchmark::benchmarkCallbackLatency()
{
    qint64 totalLatency = 0;
    int count = 0;

    auto result = runBenchmark(
        "Callback Latency",
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            hub.subscribe(&owner, "test:latency", [&](const QString&, const QVariant&) {
                // 模拟回调处理
            });
        },
        [&]() {
            DataHub& hub = DataHub::instance();
            QElapsedTimer timer;
            timer.start();
            hub.publish("test:latency", "data");
            totalLatency += timer.elapsed();
            count++;
        },
        []() {},
        100,
        200  // 阈值：200ms
    );
    result.avgTimeUs = count > 0 ? (totalLatency * 1000 / count) : 0;
    recordResult(result);
}

void DataHubBenchmark::benchmarkCallbackThroughput()
{
    std::atomic<int> callbackCount{0};

    auto result = runBenchmark(
        "Callback Throughput",
        [&]() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            hub.subscribe(&owner, "test:throughput", [&](const QString&, const QVariant&) {
                callbackCount++;
            });
        },
        [&]() {
            DataHub& hub = DataHub::instance();
            for (int i = 0; i < 10000; ++i) {
                hub.publish("test:throughput", i);
            }
        },
        []() {},
        5,
        1000  // 阈值：1000ms
    );
    result.operationsPerSecond = callbackCount.load() * 1000 / result.totalTimeMs;
    recordResult(result);
}

void DataHubBenchmark::benchmarkSubscribeMemory()
{
    double startMem = getMemoryUsageMB();

    DataHub& hub = DataHub::instance();
    QObject owner;
    for (int i = 0; i < 10000; ++i) {
        hub.subscribe(&owner, QString("test:memory:%1").arg(i),
            [](const QString&, const QVariant&) {});
    }

    double endMem = getMemoryUsageMB();
    hub.unsubscribeAll(&owner);

    BenchmarkResult result;
    result.testName = "Subscribe Memory (10000)";
    result.memoryMB = endMem - startMem;
    result.passed = result.memoryMB < 100.0;  // 阈值：100MB
    result.threshold = "< 100MB";
    recordResult(result);

    qDebug() << "  Memory usage:" << result.memoryMB << "MB"
             << (result.passed ? "PASS" : "FAIL");
}

void DataHubBenchmark::benchmarkCacheMemory()
{
    double startMem = getMemoryUsageMB();

    DataHub& hub = DataHub::instance();
    for (int i = 0; i < 10000; ++i) {
        hub.publish(QString("test:cache:%1").arg(i), QString("data_%1").arg(i));
    }

    double endMem = getMemoryUsageMB();

    BenchmarkResult result;
    result.testName = "Cache Memory (10000)";
    result.memoryMB = endMem - startMem;
    result.passed = result.memoryMB < 50.0;  // 阈值：50MB
    result.threshold = "< 50MB";
    recordResult(result);

    qDebug() << "  Memory usage:" << result.memoryMB << "MB"
             << (result.passed ? "PASS" : "FAIL");
}

void DataHubBenchmark::benchmarkConcurrentSubscribe()
{
    auto result = runBenchmark(
        "Concurrent Subscribe (4 threads)",
        []() {},
        []() {
            const int threadCount = 4;
            QVector<QThread*> threads;
            QMutex mutex;
            QWaitCondition condition;
            std::atomic<int> ready{0};

            for (int t = 0; t < threadCount; ++t) {
                QThread* thread = QThread::create([&]() {
                    DataHub& hub = DataHub::instance();
                    QObject owner;
                    for (int i = 0; i < 1000; ++i) {
                        hub.subscribe(&owner, QString("test:concurrent:%1:%2").arg(t).arg(i),
                            [](const QString&, const QVariant&) {});
                    }
                    hub.unsubscribeAll(&owner);
                });
                threads.append(thread);
                thread->start();
            }

            for (auto* thread : threads) {
                thread->wait();
                delete thread;
            }
        },
        []() {},
        5,
        2000  // 阈值：2000ms
    );
    recordResult(result);
}

void DataHubBenchmark::benchmarkConcurrentPublish()
{
    auto result = runBenchmark(
        "Concurrent Publish (4 threads)",
        []() {
            DataHub& hub = DataHub::instance();
            QObject owner;
            hub.subscribe(&owner, "test:concurrent:pub", [](const QString&, const QVariant&) {});
        },
        []() {
            const int threadCount = 4;
            QVector<QThread*> threads;

            for (int t = 0; t < threadCount; ++t) {
                QThread* thread = QThread::create([&]() {
                    DataHub& hub = DataHub::instance();
                    for (int i = 0; i < 1000; ++i) {
                        hub.publish("test:concurrent:pub", i);
                    }
                });
                threads.append(thread);
                thread->start();
            }

            for (auto* thread : threads) {
                thread->wait();
                delete thread;
            }
        },
        []() {},
        5,
        2000  // 阈值：2000ms
    );
    recordResult(result);
}

BenchmarkResult DataHubBenchmark::runBenchmark(
    const QString& name,
    std::function<void()> setup,
    std::function<void()> operation,
    std::function<void()> cleanup,
    int iterations,
    qint64 thresholdMs)
{
    BenchmarkResult result;
    result.testName = name;
    result.iterations = iterations;
    result.threshold = QString("< %1ms").arg(thresholdMs);

    qDebug() << "\nRunning:" << name << "...";

    // 预热
    setup();
    operation();
    cleanup();

    // 正式测试
    QElapsedTimer timer;
    QVector<qint64> times;
    times.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        setup();
        timer.start();
        operation();
        times.append(timer.elapsed());
        cleanup();
    }

    // 计算统计数据
    result.totalTimeMs = 0;
    result.minTimeUs = LLONG_MAX;
    result.maxTimeUs = 0;

    for (qint64 t : times) {
        result.totalTimeMs += t;
        qint64 us = t * 1000;
        if (us < result.minTimeUs) result.minTimeUs = us;
        if (us > result.maxTimeUs) result.maxTimeUs = us;
    }

    result.avgTimeUs = iterations > 0 ? result.totalTimeMs * 1000 / iterations : 0;
    result.operationsPerSecond = iterations > 0 && result.totalTimeMs > 0
        ? iterations * 1000 / result.totalTimeMs : 0;
    result.passed = result.totalTimeMs < thresholdMs;

    qDebug() << "  Total:" << result.totalTimeMs << "ms"
             << "Avg:" << result.avgTimeUs << "us"
             << "Ops/s:" << result.operationsPerSecond
             << (result.passed ? "PASS" : "FAIL");

    return result;
}

double DataHubBenchmark::getMemoryUsageMB()
{
#ifdef Q_OS_LINUX
    QFile file("/proc/self/status");
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QByteArray line = file.readLine();
            if (line.startsWith("VmRSS:")) {
                QList<QByteArray> parts = line.split(' ');
                if (parts.size() >= 2) {
                    return parts[parts.size() - 2].toDouble() / 1024.0;
                }
            }
        }
    }
#endif
    return 0.0;
}

void DataHubBenchmark::recordResult(const BenchmarkResult& result)
{
    m_results.append(result);
}

void DataHubBenchmark::generateReport(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open report file:" << filePath;
        return;
    }

    QTextStream out(&file);
    out << "# DataHub Performance Benchmark Report\n\n";
    out << "**Generated:** " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n\n";
    out << "---\n\n";

    out << "## Summary\n\n";
    out << "| Test Name | Total (ms) | Avg (us) | Ops/s | Passed | Threshold |\n";
    out << "|-----------|-----------|----------|-------|--------|-----------|\n";

    int passed = 0;
    int failed = 0;

    for (const auto& result : m_results) {
        out << "| " << result.testName
            << " | " << result.totalTimeMs
            << " | " << result.avgTimeUs
            << " | " << result.operationsPerSecond
            << " | " << (result.passed ? "✅" : "❌")
            << " | " << result.threshold
            << " |\n";

        if (result.passed) passed++;
        else failed++;
    }

    out << "\n---\n\n";
    out << "## Results\n\n";
    out << QString("- **Passed:** %1\n").arg(passed);
    out << QString("- **Failed:** %1\n").arg(failed);
    out << QString("- **Pass Rate:** %1%\n").arg(passed * 100.0 / m_results.size(), 0, 'f', 1);

    out << "\n---\n\n";
    out << "*Report generated by DataHubBenchmark*\n";

    file.close();
    qDebug() << "Report generated:" << filePath;
}

// Qt Test 主函数
QTEST_MAIN(DataHubBenchmark)
#include "DataHubBenchmark.moc"