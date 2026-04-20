/**
 * @file OrderManager.cpp
 * @brief 订单管理器实现
 *
 * @details 实现：
 * - 订单生命周期管理
 * - 条件单监控
 * - 止损止盈触发
 * - 线程安全的数据访问
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "OrderManager.h"
#include "ctp/api/CtpTradingSpi.h"
#include "utils/Logger.h"

#include <QMutexLocker>
#include <QRandomGenerator>
#include <QUuid>

// ============================================================================
// PIMPL 实现
// ============================================================================

struct OrderManager::Impl {
    // CTP交易接口
    CTPTradingSpi* tradingSpi = nullptr;
    bool initialized = false;

    // 订单存储（使用哈希表快速查找）
    QHash<QString, OrderInfo> orderMap;           ///< 订单ID -> 订单信息
    QHash<QString, QString> requestOrderMap;      ///< 请求ID -> 订单ID
    QMap<QDateTime, OrderInfo> historyOrders;     ///< 历史订单（按时间排序）

    // 成交记录
    QVector<TradeRecord> tradeRecords;

    // 条件单
    QHash<QString, ConditionOrder> conditionOrders;

    // 止损止盈
    QHash<QString, StopLossTakeProfit> stopLossTakeProfits;

    // 订单计数器
    int orderCounter = 0;

    // 定时器
    QTimer* conditionCheckTimer = nullptr;

    // 线程安全
    mutable QMutex mutex;

    // 统计数据
    OrderStats stats;
};

// ============================================================================
// 单例实现
// ============================================================================

OrderManager& OrderManager::instance()
{
    static OrderManager instance;
    return instance;
}

OrderManager::OrderManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    // 创建条件单检查定时器
    d->conditionCheckTimer = new QTimer(this);
    d->conditionCheckTimer->setInterval(500); // 500ms检查一次
    connect(d->conditionCheckTimer, &QTimer::timeout, this, [this]() {
        checkConditionOrders();
        checkStopLossTakeProfit();
    });

    LOG_DEBUG("OrderManager created");
}

OrderManager::~OrderManager()
{
    shutdown();
    LOG_DEBUG("OrderManager destroyed");
}

// ============================================================================
// 初始化
// ============================================================================

bool OrderManager::initialize(CTPTradingSpi* tradingSpi)
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        LOG_WARNING("OrderManager already initialized");
        return true;
    }

    if (!tradingSpi) {
        LOG_ERROR("Trading SPI is null");
        return false;
    }

    d->tradingSpi = tradingSpi;
    d->initialized = true;

    // 启动条件单检查
    d->conditionCheckTimer->start();

    LOG_INFO("OrderManager initialized");
    return true;
}

void OrderManager::shutdown()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    // 停止定时器
    if (d->conditionCheckTimer) {
        d->conditionCheckTimer->stop();
    }

    // 清空数据
    d->orderMap.clear();
    d->requestOrderMap.clear();
    d->historyOrders.clear();
    d->tradeRecords.clear();
    d->conditionOrders.clear();
    d->stopLossTakeProfits.clear();

    d->initialized = false;
    LOG_INFO("OrderManager shutdown");
}

// ============================================================================
// 订单操作
// ============================================================================

QString OrderManager::submitOrder(const OrderRequest& request)
{
    // 验证订单请求
    QString error = request.validate();
    if (!error.isEmpty()) {
        LOG_ERROR(QString("Order validation failed: %1").arg(error));
        emit errorOccurred(-1, error);
        return QString();
    }

    QMutexLocker locker(&d->mutex);

    if (!d->initialized || !d->tradingSpi) {
        LOG_ERROR("OrderManager not initialized");
        emit errorOccurred(-2, QStringLiteral("订单管理器未初始化"));
        return QString();
    }

    // 生成请求ID
    QString requestId = request.requestId.isEmpty() 
        ? TradingUtils::generateRequestId() 
        : request.requestId;

    // 创建订单信息
    OrderInfo order;
    order.requestId = requestId;
    order.instrumentId = request.instrumentId;
    order.exchangeId = request.exchangeId;
    order.direction = request.direction;
    order.openClose = request.openClose;
    order.orderType = request.orderType;
    order.hedgeFlag = request.hedgeFlag;
    order.price = request.price;
    order.volume = request.volume;
    order.remainingVolume = request.volume;
    order.status = OrderStatus::Pending;
    order.submitTime = QDateTime::currentDateTime();
    order.strategyId = request.strategyId;
    order.remark = request.remark;

    // 添加到缓存
    d->orderMap[requestId] = order;
    d->requestOrderMap[request.requestId] = requestId;

    // 更新统计
    d->stats.totalOrders++;

    // 提交到CTP（异步）
    // TODO: 调用 CTPTradingSpi 的下单接口
    // d->tradingSpi->reqOrderInsert(order);

    LOG_INFO(QString("Order submitted: %1 %2 %3@%4")
        .arg(requestId, request.instrumentId)
        .arg(request.volume)
        .arg(request.price));

    emit orderSubmitted(requestId);

    return requestId;
}

bool OrderManager::cancelOrder(const QString& orderId)
{
    QMutexLocker locker(&d->mutex);

    if (!d->orderMap.contains(orderId)) {
        LOG_WARNING(QString("Order not found: %1").arg(orderId));
        return false;
    }

    OrderInfo& order = d->orderMap[orderId];

    // 检查订单是否可撤销
    if (!order.isActive()) {
        LOG_WARNING(QString("Order not active, cannot cancel: %1").arg(orderId));
        return false;
    }

    // 提交撤单请求
    // TODO: 调用 CTPTradingSpi 的撤单接口
    // d->tradingSpi->reqOrderAction(order);

    LOG_INFO(QString("Cancel order requested: %1").arg(orderId));
    return true;
}

int OrderManager::cancelOrders(const QStringList& orderIds)
{
    int successCount = 0;
    for (const QString& orderId : orderIds) {
        if (cancelOrder(orderId)) {
            successCount++;
        }
    }
    return successCount;
}

bool OrderManager::modifyOrder(const QString& orderId, double newPrice, int newVolume)
{
    QMutexLocker locker(&d->mutex);

    if (!d->orderMap.contains(orderId)) {
        LOG_WARNING(QString("Order not found: %1").arg(orderId));
        return false;
    }

    OrderInfo& order = d->orderMap[orderId];

    // 检查订单是否可修改
    if (!order.isActive()) {
        LOG_WARNING(QString("Order not active, cannot modify: %1").arg(orderId));
        return false;
    }

    // CTP不支持直接改单，需要先撤单再下单
    // 这里简化处理，实际需要实现撤单+重下逻辑

    LOG_INFO(QString("Modify order requested: %1, new price: %2, new volume: %3")
        .arg(orderId).arg(newPrice).arg(newVolume));

    return true;
}

// ============================================================================
// 条件单操作
// ============================================================================

QString OrderManager::addConditionOrder(const ConditionOrder& condition)
{
    QMutexLocker locker(&d->mutex);

    QString conditionId = condition.conditionId.isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
        : condition.conditionId;

    ConditionOrder newCondition = condition;
    newCondition.conditionId = conditionId;
    newCondition.createTime = QDateTime::currentDateTime();
    newCondition.isActive = true;
    newCondition.isTriggered = false;

    d->conditionOrders[conditionId] = newCondition;

    LOG_INFO(QString("Condition order added: %1, trigger price: %2")
        .arg(conditionId).arg(condition.triggerPrice));

    return conditionId;
}

bool OrderManager::removeConditionOrder(const QString& conditionId)
{
    QMutexLocker locker(&d->mutex);

    if (!d->conditionOrders.contains(conditionId)) {
        return false;
    }

    d->conditionOrders.remove(conditionId);
    LOG_INFO(QString("Condition order removed: %1").arg(conditionId));
    return true;
}

void OrderManager::setConditionOrderActive(const QString& conditionId, bool active)
{
    QMutexLocker locker(&d->mutex);

    if (d->conditionOrders.contains(conditionId)) {
        d->conditionOrders[conditionId].isActive = active;
        LOG_DEBUG(QString("Condition order %1 set active: %2").arg(conditionId).arg(active));
    }
}

// ============================================================================
// 止损止盈操作
// ============================================================================

QString OrderManager::setStopLossTakeProfit(const StopLossTakeProfit& stop)
{
    QMutexLocker locker(&d->mutex);

    QString stopId = stop.stopId.isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
        : stop.stopId;

    StopLossTakeProfit newStop = stop;
    newStop.stopId = stopId;
    newStop.createTime = QDateTime::currentDateTime();
    newStop.updateTime = QDateTime::currentDateTime();
    newStop.isActive = true;

    d->stopLossTakeProfits[stopId] = newStop;

    LOG_INFO(QString("Stop loss/take profit set: %1, SL: %2, TP: %3")
        .arg(stopId).arg(stop.stopLossPrice).arg(stop.takeProfitPrice));

    return stopId;
}

bool OrderManager::updateStopLossTakeProfit(const QString& stopId, const StopLossTakeProfit& stop)
{
    QMutexLocker locker(&d->mutex);

    if (!d->stopLossTakeProfits.contains(stopId)) {
        return false;
    }

    StopLossTakeProfit& existing = d->stopLossTakeProfits[stopId];
    existing.stopLossPrice = stop.stopLossPrice;
    existing.takeProfitPrice = stop.takeProfitPrice;
    existing.trailingStopPercent = stop.trailingStopPercent;
    existing.trailingStopPrice = stop.trailingStopPrice;
    existing.volume = stop.volume;
    existing.updateTime = QDateTime::currentDateTime();

    LOG_INFO(QString("Stop loss/take profit updated: %1").arg(stopId));
    return true;
}

bool OrderManager::removeStopLossTakeProfit(const QString& stopId)
{
    QMutexLocker locker(&d->mutex);

    if (!d->stopLossTakeProfits.contains(stopId)) {
        return false;
    }

    d->stopLossTakeProfits.remove(stopId);
    LOG_INFO(QString("Stop loss/take profit removed: %1").arg(stopId));
    return true;
}

// ============================================================================
// 查询接口
// ============================================================================

std::optional<OrderInfo> OrderManager::getOrder(const QString& orderId) const
{
    QMutexLocker locker(&d->mutex);

    if (d->orderMap.contains(orderId)) {
        return d->orderMap[orderId];
    }
    return std::nullopt;
}

QVector<OrderInfo> OrderManager::getActiveOrders() const
{
    QMutexLocker locker(&d->mutex);

    QVector<OrderInfo> result;
    for (const auto& order : d->orderMap) {
        if (order.isActive()) {
            result.append(order);
        }
    }
    return result;
}

QVector<OrderInfo> OrderManager::getActiveOrders(const QString& instrumentId) const
{
    QMutexLocker locker(&d->mutex);

    QVector<OrderInfo> result;
    for (const auto& order : d->orderMap) {
        if (order.isActive() && order.instrumentId == instrumentId) {
            result.append(order);
        }
    }
    return result;
}

QVector<OrderInfo> OrderManager::getHistoryOrders(const QDateTime& from, const QDateTime& to) const
{
    QMutexLocker locker(&d->mutex);

    QVector<OrderInfo> result;
    for (auto it = d->historyOrders.lowerBound(from); 
         it != d->historyOrders.end() && it.key() <= to; ++it) {
        result.append(it.value());
    }
    return result;
}

QVector<TradeRecord> OrderManager::getTradeRecords(const QString& orderId) const
{
    QMutexLocker locker(&d->mutex);

    if (orderId.isEmpty()) {
        return d->tradeRecords;
    }

    QVector<TradeRecord> result;
    for (const auto& trade : d->tradeRecords) {
        if (trade.orderId == orderId) {
            result.append(trade);
        }
    }
    return result;
}

QVector<ConditionOrder> OrderManager::getConditionOrders() const
{
    QMutexLocker locker(&d->mutex);
    return d->conditionOrders.values().toVector();
}

QVector<StopLossTakeProfit> OrderManager::getStopLossTakeProfits(const QString& instrumentId) const
{
    QMutexLocker locker(&d->mutex);

    QVector<StopLossTakeProfit> result;
    for (const auto& stop : d->stopLossTakeProfits) {
        if (instrumentId.isEmpty() || stop.instrumentId == instrumentId) {
            result.append(stop);
        }
    }
    return result;
}

OrderManager::OrderStats OrderManager::getStats() const
{
    QMutexLocker locker(&d->mutex);
    return d->stats;
}

// ============================================================================
// 新增方法实现
// ============================================================================

void OrderManager::updateOrderId(const QString& orderId, const QString& ctpOrderId)
{
    QMutexLocker locker(&d->mutex);

    if (d->orderMap.contains(orderId)) {
        d->orderMap[orderId].orderId = ctpOrderId;
        LOG_DEBUG(QString("Order ID updated: %1 -> %2").arg(orderId, ctpOrderId));
    }
}

void OrderManager::updateOrderStatus(const QString& orderId, OrderStatus status, int filledVolume)
{
    QMutexLocker locker(&d->mutex);

    if (!d->orderMap.contains(orderId)) {
        LOG_WARNING(QString("Order not found for status update: %1").arg(orderId));
        return;
    }

    OrderInfo& order = d->orderMap[orderId];
    OrderStatus oldStatus = order.status;

    order.status = status;
    order.filledVolume = filledVolume;
    order.updateTime = QDateTime::currentDateTime();

    LOG_DEBUG(QString("Order status updated: %1, %2 -> %3, filled: %4")
        .arg(orderId)
        .arg(TradingUtils::statusToString(oldStatus))
        .arg(TradingUtils::statusToString(status))
        .arg(filledVolume));

    // 更新统计
    if (oldStatus != status) {
        bool wasActive = (oldStatus == OrderStatus::Pending ||
                         oldStatus == OrderStatus::Submitted ||
                         oldStatus == OrderStatus::Accepted ||
                         oldStatus == OrderStatus::PartialFilled);
        bool isActive = order.isActive();

        if (wasActive && !isActive) {
            d->stats.activeOrders--;
        }

        switch (status) {
        case OrderStatus::Filled:
            d->stats.filledOrders++;
            break;
        case OrderStatus::Cancelled:
            d->stats.cancelledOrders++;
            break;
        case OrderStatus::Rejected:
            d->stats.rejectedOrders++;
            break;
        default:
            break;
        }
    }

    emit orderUpdated(order);
}

void OrderManager::recordTrade(const TradeRecord& trade)
{
    QMutexLocker locker(&d->mutex);

    d->tradeRecords.append(trade);

    LOG_INFO(QString("Trade recorded: %1 %2 %3@%4")
        .arg(trade.instrumentId)
        .arg(TradingUtils::directionToString(trade.direction))
        .arg(trade.volume)
        .arg(trade.price));
}

void OrderManager::addStopLossTakeProfit(const StopLossTakeProfit& sltp)
{
    QMutexLocker locker(&d->mutex);

    QString id = sltp.stopId.isEmpty() 
        ? QUuid::createUuid().toString(QUuid::WithoutBraces) 
        : sltp.stopId;

    StopLossTakeProfit newSltp = sltp;
    newSltp.stopId = id;
    newSltp.createTime = QDateTime::currentDateTime();

    d->stopLossTakeProfits[id] = newSltp;

    LOG_INFO(QString("StopLoss/TakeProfit added: %1 SL=%2 TP=%3")
        .arg(sltp.instrumentId)
        .arg(sltp.stopLossPrice)
        .arg(sltp.takeProfitPrice));
}

// ============================================================================
// 内部方法
// ============================================================================

QString OrderManager::generateOrderId()
{
    return QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz") + 
           QString::number(++d->orderCounter).rightJustified(6, '0');
}

void OrderManager::updateOrderCache(const OrderInfo& order)
{
    d->orderMap[order.orderId] = order;

    // 更新统计
    if (order.isFinished()) {
        d->stats.activeOrders--;
        
        switch (order.status) {
            case OrderStatus::Filled:
                d->stats.filledOrders++;
                d->stats.totalTurnover += order.turnover;
                d->stats.totalCommission += order.commission;
                break;
            case OrderStatus::Cancelled:
                d->stats.cancelledOrders++;
                break;
            case OrderStatus::Rejected:
                d->stats.rejectedOrders++;
                break;
            default:
                break;
        }

        // 移动到历史记录
        d->historyOrders[order.updateTime] = order;
        d->orderMap.remove(order.orderId);
    }
}

void OrderManager::addTradeRecord(const TradeRecord& trade)
{
    d->tradeRecords.append(trade);

    // 限制历史记录数量
    while (d->tradeRecords.size() > 10000) {
        d->tradeRecords.removeFirst();
    }
}

// ============================================================================
// 槽函数
// ============================================================================

void OrderManager::onMarketDataUpdated(const QString& instrumentId, double lastPrice)
{
    // 行情更新时检查条件单和止损止盈
    // 这里不直接处理，由定时器统一检查
    Q_UNUSED(instrumentId)
    Q_UNUSED(lastPrice)
}

void OrderManager::onCtpOrderReturn(const OrderInfo& order)
{
    QMutexLocker locker(&d->mutex);

    // 更新订单缓存
    updateOrderCache(order);

    // 发送信号
    emit orderUpdated(order);

    // 根据状态发送不同信号
    if (order.status == OrderStatus::Filled) {
        emit orderFilled(order.orderId, TradeRecord());
    } else if (order.status == OrderStatus::Cancelled) {
        emit orderCancelled(order.orderId);
    } else if (order.status == OrderStatus::Rejected) {
        emit orderRejected(order.orderId, order.errorMsg);
    }

    LOG_DEBUG(QString("Order return: %1, status: %2")
        .arg(order.orderId, TradingUtils::statusToString(order.status)));
}

void OrderManager::onCtpTradeReturn(const TradeRecord& trade)
{
    QMutexLocker locker(&d->mutex);

    // 添加成交记录
    addTradeRecord(trade);

    // 更新订单信息
    if (d->orderMap.contains(trade.orderId)) {
        OrderInfo& order = d->orderMap[trade.orderId];
        order.filledVolume += trade.volume;
        order.remainingVolume = order.volume - order.filledVolume;
        order.turnover += trade.turnover;
        order.commission += trade.commission;
        
        // 计算均价
        if (order.filledVolume > 0) {
            order.avgPrice = order.turnover / (order.filledVolume * order.price / trade.price);
        }
    }

    LOG_DEBUG(QString("Trade return: %1, price: %2, volume: %3")
        .arg(trade.tradeId).arg(trade.price).arg(trade.volume));
}

void OrderManager::onCtpOrderError(const QString& requestId, int errorCode, const QString& errorMsg)
{
    QMutexLocker locker(&d->mutex);

    if (d->orderMap.contains(requestId)) {
        OrderInfo& order = d->orderMap[requestId];
        order.status = OrderStatus::Rejected;
        order.errorMsg = errorMsg;
        order.updateTime = QDateTime::currentDateTime();

        emit orderRejected(requestId, errorMsg);
    }

    emit errorOccurred(errorCode, errorMsg);

    LOG_ERROR(QString("Order error: %1, code: %2, msg: %3")
        .arg(requestId).arg(errorCode).arg(errorMsg));
}

void OrderManager::checkConditionOrders()
{
    QMutexLocker locker(&d->mutex);

    for (auto& condition : d->conditionOrders) {
        if (!condition.isActive || condition.isTriggered) {
            continue;
        }

        // TODO: 获取最新价格进行检查
        // checkConditionOrder(condition, lastPrice);
    }
}

void OrderManager::checkStopLossTakeProfit()
{
    QMutexLocker locker(&d->mutex);

    for (auto& stop : d->stopLossTakeProfits) {
        if (!stop.isActive) {
            continue;
        }

        // TODO: 获取最新价格进行检查
        // checkStopLossTakeProfit(stop, lastPrice);
    }
}

void OrderManager::checkConditionOrder(const ConditionOrder& condition, double price)
{
    bool triggered = false;

    switch (condition.conditionType) {
        case ConditionType::PriceAbove:
            triggered = (price >= condition.triggerPrice);
            break;
        case ConditionType::PriceBelow:
            triggered = (price <= condition.triggerPrice);
            break;
        case ConditionType::TimeReach:
            triggered = (QDateTime::currentDateTime() >= condition.triggerTime);
            break;
        default:
            break;
    }

    if (triggered) {
        // 标记为已触发
        d->conditionOrders[condition.conditionId].isTriggered = true;
        d->conditionOrders[condition.conditionId].triggerTimeActual = QDateTime::currentDateTime();

        // 提交订单
        QString orderId = submitOrder(condition.orderRequest);

        emit conditionTriggered(condition.conditionId, orderId);

        LOG_INFO(QString("Condition order triggered: %1 -> %2")
            .arg(condition.conditionId, orderId));
    }
}

void OrderManager::checkStopLossTakeProfit(const StopLossTakeProfit& stop, double price)
{
    bool triggered = false;
    bool isStopLoss = false;

    // 检查止损
    if (stop.stopLossPrice > 0) {
        if (stop.direction == PositionDirection::Long && price <= stop.stopLossPrice) {
            triggered = true;
            isStopLoss = true;
        } else if (stop.direction == PositionDirection::Short && price >= stop.stopLossPrice) {
            triggered = true;
            isStopLoss = true;
        }
    }

    // 检查止盈
    if (!triggered && stop.takeProfitPrice > 0) {
        if (stop.direction == PositionDirection::Long && price >= stop.takeProfitPrice) {
            triggered = true;
        } else if (stop.direction == PositionDirection::Short && price <= stop.takeProfitPrice) {
            triggered = true;
        }
    }

    if (triggered) {
        // 标记为非激活
        d->stopLossTakeProfits[stop.stopId].isActive = false;

        // 创建平仓订单
        OrderRequest request;
        request.instrumentId = stop.instrumentId;
        request.direction = stop.direction == PositionDirection::Long 
            ? TradeDirection::SellClose : TradeDirection::BuyClose;
        request.openClose = OpenCloseFlag::Close;
        request.orderType = OrderType::Market;
        request.volume = stop.volume;

        QString orderId = submitOrder(request);

        emit stopLossTakeProfitTriggered(stop.stopId, orderId);

        LOG_INFO(QString("Stop loss/take profit triggered: %1 (%2) -> %3")
            .arg(stop.stopId)
            .arg(isStopLoss ? "止损" : "止盈")
            .arg(orderId));
    }
}
