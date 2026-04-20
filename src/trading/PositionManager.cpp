/**
 * @file PositionManager.cpp
 * @brief 持仓管理器实现
 *
 * @details 实现：
 * - 实时持仓监控
 * - 盈亏计算
 * - 风险分析
 * - 线程安全的数据访问
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "PositionManager.h"
#include "utils/Logger.h"

#include <QMutexLocker>
#include <QtMath>

// ============================================================================
// PIMPL 实现
// ============================================================================

struct PositionManager::Impl {
    // 持仓存储（使用哈希表快速查找）
    // key: instrumentId_direction
    QHash<QString, PositionInfo> positionMap;

    // 价格缓存（用于盈亏计算）
    QHash<QString, double> priceCache;

    // 账户信息
    AccountInfo accountInfo;

    // 线程安全
    mutable QMutex mutex;

    // 初始化标志
    bool initialized = false;
};

// ============================================================================
// 单例实现
// ============================================================================

PositionManager& PositionManager::instance()
{
    static PositionManager instance;
    return instance;
}

PositionManager::PositionManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    LOG_DEBUG("PositionManager created");
}

PositionManager::~PositionManager()
{
    shutdown();
    LOG_DEBUG("PositionManager destroyed");
}

// ============================================================================
// 初始化
// ============================================================================

bool PositionManager::initialize()
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        LOG_WARNING("PositionManager already initialized");
        return true;
    }

    d->initialized = true;
    LOG_INFO("PositionManager initialized");
    return true;
}

void PositionManager::shutdown()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    d->positionMap.clear();
    d->priceCache.clear();
    d->initialized = false;

    LOG_INFO("PositionManager shutdown");
}

// ============================================================================
// 持仓操作
// ============================================================================

void PositionManager::updatePosition(const PositionInfo& position)
{
    QMutexLocker locker(&d->mutex);

    QString key = makePositionKey(position.instrumentId, position.direction);

    // 更新持仓
    d->positionMap[key] = position;

    // 更新价格缓存
    d->priceCache[position.instrumentId] = position.marketPrice;

    LOG_DEBUG(QString("Position updated: %1 %2, volume: %3, profit: %4")
        .arg(position.instrumentId)
        .arg(TradingUtils::positionDirToString(position.direction))
        .arg(position.volume)
        .arg(position.profit));

    emit positionUpdated(position);
    emit profitUpdated(getTotalProfit());
}

void PositionManager::removePosition(const QString& instrumentId, PositionDirection direction)
{
    QMutexLocker locker(&d->mutex);

    QString key = makePositionKey(instrumentId, direction);

    if (d->positionMap.contains(key)) {
        d->positionMap.remove(key);
        LOG_DEBUG(QString("Position removed: %1 %2")
            .arg(instrumentId, TradingUtils::positionDirToString(direction)));
        emit positionRemoved(instrumentId, direction);
    }
}

void PositionManager::clearPositions()
{
    QMutexLocker locker(&d->mutex);

    d->positionMap.clear();
    LOG_DEBUG("All positions cleared");
}

void PositionManager::updatePositionPrice(const QString& instrumentId, double lastPrice)
{
    QMutexLocker locker(&d->mutex);

    // 更新价格缓存
    d->priceCache[instrumentId] = lastPrice;

    // 更新所有相关持仓的盈亏
    bool profitChanged = false;

    for (auto it = d->positionMap.begin(); it != d->positionMap.end(); ++it) {
        if (it->instrumentId == instrumentId) {
            calculatePositionProfit(*it, lastPrice);
            profitChanged = true;
        }
    }

    if (profitChanged) {
        emit profitUpdated(getTotalProfit());
    }
}

// ============================================================================
// 查询接口
// ============================================================================

QVector<PositionInfo> PositionManager::getPositions() const
{
    QMutexLocker locker(&d->mutex);
    return d->positionMap.values().toVector();
}

QVector<PositionInfo> PositionManager::getPositions(const QString& instrumentId) const
{
    QMutexLocker locker(&d->mutex);

    QVector<PositionInfo> result;
    for (const auto& position : d->positionMap) {
        if (position.instrumentId == instrumentId) {
            result.append(position);
        }
    }
    return result;
}

std::optional<PositionInfo> PositionManager::getPosition(const QString& instrumentId, 
                                                          PositionDirection direction) const
{
    QMutexLocker locker(&d->mutex);

    QString key = makePositionKey(instrumentId, direction);
    if (d->positionMap.contains(key)) {
        return d->positionMap[key];
    }
    return std::nullopt;
}

bool PositionManager::hasPosition(const QString& instrumentId) const
{
    QMutexLocker locker(&d->mutex);

    for (const auto& position : d->positionMap) {
        if (position.instrumentId == instrumentId && position.volume > 0) {
            return true;
        }
    }
    return false;
}

int PositionManager::getPositionVolume(const QString& instrumentId, PositionDirection direction) const
{
    QMutexLocker locker(&d->mutex);

    QString key = makePositionKey(instrumentId, direction);
    if (d->positionMap.contains(key)) {
        return d->positionMap[key].volume;
    }
    return 0;
}

int PositionManager::getCloseableVolume(const QString& instrumentId, PositionDirection direction) const
{
    QMutexLocker locker(&d->mutex);

    QString key = makePositionKey(instrumentId, direction);
    if (d->positionMap.contains(key)) {
        return d->positionMap[key].closeableVolume();
    }
    return 0;
}

// ============================================================================
// 盈亏计算
// ============================================================================

double PositionManager::getTotalProfit() const
{
    QMutexLocker locker(&d->mutex);

    double total = 0.0;
    for (const auto& position : d->positionMap) {
        total += position.profit;
    }
    return total;
}

double PositionManager::getTodayProfit() const
{
    QMutexLocker locker(&d->mutex);

    double total = 0.0;
    for (const auto& position : d->positionMap) {
        total += position.todayProfit;
    }
    return total;
}

double PositionManager::getRealizedProfit() const
{
    QMutexLocker locker(&d->mutex);

    double total = 0.0;
    for (const auto& position : d->positionMap) {
        total += position.realizedProfit;
    }
    return total;
}

double PositionManager::getInstrumentProfit(const QString& instrumentId) const
{
    QMutexLocker locker(&d->mutex);

    double total = 0.0;
    for (const auto& position : d->positionMap) {
        if (position.instrumentId == instrumentId) {
            total += position.profit;
        }
    }
    return total;
}

double PositionManager::getProfitRatio() const
{
    QMutexLocker locker(&d->mutex);

    double totalCost = 0.0;
    double totalProfit = 0.0;

    for (const auto& position : d->positionMap) {
        totalCost += position.avgPrice * position.volume;
        totalProfit += position.profit;
    }

    if (totalCost <= 0) {
        return 0.0;
    }

    return totalProfit / totalCost * 100.0;
}

// ============================================================================
// 保证金计算
// ============================================================================

double PositionManager::getTotalMargin() const
{
    QMutexLocker locker(&d->mutex);

    double total = 0.0;
    for (const auto& position : d->positionMap) {
        total += position.margin;
    }
    return total;
}

double PositionManager::getTotalFrozenMargin() const
{
    QMutexLocker locker(&d->mutex);

    double total = 0.0;
    for (const auto& position : d->positionMap) {
        total += position.frozenMargin;
    }
    return total;
}

double PositionManager::getMarginRatio() const
{
    QMutexLocker locker(&d->mutex);

    if (d->accountInfo.balance <= 0) {
        return 0.0;
    }

    return getTotalMargin() / d->accountInfo.balance * 100.0;
}

// ============================================================================
// 风险分析
// ============================================================================

PositionManager::PositionRisk PositionManager::calculateRisk() const
{
    QMutexLocker locker(&d->mutex);

    PositionRisk risk;

    for (const auto& position : d->positionMap) {
        risk.totalProfit += position.profit;

        if (position.profit > 0) {
            risk.profitCount++;
            risk.maxProfit = qMax(risk.maxProfit, position.profit);
        } else if (position.profit < 0) {
            risk.lossCount++;
            risk.maxLoss = qMin(risk.maxLoss, position.profit);
        } else {
            risk.flatCount++;
        }
    }

    int total = risk.profitCount + risk.lossCount + risk.flatCount;
    if (total > 0) {
        risk.profitRatio = static_cast<double>(risk.profitCount) / total * 100.0;
    }

    // 风险度 = 占用保证金 / 总资产
    if (d->accountInfo.balance > 0) {
        risk.riskLevel = getTotalMargin() / d->accountInfo.balance * 100.0;
    }

    return risk;
}

PositionManager::PositionSummary PositionManager::getSummary() const
{
    QMutexLocker locker(&d->mutex);

    PositionSummary summary;

    for (const auto& position : d->positionMap) {
        summary.totalPositions++;
        summary.totalVolume += position.volume;
        summary.totalValue += position.marketValue;
        summary.totalMargin += position.margin;
        summary.totalProfit += position.profit;
        summary.todayProfit += position.todayProfit;
        summary.realizedProfit += position.realizedProfit;

        if (position.direction == PositionDirection::Long) {
            summary.longPositions++;
        } else if (position.direction == PositionDirection::Short) {
            summary.shortPositions++;
        }
    }

    return summary;
}

// ============================================================================
// 内部方法
// ============================================================================

QString PositionManager::makePositionKey(const QString& instrumentId, PositionDirection direction) const
{
    return QString("%1_%2").arg(instrumentId, QString::number(static_cast<int>(direction)));
}

void PositionManager::calculatePositionProfit(PositionInfo& position, double lastPrice)
{
    // 更新市场价格
    position.marketPrice = lastPrice;
    position.marketValue = lastPrice * position.volume;

    // 计算浮动盈亏
    if (position.direction == PositionDirection::Long) {
        // 多头：(最新价 - 持仓均价) * 持仓量 * 合约乘数
        position.profit = (lastPrice - position.avgPrice) * position.volume;
    } else if (position.direction == PositionDirection::Short) {
        // 空头：(持仓均价 - 最新价) * 持仓量 * 合约乘数
        position.profit = (position.avgPrice - lastPrice) * position.volume;
    }

    position.updateTime = QDateTime::currentDateTime();
}
