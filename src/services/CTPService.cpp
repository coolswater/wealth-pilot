/**
 * @file CTPService.cpp
 * @brief CTP期货服务实现
 */
#include "CTPService.h"
#include "../utils/Logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QtGlobal>
#include <QRandomGenerator>

CTPService* CTPService::s_instance = nullptr;

// PIMPL实现
class CTPService::Impl {
public:
    SimnowConfig config;
    bool marketConnected = false;
    bool tradeConnected = false;
    bool loggedIn = false;
    int requestId = 0;

    QTimer* reconnectTimer = nullptr;
    QTimer* heartbeatTimer = nullptr;

    QMap<QString, FuturesQuote> quotes;
    QMap<QString, OrderResponse> orders;

    // 模拟数据（用于演示）
    bool simulationMode = true;
    QTimer* simulationTimer = nullptr;
};

CTPService::CTPService(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
}

CTPService::~CTPService()
{
    shutdown();
}

CTPService* CTPService::instance()
{
    if (!s_instance) {
        s_instance = new CTPService();
    }
    return s_instance;
}

bool CTPService::initialize()
{
    LOG_INFO("CTPService initializing...");

    // 创建重连定时器
    d->reconnectTimer = new QTimer(this);
    d->reconnectTimer->setInterval(30000);  // 30秒
    connect(d->reconnectTimer, &QTimer::timeout, this, &CTPService::onReconnectTimer);

    // 创建心跳定时器
    d->heartbeatTimer = new QTimer(this);
    d->heartbeatTimer->setInterval(10000);  // 10秒
    connect(d->heartbeatTimer, &QTimer::timeout, this, &CTPService::onHeartbeatTimer);

    // 创建模拟数据定时器（演示模式）
    if (d->simulationMode) {
        d->simulationTimer = new QTimer(this);
        d->simulationTimer->setInterval(1000);  // 1秒
        connect(d->simulationTimer, &QTimer::timeout, [this]() {
            // 模拟行情数据更新
            for (auto it = d->quotes.begin(); it != d->quotes.end(); ++it) {
                FuturesQuote& quote = it.value();
                // 随机波动价格
                double change = (QRandomGenerator::global()->bounded(100) - 50) / 1000.0;
                quote.lastPrice += change;
                quote.updateTime = QDateTime::currentDateTime().toString("hh:mm:ss");
                emit marketDataReceived(quote);
            }
        });
    }

    LOG_INFO("CTPService initialized successfully");
}

void CTPService::shutdown()
{
    LOG_INFO("CTPService shutting down...");

    disconnect();

    if (d->reconnectTimer) {
        d->reconnectTimer->stop();
        d->reconnectTimer->deleteLater();
        d->reconnectTimer = nullptr;
    }

    if (d->heartbeatTimer) {
        d->heartbeatTimer->stop();
        d->heartbeatTimer->deleteLater();
        d->heartbeatTimer = nullptr;
    }

    if (d->simulationTimer) {
        d->simulationTimer->stop();
        d->simulationTimer->deleteLater();
        d->simulationTimer = nullptr;
    }

    LOG_INFO("CTPService shutdown complete");
}

void CTPService::setConfig(const SimnowConfig& config)
{
    d->config = config;
    LOG_INFO(QString("CTP config updated - Broker: %1, User: %2")
                 .arg(config.brokerId, config.userId));
}

SimnowConfig CTPService::config() const
{
    return d->config;
}

bool CTPService::connectMarket()
{
    LOG_INFO(QString("Connecting to market front: %1").arg(d->config.marketFront));

    // 实际项目中这里会调用CTP API连接行情服务器
    // 这里使用模拟模式演示

    if (d->simulationMode) {
        d->marketConnected = true;
        emit marketConnected();
        LOG_INFO("Market connected (simulation mode)");
        return true;
    }

    // TODO: 实现真实的CTP API连接
    return false;
}

bool CTPService::connectTrade()
{
    LOG_INFO(QString("Connecting to trade front: %1").arg(d->config.tradeFront));

    if (d->simulationMode) {
        d->tradeConnected = true;
        emit tradeConnected();
        LOG_INFO("Trade connected (simulation mode)");
        return true;
    }

    // TODO: 实现真实的CTP API连接
    return false;
}

void CTPService::disconnect()
{
    LOG_INFO("Disconnecting from CTP servers");

    d->marketConnected = false;
    d->tradeConnected = false;
    d->loggedIn = false;

    emit marketDisconnected();
    emit tradeDisconnected();
    emit loggedOut();

    LOG_INFO("Disconnected from CTP servers");
}

bool CTPService::isMarketConnected() const
{
    return d->marketConnected;
}

bool CTPService::isTradeConnected() const
{
    return d->tradeConnected;
}

bool CTPService::login(const QString& userId, const QString& password)
{
    LOG_INFO(QString("Logging in user: %1").arg(userId));

    d->config.userId = userId;
    d->config.password = password;

    if (d->simulationMode) {
        d->loggedIn = true;
        emit loggedIn();
        d->heartbeatTimer->start();
        LOG_INFO("Login successful (simulation mode)");
        return true;
    }

    // TODO: 实现真实的CTP登录
    return false;
}

void CTPService::logout()
{
    LOG_INFO("Logging out");

    d->loggedIn = false;
    emit loggedOut();

    if (d->heartbeatTimer) {
        d->heartbeatTimer->stop();
    }
}

bool CTPService::isLoggedIn() const
{
    return d->loggedIn;
}

void CTPService::subscribeMarketData(const QStringList& instruments)
{
    LOG_INFO(QString("Subscribing to %1 instruments").arg(instruments.size()));

    for (const QString& instrument : instruments) {
        // 初始化行情数据
        // double change = (QRandomGenerator::global()->bounded(100) - 50) / 1000.0;
        FuturesQuote quote;
        quote.instrumentId = instrument;
        quote.lastPrice = 3000.0 + QRandomGenerator::global()->bounded(1000);
        quote.preSettlementPrice = quote.lastPrice;
        quote.openPrice = quote.lastPrice;
        quote.highestPrice = quote.lastPrice * 1.02;
        quote.lowestPrice = quote.lastPrice * 0.98;
        quote.volume = QRandomGenerator::global()->bounded(10000);
        quote.openInterest = QRandomGenerator::global()->bounded(50000);
        quote.bidPrice1 = quote.lastPrice - 1;
        quote.askPrice1 = quote.lastPrice + 1;
        quote.updateTime = QDateTime::currentDateTime().toString("hh:mm:ss");

        d->quotes[instrument] = quote;
    }

    if (d->simulationMode && d->simulationTimer) {
        d->simulationTimer->start();
    }

    LOG_INFO(QString("Subscribed to %1 instruments").arg(instruments.size()));
}

void CTPService::unsubscribeMarketData(const QStringList& instruments)
{
    LOG_INFO(QString("Unsubscribing from %1 instruments").arg(instruments.size()));

    for (const QString& instrument : instruments) {
        d->quotes.remove(instrument);
    }

    if (d->quotes.isEmpty() && d->simulationTimer) {
        d->simulationTimer->stop();
    }
}

QStringList CTPService::subscribedInstruments() const
{
    return d->quotes.keys();
}

QString CTPService::sendOrder(const OrderRequest& request)
{
    QString orderId = QString("ORDER_%1").arg(++d->requestId);

    LOG_INFO(QString("Sending order: %1 %2 %3 @ %4")
                 .arg(orderId,  // QString
                      request.instrumentId,  // QString
                      QString::number(request.volume),  // int -> QString
                      QString::number(request.price, 'f', 2)));  // double -> QString，保留2位小数

    if (d->simulationMode) {
        OrderResponse response;
        response.orderId = orderId;
        response.instrumentId = request.instrumentId;
        response.status = 3;  // 全部成交
        response.price = request.price;
        response.volume = request.volume;
        response.tradedVolume = request.volume;
        response.insertTime = QDateTime::currentDateTime().toString("hh:mm:ss");

        d->orders[orderId] = response;

        // 模拟异步响应
        QTimer::singleShot(100, [this, response]() {
            emit orderResponseReceived(response);
        });

        return orderId;
    }

    // TODO: 实现真实的CTP下单
    return QString();
}

bool CTPService::cancelOrder(const QString& orderId)
{
    LOG_INFO(QString("Cancelling order: %1").arg(orderId));

    if (d->orders.contains(orderId)) {
        d->orders[orderId].status = 4;  // 已撤销

        OrderResponse response = d->orders[orderId];
        emit orderResponseReceived(response);

        return true;
    }

    return false;
}

void CTPService::queryPositions()
{
    LOG_INFO("Querying positions");

    // 模拟持仓数据
    emit positionUpdated("rb2505", 10, 3500.0);
    emit positionUpdated("m2505", 5, 2800.0);
}

void CTPService::queryAccount()
{
    LOG_INFO("Querying account");

    // 模拟账户数据
    emit accountUpdated(1000000.0, 800000.0, 200000.0);
}

void CTPService::queryOrders()
{
    LOG_INFO("Querying orders");

    for (const auto& response : qAsConst(d->orders)) {
        emit orderResponseReceived(response);
    }
}

void CTPService::queryTrades()
{
    LOG_INFO("Querying trades");
    // TODO: 实现查询成交记录
}

FuturesQuote CTPService::getQuote(const QString& instrumentId) const
{
    return d->quotes.value(instrumentId);
}

QMap<QString, FuturesQuote> CTPService::getAllQuotes() const
{
    return d->quotes;
}

void CTPService::onReconnectTimer()
{
    if (!d->marketConnected) {
        LOG_INFO("Attempting to reconnect market...");
        connectMarket();
    }

    if (!d->tradeConnected) {
        LOG_INFO("Attempting to reconnect trade...");
        connectTrade();
    }
}

void CTPService::onHeartbeatTimer()
{
    if (d->loggedIn) {
        // 发送心跳包或查询账户保持连接
        LOG_DEBUG("Sending heartbeat");
    }
}
