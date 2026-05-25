/**
 * @file WebSocketManager.cpp
 * @brief WebSocket 连接管理器实现
 */

#include "WebSocketManager.h"
#include "shared/utils/Logger.h"
#include <QMutexLocker>

WebSocketManager* WebSocketManager::instance()
{
    static WebSocketManager* inst = new WebSocketManager();
    return inst;
}

WebSocketManager::WebSocketManager(QObject* parent)
    : QObject(parent)
{
    m_webSocket = new QWebSocket();
    m_reconnectTimer = new QTimer(this);
    m_heartbeatTimer = new QTimer(this);

    // 连接信号
    connect(m_webSocket, &QWebSocket::connected, this, &WebSocketManager::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketManager::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketManager::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &WebSocketManager::onError);
    connect(m_reconnectTimer, &QTimer::timeout, this, &WebSocketManager::onReconnect);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &WebSocketManager::onHeartbeat);

    LOG_INFO("WebSocketManager initialized");
}

WebSocketManager::~WebSocketManager()
{
    disconnect();
    m_webSocket->deleteLater();
}

void WebSocketManager::connectToServer(const QString& url)
{
    m_serverUrl = url;
    m_currentRetry = 0;

    setState(WSConnectionState::Connecting);
    m_webSocket->open(QUrl(url));

    LOG_INFO(QString("Connecting to WebSocket: %1").arg(url));
}

void WebSocketManager::disconnect()
{
    stopReconnect();
    m_heartbeatTimer->stop();

    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }

    setState(WSConnectionState::Disconnected);
    LOG_INFO("WebSocket disconnected");
}

void WebSocketManager::sendMessage(const QString& message, bool cacheIfDisconnected)
{
    if (isConnected()) {
        m_webSocket->sendTextMessage(message);
        LOG_DEBUG(QString("Message sent: %1 bytes").arg(message.size()));
    } else if (cacheIfDisconnected) {
        QMutexLocker locker(&m_queueMutex);
        if (m_messageQueue.size() < m_maxQueueSize) {
            m_messageQueue.enqueue(message);
            LOG_DEBUG(QString("Message cached (queue size: %1)").arg(m_messageQueue.size()));
        } else {
            LOG_WARNING("Message queue full, dropping message");
        }
    }
}

void WebSocketManager::setReconnectConfig(int maxRetries, int retryInterval)
{
    m_maxRetries = maxRetries;
    m_retryInterval = retryInterval;
    LOG_INFO(QString("Reconnect config: maxRetries=%1, interval=%2ms")
        .arg(maxRetries).arg(retryInterval));
}

void WebSocketManager::setHeartbeat(int interval, const QString& pingMessage)
{
    m_pingMessage = pingMessage;
    if (interval > 0) {
        m_heartbeatTimer->setInterval(interval);
        m_heartbeatTimer->start();
        LOG_INFO(QString("Heartbeat enabled: interval=%1ms").arg(interval));
    } else {
        m_heartbeatTimer->stop();
        LOG_INFO("Heartbeat disabled");
    }
}

void WebSocketManager::onConnected()
{
    setState(WSConnectionState::Connected);
    m_currentRetry = 0;
    m_heartbeatPending = false;

    // 发送缓存的消息
    flushMessageQueue();

    // 启动心跳
    if (m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->start();
    }

    LOG_INFO("WebSocket connected");
}

void WebSocketManager::onDisconnected()
{
    // 检查是否需要重连
    if (!m_serverUrl.isEmpty() && m_state != WSConnectionState::Disconnected) {
        setState(WSConnectionState::Reconnecting);
        startReconnect();
        LOG_WARNING("WebSocket disconnected, starting reconnect");
    } else {
        setState(WSConnectionState::Disconnected);
        LOG_INFO("WebSocket disconnected");
    }
}

void WebSocketManager::onTextMessageReceived(const QString& message)
{
    // 收到消息，重置心跳状态
    m_heartbeatPending = false;

    emit messageReceived(message);
    LOG_DEBUG(QString("Message received: %1 bytes").arg(message.size()));
}

void WebSocketManager::onError(QAbstractSocket::SocketError error)
{
    QString errorString = m_webSocket->errorString();
    setState(WSConnectionState::Error);
    emit errorOccurred(errorString);

    LOG_ERROR(QString("WebSocket error: %1 - %2").arg(error).arg(errorString));

    // 尝试重连
    if (!m_serverUrl.isEmpty()) {
        startReconnect();
    }
}

void WebSocketManager::onHeartbeat()
{
    if (!isConnected()) return;

    // 检查心跳响应
    if (m_heartbeatPending) {
        LOG_WARNING("Heartbeat timeout, reconnecting");
        m_webSocket->close();
        startReconnect();
        return;
    }

    // 发送心跳
    m_heartbeatPending = true;
    m_webSocket->sendTextMessage(m_pingMessage);
    LOG_DEBUG("Heartbeat sent");
}

void WebSocketManager::onReconnect()
{
    if (m_maxRetries > 0 && m_currentRetry >= m_maxRetries) {
        stopReconnect();
        setState(WSConnectionState::Disconnected);
        LOG_ERROR("Max reconnect attempts reached");
        emit errorOccurred("Max reconnect attempts reached");
        return;
    }

    m_currentRetry++;
    setState(WSConnectionState::Reconnecting);

    emit reconnectAttempt(m_currentRetry, m_maxRetries);
    LOG_INFO(QString("Reconnecting... attempt %1/%2")
        .arg(m_currentRetry).arg(m_maxRetries));

    m_webSocket->open(QUrl(m_serverUrl));
}

void WebSocketManager::setState(WSConnectionState state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

void WebSocketManager::startReconnect()
{
    if (!m_reconnectTimer->isActive()) {
        m_reconnectTimer->setInterval(m_retryInterval);
        m_reconnectTimer->start();
    }
}

void WebSocketManager::stopReconnect()
{
    m_reconnectTimer->stop();
    m_currentRetry = 0;
}

void WebSocketManager::flushMessageQueue()
{
    QMutexLocker locker(&m_queueMutex);

    while (!m_messageQueue.isEmpty() && isConnected()) {
        QString message = m_messageQueue.dequeue();
        m_webSocket->sendTextMessage(message);
    }

    LOG_DEBUG(QString("Message queue flushed, remaining: %1").arg(m_messageQueue.size()));
}