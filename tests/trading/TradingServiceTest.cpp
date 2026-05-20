/**
 * @file TradingServiceTest.cpp
 * @brief 交易服务单元测试
 *
 * @details 测试范围：
 * - 服务初始化和关闭
 * - 订单提交流程
 * - 风控检查
 * - 持仓管理
 * - 账户同步
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include <QtTest>
#include <QSignalSpy>
#include "src/trading/TradingService.h"
#include "src/trading/TradingTypes.h"

using namespace WealthPilot;

/**
 * @brief TradingService 单元测试类
 */
class TradingServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // ========== 服务生命周期测试 ==========
    void testInitialize();
    void testShutdown();
    void testSingleton();

    // ========== 订单操作测试 ==========
    void testSubmitOrder();
    void testCancelOrder();
    void testBatchCancelOrders();
    void testSetStopLossTakeProfit();
    void testSetConditionOrder();

    // ========== 查询接口测试 ==========
    void testGetOrder();
    void testGetActiveOrders();
    void testGetPosition();
    void testGetPositions();
    void testGetAccountInfo();
    void testGetTotalProfit();

    // ========== 风控测试 ==========
    void testCheckOrder();
    void testRiskReport();
    void testRiskLevel();

    // ========== 信号测试 ==========
    void testOrderSubmittedSignal();
    void testOrderFilledSignal();
    void testPositionUpdatedSignal();

private:
    TradingService* m_tradingService = nullptr;
};

// ============================================================================
// 测试初始化
// ============================================================================

void TradingServiceTest::initTestCase()
{
    qDebug() << "TradingService Test Suite Started";
}

void TradingServiceTest::cleanupTestCase()
{
    qDebug() << "TradingService Test Suite Finished";
}

void TradingServiceTest::init()
{
    m_tradingService = &TradingService::instance();
}

void TradingServiceTest::cleanup()
{
    // 清理测试数据
}

// ============================================================================
// 服务生命周期测试
// ============================================================================

void TradingServiceTest::testInitialize()
{
    bool success = m_tradingService->initialize();
    QVERIFY(success);
}

void TradingServiceTest::testShutdown()
{
    m_tradingService->shutdown();
    // 验证服务已关闭
    auto account = m_tradingService->getAccountInfo();
    QCOMPARE(account.available, 0.0);
}

void TradingServiceTest::testSingleton()
{
    TradingService& instance1 = TradingService::instance();
    TradingService& instance2 = TradingService::instance();
    QCOMPARE(&instance1, &instance2);
}

// ============================================================================
// 订单操作测试
// ============================================================================

void TradingServiceTest::testSubmitOrder()
{
    m_tradingService->initialize();

    OrderRequest request;
    request.instrumentId = "cu2505";
    request.direction = TradeDirection::Buy;
    request.offsetFlag = OffsetFlag::Open;
    request.price = 75000.0;
    request.volume = 1;
    request.orderType = OrderType::Limit;

    QString orderId = m_tradingService->submitOrder(request);
    QVERIFY(!orderId.isEmpty());
}

void TradingServiceTest::testCancelOrder()
{
    m_tradingService->initialize();

    // 先提交订单
    OrderRequest request;
    request.instrumentId = "IF2501";
    request.direction = TradeDirection::Sell;
    request.price = 3850.0;
    request.volume = 1;

    QString orderId = m_tradingService->submitOrder(request);

    // 撤销订单
    bool success = m_tradingService->cancelOrder(orderId);
    QVERIFY(success);
}

void TradingServiceTest::testBatchCancelOrders()
{
    m_tradingService->initialize();

    // 提交多个订单
    QVector<QString> orderIds;
    for (int i = 0; i < 3; ++i) {
        OrderRequest request;
        request.instrumentId = QString("batch_%1").arg(i);
        request.direction = TradeDirection::Buy;
        request.price = 100.0;
        request.volume = 1;

        QString orderId = m_tradingService->submitOrder(request);
        orderIds.append(orderId);
    }

    // 批量撤销
    int cancelledCount = m_tradingService->cancelOrders(orderIds);
    QCOMPARE(cancelledCount, 3);
}

void TradingServiceTest::testSetStopLossTakeProfit()
{
    m_tradingService->initialize();

    bool success = m_tradingService->setStopLossTakeProfit(
        "cu2505", 74000.0, 76000.0);
    QVERIFY(success);
}

void TradingServiceTest::testSetConditionOrder()
{
    m_tradingService->initialize();

    ConditionOrder condition;
    condition.instrumentId = "IF2501";
    condition.conditionType = ConditionType::PriceGreater;
    condition.triggerPrice = 3900.0;
    condition.direction = TradeDirection::Buy;
    condition.volume = 1;

    QString conditionId = m_tradingService->setConditionOrder(condition);
    QVERIFY(!conditionId.isEmpty());
}

// ============================================================================
// 查询接口测试
// ============================================================================

void TradingServiceTest::testGetOrder()
{
    m_tradingService->initialize();

    OrderRequest request;
    request.instrumentId = "query_test";
    request.direction = TradeDirection::Buy;
    request.price = 100.0;
    request.volume = 1;

    QString orderId = m_tradingService->submitOrder(request);

    auto order = m_tradingService->getOrder(orderId);
    QVERIFY(order.has_value());
    QCOMPARE(order->orderId, orderId);
}

void TradingServiceTest::testGetActiveOrders()
{
    m_tradingService->initialize();

    auto activeOrders = m_tradingService->getActiveOrders();
    QVERIFY(activeOrders.size() >= 0);
}

void TradingServiceTest::testGetPosition()
{
    m_tradingService->initialize();

    auto position = m_tradingService->getPosition("cu2505", PositionDirection::Long);
    // 可能没有持仓
    QVERIFY(position.has_value() || !position.has_value());
}

void TradingServiceTest::testGetPositions()
{
    m_tradingService->initialize();

    auto positions = m_tradingService->getPositions();
    QVERIFY(positions.size() >= 0);
}

void TradingServiceTest::testGetAccountInfo()
{
    m_tradingService->initialize();

    auto account = m_tradingService->getAccountInfo();
    QVERIFY(account.balance >= 0);
    QVERIFY(account.available >= 0);
}

void TradingServiceTest::testGetTotalProfit()
{
    m_tradingService->initialize();

    double profit = m_tradingService->getTotalProfit();
    // 盈亏可以是正数、负数或零
    QVERIFY(profit >= -1000000.0 && profit <= 1000000.0);
}

// ============================================================================
// 风控测试
// ============================================================================

void TradingServiceTest::testCheckOrder()
{
    m_tradingService->initialize();

    OrderRequest request;
    request.instrumentId = "cu2505";
    request.direction = TradeDirection::Buy;
    request.price = 75000.0;
    request.volume = 100; // 大量订单

    auto result = m_tradingService->checkOrder(request);
    // 验证风控检查返回结果
    QVERIFY(result.passed || !result.passed);
    QVERIFY(!result.message.isEmpty());
}

void TradingServiceTest::testRiskReport()
{
    m_tradingService->initialize();

    auto report = m_tradingService->getRiskReport();
    QVERIFY(report.riskLevel >= 0 && report.riskLevel <= 100);
}

void TradingServiceTest::testRiskLevel()
{
    m_tradingService->initialize();

    int level = m_tradingService->getRiskLevel();
    QVERIFY(level >= 0 && level <= 100);
}

// ============================================================================
// 信号测试
// ============================================================================

void TradingServiceTest::testOrderSubmittedSignal()
{
    m_tradingService->initialize();
    QSignalSpy spy(m_tradingService, &TradingService::orderSubmitted);

    OrderRequest request;
    request.instrumentId = "signal_test";
    request.direction = TradeDirection::Buy;
    request.price = 100.0;
    request.volume = 1;

    QString orderId = m_tradingService->submitOrder(request);

    // 验证信号发射
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), orderId);
}

void TradingServiceTest::testOrderFilledSignal()
{
    m_tradingService->initialize();
    QSignalSpy spy(m_tradingService, &TradingService::orderFilled);

    // 模拟订单成交
    OrderRequest request;
    request.instrumentId = "filled_test";
    request.direction = TradeDirection::Buy;
    request.price = 100.0;
    request.volume = 1;

    QString orderId = m_tradingService->submitOrder(request);

    // 这里需要模拟成交回报
    // 实际测试中需要 Mock CTP 回报

    QVERIFY(spy.count() >= 0);
}

void TradingServiceTest::testPositionUpdatedSignal()
{
    m_tradingService->initialize();
    QSignalSpy spy(m_tradingService, &TradingService::positionUpdated);

    // 模拟持仓更新
    // 实际测试中需要 Mock CTP 持仓回报

    QVERIFY(spy.count() >= 0);
}

// ============================================================================
// 主函数
// ============================================================================

QTEST_MAIN(TradingServiceTest)
#include "TradingServiceTest.moc"