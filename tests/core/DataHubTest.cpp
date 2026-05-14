/**
 * @file DataHubTest.cpp
 * @brief DataHub 单元测试实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "DataHubTest.h"
#include <QTimer>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QDebug>

void DataHubTest::initTestCase()
{
    m_hub = &DataHub::instance();
    QVERIFY(m_hub != nullptr);
    m_callbackCount = 0;
}

void DataHubTest::cleanupTestCase()
{
    // DataHub 是单例，不需要手动删除
}

void DataHubTest::testSingleton()
{
    // 测试单例模式
    DataHub& instance1 = DataHub::instance();
    DataHub& instance2 = DataHub::instance();
    QCOMPARE(&instance1, &instance2);
}

void DataHubTest::testSubscribePublish()
{
    m_callbackCount = 0;
    QString testTopic = "test:quote:AAPL";
    QString testData = "150.25";

    // 订阅
    m_hub->subscribe(this, testTopic, [this](const QString& topic, const QVariant& value) {
        Q_UNUSED(topic)
        if (value.toString() == "150.25") {
            m_callbackCount++;
        }
    });

    // 发布
    m_hub->publish(testTopic, testData);

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(m_callbackCount, 1);

    // 清理
    m_hub->unsubscribeAll(this);
}

void DataHubTest::testUnsubscribe()
{
    m_callbackCount = 0;
    QString testTopic = "test:quote:MSFT";

    // 订阅
    m_hub->subscribe(this, testTopic, [this](const QString&, const QVariant&) {
        m_callbackCount++;
    });

    // 取消订阅
    m_hub->unsubscribeAll(this);

    // 发布
    m_hub->publish(testTopic, "100.0");

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    // 应该没有被调用
    QCOMPARE(m_callbackCount, 0);
}

void DataHubTest::testPatternMatching()
{
    m_callbackCount = 0;

    // 订阅模式
    m_hub->subscribePattern(this, "market:quote:*", [this](const QString& topic, const QVariant&) {
        Q_UNUSED(topic)
        m_callbackCount++;
    });

    // 发布匹配的消息
    m_hub->publish("market:quote:AAPL", "150.0");
    m_hub->publish("market:quote:GOOGL", "2800.0");
    m_hub->publish("market:quote:MSFT", "300.0");

    // 发布不匹配的消息
    m_hub->publish("market:futures:CL", "75.0");

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    // 应该收到 3 个匹配的消息
    QCOMPARE(m_callbackCount, 3);

    // 清理
    m_hub->unsubscribeAll(this);
}

void DataHubTest::testDataCache()
{
    QString testTopic = "cache:test:data";

    // 发布数据
    m_hub->publish(testTopic, "cached_value");

    // 等待缓存
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    // 获取缓存数据
    QVariant cached = m_hub->getCached(testTopic);
    QCOMPARE(cached.toString(), QString("cached_value"));
}

void DataHubTest::testLifecycle()
{
    // 创建一个临时对象作为 owner
    QObject* owner = new QObject();

    int callCount = 0;
    QString testTopic = "lifecycle:test";

    // 使用 owner 订阅
    m_hub->subscribe(owner, testTopic, [&callCount](const QString&, const QVariant&) {
        callCount++;
    });

    // 发布消息
    m_hub->publish(testTopic, "test");
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();
    QCOMPARE(callCount, 1);

    // 删除 owner，应该自动取消订阅
    delete owner;

    // 再次发布
    m_hub->publish(testTopic, "test2");
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    // callCount 不应该增加
    QCOMPARE(callCount, 1);
}

void DataHubTest::testPerformanceManySubscriptions()
{
    QElapsedTimer timer;
    timer.start();

    // 创建 1000 个订阅
    for (int i = 0; i < 1000; ++i) {
        QString topic = QString("perf:test:%1").arg(i);
        m_hub->subscribe(this, topic, [](const QString&, const QVariant&) {});
    }

    qint64 subscribeTime = timer.elapsed();
    qDebug() << "1000 subscriptions took" << subscribeTime << "ms";

    // 订阅时间应该小于 100ms
    QVERIFY(subscribeTime < 100);

    // 清理
    m_hub->unsubscribeAll(this);
}

void DataHubTest::testPerformanceHighFrequency()
{
    m_callbackCount = 0;
    QString testTopic = "perf:highfreq";

    m_hub->subscribe(this, testTopic, [this](const QString&, const QVariant&) {
        m_callbackCount++;
    });

    QElapsedTimer timer;
    timer.start();

    // 发布 10000 条消息
    for (int i = 0; i < 10000; ++i) {
        m_hub->publish(testTopic, i);
    }

    // 等待所有回调
    QEventLoop loop;
    QTimer::singleShot(500, &loop, &QEventLoop::quit);
    loop.exec();

    qint64 publishTime = timer.elapsed();
    qDebug() << "10000 publishes took" << publishTime << "ms";
    qDebug() << "Callbacks received:" << m_callbackCount;

    // 发布时间应该小于 500ms
    QVERIFY(publishTime < 500);

    // 清理
    m_hub->unsubscribeAll(this);
}

void DataHubTest::testMemoryUsage()
{
    // 获取初始内存（这里只是占位，实际需要更精确的内存测量）
    qDebug() << "Testing memory usage...";

    // 创建大量订阅
    for (int i = 0; i < 10000; ++i) {
        QString topic = QString("memory:test:%1").arg(i);
        m_hub->subscribe(this, topic, [](const QString&, const QVariant&) {});
    }

    // 发布一些数据
    for (int i = 0; i < 1000; ++i) {
        QString topic = QString("memory:test:%1").arg(i);
        m_hub->publish(topic, QString("data_%1").arg(i));
    }

    qDebug() << "Memory test completed";

    // 清理
    m_hub->unsubscribeAll(this);
}

// Qt Test 主函数
QTEST_MAIN(DataHubTest)
#include "DataHubTest.moc"