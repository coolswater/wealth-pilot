/**
 * @file DataHubTest.cpp
 * @brief DataHub 数据中心单元测试
 *
 * @details 测试范围：
 * - Topic 订阅/取消订阅
 * - 数据发布和缓存
 * - 模式匹配订阅
 * - 生命周期管理
 * - 缓存过期机制
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include <QtTest>
#include <QSignalSpy>
#include "src/core/datahub/DataHub.h"
#include "src/core/types/MarketTypes.h"

using namespace WealthPilot::DataHub;

/**
 * @brief DataHub 单元测试类
 */
class DataHubTest : public QObject
{
    Q_OBJECT

private slots:
    // ========== 测试初始化 ==========
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // ========== 基础功能测试 ==========
    void testSingleton();
    void testSubscribe();
    void testPublish();
    void testUnsubscribe();
    void testSubscribePattern();

    // ========== 缓存测试 ==========
    void testCacheHit();
    void testCacheMiss();
    void testCacheExpiry();
    void testPeek();

    // ========== 生命周期测试 ==========
    void testOwnerDestroyed();
    void testMultipleSubscribers();
    void testTopicIdle();

    // ========== 性能测试 ==========
    void testSubscribePerformance();
    void testPublishPerformance();

    // ========== 边界条件测试 ==========
    void testEmptyTopic();
    void testNullOwner();
    void testInvalidData();

private:
    DataHub* m_dataHub = nullptr;
    QObject* m_testOwner = nullptr;
};

// ============================================================================
// 测试初始化
// ============================================================================

void DataHubTest::initTestCase()
{
    // 测试套件初始化
    qDebug() << "DataHub Test Suite Started";
}

void DataHubTest::cleanupTestCase()
{
    // 测试套件清理
    qDebug() << "DataHub Test Suite Finished";
}

void DataHubTest::init()
{
    // 每个测试用例前初始化
    m_dataHub = &DataHub::instance();
    m_testOwner = new QObject();
}

void DataHubTest::cleanup()
{
    // 每个测试用例后清理
    if (m_testOwner) {
        delete m_testOwner;
        m_testOwner = nullptr;
    }
}

// ============================================================================
// 基础功能测试
// ============================================================================

void DataHubTest::testSingleton()
{
    // 验证单例模式
    DataHub& instance1 = DataHub::instance();
    DataHub& instance2 = DataHub::instance();
    QCOMPARE(&instance1, &instance2);
}

void DataHubTest::testSubscribe()
{
    QString topic = "test:quote:AAPL";
    bool callbackCalled = false;
    QVariant receivedValue;

    // 订阅 topic
    m_dataHub->subscribe(m_testOwner, topic,
        [&](const QVariant& value) {
            callbackCalled = true;
            receivedValue = value;
        });

    // 发布数据
    QVariant testData = 42.5;
    m_dataHub->publish(topic, testData);

    // 验证回调被触发
    QVERIFY(callbackCalled);
    QCOMPARE(receivedValue.toDouble(), 42.5);
}

void DataHubTest::testPublish()
{
    QString topic = "test:publish:topic";
    QSignalSpy spy(m_dataHub, &DataHub::topicUpdated);

    // 发布数据
    QVariant data = QString("test_data");
    m_dataHub->publish(topic, data);

    // 验证信号发射
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), topic);
}

void DataHubTest::testUnsubscribe()
{
    QString topic = "test:unsub:topic";
    int callCount = 0;

    // 订阅
    m_dataHub->subscribe(m_testOwner, topic,
        [&](const QVariant&) { callCount++; });

    // 发布数据
    m_dataHub->publish(topic, 1);
    QCOMPARE(callCount, 1);

    // 取消订阅
    m_dataHub->unsubscribe(m_testOwner, topic);

    // 再次发布，不应触发回调
    m_dataHub->publish(topic, 2);
    QCOMPARE(callCount, 1); // 仍然是 1
}

void DataHubTest::testSubscribePattern()
{
    QString pattern = "market:quote:*";
    QStringList receivedTopics;

    // 模式订阅
    m_dataHub->subscribePattern(m_testOwner, pattern,
        [&](const QString& topic, const QVariant&) {
            receivedTopics.append(topic);
        });

    // 发布多个匹配的 topic
    m_dataHub->publish("market:quote:AAPL", 100.0);
    m_dataHub->publish("market:quote:GOOG", 200.0);
    m_dataHub->publish("market:quote:MSFT", 300.0);

    // 验证所有 topic 都被接收
    QCOMPARE(receivedTopics.size(), 3);
    QVERIFY(receivedTopics.contains("market:quote:AAPL"));
    QVERIFY(receivedTopics.contains("market:quote:GOOG"));
    QVERIFY(receivedTopics.contains("market:quote:MSFT"));
}

// ============================================================================
// 缓存测试
// ============================================================================

void DataHubTest::testCacheHit()
{
    QString topic = "test:cache:hit";

    // 第一次发布
    m_dataHub->publish(topic, 100.0);

    // 订阅（应该立即收到缓存数据）
    bool callbackCalled = false;
    m_dataHub->subscribe(m_testOwner, topic,
        [&](const QVariant& value) {
            callbackCalled = true;
            QCOMPARE(value.toDouble(), 100.0);
        });

    QVERIFY(callbackCalled);
}

void DataHubTest::testCacheMiss()
{
    QString topic = "test:cache:miss:new";

    // 订阅一个从未发布过的 topic
    bool callbackCalled = false;
    m_dataHub->subscribe(m_testOwner, topic,
        [&](const QVariant&) {
            callbackCalled = true;
        });

    // 不应该触发回调（无缓存数据）
    QVERIFY(!callbackCalled);
}

void DataHubTest::testCacheExpiry()
{
    QString topic = "test:cache:expiry";

    // 设置短 TTL 策略
    TopicPolicy policy;
    policy.ttlMs = 100; // 100ms 过期
    m_dataHub->setPolicy(topic, policy);

    // 发布数据
    m_dataHub->publish(topic, 50.0);

    // 立即订阅应该收到缓存
    bool immediateCallback = false;
    m_dataHub->subscribe(m_testOwner, topic,
        [&](const QVariant&) { immediateCallback = true; });
    QVERIFY(immediateCallback);

    // 等待缓存过期
    QTest::qWait(150);

    // 取消订阅后重新订阅
    m_dataHub->unsubscribe(m_testOwner, topic);
    bool expiredCallback = false;
    m_dataHub->subscribe(m_testOwner, topic,
        [&](const QVariant&) { expiredCallback = true; });

    // 过期后不应该触发回调
    QVERIFY(!expiredCallback);
}

void DataHubTest::testPeek()
{
    QString topic = "test:peek:value";

    // 发布数据
    m_dataHub->publish(topic, 123.45);

    // 使用 peek 读取（不订阅）
    QVariant value = m_dataHub->peek(topic);
    QVERIFY(value.isValid());
    QCOMPARE(value.toDouble(), 123.45);
}

// ============================================================================
// 生命周期测试
// ============================================================================

void DataHubTest::testOwnerDestroyed()
{
    QString topic = "test:lifecycle:destroy";

    // 创建临时 owner
    QObject* tempOwner = new QObject();
    int callCount = 0;

    m_dataHub->subscribe(tempOwner, topic,
        [&](const QVariant&) { callCount++; });

    // 发布数据
    m_dataHub->publish(topic, 1);
    QCOMPARE(callCount, 1);

    // 销毁 owner
    delete tempOwner;

    // 再次发布，不应触发回调
    m_dataHub->publish(topic, 2);
    QCOMPARE(callCount, 1); // 仍然是 1
}

void DataHubTest::testMultipleSubscribers()
{
    QString topic = "test:multi:sub";

    int count1 = 0, count2 = 0, count3 = 0;
    QObject* owner1 = new QObject();
    QObject* owner2 = new QObject();
    QObject* owner3 = new QObject();

    // 三个订阅者
    m_dataHub->subscribe(owner1, topic, [&](const QVariant&) { count1++; });
    m_dataHub->subscribe(owner2, topic, [&](const QVariant&) { count2++; });
    m_dataHub->subscribe(owner3, topic, [&](const QVariant&) { count3++; });

    // 发布数据
    m_dataHub->publish(topic, 1);

    // 所有订阅者都应该收到
    QCOMPARE(count1, 1);
    QCOMPARE(count2, 1);
    QCOMPARE(count3, 1);

    // 清理
    delete owner1;
    delete owner2;
    delete owner3;
}

void DataHubTest::testTopicIdle()
{
    QString topic = "test:idle:topic";
    QSignalSpy spy(m_dataHub, &DataHub::topicIdle);

    QObject* owner = new QObject();
    m_dataHub->subscribe(owner, topic, [](const QVariant&) {});

    // 销毁唯一的订阅者
    delete owner;

    // 应该触发 topicIdle 信号
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), topic);
}

// ============================================================================
// 性能测试
// ============================================================================

void DataHubTest::testSubscribePerformance()
{
    // 测试大量订阅的性能
    QBENCHMARK {
        for (int i = 0; i < 1000; ++i) {
            QString topic = QString("perf:sub:%1").arg(i);
            m_dataHub->subscribe(m_testOwner, topic, [](const QVariant&) {});
        }
    }
}

void DataHubTest::testPublishPerformance()
{
    // 先订阅
    QString topic = "perf:pub:test";
    m_dataHub->subscribe(m_testOwner, topic, [](const QVariant&) {});

    // 测试发布性能
    QBENCHMARK {
        m_dataHub->publish(topic, 42.0);
    }
}

// ============================================================================
// 边界条件测试
// ============================================================================

void DataHubTest::testEmptyTopic()
{
    // 空 topic 不应该崩溃
    m_dataHub->subscribe(m_testOwner, "", [](const QVariant&) {});
    m_dataHub->publish("", QVariant());
    // 测试通过即表示没有崩溃
    QVERIFY(true);
}

void DataHubTest::testNullOwner()
{
    // null owner 不应该崩溃
    m_dataHub->subscribe(nullptr, "test:null", [](const QVariant&) {});
    QVERIFY(true);
}

void DataHubTest::testInvalidData()
{
    QString topic = "test:invalid";

    // 发布无效数据
    m_dataHub->publish(topic, QVariant());

    // 订阅应该能处理无效数据
    bool callbackCalled = false;
    m_dataHub->subscribe(m_testOwner, topic,
        [&](const QVariant& value) {
            callbackCalled = true;
            QVERIFY(!value.isValid());
        });

    QVERIFY(callbackCalled);
}

// ============================================================================
// 主函数
// ============================================================================

QTEST_MAIN(DataHubTest)
#include "DataHubTest.moc"
