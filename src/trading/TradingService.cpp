/**
 * @file TradingService.cpp
 * @brief 交易服务实现
 *
 * @details 实现：
 * - 统一交易入口
 * - 整合各交易组件
 * - 对接 CTP 交易接口
 * - 管理交易生命周期
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "TradingService.h"
#include "OrderManager.h"
#include "PositionManager.h"
#include "RiskController.h"
#include "ctp/service/CTPService.h"
#include "utils/Logger.h"

#include <QMutexLocker>

// ============================================================================
// PIMPL 实现
// ============================================================================

struct TradingService::Impl {
    CTP::CTPService* ctpService = nullptr;

    // 账户信息缓存
    AccountInfo accountInfo;

    // 线程安全
    mutable QMutex mutex;

    // 初始化标志
    bool initialized = false;
};

// ============================================================================
// 单例实现
// ============================================================================

TradingService& TradingService::instance()
{
    static TradingService instance;
    return instance;
}

TradingService::TradingService(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    LOG_DEBUG("TradingService created");
}

TradingService::~TradingService()
{
    shutdown();
    LOG_DEBUG("TradingService destroyed");
}

// ============================================================================
// 初始化
// ============================================================================

bool TradingService::initialize()
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        LOG_WARNING("TradingService already initialized");
        return true;
    }

    // 初始化各组件
    // 后续通过 setCtpService 设置后初始化CTP相关组件
    // 当前先初始化非CTP依赖的组件

    if (!PositionManager::instance().initialize()) {
        LOG_ERROR("Failed to initialize PositionManager");
        return false;
    }

    if (!RiskController::instance().initialize()) {
        LOG_ERROR("Failed to initialize RiskController");
        return false;
    }

    // 连接内部信号
    connect(&OrderManager::instance(), &OrderManager::orderSubmitted,
            this, &TradingService::orderSubmitted);
    connect(&OrderManager::instance(), &OrderManager::orderRejected,
            this, &TradingService::orderRejected);
    connect(&OrderManager::instance(), &OrderManager::orderFilled,
            this, &TradingService::orderFilled);
    connect(&OrderManager::instance(), &OrderManager::orderCancelled,
            this, &TradingService::orderCancelled);

    // 使用 orderUpdated 信号来处理 orderAccepted
    connect(&OrderManager::instance(), &OrderManager::orderUpdated,
            this, [this](const OrderInfo& order) {
                if (order.status == OrderStatus::Accepted) {
                    emit orderAccepted(order.orderId);
                }
            });

    connect(&PositionManager::instance(), &PositionManager::positionUpdated,
            this, &TradingService::positionUpdated);
    connect(&PositionManager::instance(), &PositionManager::profitUpdated,
            this, &TradingService::profitUpdated);

    connect(&RiskController::instance(), &RiskController::riskLevelChanged,
            this, &TradingService::riskLevelChanged);
    connect(&RiskController::instance(), &RiskController::riskWarning,
            this, &TradingService::riskWarning);

    d->initialized = true;
    LOG_INFO("TradingService initialized");
    return true;
}

void TradingService::shutdown()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    // 断开 CTP 信号
    disconnectSignals();

    // 关闭各组件
    RiskController::instance().shutdown();
    PositionManager::instance().shutdown();
    OrderManager::instance().shutdown();

    d->ctpService = nullptr;
    d->initialized = false;

    LOG_INFO("TradingService shutdown");
}

void TradingService::setCtpService(CTP::CTPService* ctpService)
{
    QMutexLocker locker(&d->mutex);

    // 断开旧连接
    if (d->ctpService) {
        disconnectSignals();
    }

    d->ctpService = ctpService;

    // 建立新连接
    if (d->ctpService) {
        connectSignals();
        LOG_INFO("CTPService connected to TradingService");
    }
}

// ============================================================================
// 交易操作
// ============================================================================

QString TradingService::submitOrder(const OrderRequest& request)
{
    // 1. 风控检查
    auto riskResult = RiskController::instance().checkOrder(request);
    if (!riskResult.passed) {
        LOG_WARNING(QString("Order rejected by risk control: %1 - %2")
            .arg(riskResult.ruleName, riskResult.message));
        emit orderRejected(request.instrumentId, riskResult.message);
        emit tradingLog(QString("风控拒绝: %1").arg(riskResult.message), 1);
        return QString();
    }

    // 2. 提交到 OrderManager
    QString orderId = OrderManager::instance().submitOrder(request);
    if (orderId.isEmpty()) {
        LOG_ERROR("Failed to submit order to OrderManager");
        return QString();
    }

    // 3. 提交到 CTP
    if (d->ctpService) {
        // 转换为 CTP 格式
        CTP::OrderInfo ctpOrder;
        ctpOrder.instrumentId = request.instrumentId;
        ctpOrder.direction = request.direction == TradeDirection::Buy 
            ? CTP::Direction::Buy : CTP::Direction::Sell;
        ctpOrder.offset = request.openClose == OpenCloseFlag::Open 
            ? CTP::OffsetFlag::Open : CTP::OffsetFlag::Close;
        ctpOrder.price = request.price;
        ctpOrder.totalVolume = request.volume;

        auto orderRef = d->ctpService->insertOrder(ctpOrder);
        if (orderRef) {
            // 更新订单的 CTP 引用
            OrderManager::instance().updateOrderId(orderId, *orderRef);
            LOG_INFO(QString("Order submitted to CTP: %1 -> %2")
                .arg(orderId, *orderRef));
        } else {
            LOG_ERROR("Failed to submit order to CTP");
            // 标记订单失败
            OrderManager::instance().cancelOrder(orderId);
            return QString();
        }
    }

    emit tradingLog(QString("下单成功: %1 %2 %3@%4")
        .arg(request.instrumentId)
        .arg(TradingUtils::directionToString(request.direction))
        .arg(request.volume)
        .arg(request.price), 0);

    return orderId;
}

bool TradingService::cancelOrder(const QString& orderId)
{
    // 1. 获取订单信息
    auto order = OrderManager::instance().getOrder(orderId);
    if (!order) {
        LOG_WARNING(QString("Order not found: %1").arg(orderId));
        return false;
    }

    // 2. 检查是否可撤销
    if (!order->isActive()) {
        LOG_WARNING(QString("Order cannot be cancelled: %1, status: %2")
            .arg(orderId, TradingUtils::statusToString(order->status)));
        return false;
    }

    // 3. 提交撤单到 CTP
    if (d->ctpService && !order->orderId.isEmpty()) {
        d->ctpService->cancelOrder(order->orderId);
    }

    // 4. 更新 OrderManager
    OrderManager::instance().cancelOrder(orderId);

    emit tradingLog(QString("撤单成功: %1").arg(orderId), 0);

    return true;
}

int TradingService::cancelOrders(const QVector<QString>& orderIds)
{
    int successCount = 0;
    for (const auto& orderId : orderIds) {
        if (cancelOrder(orderId)) {
            successCount++;
        }
    }
    return successCount;
}

bool TradingService::setStopLossTakeProfit(const QString& instrumentId, 
                                           double stopLoss, 
                                           double takeProfit)
{
    // 创建止损止盈规则
    StopLossTakeProfit sltp;
    sltp.instrumentId = instrumentId;
    sltp.stopLossPrice = stopLoss;
    sltp.takeProfitPrice = takeProfit;
    sltp.isActive = true;

    // 添加到 OrderManager
    OrderManager::instance().addStopLossTakeProfit(sltp);

    emit tradingLog(QString("设置止损止盈: %1 止损=%2 止盈=%3")
        .arg(instrumentId).arg(stopLoss).arg(takeProfit), 0);

    return true;
}

QString TradingService::setConditionOrder(const ConditionOrder& condition)
{
    return OrderManager::instance().addConditionOrder(condition);
}

// ============================================================================
// 查询接口
// ============================================================================

std::optional<OrderInfo> TradingService::getOrder(const QString& orderId) const
{
    return OrderManager::instance().getOrder(orderId);
}

QVector<OrderInfo> TradingService::getActiveOrders() const
{
    return OrderManager::instance().getActiveOrders();
}

std::optional<PositionInfo> TradingService::getPosition(const QString& instrumentId, 
                                                        PositionDirection direction) const
{
    return PositionManager::instance().getPosition(instrumentId, direction);
}

QVector<PositionInfo> TradingService::getPositions() const
{
    return PositionManager::instance().getPositions();
}

AccountInfo TradingService::getAccountInfo() const
{
    QMutexLocker locker(&d->mutex);
    return d->accountInfo;
}

double TradingService::getTotalProfit() const
{
    return PositionManager::instance().getTotalProfit();
}

int TradingService::getRiskLevel() const
{
    return RiskController::instance().getRiskLevel();
}

// ============================================================================
// 风控接口
// ============================================================================

RiskCheckResult TradingService::checkOrder(const OrderRequest& request)
{
    return RiskController::instance().checkOrder(request);
}

TradingService::RiskReport TradingService::getRiskReport() const
{
    auto report = RiskController::instance().generateReport();

    RiskReport result;
    result.riskLevel = report.riskLevel;
    result.totalRisk = report.totalRisk;
    result.warnings = report.warnings;
    result.suggestions = report.suggestions;

    return result;
}

// ============================================================================
// 内部方法
// ============================================================================

void TradingService::connectSignals()
{
    if (!d->ctpService) return;

    connect(d->ctpService, &CTP::CTPService::orderUpdated,
            this, &TradingService::onCtpOrderUpdated);
    connect(d->ctpService, &CTP::CTPService::tradeReceived,
            this, &TradingService::onCtpTradeReceived);
    connect(d->ctpService, &CTP::CTPService::accountInfoReceived,
            this, &TradingService::onCtpAccountInfo);
    connect(d->ctpService, &CTP::CTPService::positionReceived,
            this, &TradingService::onCtpPositionReceived);
}

void TradingService::disconnectSignals()
{
    if (!d->ctpService) return;

    disconnect(d->ctpService, &CTP::CTPService::orderUpdated,
               this, &TradingService::onCtpOrderUpdated);
    disconnect(d->ctpService, &CTP::CTPService::tradeReceived,
               this, &TradingService::onCtpTradeReceived);
    disconnect(d->ctpService, &CTP::CTPService::accountInfoReceived,
               this, &TradingService::onCtpAccountInfo);
    disconnect(d->ctpService, &CTP::CTPService::positionReceived,
               this, &TradingService::onCtpPositionReceived);
}

void TradingService::onCtpOrderUpdated(const CTP::OrderInfo& ctpOrder)
{
    // 转换 CTP 订单状态
    OrderStatus status = OrderStatus::Unknown;
    switch (ctpOrder.status) {
    case CTP::OrderStatus::AllTraded:
        status = OrderStatus::Filled;
        break;
    case CTP::OrderStatus::PartTradedQueueing:
    case CTP::OrderStatus::PartTradedNotQueueing:
        status = OrderStatus::PartialFilled;
        break;
    case CTP::OrderStatus::NoTradeQueueing:
    case CTP::OrderStatus::NoTradeNotQueueing:
        status = OrderStatus::Accepted;
        break;
    case CTP::OrderStatus::Canceled:
        status = OrderStatus::Cancelled;
        break;
    default:
        status = OrderStatus::Unknown;
        break;
    }

    // 更新 OrderManager
    OrderManager::instance().updateOrderStatus(ctpOrder.orderRef, status, 
                                               ctpOrder.tradedVolume);

    LOG_DEBUG(QString("CTP order updated: %1, status: %2, filled: %3")
        .arg(ctpOrder.orderRef)
        .arg(TradingUtils::statusToString(status))
        .arg(ctpOrder.tradedVolume));
}

void TradingService::onCtpTradeReceived(const CTP::TradeInfo& ctpTrade)
{
    // 创建成交记录
    TradeRecord trade;
    trade.instrumentId = ctpTrade.instrumentId;
    trade.orderId = ctpTrade.orderRef;
    trade.tradeId = ctpTrade.tradeId;
    trade.direction = ctpTrade.direction == CTP::Direction::Buy 
        ? TradeDirection::Buy : TradeDirection::Sell;
    trade.openClose = ctpTrade.offset == CTP::OffsetFlag::Open 
        ? OpenCloseFlag::Open : OpenCloseFlag::Close;
    trade.price = ctpTrade.price;
    trade.volume = ctpTrade.volume;
    trade.tradeTime = ctpTrade.tradeTime;

    // 记录成交
    OrderManager::instance().recordTrade(trade);

    // 更新持仓
    // 根据成交更新持仓信息
    updatePositionFromTrade(trade);

    emit tradingLog(QString("成交回报: %1 %2 %3@%4")
        .arg(trade.instrumentId)
        .arg(TradingUtils::directionToString(trade.direction))
        .arg(trade.volume)
        .arg(trade.price), 0);
}

void TradingService::updatePositionFromTrade(const TradeRecord& trade)
{
    // 根据成交更新持仓信息
    // 这里简化实现，实际需要调用 PositionManager
    LOG_INFO(QString("Position update from trade: %1, volume: %2, price: %3")
        .arg(trade.instrumentId)
        .arg(trade.volume)
        .arg(trade.price));
    
    // 实际实现需要调用 PositionManager::instance().updatePosition(trade)
    // PositionManager::instance().updatePosition(trade.instrumentId, trade.direction, trade.volume, trade.price);
}

void TradingService::onCtpAccountInfo(double available, double balance)
{
    QMutexLocker locker(&d->mutex);

    d->accountInfo.available = available;
    d->accountInfo.balance = balance;

    // 更新风控系统
    RiskController::instance().setAccountInfo(d->accountInfo);

    LOG_DEBUG(QString("Account info updated: available=%1, balance=%2")
        .arg(available).arg(balance));
}

void TradingService::onCtpPositionReceived(const QString& instrument, 
                                           int longPos, int shortPos)
{
    // 更新持仓
    if (longPos > 0) {
        PositionInfo pos;
        pos.instrumentId = instrument;
        pos.direction = PositionDirection::Long;
        pos.volume = longPos;
        PositionManager::instance().updatePosition(pos);
    }

    if (shortPos > 0) {
        PositionInfo pos;
        pos.instrumentId = instrument;
        pos.direction = PositionDirection::Short;
        pos.volume = shortPos;
        PositionManager::instance().updatePosition(pos);
    }

    LOG_DEBUG(QString("Position received: %1, long=%2, short=%3")
        .arg(instrument).arg(longPos).arg(shortPos));
}
