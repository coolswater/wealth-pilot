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

/**
 * @brief 提交订单
 * @param request 订单请求（合约、方向、开平、价格、数量）
 * @return 订单ID，失败返回空字符串
 *
 * @details 订单提交流程：
 * 1. 风控检查 - RiskController 检查订单是否符合风控规则
 * 2. 创建订单 - OrderManager 创建订单记录
 * 3. 提交CTP - 转换为CTP格式并提交到交易接口
 * 4. 更新引用 - 将CTP订单引用关联到内部订单ID
 *
 * 风控检查失败会直接拒绝订单，不会提交到CTP
 */
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

/**
 * @brief 撤销订单
 * @param orderId 内部订单ID
 * @return true撤销成功，false撤销失败
 *
 * @details 撤单流程：
 * 1. 获取订单信息 - 从 OrderManager 获取订单详情
 * 2. 检查订单状态 - 只有活动状态的订单可以撤销
 * 3. 提交CTP撤单 - 调用 CTPService 的撤单接口
 * 4. 更新订单状态 - OrderManager 更新订单为已撤销
 *
 * 已成交或部分成交的订单不能撤销
 */
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

/**
 * @brief 批量撤销订单
 * @param orderIds 订单ID列表
 * @return 成功撤销的订单数量
 *
 * 遍历订单列表，逐个调用 cancelOrder
 * 用于快速清理多个挂单
 */
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

/**
 * @brief 设置止损止盈
 * @param instrumentId 合约代码
 * @param stopLoss 止损价格
 * @param takeProfit 止盈价格
 * @return true设置成功
 *
 * @details 止损止盈机制：
 * - 创建止损止盈规则并添加到 OrderManager
 * - 当市场价格触及止损/止盈价格时自动触发平仓
 * - 用于风险管理和自动化交易
 */
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

/**
 * @brief 设置条件单
 * @param condition 条件单配置（触发条件、订单内容）
 * @return 条件单ID
 *
 * @details 条件单类型：
 * - 价格触发单：价格达到指定值时触发
 * - 时间触发单：指定时间触发
 * - 指标触发单：技术指标满足条件时触发
 *
 * 条件单由 OrderManager 管理，满足条件时自动提交订单
 */
QString TradingService::setConditionOrder(const ConditionOrder& condition)
{
    return OrderManager::instance().addConditionOrder(condition);
}

// ============================================================================
// 查询接口
// ============================================================================

/**
 * @brief 获取订单信息
 * @param orderId 订单ID
 * @return 订单信息，不存在返回空
 *
 * 从 OrderManager 查询订单详情
 */
std::optional<OrderInfo> TradingService::getOrder(const QString& orderId) const
{
    return OrderManager::instance().getOrder(orderId);
}

/**
 * @brief 获取所有活动订单
 * @return 活动订单列表（未成交、部分成交）
 *
 * 用于监控当前挂单状态
 */
QVector<OrderInfo> TradingService::getActiveOrders() const
{
    return OrderManager::instance().getActiveOrders();
}

/**
 * @brief 获取持仓信息
 * @param instrumentId 合约代码
 * @param direction 持仓方向（多头/空头）
 * @return 持仓信息，不存在返回空
 *
 * 从 PositionManager 查询指定合约的持仓
 */
std::optional<PositionInfo> TradingService::getPosition(const QString& instrumentId, 
                                                        PositionDirection direction) const
{
    return PositionManager::instance().getPosition(instrumentId, direction);
}

/**
 * @brief 获取所有持仓
 * @return 持仓列表
 *
 * 用于账户持仓概览和风险管理
 */
QVector<PositionInfo> TradingService::getPositions() const
{
    return PositionManager::instance().getPositions();
}

/**
 * @brief 获取账户信息
 * @return 账户信息（可用资金、总资产等）
 *
 * 账户信息由 CTP 推送更新，本地缓存
 */
AccountInfo TradingService::getAccountInfo() const
{
    QMutexLocker locker(&d->mutex);
    return d->accountInfo;
}

/**
 * @brief 获取总盈亏
 * @return 总浮动盈亏
 *
 * 计算所有持仓的浮动盈亏总和
 */
double TradingService::getTotalProfit() const
{
    return PositionManager::instance().getTotalProfit();
}

/**
 * @brief 获取风险等级
 * @return 风险等级（0-100）
 *
 * 由 RiskController 根据持仓、盈亏、杠杆等计算
 */
int TradingService::getRiskLevel() const
{
    return RiskController::instance().getRiskLevel();
}

// ============================================================================
// 风控接口
// ============================================================================

/**
 * @brief 检查订单风控
 * @param request 订单请求
 * @return 风控检查结果（是否通过、规则名称、消息）
 *
 * @details 风控检查内容：
 * - 持仓限制检查（最大持仓金额、数量）
 * - 亏损限制检查（日最大亏损、单笔最大亏损）
 * - 杠杆限制检查（最大杠杆、保证金比例）
 * - 交易限制检查（夜盘、反向交易）
 *
 * 检查失败会返回具体的规则名称和拒绝原因
 */
RiskCheckResult TradingService::checkOrder(const OrderRequest& request)
{
    return RiskController::instance().checkOrder(request);
}

/**
 * @brief 获取风险报告
 * @return 风险报告（风险等级、总风险值、警告列表、建议列表）
 *
 * @details 风险报告内容：
 * - 当前风险等级评估
 * - 各项风险指标分析
 * - 风险警告提示
 * - 风险控制建议
 *
 * 用于向用户展示当前账户的风险状况
 */
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
