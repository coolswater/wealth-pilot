/**
 * @file TestCacheManager.cpp
 * @brief CacheManager单元测试
 */

#include <QtTest/QtTest>
#include "../src/core/cache/CacheManager.h"

class TestCacheManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // 初始化缓存管理器
        QVERIFY(CacheManager::instance()->initialize(
            10 * 1024 * 1024,  // 10MB内存
            100 * 1024 * 1024  // 100MB磁盘
        ));
        qDebug() << "CacheManager Test Suite Started";
    }

    void cleanupTestCase()
    {
        CacheManager::instance()->clearAll();
        qDebug() << "CacheManager Test Suite Completed";
    }

    void init()
    {
        // 每个测试前清空缓存
        CacheManager::instance()->clearAll();
    }

    void testSetAndGet()
    {
        QString key = "test_key";
        QString value = "test_value";
        
        // 设置缓存
        CacheManager::instance()->set(key, value, 60, CacheLevel::L1_Memory);
        
        // 获取缓存
        QVariant result = CacheManager::instance()->get(key);
        QVERIFY(result.isValid());
        QCOMPARE(result.toString(), value);
    }

    void testContains()
    {
        QString key = "test_contains";
        
        // 未设置前不存在
        QVERIFY(!CacheManager::instance()->contains(key));
        
        // 设置后存在
        CacheManager::instance()->set(key, "value", 60, CacheLevel::L1_Memory);
        QVERIFY(CacheManager::instance()->contains(key));
    }

    void testRemove()
    {
        QString key = "test_remove";
        
        CacheManager::instance()->set(key, "value", 60, CacheLevel::L1_Memory);
        QVERIFY(CacheManager::instance()->contains(key));
        
        CacheManager::instance()->remove(key);
        QVERIFY(!CacheManager::instance()->contains(key));
    }

    void testClear()
    {
        // 设置多个缓存
        CacheManager::instance()->set("key1", "value1", 60, CacheLevel::L1_Memory);
        CacheManager::instance()->set("key2", "value2", 60, CacheLevel::L1_Memory);
        CacheManager::instance()->set("key3", "value3", 60, CacheLevel::L1_Memory);
        
        // 清空L1缓存
        CacheManager::instance()->clear(CacheLevel::L1_Memory);
        
        QVERIFY(!CacheManager::instance()->contains("key1"));
        QVERIFY(!CacheManager::instance()->contains("key2"));
        QVERIFY(!CacheManager::instance()->contains("key3"));
    }

    void testBatchOperations()
    {
        QMap<QString, QVariant> data;
        data["batch_key1"] = "value1";
        data["batch_key2"] = "value2";
        data["batch_key3"] = "value3";
        
        // 批量设置
        CacheManager::instance()->setBatch(data, 60);
        
        // 批量获取
        QStringList keys;
        keys << "batch_key1" << "batch_key2" << "batch_key3" << "batch_key4";
        
        QMap<QString, QVariant> result = CacheManager::instance()->getBatch(keys);
        QCOMPARE(result.size(), 3);
        QCOMPARE(result["batch_key1"].toString(), QString("value1"));
        QCOMPARE(result["batch_key2"].toString(), QString("value2"));
        QCOMPARE(result["batch_key3"].toString(), QString("value3"));
    }

    void testExpiration()
    {
        QString key = "test_expiration";
        
        // 设置1秒过期
        CacheManager::instance()->set(key, "value", 1, CacheLevel::L1_Memory);
        
        // 立即获取应该成功
        QVERIFY(CacheManager::instance()->contains(key));
        
        // 等待2秒
        QTest::qWait(2000);
        
        // 过期后应该不存在
        QVERIFY(!CacheManager::instance()->contains(key));
    }

    void testStatistics()
    {
        // 设置一些缓存
        CacheManager::instance()->set("stat_key1", "value1", 60, CacheLevel::L1_Memory);
        CacheManager::instance()->set("stat_key2", "value2", 60, CacheLevel::L1_Memory);
        
        // 获取缓存（命中）
        CacheManager::instance()->get("stat_key1");
        CacheManager::instance()->get("stat_key2");
        
        // 获取不存在的缓存（未命中）
        CacheManager::instance()->get("not_exist");
        
        CacheStats stats = CacheManager::instance()->statistics();
        QVERIFY(stats.totalHits >= 2);
        QVERIFY(stats.totalMisses >= 1);
        QVERIFY(stats.itemCount >= 2);
    }

    void testPerformance()
    {
        // 性能测试：设置1000个缓存
        QElapsedTimer timer;
        timer.start();
        
        for (int i = 0; i < 1000; ++i) {
            QString key = QString("perf_key_%1").arg(i);
            CacheManager::instance()->set(key, QString("value_%1").arg(i), 60, CacheLevel::L1_Memory);
        }
        
        qint64 setTime = timer.elapsed();
        qDebug() << "Set 1000 items in" << setTime << "ms";
        
        // 性能测试：获取1000个缓存
        timer.restart();
        
        for (int i = 0; i < 1000; ++i) {
            QString key = QString("perf_key_%1").arg(i);
            CacheManager::instance()->get(key);
        }
        
        qint64 getTime = timer.elapsed();
        qDebug() << "Get 1000 items in" << getTime << "ms";
        
        // 平均每次操作应该小于1ms
        QVERIFY(setTime < 1000);
        QVERIFY(getTime < 1000);
    }

    void testDifferentTypes()
    {
        // 测试不同数据类型
        CacheManager::instance()->set("int_value", 12345, 60, CacheLevel::L1_Memory);
        CacheManager::instance()->set("double_value", 123.456, 60, CacheLevel::L1_Memory);
        CacheManager::instance()->set("bool_value", true, 60, CacheLevel::L1_Memory);
        CacheManager::instance()->set("string_value", QString("test"), 60, CacheLevel::L1_Memory);
        
        QCOMPARE(CacheManager::instance()->get("int_value").toInt(), 12345);
        QCOMPARE(CacheManager::instance()->get("double_value").toDouble(), 123.456);
        QCOMPARE(CacheManager::instance()->get("bool_value").toBool(), true);
        QCOMPARE(CacheManager::instance()->get("string_value").toString(), QString("test"));
    }
};

QTEST_MAIN(TestCacheManager)
#include "TestCacheManager.moc"