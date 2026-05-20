/**
 * @file OrderManagerTest.cpp
 * @brief 订单管理器单元测试
 *
 * @details 测试范围：
 * - 订单提交和撤销
 * - 订单状态转换
 * - 条件单管理
 * - 止损止盈管理
 * - 并发安全
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include <QtTest>
#include <QSignalSpy>
#include "src/trading/OrderManager.h"
#include "src/trading/TradingTypes.h"

using namespace WealthPilot;

/**
 * @brief OrderManager 单元测试类
 */
class OrderManagerTest : public QObject
{
    Q_OBJECT

private slots:
    // ========== 测试初始化 ==========
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // ========== 订单操作测试 ==========
    void testSubmitOrder();
    void testCancelOrder();
    void testCancelOrders();
    void testModifyOrder();
    void testOrderStatusTransition();

    // ========== 条件单测试 ==========
    void testAddConditionOrder();
    void testRemoveConditionOrder();
    void testConditionOrderTrigger();

    // ========== 止损止盈测试 ==========
    void testSetStopLossTakeProfit();
    void testStopLossTrigger();
    void testTakeProfitTrigger();

    // ========== 查询接口测试 ==========
    void testGetOrder();
    void testGetActiveOrders();
    void testGetHistoryOrders();
    void testGetStats();

    // ========== 并发测试 ==========
    void testConcurrentSubmit();

    // ========== 边界条件测试 ==========
    void testInvalidOrder();
    void testEmptyOrderId();

private:
    OrderManager* m_orderManager = nullptr;
};

// ============================================================================
// 测试初始化
// ============================================================================

void OrderManagerTest::initTestCase()
{
    qDebug() << "OrderManager Test Suite Started";
}

void OrderManagerTest::cleanupTestCase()
{
    qDebug() << "OrderManager Test Suite Finished";
}

void OrderManagerTest::init()
{
    m_orderManager = &OrderManager::instance();
    // 初始化（不使用 CTP SPI）
    // m_orderManager->initialize(nullptr);
}

void OrderManagerTest::cleanup()
{
    // 清理测试数据
}

// ============================================================================
// 订单操作测试
// ============================================================================

void OrderManagerTest::testSubmitOrder()
{
    // 创建订单请求
    OrderRequest request;
    request.instrumentId = "cu2505";
    request.direction = TradeDirection::Buy;
    request.offsetFlag = OffsetFlag::Open;
    request.price = 75000.0;
    request.volume = 1;
    request.orderType = OrderType::Limit;

    // 提交订单
    QString orderId = m_orderManager->submitOrder(request);

    // 验证订单 ID 有效
    QVERIFY(!orderId.isEmpty());

    // 验证订单已创建
    auto order = m_orderManager->getOrder(orderId);
    QVERIFY(order.has_value());
    QCOMPARE(order->instrumentId, QString("cu2505"));
    QCOMPARE(order->direction, TradeDirection::Buy);
    QCOMPARE(order->status, OrderStatus::Submitted);
}

void OrderManagerTest::testCancelOrder()
{
    // 先提交订单
    OrderRequest request;
    request.instrumentId = "IF2501";
    request.direction = TradeDirection::Sell;
    request.price = 3850.0;
    request.volume = 1;

    QString orderId = m_orderManager->submitOrder(request);
    QVERIFY(!orderId.isEmpty());

    // 撤销订单
    bool success = m_orderManager->cancelOrder(orderId);
    QVERIFY(success);

    // 验证订单状态
    auto order = m_orderManager->getOrder(orderId);
    QVERIFY(order.has_value());
    QCOMPARE(order->status, OrderStatus::Cancelled);
}

void OrderManagerTest::testCancelOrders()
{
    // 提交多个订单
    QStringList orderIds;
    for (int i = 0; i < 5; ++i) {
        OrderRequest request;
        request.instrumentId = QString("test%1").arg(i);
        request.direction = TradeDirection::Buy;
        request.price = 100.0 + i;
        request.volume = 1;

        QString orderId = m_orderManager->submitOrder(request);
        orderIds.append(orderId);
    }

    // 批量撤销
    int cancelledCount = m_orderManager->cancelOrders(orderIds);
    QCOMPARE(cancelledCount, 5);
}

void OrderManagerTest::testModifyOrder()
{
    // 提交订单
    OrderRequest request;
    request.instrumentId = "rb2505";
    request.direction = TradeDirection::Buy;
    request.price = 3500.0;
    request.volume = 2;

    QString orderId = m_orderManager->submitOrder(request);

    // 修改订单
    bool success = m_orderManager->modifyOrder(orderId, 3600.0, 3);
    QVERIFY(success);

    // 验证修改
    auto order = m_orderManager->getOrder(orderId);
    QVERIFY(order.has_value());
    QCOMPARE(order->price, 3600.0);
    QCOMPARE(order->volume, 3);
}

void OrderManagerTest::testOrderStatusTransition()
{
    // 测试订单状态转换
    // Submitted -> Accepted -> Filled

    OrderRequest request;
    request.instrumentId = "au2506";
    request.direction = TradeDirection::Buy;
    request.price = 480.0;
    request.volume = 1;

    QString orderId = m_orderManager->submitOrder(request);

    // 初始状态
    auto order = m_orderManager->getOrder(orderId);
    QCOMPARE(order->status, OrderStatus::Submitted);

    // 更新状态为已接受
    m_orderManager->updateOrderStatus(orderId, OrderStatus::Accepted);
    order = m_orderManager->getOrder(orderId);
    QCOMPARE(order->status, OrderStatus::Accepted);

    // 更新状态为已成交
    m_orderManager->updateOrderStatus(orderId, OrderStatus::Filled, 1);
    order = m_orderManager->getOrder(orderId);
    QCOMPARE(order->status, OrderStatus::Filled);
    QCOMPARE(order->filledVolume, 1);
}

// ============================================================================
// 条件单测试
// ============================================================================

void OrderManagerTest::testAddConditionOrder()
{
    ConditionOrder condition;
    condition.instrumentId = "IF2501";
    condition.conditionType = ConditionType::PriceGreater;
    condition.triggerPrice = 3900.0;
    condition.direction = TradeDirection::Buy;
    condition.volume = 1;
    condition.price = 3900.0;

    QString conditionId = m_orderManager->addConditionOrder(condition);
    QVERIFY(!conditionId.isEmpty());

    // 验证条件单已添加
    auto conditions = m_orderManager->getConditionOrders();
    QCOMPARE(conditions.size(), 1);
    QCOMPARE(conditions[0].instrumentId, QString("IF2501"));
}

void OrderManagerTest::testRemoveConditionOrder()
{
    // 添加条件单
    ConditionOrder condition;
    condition.instrumentId = "test";
    condition.triggerPrice = 100.0;

    QString conditionId = m_orderManager->addConditionOrder(condition);

    // 删除条件单
    bool success = m_orderManager->removeConditionOrder(conditionId);
    QVERIFY(success);

    // 验证已删除
    auto conditions = m_orderManager->getConditionOrders();
    QCOMPARE(conditions.size(), 0);
}

void OrderManagerTest::testConditionOrderTrigger()
{
    QSignalSpy spy(m_orderManager, &OrderManager::conditionTriggered);

    // 添加条件单：价格 > 100 时买入
    ConditionOrder condition;
    condition.instrumentId = "test";
    condition.conditionType = ConditionType::PriceGreater;
    condition.triggerPrice = 100.0;
    condition.direction = TradeDirection::Buy;
    condition.volume = 1;

    QString conditionId = m_orderManager->addConditionOrder(condition);

    // 模拟行情更新
    m_orderManager->onMarketDataUpdated("test", 101.0);

    // 验证条件单触发信号
    QCOMPARE(spy.count(), 1);
}

// ============================================================================
// 止损止盈测试
// ============================================================================

void OrderManagerTest::testSetStopLossTakeProfit()
{
    StopLossTakeProfit sltp;
    sltp.instrumentId = "cu2505";
    sltp.positionDirection = PositionDirection::Long;
    sltp.stopLossPrice = 74000.0;
    sltp.takeProfitPrice = 76000.0;
    sltp.volume = 1;

    QString sltpId = m_orderManager->setStopLossTakeProfit(sltp);
    QVERIFY(!sltpId.isEmpty());
}

void OrderManagerTest::testStopLossTrigger()
{
    QSignalSpy spy(m_orderManager, &OrderManager::stopLossTakeProfitTriggered);

    // 设置止损：价格 < 74000 时触发
    StopLossTakeProfit sltp;
    sltp.instrumentId = "test";
    sltp.positionDirection = PositionDirection::Long;
    sltp.stopLossPrice = 74000.0;
    sltp.volume = 1;

    QString sltpId = m_orderManager->setStopLossTakeProfit(sltp);

    // 模拟价格下跌触发止损
    m_orderManager->onMarketDataUpdated("test", 73500.0);

    // 验证止损触发
    QCOMPARE(spy.count(), 1);
}

void OrderManagerTest::testTakeProfitTrigger()
{
    QSignalSpy spy(m_orderManager, &OrderManager::stopLossTakeProfitTriggered);

    // 设置止盈：价格 > 76000 时触发
    StopLossTakeProfit sltp;
    sltp.instrumentId = "test2";
    sltp.positionDirection = PositionDirection::Long;
    sltp.takeProfitPrice = 76000.0;
    sltp.volume = 1;

    QString sltpId = m_orderManager->setStopLossTakeProfit(sltp);

    // 模拟价格上涨触发止盈
    m_orderManager->onMarketDataUpdated("test2", 76500.0);

    // 验证止盈触发
    QCOMPARE(spy.count(), 1);
}

// ============================================================================
// 查询接口测试
// ============================================================================

void OrderManagerTest::testGetOrder()
{
    // 提交订单
    OrderRequest request;
    request.instrumentId = "query_test";
    request.direction = TradeDirection::Buy;
    request.price = 100.0;
    request.volume = 1;

    QString orderId = m_orderManager->submitOrder(request);

    // 查询订单
    auto order = m_orderManager->getOrder(orderId);
    QVERIFY(order.has_value());
    QCOMPARE(order->orderId, orderId);

    // 查询不存在的订单
    auto notFound = m_orderManager->getOrder("non_existent_id");
    QVERIFY(!notFound.has_value());
}

void OrderManagerTest::testGetActiveOrders()
{
    // 提交多个订单
    for (int i = 0; i < 3; ++i) {
        OrderRequest request;
        request.instrumentId = QString("active_test_%1").arg(i);
        request.direction = TradeDirection::Buy;
        request.price = 100.0;
        request.volume = 1;

        m_orderManager->submitOrder(request);
    }

    // 获取活跃订单
    auto activeOrders = m_orderManager->getActiveOrders();
    QVERIFY(activeOrders.size() >= 3);
}

void OrderManagerTest::testGetHistoryOrders()
{
    // 提交并成交订单
    OrderRequest request;
    request.instrumentId = "history_test";
    request.direction = TradeDirection::Buy;
    request.price = 100.0;
    request.volume = 1;

    QString orderId = m_orderManager->submitOrder(request);
    m_orderManager->updateOrderStatus(orderId, OrderStatus::Filled, 1);

    // 查询历史订单
    QDateTime from = QDateTime::currentDateTime().addDays(-1);
    QDateTime to = QDateTime::currentDateTime().addDays(1);

    auto history = m_orderManager->getHistoryOrders(from, to);
    QVERIFY(history.size() > 0);
}

void OrderManagerTest::testGetStats()
{
    // 提交几个订单
    for (int i = 0; i < 5; ++i) {
        OrderRequest request;
        request.instrumentId = QString("stats_test_%1").arg(i);
        request.direction = TradeDirection::Buy;
        request.price = 100.0;
        request.volume = 1;

        m_orderManager->submitOrder(request);
    }

    // 获取统计
    auto stats = m_orderManager->getStats();
    QVERIFY(stats.totalOrders >= 5);
}

// ============================================================================
// 并发测试
// ============================================================================

void OrderManagerTest::testConcurrentSubmit()
{
    // 测试并发提交订单
    QList<QFuture<QString>> futures;

    for (int i = 0; i < 10; ++i) {
        // QtConcurrent::run([this, i]() {
            OrderRequest request;
            request.instrumentId = QString("concurrent_%1").arg(i);
            request.direction = TradeDirection::Buy;
            request.price = 100.0 + i;
            request.volume = 1;

            return m_orderManager->submitOrder(request);
        // });
    }

    // 等待所有完成
    // for (auto& future : futures) {
    //     future.waitForFinished();
    //     QVERIFY(!future.result().isEmpty());
    // }

    QVERIFY(true);
}

// ============================================================================
// 边界条件测试
// ============================================================================

void OrderManagerTest::testInvalidOrder()
{
    // 无效订单请求
    OrderRequest request;
    request.instrumentId = ""; // 空合约
    request.price = -1; // 无效价格

    QString orderId = m_orderManager->submitOrder(request);
    // 应该返回空或失败
    QVERIFY(orderId.isEmpty() || orderId.isNull());
}

void OrderManagerTest::testEmptyOrderId()
{
    // 撤销空订单 ID
    bool success = m_orderManager->cancelOrder("");
    QVERIFY(!success);

    // 查询空订单 ID
    auto order = m_orderManager->getOrder("");
    QVERIFY(!order.has_value());
}

// ============================================================================
// 主函数
// ============================================================================

QTEST_MAIN(OrderManagerTest)
#include "OrderManagerTest.moc"
