/**
 * @file TradingIntegration.cpp
 * @brief 交易页面与TradingService集成实现
 */

#include "TradingIntegration.h"
#include "../account/AccountPage.h"
#include "TradeHistoryPage.h"
#include "ConditionOrderPage.h"
#include "../../trading/TradingService.h"
#include "../../trading/TradingTypes.h"
#include "../../utils/Logger.h"

namespace Trading {

TradingIntegration& TradingIntegration::instance()
{
    static TradingIntegration instance;
    return instance;
}

TradingIntegration::TradingIntegration(QObject* parent)
    : QObject(parent)
{
}

void TradingIntegration::initialize()
{
    LOG_INFO("Initializing TradingIntegration...");
    connectTradingService();
    LOG_INFO("TradingIntegration initialized");
}

void TradingIntegration::connectTradingService()
{
    auto& service = TradingService::instance();
    
    connect(&service, &TradingService::orderFilled,
            this, &TradingIntegration::onOrderFilled);
    
    connect(&service, &TradingService::positionUpdated,
            this, &TradingIntegration::onPositionUpdated);
    
    connect(&service, &TradingService::profitUpdated,
            this, &TradingIntegration::onProfitUpdated);
}

void TradingIntegration::registerAccountPage(AccountPage* page)
{
    m_accountPage = page;
    updateAccountPage();
}

void TradingIntegration::registerTradeHistoryPage(TradeHistoryPage* page)
{
    m_tradeHistoryPage = page;
}

void TradingIntegration::registerConditionOrderPage(ConditionOrderPage* page)
{
    m_conditionOrderPage = page;
}

void TradingIntegration::handleOrderSubmit(const QString& instrumentId, OrderType orderType,
                                            TradeDirection direction, OpenCloseFlag openClose,
                                            int quantity, double price, double stopPrice)
{
    LOG_INFO(QString("Handling order submit: %1, qty=%2, price=%3")
             .arg(instrumentId).arg(quantity).arg(price));
    
    // 转换为OrderRequest
    OrderRequest request;
    request.instrumentId = instrumentId;
    request.volume = quantity;
    request.price = price;
    request.direction = direction;
    request.openClose = openClose;
    request.orderType = orderType;
    request.stopPrice = stopPrice;
    
    // 提交订单
    QString orderId = TradingService::instance().submitOrder(request);
    
    if (!orderId.isEmpty()) {
        LOG_INFO(QString("Order submitted successfully: %1").arg(orderId));
    } else {
        LOG_ERROR("Order submission failed");
    }
}

void TradingIntegration::onOrderFilled(const QString& orderId, const TradeRecord& trade)
{
    LOG_INFO(QString("Order filled: %1").arg(orderId));
    
    // 更新账户页面
    updateAccountPage();
}

void TradingIntegration::onPositionUpdated(const PositionInfo& position)
{
    LOG_DEBUG(QString("Position updated: %1").arg(position.instrumentId));
    updateAccountPage();
}

void TradingIntegration::onProfitUpdated(double totalProfit)
{
    LOG_DEBUG(QString("Profit updated: %1").arg(totalProfit));
    updateAccountPage();
}

void TradingIntegration::updateAccountPage()
{
    if (!m_accountPage) return;
    
    auto& service = TradingService::instance();
    auto accountInfo = service.getAccountInfo();
    
    m_accountPage->setAccountData(
        accountInfo.balance,
        accountInfo.available,
        accountInfo.margin,
        accountInfo.frozenMargin,
        accountInfo.commission,
        accountInfo.closeProfit,
        accountInfo.positionProfit
    );
    
    // 设置统计数据
    double totalProfit = service.getTotalProfit();
    m_accountPage->setStatistics(
        totalProfit > 0 ? totalProfit : 0,
        totalProfit < 0 ? std::abs(totalProfit) : 0,
        accountInfo.commission,
        0.0,  // maxDrawdown - TODO
        0.0   // winRate - TODO
    );
}

} // namespace Trading
