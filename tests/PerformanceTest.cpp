/**
 * @file PerformanceTest.cpp
 * @brief 性能测试套件
 */

#include <QtTest/QtTest>
#include <QElapsedTimer>
#include <QRandomGenerator>
#include "../src/core/cache/CacheManager.h"
#include "../src/core/di/ServiceLocator.h"
#include "../src/core/task/AsyncTaskManager.h"
#include "../src/utils/TechnicalIndicators.h"

class PerformanceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        CacheManager::instance()->initialize(100 * 1024 * 1024, 1024 * 1024 * 1024);
        AsyncTaskManager::instance().initialize(QThread::idealThreadCount());
        qDebug() << "Performance Test Suite Started";
    }

    void cleanupTestCase()
    {
        CacheManager::instance()->clearAll();
        qDebug() << "Performance Test Suite Completed";
    }

    // ========== 缓存性能测试 ==========

    void testCacheWritePerformance()
    {
        qDebug() << "\n=== Cache Write Performance ===";
        
        QElapsedTimer timer;
        
        // 测试不同数据量的写入性能
        QList<int> counts = {100, 1000, 10000};
        
        for (int count : counts) {
            CacheManager::instance()->clearAll();
            timer.start();
            
            for (int i = 0; i < count; ++i) {
                QString key = QString("key_%1").arg(i);
                QString value = QString("value_%1_%2").arg(i).arg(QString(100, 'x'));
                CacheManager::instance()->set(key, value, 60, CacheLevel::L1_Memory);
            }
            
            qint64 elapsed = timer.elapsed();
            double opsPerSec = count * 1000.0 / elapsed;
            
            qDebug() << QString("Write %1 items: %2ms (%3 ops/sec)")
                .arg(count).arg(elapsed).arg(opsPerSec, 0, 'f', 0);
            
            // 性能要求：至少1000 ops/sec
            QVERIFY(opsPerSec > 1000);
        }
    }

    void testCacheReadPerformance()
    {
        qDebug() << "\n=== Cache Read Performance ===";
        
        // 准备数据
        int count = 10000;
        for (int i = 0; i < count; ++i) {
            QString key = QString("read_key_%1").arg(i);
            CacheManager::instance()->set(key, QString("value_%1").arg(i), 60, CacheLevel::L1_Memory);
        }
        
        QElapsedTimer timer;
        
        // 测试读取性能
        timer.start();
        for (int i = 0; i < count; ++i) {
            QString key = QString("read_key_%1").arg(i);
            CacheManager::instance()->get(key);
        }
        
        qint64 elapsed = timer.elapsed();
        double opsPerSec = count * 1000.0 / elapsed;
        
        qDebug() << QString("Read %1 items: %2ms (%3 ops/sec)")
            .arg(count).arg(elapsed).arg(opsPerSec, 0, 'f', 0);
        
        // 性能要求：至少5000 ops/sec
        QVERIFY(opsPerSec > 5000);
    }

    void testCacheHitRate()
    {
        qDebug() << "\n=== Cache Hit Rate ===";
        
        CacheManager::instance()->clearAll();
        
        // 设置100个缓存项
        for (int i = 0; i < 100; ++i) {
            CacheManager::instance()->set(QString("hit_key_%1").arg(i), i, 60, CacheLevel::L1_Memory);
        }
        
        // 访问存在的缓存（命中）
        for (int i = 0; i < 100; ++i) {
            CacheManager::instance()->get(QString("hit_key_%1").arg(i));
        }
        
        // 访问不存在的缓存（未命中）
        for (int i = 0; i < 50; ++i) {
            CacheManager::instance()->get(QString("miss_key_%1").arg(i));
        }
        
        CacheStats stats = CacheManager::instance()->statistics();
        qDebug() << QString("Hits: %1, Misses: %2, Hit Rate: %3%")
            .arg(stats.totalHits).arg(stats.totalMisses).arg(stats.hitRate * 100, 0, 'f', 1);
        
        // 命中率应该大于60%
        QVERIFY(stats.hitRate > 0.6);
    }

    // ========== 技术指标性能测试 ==========

    void testTechnicalIndicatorsPerformance()
    {
        qDebug() << "\n=== Technical Indicators Performance ===";
        
        // 生成测试数据
        int dataSize = 10000;
        QVector<double> data(dataSize);
        QRandomGenerator rng(42);
        for (int i = 0; i < dataSize; ++i) {
            data[i] = 100.0 + qSin(i * 0.1) * 10.0 + rng.bounded(100) / 100.0;
        }
        
        QElapsedTimer timer;
        
        // 测试SMA性能
        timer.start();
        QVector<double> sma = TechnicalIndicators::SMA(data, 20);
        qint64 smaTime = timer.elapsed();
        qDebug() << QString("SMA(20) on %1 items: %2ms").arg(dataSize).arg(smaTime);
        QVERIFY(smaTime < 100);
        
        // 测试EMA性能
        timer.restart();
        QVector<double> ema = TechnicalIndicators::EMA(data, 20);
        qint64 emaTime = timer.elapsed();
        qDebug() << QString("EMA(20) on %1 items: %2ms").arg(dataSize).arg(emaTime);
        QVERIFY(emaTime < 100);
        
        // 测试MACD性能
        timer.restart();
        IndicatorResult macd = TechnicalIndicators::MACD(data, 12, 26, 9);
        qint64 macdTime = timer.elapsed();
        qDebug() << QString("MACD on %1 items: %2ms").arg(dataSize).arg(macdTime);
        QVERIFY(macdTime < 200);
        
        // 测试RSI性能
        timer.restart();
        QVector<double> rsi = TechnicalIndicators::RSI(data, 14);
        qint64 rsiTime = timer.elapsed();
        qDebug() << QString("RSI(14) on %1 items: %2ms").arg(dataSize).arg(rsiTime);
        QVERIFY(rsiTime < 200);
    }

    // ========== 异步任务性能测试 ==========

    void testAsyncTaskPerformance()
    {
        qDebug() << "\n=== Async Task Performance ===";
        
        int taskCount = 100;
        QElapsedTimer timer;
        
        // 测试任务提交性能
        timer.start();
        for (int i = 0; i < taskCount; ++i) {
            AsyncTaskManager::instance().submitTask<int>(
                QString("perf_task_%1").arg(i),
                []() {
                    // 模拟计算任务
                    int result = 0;
                    for (int j = 0; j < 1000; ++j) {
                        result += j;
                    }
                    return result;
                },
                TaskPriority::Normal
            );
        }
        qint64 submitTime = timer.elapsed();
        qDebug() << QString("Submit %1 tasks: %2ms").arg(taskCount).arg(submitTime);
        
        // 等待所有任务完成
        AsyncTaskManager::instance().waitForAll(5000);
        
        qint64 totalTime = timer.elapsed();
        qDebug() << QString("Complete %1 tasks: %2ms").arg(taskCount).arg(totalTime);
        
        // 提交时间应该小于500ms
        QVERIFY(submitTime < 500);
    }

    // ========== 内存使用测试 ==========

    void testMemoryUsage()
    {
        qDebug() << "\n=== Memory Usage ===";
        
        CacheManager::instance()->clearAll();
        
        // 设置大量缓存
        int itemCount = 10000;
        int itemSize = 1024; // 1KB per item
        
        for (int i = 0; i < itemCount; ++i) {
            QString key = QString("mem_key_%1").arg(i);
            QString value(itemSize, 'x');
            CacheManager::instance()->set(key, value, 60, CacheLevel::L1_Memory);
        }
        
        CacheStats stats = CacheManager::instance()->statistics();
        double memoryMB = stats.memoryUsage / 1024.0 / 1024.0;
        
        qDebug() << QString("Memory usage: %1 MB for %2 items (%3 KB/item)")
            .arg(memoryMB, 0, 'f', 2).arg(itemCount).arg(itemSize / 1024.0, 0, 'f', 1);
        
        // 内存使用应该合理（不超过预期的2倍）
        double expectedMB = itemCount * itemSize / 1024.0 / 1024.0;
        QVERIFY(memoryMB < expectedMB * 2);
    }

    // ========== 综合性能测试 ==========

    void testOverallPerformance()
    {
        qDebug() << "\n=== Overall Performance Summary ===";
        
        QElapsedTimer timer;
        
        // 模拟实际使用场景
        timer.start();
        
        // 1. 缓存操作
        for (int i = 0; i < 1000; ++i) {
            CacheManager::instance()->set(QString("overall_%1").arg(i), i, 60, CacheLevel::L1_Memory);
        }
        qint64 cacheTime = timer.elapsed();
        
        // 2. 技术指标计算
        timer.restart();
        QVector<double> data(1000);
        for (int i = 0; i < 1000; ++i) {
            data[i] = 100.0 + qSin(i * 0.1) * 10.0;
        }
        TechnicalIndicators::MACD(data, 12, 26, 9);
        qint64 indicatorTime = timer.elapsed();
        
        // 3. 异步任务
        timer.restart();
        for (int i = 0; i < 50; ++i) {
            AsyncTaskManager::instance().submitTask<int>(
                QString("overall_task_%1").arg(i),
                []() { return 42; },
                TaskPriority::Normal
            );
        }
        AsyncTaskManager::instance().waitForAll(2000);
        qint64 asyncTime = timer.elapsed();
        
        qDebug() << "\nPerformance Summary:";
        qDebug() << QString("  Cache operations: %1ms").arg(cacheTime);
        qDebug() << QString("  Technical indicators: %1ms").arg(indicatorTime);
        qDebug() << QString("  Async tasks: %1ms").arg(asyncTime);
        qDebug() << QString("  Total: %1ms").arg(cacheTime + indicatorTime + asyncTime);
        
        // 总时间应该小于2000ms
        QVERIFY(cacheTime + indicatorTime + asyncTime < 2000);
    }
};

QTEST_MAIN(PerformanceTest)
#include "PerformanceTest.moc"