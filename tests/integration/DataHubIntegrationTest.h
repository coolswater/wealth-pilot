/**
 * @file DataHubIntegrationTest.h
 * @brief DataHub 集成测试
 *
 * @details 测试内容：
 * - 页面集成测试
 * - 数据流集成测试
 * - 生命周期集成测试
 * - 多页面协同测试
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAHUBINTEGRATIONTEST_H
#define DATAHUBINTEGRATIONTEST_H

#include <QObject>
#include <QtTest/QtTest>

/**
 * @brief DataHub 集成测试类
 *
 * @details 测试 DataHub 与各页面的集成：
 * - 页面初始化集成
 * - 数据订阅集成
 * - 页面切换集成
 * - 数据刷新集成
 */
class DataHubIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    /**
     * @brief 初始化测试
     */
    void initTestCase();

    /**
     * @brief 清理测试
     */
    void cleanupTestCase();

    // ========== 页面集成测试 ==========

    /**
     * @brief 测试 StockQuotesPage 集成
     */
    void testStockQuotesPageIntegration();

    /**
     * @brief 测试 WatchListPage 集成
     */
    void testWatchListPageIntegration();

    /**
     * @brief 测试 DashboardPage 集成
     */
    void testDashboardPageIntegration();

    /**
     * @brief 测试 PortfolioPage 集成
     */
    void testPortfolioPageIntegration();

    // ========== 数据流集成测试 ==========

    /**
     * @brief 测试行情数据流
     */
    void testMarketDataFlow();

    /**
     * @brief 测试新闻数据流
     */
    void testNewsDataFlow();

    /**
     * @brief 测试交易数据流
     */
    void testTradeDataFlow();

    // ========== 生命周期集成测试 ==========

    /**
     * @brief 测试页面生命周期
     */
    void testPageLifecycle();

    /**
     * @brief 测试订阅自动清理
     */
    void testSubscriptionAutoCleanup();

    /**
     * @brief 测试多页面订阅管理
     */
    void testMultiPageSubscriptionManagement();

    // ========== 多页面协同测试 ==========

    /**
     * @brief 测试多页面数据同步
     */
    void testMultiPageDataSync();

    /**
     * @brief 测试页面切换数据保持
     */
    void testPageSwitchDataPersistence();

    /**
     * @brief 测试跨页面事件传递
     */
    void testCrossPageEventDelivery();

private:
    /**
     * @brief 等待信号
     */
    bool waitForSignal(QObject* obj, const char* signal, int timeoutMs = 5000);

    /**
     * @brief 创建测试数据
     */
    QVariant createTestQuote(const QString& symbol, double price);
};

#endif // DATAHUBINTEGRATIONTEST_H