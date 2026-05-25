/**
 * @file NetworkManager.cpp
 * @brief 网络管理器实现
 */

#include "NetworkManager.h"
#include "shared/utils/Logger.h"
#include "infrastructure/config/ConfigManager.h"

#include <QNetworkInformation>
#include <QTimer>
#include <QUuid>

NetworkManager::NetworkManager(QObject* parent)
    : QObject(parent)
    , m_httpManager(std::make_unique<QNetworkAccessManager>())
    , m_webSocket(std::make_unique<QWebSocket>())
    , m_cacheEnabled(true)
    , m_requestCacheTimeout(300000) // 5分钟缓存
{
}

NetworkManager::~NetworkManager()
{
    shutdown();
}

bool NetworkManager::initialize()
{
    if (m_initialized) return m_initialized;

    // 连接 HTTP 管理器
    connect(m_httpManager.get(), &QNetworkAccessManager::finished,
            this, &NetworkManager::onNetworkReplyFinished);

    // 连接 WebSocket 信号
    connect(m_webSocket.get(), &QWebSocket::connected,
            this, &NetworkManager::onWebSocketConnected);
    connect(m_webSocket.get(), &QWebSocket::disconnected,
            this, &NetworkManager::onWebSocketDisconnected);
    connect(m_webSocket.get(), &QWebSocket::textMessageReceived,
            this, &NetworkManager::onWebSocketMessage);

    // 监控网络状态(Qt 6.1+)
    if (QNetworkInformation::loadDefaultBackend()) {
        QNetworkInformation* netInfo = QNetworkInformation::instance();
        if (netInfo) {
            connect(netInfo, &QNetworkInformation::reachabilityChanged,
                    this, &NetworkManager::onNetworkInformationChanged);
            m_online = netInfo->reachability() == QNetworkInformation::Reachability::Online;
        }
    }

    // 加载配置
    m_defaultTimeout = ConfigManager::instance()->getInt("network/request_timeout", 30000);
    m_maxRetries = ConfigManager::instance()->getInt("network/max_retries", 3);

    m_initialized = true;
    LOG_INFO("NetworkManager initialized");
    return m_initialized;
}

void NetworkManager::setCacheEnabled(bool enabled)
{
    m_cacheEnabled = enabled;
    if (!enabled) {
        clearCache();
    }
}

void NetworkManager::setCacheTimeout(int timeoutMs)
{
    m_requestCacheTimeout = timeoutMs;
    cleanupCache();
}

void NetworkManager::shutdown()
{
    // 断开 WebSocket
    if (m_webSocket && m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }

    // 取消所有活跃请求
    QMutexLocker locker(&m_mutex);
    for (auto it = m_activeRequests.begin(); it != m_activeRequests.end(); ++it) {
        if (it.key()) {
            it.key()->abort();
        }
    }
    m_activeRequests.clear();
    m_requestQueue.clear();
    m_requestCache.clear();

    m_initialized = false;
    LOG_INFO("NetworkManager shutdown");
}

// ==================== HTTP 请求 ====================

QNetworkReply* NetworkManager::get(const QString& url, const QMap<QString, QString>& headers)
{
    NetworkRequest req;
    req.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    req.url = url;
    req.method = "GET";
    req.headers = headers;
    req.timeout = m_defaultTimeout;

    return sendRequest(req);
}

QNetworkReply* NetworkManager::post(const QString& url, const QByteArray& data,
                                     const QMap<QString, QString>& headers)
{
    NetworkRequest req;
    req.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    req.url = url;
    req.method = "POST";
    req.data = data;
    req.headers = headers;
    req.timeout = m_defaultTimeout;

    return sendRequest(req);
}

QNetworkReply* NetworkManager::put(const QString& url, const QByteArray& data,
                                    const QMap<QString, QString>& headers)
{
    NetworkRequest req;
    req.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    req.url = url;
    req.method = "PUT";
    req.data = data;
    req.headers = headers;
    req.timeout = m_defaultTimeout;

    return sendRequest(req);
}

QNetworkReply* NetworkManager::del(const QString& url, const QMap<QString, QString>& headers)
{
    NetworkRequest req;
    req.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    req.url = url;
    req.method = "DELETE";
    req.headers = headers;
    req.timeout = m_defaultTimeout;

    return sendRequest(req);
}

void NetworkManager::getAsync(const QString& url,
                               std::function<void(NetResult<QByteArray>)> callback,
                               const QMap<QString, QString>& headers)
{
    // Check cache
    if (m_cacheEnabled) {
        QString cacheKey = QString("GET_%1").arg(url);
        QVariant cached = getCachedResponse(cacheKey);
        if (cached.isValid()) {
            callback(NetResult<QByteArray>::ok(cached.toByteArray()));
            return;
        }
    }

    QNetworkReply* reply = get(url, headers);

    connect(reply, &QNetworkReply::finished, [this, reply, callback, cacheKey = QString("GET_%1").arg(url)]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            callback(NetResult<QByteArray>::error(reply->errorString()));
            return;
        }

        QByteArray data = reply->readAll();

        // 缓存响应
        if (m_cacheEnabled) {
            setCachedResponse(cacheKey, data);
        }

        callback(NetResult<QByteArray>::ok(data));
    });
}

void NetworkManager::postAsync(const QString& url, const QByteArray& data,
                                std::function<void(NetResult<QByteArray>)> callback,
                                const QMap<QString, QString>& headers)
{
    // Check cache
    if (m_cacheEnabled) {
        QString cacheKey = QString("POST_%1_%2").arg(url, QString::fromUtf8(data.toBase64()));
        QVariant cached = getCachedResponse(cacheKey);
        if (cached.isValid()) {
            callback(NetResult<QByteArray>::ok(cached.toByteArray()));
            return;
        }
    }

    QNetworkReply* reply = post(url, data, headers);

    connect(reply, &QNetworkReply::finished, [this, reply, callback, cacheKey = QString("POST_%1_%2").arg(url, QString::fromUtf8(data.toBase64()))]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            callback(NetResult<QByteArray>::error(reply->errorString()));
            return;
        }

        QByteArray responseData = reply->readAll();

        // 缓存响应
        if (m_cacheEnabled) {
            setCachedResponse(cacheKey, responseData);
        }

        callback(NetResult<QByteArray>::ok(responseData));
    });
}

// ==================== WebSocket ====================

void NetworkManager::connectWebSocket(const QString& url)
{
    if (m_webSocket->state() != QAbstractSocket::UnconnectedState) {
        m_webSocket->close();
    }

    LOG_INFO(QString("Connecting to WebSocket: %1").arg(url));
    m_webSocket->open(QUrl(url));
}

void NetworkManager::disconnectWebSocket()
{
    if (m_webSocket) {
        m_webSocket->close();
    }
}

bool NetworkManager::isWebSocketConnected() const
{
    return m_webSocket && m_webSocket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::sendWebSocketMessage(const QString& message)
{
    if (isWebSocketConnected()) {
        m_webSocket->sendTextMessage(message);
    }
}

// ==================== Network status ====================

bool NetworkManager::isOnline() const
{
    return m_online;
}

void NetworkManager::setOnline(bool online)
{
    if (m_online != online) {
        m_online = online;
        emit networkStateChanged(online);

        if (online) {
            // Restore Online state when network restored
            QTimer::singleShot(100, this, &NetworkManager::processQueue);
        }
    }
}

// ==================== 配置 ====================

void NetworkManager::setDefaultTimeout(int ms)
{
    m_defaultTimeout = ms;
}

void NetworkManager::setMaxRetries(int retries)
{
    m_maxRetries = retries;
}

// ==================== Private slots ====================

void NetworkManager::onNetworkReplyFinished(QNetworkReply* reply)
{
    QMutexLocker locker(&m_mutex);

    if (!m_activeRequests.contains(reply)) {
        return;
    }

    NetworkRequest request = m_activeRequests.take(reply);

    // Retry
    if (reply->error() != QNetworkReply::NoError) {
        LOG_WARNING(QString("Request failed: %1 - %2").arg(request.url, reply->errorString()));

        // 重试
        if (request.retryCount < request.maxRetries) {
            request.retryCount++;
            LOG_INFO(QString("Retrying request (%1/%2): %3")
                .arg(request.retryCount).arg(request.maxRetries).arg(request.url));
            retryRequest(request);
            return;
        }

        emit requestFailed(request.id, reply->errorString());
    }
}

void NetworkManager::onWebSocketConnected()
{
    LOG_INFO("WebSocket connected");
    emit webSocketConnected();
}

void NetworkManager::onWebSocketDisconnected()
{
    LOG_INFO("WebSocket disconnected");
    emit webSocketDisconnected();
}

void NetworkManager::onWebSocketMessage(const QString& message)
{
    emit webSocketMessageReceived(message);
}

void NetworkManager::onNetworkInformationChanged()
{
    QNetworkInformation* netInfo = QNetworkInformation::instance();
    if (netInfo) {
        bool online = netInfo->reachability() == QNetworkInformation::Reachability::Online;
        setOnline(online);
    }
}

// ==================== 请求缓存 ====================

void NetworkManager::clearCache()
{
    QMutexLocker locker(&m_mutex);
    m_requestCache.clear();
}

void NetworkManager::cleanupCache()
{
    QMutexLocker locker(&m_mutex);

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto it = m_requestCache.begin();

    while (it != m_requestCache.end()) {
        if (it.value().expiryTime < now || m_requestCache.size() > 100) {
            it = m_requestCache.erase(it);
        } else {
            ++it;
        }
    }
}

QVariant NetworkManager::getCachedResponse(const QString& key)
{
    QMutexLocker locker(&m_mutex);

    if (!m_requestCache.contains(key)) {
        return QVariant();
    }

    RequestCacheEntry& entry = m_requestCache[key];
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (entry.expiryTime < now) {
        m_requestCache.remove(key);
        return QVariant();
    }

    return entry.response;
}

void NetworkManager::setCachedResponse(const QString& key, const QByteArray& response)
{
    QMutexLocker locker(&m_mutex);

    RequestCacheEntry entry;
    entry.response = response;
    entry.expiryTime = QDateTime::currentMSecsSinceEpoch() + m_requestCacheTimeout;

    m_requestCache[key] = entry;

    // 限制缓存大小
    if (m_requestCache.size() > 100) {
        auto it = m_requestCache.begin();
        m_requestCache.erase(it);
    }
}

void NetworkManager::processQueue()
{
    QMutexLocker locker(&m_mutex);

    while (!m_requestQueue.isEmpty() && m_online) {
        NetworkRequest req = m_requestQueue.dequeue();
        sendRequest(req);
    }
}

// ==================== 私有方法 ====================

QNetworkReply* NetworkManager::sendRequest(const NetworkRequest& request)
{
    QNetworkRequest netRequest(QUrl(request.url));

    // Process queue
    for (auto it = request.headers.begin(); it != request.headers.end(); ++it) {
        netRequest.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    // 设置超时
    netRequest.setTransferTimeout(request.timeout);

    QNetworkReply* reply = nullptr;

    if (request.method == "GET") {
        reply = m_httpManager->get(netRequest);
    } else if (request.method == "POST") {
        reply = m_httpManager->post(netRequest, request.data);
    } else if (request.method == "PUT") {
        reply = m_httpManager->put(netRequest, request.data);
    } else if (request.method == "DELETE") {
        reply = m_httpManager->deleteResource(netRequest);
    }

    if (reply) {
        QMutexLocker locker(&m_mutex);
        m_activeRequests[reply] = request;
    }

    return reply;
}

void NetworkManager::retryRequest(const NetworkRequest& request)
{
    if (!m_online) {
        enqueueRequest(request);
        return;
    }

    // 延迟重试
    QTimer::singleShot(1000 * request.retryCount, this, [this, request]() {
        sendRequest(request);
    });
}

void NetworkManager::enqueueRequest(const NetworkRequest& request)
{
    QMutexLocker locker(&m_mutex);
    m_requestQueue.enqueue(request);
}


// ========== 批量请求 ==========

void NetworkManager::getBatchAsync(const QStringList& urls,
                                   std::function<void(const QString &, NetResult<QByteArray>)> callback,
                                   std::function<void()> allCompleteCallback,
                                   const QMap<QString, QString>& headers)
{
    if (urls.isEmpty())
    {
        if (allCompleteCallback) allCompleteCallback();
        return;
    }

    int completedCount = 0;
    int totalCount = urls.size();

    for (const QString& url : urls)
    {
        getAsync(url, [this, url, callback, &completedCount, totalCount, allCompleteCallback](NetResult<QByteArray> result)
        {
            if (callback)
            {
                callback(url, result);
            }

            completedCount++;
            if (completedCount == totalCount && allCompleteCallback)
            {
                allCompleteCallback();
            }
        }, headers);
    }
}

void NetworkManager::getBatchMerged(const QStringList& urls,
                                    std::function<void(QMap<QString, NetResult<QByteArray>>)> callback,
                                    const QMap<QString, QString>& headers)
{
    if (urls.isEmpty())
    {
        callback({});
        return;
    }

    auto results = std::make_shared<QMap<QString, NetResult<QByteArray>>>();
    auto completedCount = std::make_shared<int>(0);
    int totalCount = urls.size();

    for (const QString& url : urls)
    {
        getAsync(url, [results, completedCount, totalCount, callback, url](NetResult<QByteArray> result)
        {
            results->insert(url, result);
            (*completedCount)++;

            if (*completedCount == totalCount)
            {
                callback(*results);
            }
        }, headers);
    }
}
