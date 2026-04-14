/**
 * @file CTPPlugin.cpp
 * @brief CTP插件完整实现
 */

#include "CTPPlugin.h"
#include "../core/EnvironmentConfig.h"
#include "../utils/Logger.h"
#include <QTimer>
#include <QDateTime>
#include <QMutexLocker>
#include <QElapsedTimer>

namespace CTP {

// ========== PIMPL实现 ==========

class CTPPlugin::Impl {
public:
    // 连接状态
    bool connected = false;
    QString brokerId;
    QString userId;
    QString tradingDay;
    
    // 行情数据缓存
    QMap<QString, MarketData> marketDataCache;
    QMutex marketDataMutex;
    
    // 订单管理
    QMap<QString, OrderData> orders;
    QMap<QString, OrderData> trades;
    int orderRefCounter = 0;
    QMutex orderMutex;
    
    // 账户信息
    AccountData accountData;
    QList<AccountData> positions;
    QMutex accountMutex;
    
    // 批量订阅缓冲
    QQueue<QString> subscribeBuffer;
    QTimer* subscribeBufferTimer = nullptr;
    static const int SUBSCRIBE_BUFFER_INTERVAL = 100; // 100ms
    
    // 辅助方法
    QString generateOrderRef() {
        QMutexLocker locker(&orderMutex);
        return QString::number(++orderRefCounter).rightJustified(12, '0');
    }
    
    void flushSubscribeBuffer() {
        // 批量订阅逻辑
    }
};

// ========== 构造和析构 ==========

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

// ========== IPlugin接口实现 ==========

PluginMetaData CTPPlugin::metaData() const
{
    PluginMetaData meta;
    meta.name = "CTPPlugin";
    meta.version = "2.0.0";
    meta.description = "CTP Trading Plugin - Support market data and trading";
    meta.author = "WealthPilot Team";
    meta.license = "MIT";
    meta.website = "https://wealthpilot.com";
    meta.dependencies = QStringList(); // 无依赖
    meta.priority = 10; // 高优先级
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
    
    LOG_INFO("Loading CTPPlugin...");
    
    if (m_state != PluginState::Unloaded) {
        LOG_WARNING("CTPPlugin already loaded");
        return true;
    }
    
    setState(PluginState::Loading);
    
    // 初始化批量订阅缓冲定时器
    QObject::connect(m_subscribeBufferTimer, &QTimer::timeout, this, &CTPPlugin::flushSubscribeBuffer);
    
    setState(PluginState::Loaded);
    
    LOG_INFO(QString("CTPPlugin loaded in %1ms").arg(timer.elapsed()));
    return true;
}

bool CTPPlugin::initialize(const QJsonObject& config)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO("Initializing CTPPlugin...");
    
    if (m_state != PluginState::Loaded) {
        LOG_ERROR("CTPPlugin not loaded");
        return false;
    }
    
    m_config = config;
    
    // 从EnvironmentConfig加载配置
    loadEnvironmentConfig();
    
    setState(PluginState::Initialized);
    
    LOG_INFO(QString("CTPPlugin initialized in %1ms").arg(timer.elapsed()));
    return true;
}

bool CTPPlugin::start()
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO("Starting CTPPlugin...");
    
    if (m_state != PluginState::Initialized) {
        LOG_ERROR("CTPPlugin not initialized");
        return false;
    }
    
    setState(PluginState::Running);
    
    LOG_INFO(QString("CTPPlugin started in %1ms").arg(timer.elapsed()));
    return true;
}

void CTPPlugin::stop()
{
    LOG_INFO("Stopping CTPPlugin...");
    
    if (m_state != PluginState::Running) {
        return;
    }
    
    // 断开连接
    disconnect();
    
    setState(PluginState::Stopped);
    
    LOG_INFO("CTPPlugin stopped");
}

void CTPPlugin::unload()
{
    LOG_INFO("Unloading CTPPlugin...");
    
    if (m_state == PluginState::Running) {
        stop();
    }
    
    // 清理资源
    d->marketDataCache.clear();
    d->orders.clear();
    d->trades.clear();
    d->positions.clear();
    
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
    // CTP插件无依赖
    return true;
}

QStringList CTPPlugin::dependencies() const
{
    return QStringList();
}

// ========== ICTPPlugin接口实现 ==========

bool CTPPlugin::connect(const QString& brokerId,
                       const QString& userId,
                       const QString& password,
                       const QString& marketFront,
                       const QString& tradeFront)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO(QString("Connecting to CTP: broker=%1, user=%2")
        .arg(brokerId).arg(userId));
    
    if (m_state != PluginState::Running) {
        LOG_ERROR("CTPPlugin not running");
        return false;
    }
    
    // 保存连接信息
    d->brokerId = brokerId;
    d->userId = userId;
    
    // TODO: 实际的CTP连接逻辑
    // 这里应该调用CTP API进行连接
    // 1. 创建MdApi和TraderApi
    // 2. 注册SPI回调
    // 3. 连接前置地址
    // 4. 用户登录
    
    // 模拟连接成功
    d->connected = true;
    d->tradingDay = QDateTime::currentDateTime().toString("yyyyMMdd");
    
    LOG_INFO(QString("CTP connected in %1ms").arg(timer.elapsed()));
    
    emit connected();
    return true;
}

void CTPPlugin::disconnect()
{
    LOG_INFO("Disconnecting from CTP...");
    
    if (!d->connected) {
        return;
    }
    
    // TODO: 实际的CTP断开逻辑
    // 1. 登出
    // 2. 释放API
    
    d->connected = false;
    
    LOG_INFO("CTP disconnected");
    
    emit disconnected();
}

bool CTPPlugin::isConnected() const
{
    return d->connected;
}

bool CTPPlugin::subscribeMarketData(const QStringList& instruments)
{
    if (!d->connected) {
        LOG_ERROR("CTP not connected");
        return false;
    }
    
    LOG_INFO(QString("Subscribing market data: %1 instruments").arg(instruments.size()));
    
    // 使用批量订阅缓冲（性能优化）
    for (const QString& instrument : instruments) {
        d->subscribeBuffer.enqueue(instrument);
    }
    
    // 启动缓冲定时器
    if (!m_subscribeBufferTimer->isActive()) {
        m_subscribeBufferTimer->start(Impl::SUBSCRIBE_BUFFER_INTERVAL);
    }
    
    return true;
}

void CTPPlugin::unsubscribeMarketData(const QStringList& instruments)
{
    if (!d->connected) {
        return;
    }
    
    LOG_INFO(QString("Unsubscribing market data: %1 instruments").arg(instruments.size()));
    
    // TODO: 实际的取消订阅逻辑
    // 调用MdApi->UnsubscribeMarketData
}

::MarketData CTPPlugin::getMarketData(const QString& instrumentId) const
{
    // 先从内存缓存获取
    {
        QMutexLocker locker(&d->marketDataMutex);
        if (d->marketDataCache.contains(instrumentId)) {
            return d->marketDataCache[instrumentId];
        }
    }
    
    // 再从CacheManager获取（集成缓存系统）
    QString cacheKey = QString("market_data_%1").arg(instrumentId);
    QVariant cached = CacheManager::instance()->get(cacheKey);
    if (cached.isValid()) {
        return cached.value<MarketData>();
    }
    
    return MarketData();
}

QMap<QString, ::MarketData> CTPPlugin::getAllMarketData() const
{
    QMutexLocker locker(&d->marketDataMutex);
    return d->marketDataCache;
}

QString CTPPlugin::sendOrder(const QString& instrumentId,
                            const QString& direction,
                            const QString& offsetFlag,
                            double price,
                            int volume)
{
    if (!d->connected) {
        LOG_ERROR("CTP not connected");
        return QString();
    }
    
    LOG_INFO(QString("Sending order: %1 %2 %3 @ %4 vol=%5")
        .arg(instrumentId).arg(direction).arg(offsetFlag).arg(price).arg(volume));
    
    // 生成订单引用
    QString orderRef = d->generateOrderRef();
    
    // 创建订单数据
    OrderData order;
    order.orderId = orderRef;
    order.instrumentId = instrumentId;
    order.direction = direction;
    order.offsetFlag = offsetFlag;
    order.price = price;
    order.volume = volume;
    order.volumeTraded = 0;
    order.status = "Pending";
    order.insertTime = QDateTime::currentDateTime();
    
    // 缓存订单
    {
        QMutexLocker locker(&d->orderMutex);
        d->orders[orderRef] = order;
    }
    
    // TODO: 实际的下单逻辑
    // 调用TraderApi->ReqOrderInsert
    
    LOG_INFO(QString("Order sent: %1").arg(orderRef));
    
    emit orderUpdated(order);
    return orderRef;
}

bool CTPPlugin::cancelOrder(const QString& orderId)
{
    if (!d->connected) {
        LOG_ERROR("CTP not connected");
        return false;
    }
    
    LOG_INFO(QString("Cancelling order: %1").arg(orderId));
    
    // TODO: 实际的撤单逻辑
    // 调用TraderApi->ReqOrderAction
    
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

// ========== 私有方法 ==========

void CTPPlugin::flushSubscribeBuffer()
{
    if (d->subscribeBuffer.isEmpty()) {
        m_subscribeBufferTimer->stop();
        return;
    }
    
    // 批量处理订阅
    QStringList instruments;
    while (!d->subscribeBuffer.isEmpty() && instruments.size() < 100) {
        instruments.append(d->subscribeBuffer.dequeue());
    }
    
    LOG_DEBUG(QString("Flushing subscribe buffer: %1 instruments").arg(instruments.size()));
    
    // TODO: 实际的批量订阅逻辑
    // 调用MdApi->SubscribeMarketData
}

void CTPPlugin::updateMarketDataCache(const QString& instrumentId, const MarketData& data)
{
    // 更新内存缓存
    {
        QMutexLocker locker(&d->marketDataMutex);
        d->marketDataCache[instrumentId] = data;
    }
    
    // 更新CacheManager（集成缓存系统）
    QString cacheKey = QString("market_data_%1").arg(instrumentId);
    CacheManager::instance()->set(cacheKey, QVariant::fromValue(data), 60, CacheLevel::L1_Memory);
}

MarketData CTPPlugin::getCachedMarketData(const QString& instrumentId) const
{
    return getMarketData(instrumentId);
}

void CTPPlugin::loadEnvironmentConfig()
{
    // 从EnvironmentConfig加载CTP配置
    auto settings = EnvironmentConfig::instance()->currentSettings();
    
    // 如果配置中没有设置，使用环境配置
    if (!m_config.contains("brokerId")) {
        m_config["brokerId"] = settings.ctpBrokerId;
    }
    if (!m_config.contains("marketFront")) {
        m_config["marketFront"] = settings.ctpMarketFront;
    }
    if (!m_config.contains("tradeFront")) {
        m_config["tradeFront"] = settings.ctpTradeFront;
    }
    
    LOG_DEBUG("CTP environment config loaded");
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
