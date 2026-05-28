/**
 * @file NetworkCache.h
 * @brief Network Request Cache - Optimize network request performance
 *
 * @details Features:
 * - HTTP request caching
 * - Request deduplication
 * - Batch request merging
 * - Failure retry
 * - Offline support
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef NETWORKCACHE_H
#define NETWORKCACHE_H

#include "services/cache/CacheManager.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMap>
#include <QMutex>
#include <QTimer>
#include <QStringList>
#include <memory>

/**
 * @brief Cache configuration
 */
struct NetworkCacheConfig {
    bool enabled = true;                // Enable cache
    int defaultTTL = 300;               // Default cache time (seconds)
    int maxRetries = 3;                 // Max retry count
    int retryDelay = 1000;              // Retry delay (milliseconds)
    int timeout = 30000;                // Request timeout (milliseconds)
    QStringList cacheablePaths;         // Cacheable path patterns
    QStringList excludePaths;           // Excluded path patterns
};

/**
 * @brief Network request cache
 */
class NetworkCache : public QObject
{
    Q_OBJECT

public:
    static NetworkCache& instance();

    bool initialize(const NetworkCacheConfig& config = NetworkCacheConfig());

    // GET request with cache
    void get(const QString& url, 
             const QMap<QString, QString>& headers = QMap<QString, QString>(),
             const QString& cacheKey = QString());

    // POST request with cache
    void post(const QString& url,
              const QByteArray& data,
              const QMap<QString, QString>& headers = QMap<QString, QString>(),
              const QString& cacheKey = QString());

    // Clear cache
    void clearCache();

    // Set configuration
    void setConfig(const NetworkCacheConfig& config);
    NetworkCacheConfig config() const;

signals:
    void requestFinished(const QString& cacheKey, const QByteArray& data);
    void requestError(const QString& cacheKey, const QString& error);
    void cacheHit(const QString& cacheKey);
    void cacheMiss(const QString& cacheKey);

private slots:
    void handleReply(QNetworkReply* reply, const QString& cacheKey, const QString& method);

private:
    NetworkCache();
    ~NetworkCache();
    NetworkCache(const NetworkCache&) = delete;
    NetworkCache& operator=(const NetworkCache&) = delete;

    QVariant getCached(const QString& cacheKey);
    void setCached(const QString& cacheKey, const QVariant& data, int ttl);
    void retryRequest(const QString& url, const QString& method, 
                     const QByteArray& data, const QMap<QString, QString>& headers,
                     const QString& cacheKey, int retryCount);

    QNetworkAccessManager* m_networkManager;
    NetworkCacheConfig m_config;
    QMap<QString, QNetworkReply*> m_pendingRequests;
    mutable QMutex m_mutex;
};

#endif // NETWORKCACHE_H
