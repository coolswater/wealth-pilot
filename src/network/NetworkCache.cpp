/**
 * @file NetworkCache.cpp
 * @brief Network Request Cache Implementation
 */

#include "NetworkCache.h"
#include "../utils/Logger.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>

NetworkCache& NetworkCache::instance()
{
    static NetworkCache instance;
    return instance;
}

NetworkCache::NetworkCache()
    : m_networkManager(new QNetworkAccessManager(this))
{
    LOG_DEBUG("NetworkCache created");
}

NetworkCache::~NetworkCache()
{
    clearCache();
    LOG_DEBUG("NetworkCache destroyed");
}

bool NetworkCache::initialize(const NetworkCacheConfig& config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;
    
    LOG_INFO(QString("NetworkCache initialized (enabled: %1, TTL: %2s)")
        .arg(config.enabled).arg(config.defaultTTL));
    return true;
}

void NetworkCache::get(const QString& url, const QMap<QString, QString>& headers, const QString& cacheKey)
{
    QString key = cacheKey.isEmpty() ? url : cacheKey;
    
    // Check cache
    if (m_config.enabled) {
        QVariant cached = getCached(key);
        if (cached.isValid()) {
            LOG_DEBUG(QString("Cache hit: %1").arg(key));
            emit cacheHit(key);
            emit requestFinished(key, cached.toByteArray());
            return;
        }
    }
    
    emit cacheMiss(key);
    
    // Create request
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    
    // Send request
    QNetworkReply* reply = m_networkManager->get(request);
    
    {
        QMutexLocker locker(&m_mutex);
        m_pendingRequests[key] = reply;
    }
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, key]() {
        handleReply(reply, key, "GET");
    });
}

void NetworkCache::post(const QString& url, const QByteArray& data, const QMap<QString, QString>& headers, const QString& cacheKey)
{
    QString key = cacheKey.isEmpty() ? url : cacheKey;
    
    // Create request
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    
    // Send request
    QNetworkReply* reply = m_networkManager->post(request, data);
    
    {
        QMutexLocker locker(&m_mutex);
        m_pendingRequests[key] = reply;
    }
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, key]() {
        handleReply(reply, key, "POST");
    });
}

void NetworkCache::clearCache()
{
    QMutexLocker locker(&m_mutex);
    m_pendingRequests.clear();
    LOG_DEBUG("NetworkCache cleared");
}

void NetworkCache::setConfig(const NetworkCacheConfig& config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;
}

NetworkCacheConfig NetworkCache::config() const
{
    QMutexLocker locker(&m_mutex);
    return m_config;
}

QVariant NetworkCache::getCached(const QString& cacheKey)
{
    return CacheManager::instance()->get("network_" + cacheKey);
}

void NetworkCache::setCached(const QString& cacheKey, const QVariant& data, int ttl)
{
    CacheManager::instance()->set("network_" + cacheKey, data, ttl, CacheLevel::L1_Memory);
}

void NetworkCache::handleReply(QNetworkReply* reply, const QString& cacheKey, const QString& method)
{
    QElapsedTimer timer;
    timer.start();
    
    {
        QMutexLocker locker(&m_mutex);
        m_pendingRequests.remove(cacheKey);
    }
    
    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        LOG_ERROR(QString("Network error: %1 - %2").arg(cacheKey, error));
        emit requestError(cacheKey, error);
        
        // Retry if enabled
        int retryCount = reply->property("retryCount").toInt();
        if (retryCount < m_config.maxRetries) {
            QTimer::singleShot(m_config.retryDelay, this, [this, cacheKey, method, retryCount]() {
                // Note: Original request data would need to be stored for retry
                LOG_DEBUG(QString("Retrying request: %1 (attempt %2)").arg(cacheKey).arg(retryCount + 1));
            });
        }
        
        reply->deleteLater();
        return;
    }
    
    // Read response
    QByteArray data = reply->readAll();
    
    // Cache if enabled and GET request
    if (m_config.enabled && method == "GET") {
        setCached(cacheKey, data, m_config.defaultTTL);
    }
    
    LOG_DEBUG(QString("Request completed: %1 (%2ms, %3 bytes)")
        .arg(cacheKey)
        .arg(timer.elapsed())
        .arg(data.size()));
    
    emit requestFinished(cacheKey, data);
    reply->deleteLater();
}

void NetworkCache::retryRequest(const QString& url, const QString& method, 
                               const QByteArray& data, const QMap<QString, QString>& headers,
                               const QString& cacheKey, int retryCount)
{
    if (retryCount >= m_config.maxRetries) {
        LOG_ERROR(QString("Max retries exceeded: %1").arg(url));
        emit requestError(cacheKey, "Max retries exceeded");
        return;
    }
    
    // Wait before retry
    QTimer::singleShot(m_config.retryDelay, this, [this, url, method, data, headers, cacheKey, retryCount]() {
        if (method == "GET") {
            get(url, headers, cacheKey);
        } else if (method == "POST") {
            post(url, data, headers, cacheKey);
        }
    });
}
