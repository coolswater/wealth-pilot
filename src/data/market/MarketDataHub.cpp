/**
 * @file MarketDataHub.cpp
 * @brief 行情数据中心实现
 */

#include "MarketDataHub.h"
#include "infrastructure/ctp/service/CTPService.h"
#include "shared/utils/Logger.h"

#include <QMutexLocker>

// ============================================================================
// PIMPL 实现
// ============================================================================

struct MarketDataHub::Impl {
    // 行情缓存
    QHash<QString, MarketSnapshot> quoteCache;

    // 订阅者映射：合约 -> 订阅者集合
    QHash<QString, QSet<IMarketDataSubscriber*>> subscribers;

    // 订阅的合约列表
    QSet<QString> subscribedInstruments;

    // CTP服务指针
    CTP::CTPService* ctpService = nullptr;

    // 线程安全
    mutable QMutex mutex;

    // 批量更新缓冲
    QVector<MarketSnapshot> batchBuffer;
    QTimer* batchTimer = nullptr;
    bool initialized = false;

    // 统计
    int updateCount = 0;
    qint64 lastUpdateTime = 0;
};

// ============================================================================
// 单例实现
// ============================================================================

MarketDataHub& MarketDataHub::instance()
{
    static MarketDataHub instance;
    return instance;
}

MarketDataHub::MarketDataHub(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    // 创建批量更新定时器
    d->batchTimer = new QTimer(this);
    d->batchTimer->setInterval(50); // 50ms批量发送一次
    connect(d->batchTimer, &QTimer::timeout, this, &MarketDataHub::checkAndEmitSignals);

    LOG_DEBUG("MarketDataHub created");
}

MarketDataHub::~MarketDataHub()
{
    shutdown();
    LOG_DEBUG("MarketDataHub destroyed");
}

// ============================================================================
// 初始化
// ============================================================================

bool MarketDataHub::initialize()
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        LOG_WARNING("MarketDataHub already initialized");
        return true;
    }

    d->batchTimer->start();
    d->initialized = true;

    LOG_INFO("MarketDataHub initialized");
    return true;
}

void MarketDataHub::shutdown()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    if (d->batchTimer) {
        d->batchTimer->stop();
    }

    d->quoteCache.clear();
    d->subscribers.clear();
    d->subscribedInstruments.clear();
    d->batchBuffer.clear();

    d->initialized = false;
    LOG_INFO("MarketDataHub shutdown");
}

// ============================================================================
// 订阅管理
// ============================================================================

void MarketDataHub::subscribe(const QString& instrumentId, IMarketDataSubscriber* subscriber)
{
    QMutexLocker locker(&d->mutex);

    // 添加到订阅者列表
    d->subscribers[instrumentId].insert(subscriber);

    // 如果是新合约，订阅CTP行情
    if (!d->subscribedInstruments.contains(instrumentId)) {
        d->subscribedInstruments.insert(instrumentId);

        // 调用CTP订阅行情
        if (d->ctpService) {
            // 调用CTP服务订阅行情
            d->ctpService->subscribeMarketData({instrumentId});
            LOG_DEBUG(QString("CTP subscribed to: %1").arg(instrumentId));
        } else {
            LOG_WARNING(QString("CTP service not available for subscription: %1").arg(instrumentId));
        }

        LOG_DEBUG(QString("Subscribed to: %1").arg(instrumentId));
    }
}

void MarketDataHub::unsubscribe(const QString& instrumentId, IMarketDataSubscriber* subscriber)
{
    QMutexLocker locker(&d->mutex);

    if (d->subscribers.contains(instrumentId)) {
        d->subscribers[instrumentId].remove(subscriber);

        // 如果没有订阅者了，取消CTP订阅
        if (d->subscribers[instrumentId].isEmpty()) {
            d->subscribers.remove(instrumentId);
            d->subscribedInstruments.remove(instrumentId);

            // 调用CTP取消订阅
            if (d->ctpService) {
                d->ctpService->unsubscribeMarketData({instrumentId});
                LOG_DEBUG(QString("CTP unsubscribed from: %1").arg(instrumentId));
            }
            
            LOG_DEBUG(QString("Unsubscribed from: %1").arg(instrumentId));
        }
    }
}

void MarketDataHub::subscribeBatch(const QStringList& instrumentIds)
{
    QMutexLocker locker(&d->mutex);

    QStringList newInstruments;
    for (const QString& id : instrumentIds) {
        if (!d->subscribedInstruments.contains(id)) {
            d->subscribedInstruments.insert(id);
            newInstruments.append(id);
        }
    }

    // 批量订阅CTP行情
    if (d->ctpService && !newInstruments.isEmpty()) {
        // 批量订阅行情
        d->ctpService->subscribeMarketData(newInstruments);
        LOG_INFO(QString("Batch subscribed: %1 instruments").arg(newInstruments.size()));
    } else if (newInstruments.isEmpty()) {
        LOG_DEBUG("No new instruments to subscribe");
    } else {
        LOG_WARNING("CTP service not available for batch subscription");
    }
}

// ============================================================================
// 数据获取
// ============================================================================

std::optional<MarketSnapshot> MarketDataHub::getQuote(const QString& instrumentId) const
{
    QMutexLocker locker(&d->mutex);

    if (d->quoteCache.contains(instrumentId)) {
        return d->quoteCache[instrumentId];
    }
    return std::nullopt;
}

QVector<MarketSnapshot> MarketDataHub::getAllQuotes() const
{
    QMutexLocker locker(&d->mutex);
    return d->quoteCache.values().toVector();
}

MarketSnapshot MarketDataHub::getSnapshot(const QString& instrumentId) const
{
    QMutexLocker locker(&d->mutex);
    return d->quoteCache.value(instrumentId);
}

// ============================================================================
// 数据更新
// ============================================================================

void MarketDataHub::updateCtpMarketData(const CTP::MarketData& data)
{
    MarketSnapshot quote;
    quote.instrumentId = data.InstrumentID;
    quote.lastPrice = data.lastPrice;
    quote.volume = data.Volume;
    quote.openInterest = data.OpenInterest;
    quote.preClose = data.PreClosePrice;
    quote.updateTime = data.UpdateTime;

    updateMarketData(quote.instrumentId, quote);
}

void MarketDataHub::updateMarketDataBatch(const QVector<MarketSnapshot>& dataList)
{
    QMutexLocker locker(&d->mutex);

    for (const auto& data : dataList) {
        d->quoteCache[data.instrumentId] = data;
        d->batchBuffer.append(data);
    }

    d->updateCount += dataList.size();
}

void MarketDataHub::updateMarketData(const QString& instrumentId, const MarketSnapshot& data)
{
    QMutexLocker locker(&d->mutex);

    // 更新缓存
    d->quoteCache[instrumentId] = data;

    // 添加到批量缓冲
    d->batchBuffer.append(data);

    d->updateCount++;
    d->lastUpdateTime = QDateTime::currentMSecsSinceEpoch();
}

void MarketDataHub::notifySubscribers(const QString& instrumentId, const MarketSnapshot& data)
{
    // 注意：此方法在锁内调用，订阅者的onMarketData应该是线程安全的
    if (d->subscribers.contains(instrumentId)) {
        for (auto* subscriber : d->subscribers[instrumentId]) {
            subscriber->onMarketData(instrumentId, data);
        }
    }
}

void MarketDataHub::checkAndEmitSignals()
{
    QMutexLocker locker(&d->mutex);

    if (d->batchBuffer.isEmpty()) {
        return;
    }

    // 批量发送信号
    QVector<MarketSnapshot> batch = std::move(d->batchBuffer);
    d->batchBuffer.clear();

    // 发送批量更新信号
    emit marketDataBatchUpdated(batch);

    // 同时发送单个更新信号（兼容旧代码）
    for (const auto& data : batch) {
        emit marketDataUpdated(data.instrumentId, data);

        // 通知订阅者
        notifySubscribers(data.instrumentId, data);
    }
}
