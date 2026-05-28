/**
 * @file TradingIntegration.cpp
 * @brief 交易页面与TradingService集成实现
 */

#include "TradingIntegration.h"
#include "TradeHistoryPage.h"
#include "ConditionOrderPage.h"
#include "domain/trading/TradingService.h"
#include "services/trading/TradingService.h"
#include "domain/trading/TradingTypes.h"
#include "shared/utils/Logger.h"

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

void TradingIntegration::registerAccountPage(WealthPilot::AccountPage* page)
{
    m_accountPage = page;
    updateAccountPage();
}

void TradingIntegration::registerTradeHistoryPage(WealthPilot::TradeHistoryPage* page)
{
    m_tradeHistoryPage = page;
}

void TradingIntegration::registerConditionOrderPage(WealthPilot::ConditionOrderPage* page)
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
    // 新的 AccountPage 是整合页面，不再直接设置数据
    // 数据更新通过各个子页面自行处理
    LOG_DEBUG("AccountPage update skipped - using integrated page");
}

} // namespace Trading
