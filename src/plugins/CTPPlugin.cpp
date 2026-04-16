/**
 * @file CTPPlugin.cpp
 * @brief CTP Plugin Implementation
 */

#include "CTPPlugin.h"
#include "../core/config/EnvironmentConfig.h"
#include "../utils/Logger.h"
#include <QTimer>
#include <QDateTime>
#include <QMutexLocker>
#include <QElapsedTimer>

namespace CTP {

// ========== PIMPL Implementation ==========

class CTPPlugin::Impl {
public:
    // Connection state
    bool connected = false;
    QString brokerId;
    QString userId;
    QString tradingDay;
    
    // Market data cache
    QMap<QString, MarketData> marketDataCache;
    QMutex marketDataMutex;
    
    // Order management
    QMap<QString, OrderData> orders;
    QMap<QString, OrderData> trades;
    int orderRefCounter = 0;
    QMutex orderMutex;
    
    // Account info
    AccountData accountData;
    QList<AccountData> positions;
    QMutex accountMutex;
    
    // Batch subscription buffer
    QQueue<QString> subscribeBuffer;
    QTimer* subscribeBufferTimer = nullptr;
    static const int SUBSCRIBE_BUFFER_INTERVAL = 100; // 100ms
    
    // Helper methods
    QString generateOrderRef() {
        QMutexLocker locker(&orderMutex);
        return QString::number(++orderRefCounter).rightJustified(12, '0');
    }
    
    void flushSubscribeBuffer() {
        // Batch subscription logic
    }
};

// ========== Constructor and Destructor ==========

CTPPlugin::CTPPlugin()
    : d(std::make_unique<Impl>())
    , m_state(PluginState::Unloaded)
    , m_subscribeBufferTimer(new QTimer(this))
{
    LOG_DEBUG("CTPPlugin created");
}

CTPPlugin::~CTPPlugin()
{
    if (m_state == PluginState::Running) {
        stop();
    }
    if (m_state == PluginState::Loaded) {
        unload();
    }
    LOG_DEBUG("CTPPlugin destroyed");
}

// ========== IPlugin Interface Implementation ==========

PluginMetaData CTPPlugin::metaData() const
{
    PluginMetaData meta;
    meta.name = "CTPPlugin";
    meta.version = "2.0.0";
    meta.description = "CTP Trading Plugin - Support market data and trading";
    meta.author = "WealthPilot Team";
    meta.license = "MIT";
    meta.website = "https://wealthpilot.com";
    meta.dependencies = QStringList();
    meta.priority = 10;
    meta.enableHotReload = true;
    return meta;
}

PluginState CTPPlugin::state() const
{
    return m_state;
}

bool CTPPlugin::load()
{
    QElapsedTimer timer;
    timer.start();
    
    if (m_state != PluginState::Unloaded) {
        LOG_WARNING("CTPPlugin already loaded");
        return true;
    }
    
    setState(PluginState::Loading);
    
    // Initialize subscription buffer timer
    QObject::connect(m_subscribeBufferTimer, &QTimer::timeout, this, &CTPPlugin::flushSubscribeBuffer);
    
    setState(PluginState::Loaded);
    
    LOG_INFO(QString("CTPPlugin loaded in %1ms").arg(timer.elapsed()));
    return true;
}

bool CTPPlugin::initialize(const QJsonObject& config)
{
    QElapsedTimer timer;
    timer.start();
    
    if (m_state != PluginState::Loaded) {
        LOG_ERROR("CTPPlugin not loaded, cannot initialize");
        return false;
    }
    
    setState(PluginState::Loading);
    
    m_config = config;
    
    // Load environment config
    loadEnvironmentConfig();
    
    setState(PluginState::Initialized);
    
    LOG_INFO(QString("CTPPlugin initialized in %1ms").arg(timer.elapsed()));
    return true;
}

bool CTPPlugin::start()
{
    QElapsedTimer timer;
    timer.start();
    
    if (m_state != PluginState::Initialized) {
        LOG_ERROR("CTPPlugin not initialized, cannot start");
        return false;
    }
    
    setState(PluginState::Loaded);
    
    // Start subscription buffer timer
    m_subscribeBufferTimer->start(Impl::SUBSCRIBE_BUFFER_INTERVAL);
    
    setState(PluginState::Running);
    
    LOG_INFO(QString("CTPPlugin started in %1ms").arg(timer.elapsed()));
    return true;
}

void CTPPlugin::stop()
{
    if (m_state != PluginState::Running) {
        return;
    }
    
    setState(PluginState::Stopped);
    
    // Stop subscription buffer timer
    m_subscribeBufferTimer->stop();
    
    // Disconnect if connected
    if (d->connected) {
        disconnect();
    }
    
    setState(PluginState::Loaded);
    LOG_INFO("CTPPlugin stopped");
}

void CTPPlugin::unload()
{
    if (m_state == PluginState::Running) {
        stop();
    }
    
    if (m_state == PluginState::Initialized) {
        setState(PluginState::Loading);
    }
    
    d.reset();
    setState(PluginState::Unloaded);
    LOG_INFO("CTPPlugin unloaded");
}

QJsonObject CTPPlugin::configuration() const
{
    return m_config;
}

void CTPPlugin::setConfiguration(const QJsonObject& config)
{
    m_config = config;
    emit configurationChanged();
}

bool CTPPlugin::checkDependencies() const
{
    // No dependencies
    return true;
}

QStringList CTPPlugin::dependencies() const
{
    return QStringList();
}

// ========== ICTPPlugin Interface Implementation ==========

bool CTPPlugin::connect(const QString& brokerId, const QString& userId,
                       const QString& password, const QString& marketFront,
                       const QString& tradeFront)
{
    LOG_INFO(QString("Connecting to CTP: %1:%2").arg(brokerId, userId));
    
    d->brokerId = brokerId;
    d->userId = userId;
    
    // TODO: Actual CTP connection implementation
    d->connected = true;
    d->tradingDay = QDateTime::currentDateTime().toString("yyyyMMdd");
    
    emit connected();
    LOG_INFO("CTP connected");
    return true;
}

void CTPPlugin::disconnect()
{
    if (!d->connected) {
        return;
    }
    
    LOG_INFO("Disconnecting from CTP");
    
    // TODO: Actual CTP disconnection implementation
    d->connected = false;
    
    emit disconnected();
    LOG_INFO("CTP disconnected");
}

bool CTPPlugin::isConnected() const
{
    return d->connected;
}

bool CTPPlugin::subscribeMarketData(const QStringList& instruments)
{
    if (!d->connected) {
        LOG_ERROR("Not connected to CTP");
        return false;
    }
    
    LOG_INFO(QString("Subscribing market data: %1 instruments").arg(instruments.size()));
    
    // Add to buffer for batch subscription
    for (const QString& instrument : instruments) {
        d->subscribeBuffer.enqueue(instrument);
    }
    
    return true;
}

void CTPPlugin::unsubscribeMarketData(const QStringList& instruments)
{
    LOG_INFO(QString("Unsubscribing market data: %1 instruments").arg(instruments.size()));
    // TODO: Actual unsubscription implementation
}

MarketData CTPPlugin::getMarketData(const QString& instrumentId) const
{
    QMutexLocker locker(&d->marketDataMutex);
    return d->marketDataCache.value(instrumentId);
}

QMap<QString, MarketData> CTPPlugin::getAllMarketData() const
{
    QMutexLocker locker(&d->marketDataMutex);
    return d->marketDataCache;
}

QString CTPPlugin::sendOrder(const QString& instrumentId, const QString& direction,
                            const QString& offsetFlag, double price, int volume)
{
    if (!d->connected) {
        LOG_ERROR("Not connected to CTP");
        return QString();
    }
    
    QString orderId = d->generateOrderRef();
    
    LOG_INFO(QString("Sending order: %1 %2 %3 @ %4 x %5")
        .arg(instrumentId, direction, offsetFlag)
        .arg(price).arg(volume));
    
    // TODO: Actual order submission implementation
    
    return orderId;
}

bool CTPPlugin::cancelOrder(const QString& orderId)
{
    LOG_INFO(QString("Canceling order: %1").arg(orderId));
    // TODO: Actual order cancellation implementation
    return true;
}

QList<OrderData> CTPPlugin::queryOrders()
{
    QMutexLocker locker(&d->orderMutex);
    return d->orders.values();
}

QList<OrderData> CTPPlugin::queryTrades()
{
    QMutexLocker locker(&d->orderMutex);
    return d->trades.values();
}

AccountData CTPPlugin::queryAccount()
{
    QMutexLocker locker(&d->accountMutex);
    return d->accountData;
}

QList<AccountData> CTPPlugin::queryPositions()
{
    QMutexLocker locker(&d->accountMutex);
    return d->positions;
}

// ========== Private Methods ==========

void CTPPlugin::flushSubscribeBuffer()
{
    if (d->subscribeBuffer.isEmpty()) {
        return;
    }
    
    QStringList instruments;
    while (!d->subscribeBuffer.isEmpty() && instruments.size() < 100) {
        instruments.append(d->subscribeBuffer.dequeue());
    }
    
    if (!instruments.isEmpty()) {
        LOG_DEBUG(QString("Flushing subscription buffer: %1 instruments").arg(instruments.size()));
        // TODO: Actual batch subscription implementation
    }
}

void CTPPlugin::updateMarketDataCache(const QString& instrumentId, const MarketData& data)
{
    QMutexLocker locker(&d->marketDataMutex);
    d->marketDataCache[instrumentId] = data;
    
    // Also update CacheManager
    CacheManager::instance()->set(
        QString("market_%1").arg(instrumentId),
        QVariant::fromValue(data),
        60  // 60 seconds TTL
    );
}

MarketData CTPPlugin::getCachedMarketData(const QString& instrumentId) const
{
    // Try CacheManager first
    QVariant cached = CacheManager::instance()->get(
        QString("market_%1").arg(instrumentId)
    );
    
    if (cached.isValid()) {
        return cached.value<MarketData>();
    }
    
    // Fall back to local cache
    QMutexLocker locker(&d->marketDataMutex);
    return d->marketDataCache.value(instrumentId);
}

void CTPPlugin::loadEnvironmentConfig()
{
    auto settings = EnvironmentConfig::instance()->currentSettings();
    if (settings) {
        if (!m_config.contains("brokerId")) {
            m_config["brokerId"] = settings->ctpBrokerId;
        }
        if (!m_config.contains("marketFront")) {
            m_config["marketFront"] = settings->ctpMarketFront;
        }
        if (!m_config.contains("tradeFront")) {
            m_config["tradeFront"] = settings->ctpTradeFront;
        }
        
        LOG_DEBUG("CTP environment config loaded");
    }
}

void CTPPlugin::setState(PluginState newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
    }
}

QString CTPPlugin::generateOrderRef()
{
    return d->generateOrderRef();
}

} // namespace CTP
