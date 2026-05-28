/**
 * @file ConditionOrderEngine.cpp
 * @brief 条件单引擎实现
 */

#include "ConditionOrderEngine.h"
#include "TradingService.h"
#include "shared/utils/Logger.h"

#include <QMutexLocker>
#include <QSettings>

struct ConditionOrderEngine::Impl {
    QHash<QString, ConditionOrder> conditionOrders;
    QHash<QString, double> priceCache;  // instrumentId -> lastPrice
    QTimer* checkTimer = nullptr;
    mutable QMutex mutex;
    bool initialized = false;
    int checkInterval = 500;  // 500ms
};

ConditionOrderEngine& ConditionOrderEngine::instance()
{
    static ConditionOrderEngine instance;
    return instance;
}

ConditionOrderEngine::ConditionOrderEngine(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->checkTimer = new QTimer(this);
    connect(d->checkTimer, &QTimer::timeout, this, &ConditionOrderEngine::onCheckTimer);
    LOG_DEBUG("ConditionOrderEngine created");
}

ConditionOrderEngine::~ConditionOrderEngine()
{
    shutdown();
    LOG_DEBUG("ConditionOrderEngine destroyed");
}

bool ConditionOrderEngine::initialize()
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        return true;
    }

    // 加载保存的条件单
    loadConditionOrders();

    // 启动检查定时器
    d->checkTimer->start(d->checkInterval);

    // 连接交易服务信号
    connect(&TradingService::instance(), &TradingService::orderSubmitted,
            this, &ConditionOrderEngine::onOrderSubmitted);
    connect(&TradingService::instance(), &TradingService::orderRejected,
            this, &ConditionOrderEngine::onOrderRejected);

    d->initialized = true;
    LOG_INFO("ConditionOrderEngine initialized");
    return true;
}

void ConditionOrderEngine::shutdown()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    d->checkTimer->stop();
    saveConditionOrders();

    d->conditionOrders.clear();
    d->priceCache.clear();
    d->initialized = false;

    LOG_INFO("ConditionOrderEngine shutdown");
}

QString ConditionOrderEngine::addConditionOrder(const ConditionOrder& condition)
{
    QMutexLocker locker(&d->mutex);

    QString id = condition.conditionId.isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
        : condition.conditionId;

    ConditionOrder newCondition = condition;
    newCondition.conditionId = id;
    newCondition.createTime = QDateTime::currentDateTime();
    newCondition.isActive = true;
    newCondition.isTriggered = false;

    d->conditionOrders[id] = newCondition;
    saveConditionOrders();

    LOG_INFO(QString("Condition order added: %1, type: %2, trigger price: %3")
        .arg(id)
        .arg(static_cast<int>(condition.conditionType))
        .arg(condition.triggerPrice));

    emit conditionAdded(id);
    return id;
}

bool ConditionOrderEngine::removeConditionOrder(const QString& conditionId)
{
    QMutexLocker locker(&d->mutex);

    if (!d->conditionOrders.contains(conditionId)) {
        return false;
    }

    d->conditionOrders.remove(conditionId);
    saveConditionOrders();

    LOG_INFO(QString("Condition order removed: %1").arg(conditionId));
    emit conditionRemoved(conditionId);
    return true;
}

void ConditionOrderEngine::setConditionOrderActive(const QString& conditionId, bool active)
{
    QMutexLocker locker(&d->mutex);

    if (d->conditionOrders.contains(conditionId)) {
        d->conditionOrders[conditionId].isActive = active;
        saveConditionOrders();
        LOG_DEBUG(QString("Condition order %1 set active: %2").arg(conditionId).arg(active));
    }
}

QVector<ConditionOrder> ConditionOrderEngine::getConditionOrders() const
{
    QMutexLocker locker(&d->mutex);
    return d->conditionOrders.values().toVector();
}

QVector<ConditionOrder> ConditionOrderEngine::getConditionOrders(const QString& instrumentId) const
{
    QMutexLocker locker(&d->mutex);

    QVector<ConditionOrder> result;
    for (const auto& condition : d->conditionOrders) {
        if (condition.instrumentId == instrumentId) {
            result.append(condition);
        }
    }
    return result;
}

void ConditionOrderEngine::updatePrice(const QString& instrumentId, double lastPrice)
{
    QMutexLocker locker(&d->mutex);
    d->priceCache[instrumentId] = lastPrice;
}

void ConditionOrderEngine::setCheckInterval(int intervalMs)
{
    d->checkInterval = intervalMs;
    if (d->checkTimer->isActive()) {
        d->checkTimer->setInterval(intervalMs);
    }
}

void ConditionOrderEngine::onCheckTimer()
{
    QMutexLocker locker(&d->mutex);

    for (auto& condition : d->conditionOrders) {
        if (!condition.isActive || condition.isTriggered) {
            continue;
        }

        // 获取最新价格
        if (!d->priceCache.contains(condition.instrumentId)) {
            continue;
        }

        double price = d->priceCache[condition.instrumentId];
        checkCondition(condition, price);
    }
}

void ConditionOrderEngine::checkCondition(const ConditionOrder& condition, double price)
{
    bool triggered = false;

    switch (condition.conditionType) {
    case ConditionType::PriceAbove:
        // 价格高于触发价
        triggered = (price >= condition.triggerPrice);
        break;

    case ConditionType::PriceBelow:
        // 价格低于触发价
        triggered = (price <= condition.triggerPrice);
        break;

    case ConditionType::TimeReach:
        // 时间到达
        triggered = (QDateTime::currentDateTime() >= condition.triggerTime);
        break;

    default:
        break;
    }

    if (triggered) {
        ConditionOrder& c = d->conditionOrders[condition.conditionId];
        triggerCondition(c);
    }
}

void ConditionOrderEngine::triggerCondition(ConditionOrder& condition)
{
    LOG_INFO(QString("Condition order triggered: %1").arg(condition.conditionId));

    // 标记为已触发
    condition.isTriggered = true;
    condition.triggerTimeActual = QDateTime::currentDateTime();

    // 提交订单
    QString orderId = TradingService::instance().submitOrder(condition.orderRequest);

    if (!orderId.isEmpty()) {
        condition.triggeredOrderId = orderId;
        emit conditionTriggered(condition.conditionId, orderId);
    } else {
        emit triggerFailed(condition.conditionId, "订单提交失败");
    }

    saveConditionOrders();
}

void ConditionOrderEngine::onOrderSubmitted(const QString& orderId)
{
    Q_UNUSED(orderId)
    // 订单提交成功，无需处理
}

void ConditionOrderEngine::onOrderRejected(const QString& orderId, const QString& reason)
{
    // 检查是否是条件单触发的订单
    QMutexLocker locker(&d->mutex);

    for (auto& condition : d->conditionOrders) {
        if (condition.triggeredOrderId == orderId) {
            LOG_WARNING(QString("Condition order %1 trigger failed: %2")
                .arg(condition.conditionId).arg(reason));
            emit triggerFailed(condition.conditionId, reason);
            break;
        }
    }
}

void ConditionOrderEngine::saveConditionOrders()
{
    QSettings settings("WealthPilot", "ConditionOrders");
    settings.clear();

    int index = 0;
    for (const auto& condition : d->conditionOrders) {
        QString key = QString("condition_%1").arg(index++);
        settings.setValue(key + "/id", condition.conditionId);
        settings.setValue(key + "/instrumentId", condition.instrumentId);
        settings.setValue(key + "/conditionType", static_cast<int>(condition.conditionType));
        settings.setValue(key + "/triggerPrice", condition.triggerPrice);
        settings.setValue(key + "/triggerTime", condition.triggerTime);
        settings.setValue(key + "/isActive", condition.isActive);
        settings.setValue(key + "/isTriggered", condition.isTriggered);
    }

    settings.setValue("count", index);
    settings.sync();
}

void ConditionOrderEngine::loadConditionOrders()
{
    QSettings settings("WealthPilot", "ConditionOrders");
    int count = settings.value("count", 0).toInt();

    for (int i = 0; i < count; ++i) {
        QString key = QString("condition_%1/").arg(i);
        ConditionOrder condition;
        condition.conditionId = settings.value(key + "id").toString();
        condition.instrumentId = settings.value(key + "instrumentId").toString();
        condition.conditionType = static_cast<ConditionType>(settings.value(key + "conditionType").toInt());
        condition.triggerPrice = settings.value(key + "triggerPrice").toDouble();
        condition.triggerTime = settings.value(key + "triggerTime").toDateTime();
        condition.isActive = settings.value(key + "isActive", true).toBool();
        condition.isTriggered = settings.value(key + "isTriggered", false).toBool();

        if (!condition.conditionId.isEmpty() && !condition.instrumentId.isEmpty()) {
            d->conditionOrders[condition.conditionId] = condition;
        }
    }

    LOG_DEBUG(QString("Loaded %1 condition orders").arg(d->conditionOrders.size()));
}
