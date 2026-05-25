/**
 * @file MarketDataWebSocket.cpp
 * @brief 实时行情 WebSocket 推送实现
 */

#include "MarketDataWebSocket.h"
#include "shared/utils/Logger.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ========== PIMPL 实现 ==========

struct MarketDataWebSocket::Impl
{
    QWebSocket* socket = nullptr;
    QString serverUrl;

    // 订阅管理
    QMap<QString, int> subscriptions; ///< symbol -> subscribeType

    // 定时器
    QTimer* heartbeatTimer = nullptr;
    QTimer* reconnectTimer = nullptr;

    // 配置
    int reconnectInterval = 5000; ///< 重连间隔 5秒
    int heartbeatInterval = 30000; ///< 心跳间隔 30秒
    bool autoReconnect = true;

    // 状态
    bool connecting = false;
};

// ========== 构造函数和析构函数 ==========

MarketDataWebSocket::MarketDataWebSocket(QObject* parent)
    : QObject(parent)
      , d(std::make_unique<Impl>())
{
    d->socket = new QWebSocket();
    d->heartbeatTimer = new QTimer(this);
    d->reconnectTimer = new QTimer(this);

    // 连接信号
    connect(d->socket, &QWebSocket::connected, this, &MarketDataWebSocket::onConnected);
    connect(d->socket, &QWebSocket::disconnected, this, &MarketDataWebSocket::onDisconnected);
    connect(d->socket, &QWebSocket::errorOccurred, this, &MarketDataWebSocket::onError);
    connect(d->socket, &QWebSocket::textMessageReceived, this, &MarketDataWebSocket::onTextMessageReceived);

    connect(d->heartbeatTimer, &QTimer::timeout, this, &MarketDataWebSocket::onHeartbeat);
    connect(d->reconnectTimer, &QTimer::timeout, this, &MarketDataWebSocket::onReconnect);

    LOG_DEBUG("MarketDataWebSocket created");
}

MarketDataWebSocket::~MarketDataWebSocket()
{
    disconnect();
    d->socket->deleteLater();
    LOG_DEBUG("MarketDataWebSocket destroyed");
}

// ========== 连接管理 ==========

void MarketDataWebSocket::connectToServer(const QString& url)
{
    d->serverUrl = url;
    d->connecting = true;

    LOG_INFO(QString("Connecting to market data server: %1").arg(url));
    d->socket->open(url);
}

void MarketDataWebSocket::disconnect()
{
    d->connecting = false;
    d->autoReconnect = false;
    d->heartbeatTimer->stop();
    d->reconnectTimer->stop();

    if (d->socket->state() == QAbstractSocket::ConnectedState)
    {
        d->socket->close();
        LOG_INFO("Disconnected from market data server");
    }
}

bool MarketDataWebSocket::isConnected() const
{
    return d->socket->state() == QAbstractSocket::ConnectedState;
}

// ========== 订阅管理 ==========

void MarketDataWebSocket::subscribe(const QString& symbol, MarketSubscribeType type)
{
    if (!isConnected())
    {
        LOG_WARNING(QString("Cannot subscribe %1: not connected").arg(symbol));
        return;
    }

    // 构建订阅消息
    QJsonObject msg;
    msg["action"] = "subscribe";
    msg["symbol"] = symbol;
    msg["type"] = static_cast<int>(type);

    QJsonDocument doc(msg);
    d->socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

    // 记录订阅
    d->subscriptions[symbol] = static_cast<int>(type);

    LOG_DEBUG(QString("Subscribed: %1 (type: %2)").arg(symbol).arg(static_cast<int>(type)));
}

void MarketDataWebSocket::unsubscribe(const QString& symbol)
{
    if (!isConnected())
    {
        return;
    }

    // 构建取消订阅消息
    QJsonObject msg;
    msg["action"] = "unsubscribe";
    msg["symbol"] = symbol;

    QJsonDocument doc(msg);
    d->socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));

    // 移除订阅记录
    d->subscriptions.remove(symbol);

    LOG_DEBUG(QString("Unsubscribed: %1").arg(symbol));
}

void MarketDataWebSocket::unsubscribeAll()
{
    for (const QString& symbol : d->subscriptions.keys())
    {
        unsubscribe(symbol);
    }
    d->subscriptions.clear();
}

QSet<QString> MarketDataWebSocket::subscribedSymbols() const
{
    return QSet<QString>(d->subscriptions.keys().begin(), d->subscriptions.keys().end());
}

// ========== 配置 ==========

void MarketDataWebSocket::setReconnectInterval(int ms)
{
    d->reconnectInterval = ms;
}

void MarketDataWebSocket::setHeartbeatInterval(int ms)
{
    d->heartbeatInterval = ms;
    if (d->heartbeatTimer->isActive())
    {
        d->heartbeatTimer->setInterval(ms);
    }
}

void MarketDataWebSocket::setAutoReconnect(bool enabled)
{
    d->autoReconnect = enabled;
}

// ========== 信号处理 ==========

void MarketDataWebSocket::onConnected()
{
    d->connecting = false;
    d->reconnectTimer->stop();

    // 启动心跳
    d->heartbeatTimer->start(d->heartbeatInterval);

    // 重新订阅
    resubscribeAll();

    LOG_INFO("Connected to market data server");
    emit connected();
}

void MarketDataWebSocket::onDisconnected()
{
    d->heartbeatTimer->stop();

    LOG_INFO("Disconnected from market data server");
    emit disconnected();

    // 自动重连
    if (d->autoReconnect && !d->connecting)
    {
        LOG_INFO(QString("Will reconnect in %1ms").arg(d->reconnectInterval));
        d->reconnectTimer->start(d->reconnectInterval);
    }
}

void MarketDataWebSocket::onError(QAbstractSocket::SocketError error)
{
    QString errorString = d->socket->errorString();
    LOG_ERROR(QString("WebSocket error: %1 - %2").arg(error).arg(errorString));
    emit connectionError(errorString);
}

void MarketDataWebSocket::onTextMessageReceived(const QString& message)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError)
    {
        LOG_WARNING(QString("Failed to parse message: %1").arg(parseError.errorString()));
        return;
    }

    QJsonObject json = doc.object();
    QString type = json["type"].toString();

    if (type == "tick")
    {
        parseTickData(json["data"].toObject());
    }
    else if (type == "kline")
    {
        parseKLineData(json["data"].toObject());
    }
    else if (type == "depth")
    {
        parseDepthData(json["data"].toObject());
    }
    else if (type == "pong")
    {
        // 心跳响应
        LOG_DEBUG("Heartbeat response received");
    }
    else if (type == "subscribe_result")
    {
        QString symbol = json["symbol"].toString();
        bool success = json["success"].toBool();
        if (success)
        {
            emit subscribeSuccess(symbol);
        }
    }
}

void MarketDataWebSocket::onHeartbeat()
{
    sendHeartbeat();
}

void MarketDataWebSocket::onReconnect()
{
    d->reconnectTimer->stop();
    if (d->autoReconnect)
    {
        LOG_INFO("Reconnecting...");
        connectToServer(d->serverUrl);
    }
}

// ========== 私有方法 ==========

void MarketDataWebSocket::sendHeartbeat()
{
    if (!isConnected()) return;

    QJsonObject msg;
    msg["action"] = "ping";
    msg["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QJsonDocument doc(msg);
    d->socket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void MarketDataWebSocket::resubscribeAll()
{
    for (auto it = d->subscriptions.begin(); it != d->subscriptions.end(); ++it)
    {
        subscribe(it.key(), static_cast<MarketSubscribeType>(it.value()));
    }
}

void MarketDataWebSocket::parseTickData(const QJsonObject& json)
{
    MarketTick tick;
    tick.symbol = json["symbol"].toString();
    tick.name = json["name"].toString();
    tick.price = json["price"].toDouble();
    tick.open = json["open"].toDouble();
    tick.high = json["high"].toDouble();
    tick.low = json["low"].toDouble();
    tick.preClose = json["preClose"].toDouble();
    tick.volume = json["volume"].toVariant().toLongLong();
    tick.amount = json["amount"].toDouble();
    tick.timestamp = json["timestamp"].toVariant().toLongLong();

    // 五档行情
    if (json.contains("bidPrice1"))
    {
        tick.bidPrice1 = json["bidPrice1"].toDouble();
        tick.bidVolume1 = json["bidVolume1"].toVariant().toLongLong();
        tick.askPrice1 = json["askPrice1"].toDouble();
        tick.askVolume1 = json["askVolume1"].toVariant().toLongLong();
    }

    emit tickReceived(tick);
}

void MarketDataWebSocket::parseKLineData(const QJsonObject& json)
{
    // TODO: 解析K线数据
}

void MarketDataWebSocket::parseDepthData(const QJsonObject& json)
{
    // TODO: 解析盘口深度数据
}