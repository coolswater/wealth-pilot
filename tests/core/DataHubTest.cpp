/**
 * @file DataHubTest.cpp
 * @brief DataHub 单元测试
 */

#include <QtTest/QtTest>
#include "data/datahub/DataHub.h"

using namespace WealthPilot::DataHub;

class DataHubTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // 发布订阅测试
    void testPublishSubscribe();
    void testMultipleSubscribers();
    void testUnsubscribe();
    
    // 通配符测试
    void testWildcardSubscribe();
    void testPatternMatch();
    
    // 生命周期测试
    void testTTL();
    void testCleanup();
    
    // 性能测试
    void testPublishPerformance();

private:
    DataHub* m_hub = nullptr;
};

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

void DataHubTest::testPublishSubscribe()
{
    int callCount = 0;
    QVariant receivedData;
    
    auto subId = m_hub->subscribe("test:topic", [&](const QVariant& data) {
        callCount++;
        receivedData = data;
    });
    
    QVERIFY(subId != 0);
    
    // 发布数据
    m_hub->publish("test:topic", QVariant(42));
    QTest::qWait(10);
    
    QCOMPARE(callCount, 1);
    QCOMPARE(receivedData.toInt(), 42);
    
    m_hub->unsubscribe(subId);
}

void DataHubTest::testMultipleSubscribers()
{
    int count1 = 0, count2 = 0, count3 = 0;
    
    auto id1 = m_hub->subscribe("multi:topic", [&](const QVariant&) { count1++; });
    auto id2 = m_hub->subscribe("multi:topic", [&](const QVariant&) { count2++; });
    auto id3 = m_hub->subscribe("multi:topic", [&](const QVariant&) { count3++; });
    
    m_hub->publish("multi:topic", QVariant("test"));
    QTest::qWait(10);
    
    QCOMPARE(count1, 1);
    QCOMPARE(count2, 1);
    QCOMPARE(count3, 1);
    
    m_hub->unsubscribe(id1);
    m_hub->unsubscribe(id2);
    m_hub->unsubscribe(id3);
}

void DataHubTest::testUnsubscribe()
{
    int callCount = 0;
    
    auto subId = m_hub->subscribe("unsub:topic", [&](const QVariant&) {
        callCount++;
    });
    
    m_hub->publish("unsub:topic", QVariant(1));
    QTest::qWait(10);
    QCOMPARE(callCount, 1);
    
    // 取消订阅
    m_hub->unsubscribe(subId);
    
    m_hub->publish("unsub:topic", QVariant(2));
    QTest::qWait(10);
    
    // 不应再收到消息
    QCOMPARE(callCount, 1);
}

void DataHubTest::testWildcardSubscribe()
{
    QStringList received;
    
    // 订阅通配符
    auto subId = m_hub->subscribe("market:quote:*", [&](const QVariant& data) {
        received << data.toString();
    });
    
    m_hub->publish("market:quote:sh600519", QVariant("茅台"));
    m_hub->publish("market:quote:sz000001", QVariant("平安"));
    QTest::qWait(10);
    
    QCOMPARE(received.size(), 2);
    QVERIFY(received.contains("茅台"));
    QVERIFY(received.contains("平安"));
    
    m_hub->unsubscribe(subId);
}

void DataHubTest::testPatternMatch()
{
    QStringList received;
    
    auto subId = m_hub->subscribe("stock:*:price", [&](const QVariant& data) {
        received << data.toString();
    });
    
    m_hub->publish("stock:sh600519:price", QVariant("1800"));
    m_hub->publish("stock:sz000001:price", QVariant("10"));
    m_hub->publish("stock:sh600519:volume", QVariant("10000")); // 不匹配
    QTest::qWait(10);
    
    QCOMPARE(received.size(), 2);
    
    m_hub->unsubscribe(subId);
}

void DataHubTest::testTTL()
{
    // 发布带 TTL 的数据
    m_hub->publish("ttl:topic", QVariant("data"), 1);
    
    QVERIFY(m_hub->hasData("ttl:topic"));
    
    // 等待过期
    QTest::qWait(1100);
    
    QVERIFY(!m_hub->hasData("ttl:topic"));
}

void DataHubTest::testCleanup()
{
    // 发布多个带 TTL 的数据
    for (int i = 0; i < 10; ++i) {
        m_hub->publish(QString("cleanup:%1").arg(i), QVariant(i), 1);
    }
    
    QTest::qWait(1100);
    
    // 清理过期数据
    m_hub->cleanupExpired();
    
    for (int i = 0; i < 10; ++i) {
        QVERIFY(!m_hub->hasData(QString("cleanup:%1").arg(i)));
    }
}

void DataHubTest::testPublishPerformance()
{
    // 预订阅
    auto subId = m_hub->subscribe("perf:topic", [&](const QVariant&) {});
    
    QBENCHMARK {
        for (int i = 0; i < 100; ++i) {
            m_hub->publish("perf:topic", QVariant(i));
        }
        QTest::qWait(10);
    }
    
    m_hub->unsubscribe(subId);
}

QTEST_MAIN(DataHubTest)
#include "DataHubTest.moc"
