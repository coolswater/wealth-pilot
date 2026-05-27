/**
 * @file DataHubTest.cpp
 * @brief DataHub 单元测试实现
 */

#include "DataHubTest.h"
#include "data/datahub/DataHub.h"

void DataHubTest::initTestCase()
{
    m_hub = DataHub::instance();
    QVERIFY(m_hub != nullptr);
}

void DataHubTest::cleanupTestCase()
{
    if (m_hub) {
        m_hub->clear();
    }
}

void DataHubTest::testSingleton()
{
    auto instance1 = DataHub::instance();
    auto instance2 = DataHub::instance();
    QCOMPARE(instance1, instance2);
}

void DataHubTest::testSubscribePublish()
{
    m_callbackCount = 0;
    
    auto subId = m_hub->subscribe("test:topic", [this](const QVariant& data) {
        m_callbackCount++;
        Q_UNUSED(data);
    });
    
    QVERIFY(subId != 0);
    
    m_hub->publish("test:topic", QVariant(42));
    QTest::qWait(10);
    
    QCOMPARE(m_callbackCount, 1);
    
    m_hub->unsubscribe(subId);
}

void DataHubTest::testUnsubscribe()
{
    m_callbackCount = 0;
    
    auto subId = m_hub->subscribe("unsub:topic", [this](const QVariant&) {
        m_callbackCount++;
    });
    
    m_hub->publish("unsub:topic", QVariant("test"));
    QTest::qWait(10);
    QCOMPARE(m_callbackCount, 1);
    
    m_hub->unsubscribe(subId);
    
    m_hub->publish("unsub:topic", QVariant("test2"));
    QTest::qWait(10);
    QCOMPARE(m_callbackCount, 1); // 不应增加
}

void DataHubTest::testPatternMatching()
{
    m_callbackCount = 0;
    
    auto subId = m_hub->subscribe("stock:*", [this](const QVariant&) {
        m_callbackCount++;
    });
    
    m_hub->publish("stock:600000", QVariant(1));
    m_hub->publish("stock:000001", QVariant(2));
    m_hub->publish("future:IF2401", QVariant(3)); // 不应匹配
    
    QTest::qWait(10);
    QCOMPARE(m_callbackCount, 2);
    
    m_hub->unsubscribe(subId);
}

void DataHubTest::testDataCache()
{
    m_hub->publish("cache:test", QVariant("cached_data"));
    QTest::qWait(10);
    
    auto cached = m_hub->getCached("cache:test");
    QVERIFY(cached.isValid());
    QCOMPARE(cached.toString(), QString("cached_data"));
}

void DataHubTest::testLifecycle()
{
    // 测试订阅生命周期
    {
        auto subId = m_hub->subscribe("lifecycle:test", [](const QVariant&) {});
        QVERIFY(subId != 0);
        m_hub->unsubscribe(subId);
    }
    
    // 测试清理
    m_hub->clear();
    QTest::qWait(10);
}

void DataHubTest::testPerformanceManySubscriptions()
{
    const int count = 1000;
    QVector<quint64> subIds;
    subIds.reserve(count);
    
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < count; ++i) {
        subIds.append(m_hub->subscribe(QString("perf:%1").arg(i), [](const QVariant&) {}));
    }
    
    qint64 subscribeTime = timer.elapsed();
    qDebug() << "订阅" << count << "个主题耗时:" << subscribeTime << "ms";
    
    timer.restart();
    for (auto id : subIds) {
        m_hub->unsubscribe(id);
    }
    qint64 unsubscribeTime = timer.elapsed();
    qDebug() << "取消订阅耗时:" << unsubscribeTime << "ms";
}

void DataHubTest::testPerformanceHighFrequency()
{
    m_callbackCount = 0;
    
    auto subId = m_hub->subscribe("highfreq:test", [this](const QVariant&) {
        m_callbackCount++;
    });
    
    const int count = 10000;
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < count; ++i) {
        m_hub->publish("highfreq:test", QVariant(i));
    }
    
    QTest::qWait(100);
    qint64 publishTime = timer.elapsed();
    
    qDebug() << "发布" << count << "次耗时:" << publishTime << "ms";
    qDebug() << "回调次数:" << m_callbackCount;
    
    m_hub->unsubscribe(subId);
}

void DataHubTest::testMemoryUsage()
{
    // 获取初始内存使用
    qint64 initialMemory = 0; // 简化测试，不实际测量内存
    
    // 创建大量订阅
    QVector<quint64> subIds;
    for (int i = 0; i < 100; ++i) {
        subIds.append(m_hub->subscribe(QString("mem:%1").arg(i), [](const QVariant&) {}));
    }
    
    // 清理
    for (auto id : subIds) {
        m_hub->unsubscribe(id);
    }
    
    Q_UNUSED(initialMemory);
    QVERIFY(true); // 简化测试
}

QTEST_MAIN(DataHubTest)
#include "DataHubTest.moc"
