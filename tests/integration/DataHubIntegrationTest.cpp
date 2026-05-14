/**
 * @file DataHubIntegrationTest.cpp
 * @brief DataHub 集成测试实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "DataHubIntegrationTest.h"
#include "core/datahub/DataHub.h"
#include "core/datahub/DataHubBootstrap.h"

#include <QTimer>
#include <QEventLoop>
#include <QDebug>

void DataHubIntegrationTest::initTestCase()
{
    // 初始化 DataHub
    DataHubBootstrap::initialize();
    qDebug() << "DataHub initialized for integration tests";
}

void DataHubIntegrationTest::cleanupTestCase()
{
    // 清理 DataHub
    DataHubBootstrap::shutdown();
    qDebug() << "DataHub shutdown completed";
}

// ========== 页面集成测试 ==========

void DataHubIntegrationTest::testStockQuotesPageIntegration()
{
    DataHub& hub = DataHub::instance();

    // 模拟页面订阅
    QObject pageOwner;
    QStringList symbols = {"sh600000", "sh600519", "sz000001"};
    int updateCount = 0;

    for (const QString& symbol : symbols) {
        hub.subscribe(&pageOwner, QString("market:quote:%1").arg(symbol),
            [&updateCount](const QString&, const QVariant&) {
                updateCount++;
            });
    }

    // 发布行情数据
    for (const QString& symbol : symbols) {
        hub.publish(QString("market:quote:%1").arg(symbol), createTestQuote(symbol, 10.0 + qrand() % 100));
    }

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(updateCount, symbols.size());

    // 清理
    hub.unsubscribeAll(&pageOwner);
}

void DataHubIntegrationTest::testWatchListPageIntegration()
{
    DataHub& hub = DataHub::instance();

    // 模拟自选股页面
    QObject watchlistPage;
    QStringList watchlist = {"sh600519", "sh601318", "sz000858"};
    QHash<QString, double> prices;

    // 订阅自选股
    for (const QString& symbol : watchlist) {
        hub.subscribe(&watchlistPage, QString("market:quote:%1").arg(symbol),
            [&prices, symbol](const QString&, const QVariant& value) {
                // 模拟更新价格
                prices[symbol] = value.toDouble();
            });
    }

    // 发布价格更新
    hub.publish("market:quote:sh600519", 1800.0);
    hub.publish("market:quote:sh601318", 45.0);
    hub.publish("market:quote:sz000858", 160.0);

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    // 验证
    QCOMPARE(prices.size(), 3);
    QCOMPARE(prices["sh600519"], 1800.0);

    // 清理
    hub.unsubscribeAll(&watchlistPage);
}

void DataHubIntegrationTest::testDashboardPageIntegration()
{
    DataHub& hub = DataHub::instance();

    // 模拟仪表盘页面
    QObject dashboardPage;
    int indexUpdateCount = 0;
    int rankUpdateCount = 0;

    // 订阅指数
    hub.subscribe(&dashboardPage, "market:index:sh000001",
        [&indexUpdateCount](const QString&, const QVariant&) {
            indexUpdateCount++;
        });

    // 订阅排行榜
    hub.subscribePattern(&dashboardPage, "market:rank:*",
        [&rankUpdateCount](const QString&, const QVariant&) {
            rankUpdateCount++;
        });

    // 发布数据
    hub.publish("market:index:sh000001", 3100.0);
    hub.publish("market:rank:gain", "top_gainers");
    hub.publish("market:rank:loss", "top_losers");

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(indexUpdateCount, 1);
    QCOMPARE(rankUpdateCount, 2);

    // 清理
    hub.unsubscribeAll(&dashboardPage);
}

void DataHubIntegrationTest::testPortfolioPageIntegration()
{
    DataHub& hub = DataHub::instance();

    // 模拟持仓页面
    QObject portfolioPage;
    double totalValue = 0.0;

    // 订阅持仓
    hub.subscribe(&portfolioPage, "position:update",
        [&totalValue](const QString&, const QVariant& value) {
            totalValue += value.toDouble();
        });

    // 订阅账户
    hub.subscribe(&portfolioPage, "account:balance",
        [&totalValue](const QString&, const QVariant& value) {
            totalValue = value.toDouble();
        });

    // 发布持仓更新
    hub.publish("position:update", 10000.0);
    hub.publish("account:balance", 50000.0);

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(totalValue, 50000.0);

    // 清理
    hub.unsubscribeAll(&portfolioPage);
}

// ========== 数据流集成测试 ==========

void DataHubIntegrationTest::testMarketDataFlow()
{
    DataHub& hub = DataHub::instance();

    // 模拟完整的数据流：Producer -> DataHub -> Consumer

    QObject consumer;
    QStringList receivedSymbols;

    // Consumer 订阅
    hub.subscribePattern(&consumer, "market:quote:*",
        [&receivedSymbols](const QString& topic, const QVariant&) {
            // 提取股票代码
            QStringList parts = topic.split(":");
            if (parts.size() >= 3) {
                receivedSymbols.append(parts[2]);
            }
        });

    // Producer 发布
    QStringList symbols = {"sh600000", "sh600519", "sz000001", "sz000858"};
    for (const QString& symbol : symbols) {
        hub.publish(QString("market:quote:%1").arg(symbol), createTestQuote(symbol, 100.0));
    }

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(receivedSymbols.size(), symbols.size());

    // 清理
    hub.unsubscribeAll(&consumer);
}

void DataHubIntegrationTest::testNewsDataFlow()
{
    DataHub& hub = DataHub::instance();

    QObject newsConsumer;
    int newsCount = 0;

    // 订阅新闻
    hub.subscribePattern(&newsConsumer, "news:*",
        [&newsCount](const QString&, const QVariant&) {
            newsCount++;
        });

    // 发布新闻
    hub.publish("news:market", "市场新闻1");
    hub.publish("news:symbol:sh600519", "茅台新闻");
    hub.publish("news:category:要闻", "重要新闻");

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(newsCount, 3);

    // 清理
    hub.unsubscribeAll(&newsConsumer);
}

void DataHubIntegrationTest::testTradeDataFlow()
{
    DataHub& hub = DataHub::instance();

    QObject tradeConsumer;
    QStringList tradeEvents;

    // 订阅交易事件
    hub.subscribePattern(&tradeConsumer, "trade:*",
        [&tradeEvents](const QString& topic, const QVariant&) {
            tradeEvents.append(topic);
        });

    // 发布交易事件
    hub.publish("trade:new", "新订单");
    hub.publish("trade:filled", "订单成交");
    hub.publish("trade:cancelled", "订单取消");

    // 等待回调
    QEventLoop loop;
    QTimer::singleShot(100, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(tradeEvents.size(), 3);

    // 清理
    hub.unsubscribeAll(&tradeConsumer);
}

// ========== 生命周期集成测试 ==========

void DataHubIntegrationTest::testPageLifecycle()
{
    DataHub& hub = DataHub::instance();

    // 模拟页面创建、激活、停用、销毁

    // 1. 创建页面
    QObject* page1 = new QObject();
    int callCount = 0;

    // 2. 页面激活时订阅
    hub.subscribe(page1, "test:lifecycle", [&callCount](const QString&, const QVariant&) {
        callCount++;
    });

    // 3. 发布数据
    hub.publish("test:lifecycle", "data1");
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();
    QCOMPARE(callCount, 1);

    // 4. 页面销毁
    delete page1;

    // 5. 再次发布，不应收到回调
    hub.publish("test:lifecycle", "data2");
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();
    QCOMPARE(callCount, 1); // 仍然是 1，说明订阅已自动清理
}

void DataHubIntegrationTest::testSubscriptionAutoCleanup()
{
    DataHub& hub = DataHub::instance();

    // 创建多个临时页面
    for (int i = 0; i < 10; ++i) {
        QObject* tempPage = new QObject();
        hub.subscribe(tempPage, QString("test:cleanup:%1").arg(i),
            [](const QString&, const QVariant&) {});
        // 立即删除
        delete tempPage;
    }

    // 验证订阅已被清理
    // 发布数据，不应有任何回调
    int callbackCount = 0;
    QObject monitor;
    hub.subscribe(&monitor, "test:cleanup:monitor", [&callbackCount](const QString&, const QVariant&) {
        callbackCount++;
    });

    for (int i = 0; i < 10; ++i) {
        hub.publish(QString("test:cleanup:%1").arg(i), "data");
    }

    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    // monitor 不应收到任何回调
    QCOMPARE(callbackCount, 0);

    hub.unsubscribeAll(&monitor);
}

void DataHubIntegrationTest::testMultiPageSubscriptionManagement()
{
    DataHub& hub = DataHub::instance();

    // 创建多个页面订阅同一 Topic
    QObject page1, page2, page3;
    int count1 = 0, count2 = 0, count3 = 0;

    hub.subscribe(&page1, "test:multi", [&count1](const QString&, const QVariant&) { count1++; });
    hub.subscribe(&page2, "test:multi", [&count2](const QString&, const QVariant&) { count2++; });
    hub.subscribe(&page3, "test:multi", [&count3](const QString&, const QVariant&) { count3++; });

    // 发布一次，所有页面都应收到
    hub.publish("test:multi", "data");

    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(count1, 1);
    QCOMPARE(count2, 1);
    QCOMPARE(count3, 1);

    // 删除 page2
    delete &page2;

    // 再次发布，只有 page1 和 page3 收到
    hub.publish("test:multi", "data2");
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(count1, 2);
    QCOMPARE(count3, 2);
    // count2 仍然是 1

    hub.unsubscribeAll(&page1);
    hub.unsubscribeAll(&page3);
}

// ========== 多页面协同测试 ==========

void DataHubIntegrationTest::testMultiPageDataSync()
{
    DataHub& hub = DataHub::instance();

    // 模拟自选股页面和行情页面同步
    QObject watchlistPage, quotesPage;
    double latestPrice = 0.0;

    // 自选股页面订阅
    hub.subscribe(&watchlistPage, "market:quote:sh600519",
        [&latestPrice](const QString&, const QVariant& value) {
            latestPrice = value.toDouble();
        });

    // 行情页面也订阅
    hub.subscribe(&quotesPage, "market:quote:sh600519",
        [&latestPrice](const QString&, const QVariant& value) {
            latestPrice = value.toDouble();
        });

    // 发布价格
    hub.publish("market:quote:sh600519", 1800.0);

    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    // 两个页面应该同步到相同的价格
    QCOMPARE(latestPrice, 1800.0);

    hub.unsubscribeAll(&watchlistPage);
    hub.unsubscribeAll(&quotesPage);
}

void DataHubIntegrationTest::testPageSwitchDataPersistence()
{
    DataHub& hub = DataHub::instance();

    // 模拟页面切换时数据保持
    QObject page1;
    QString lastData;

    hub.subscribe(&page1, "test:persist", [&lastData](const QString&, const QVariant& value) {
        lastData = value.toString();
    });

    // 发布数据
    hub.publish("test:persist", "data1");
    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();
    QCOMPARE(lastData, "data1");

    // 获取缓存数据
    QVariant cached = hub.getCached("test:persist");
    QCOMPARE(cached.toString(), "data1");

    hub.unsubscribeAll(&page1);
}

void DataHubIntegrationTest::testCrossPageEventDelivery()
{
    DataHub& hub = DataHub::instance();

    // 模拟跨页面事件传递
    QObject senderPage, receiverPage;
    bool eventReceived = false;

    // 接收页面订阅事件
    hub.subscribe(&receiverPage, "event:navigate",
        [&eventReceived](const QString&, const QVariant&) {
            eventReceived = true;
        });

    // 发送页面发布事件
    hub.publish("event:navigate", "goto:kline:sh600519");

    QEventLoop loop;
    QTimer::singleShot(50, &loop, &QEventLoop::quit);
    loop.exec();

    QVERIFY(eventReceived);

    hub.unsubscribeAll(&senderPage);
    hub.unsubscribeAll(&receiverPage);
}

// ========== 辅助方法 ==========

bool DataHubIntegrationTest::waitForSignal(QObject* obj, const char* signal, int timeoutMs)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    connect(obj, signal, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timer.start(timeoutMs);
    loop.exec();

    return timer.isActive();
}

QVariant DataHubIntegrationTest::createTestQuote(const QString& symbol, double price)
{
    QVariantMap map;
    map["symbol"] = symbol;
    map["price"] = price;
    map["change"] = 0.0;
    map["changePercent"] = 0.0;
    map["volume"] = 1000000;
    return map;
}

// Qt Test 主函数
QTEST_MAIN(DataHubIntegrationTest)
#include "DataHubIntegrationTest.moc"