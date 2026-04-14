/**
 * @file NetworkCache.h
 * @brief 网络请求缓存 - 优化网络请求性能
 *
 * @details 功能：
 * - HTTP请求缓存
 * - 请求去重
 * - 批量请求合并
 * - 失败重试
 * - 离线支持
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef NETWORKCACHE_H
#define NETWORKCACHE_H

#include "../core/CacheManager.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMap>
#include <QMutex>
#include <QTimer>
#include <memory>

/**
 * @brief 缓存配置
 */
struct NetworkCacheConfig {
    bool enabled;               // 是否启用缓存
    int defaultTTL;            // 默认缓存时间（秒）
    int maxRetries;            // 最大重试次数
    int retryDelay;            // 重试延迟（毫秒）
    int timeout;               // 请求超时（毫秒）
    QStringList cacheablePaths; // 可缓存的路径模式
    QStringList excludePaths;   // 排除缓存的路径模式
};

/**
 * @brief 网络请求缓存
 */
class NetworkCache : public QObject
{
    Q_OBJECT

public:
    static NetworkCache& instance();

    /**
     * @brief 初始化
     */
    bool initialize(const NetworkCacheConfig& config = NetworkCacheConfig());

    /**
     * @brief 发送GET请求（带缓存）
     */
    void get(const QString& url, 
            const QMap<QString, QString>& headers = QMap<QString, QString>(),
            int cacheTTL = -1);

    /**
     * @brief 发送POST请求
     */
    void post(const QString& url,
             const QByteArray& data,
             const QMap<QString, QString>& headers = QMap<QString, QString>());

    /**
     * @brief 发送PUT请求
     */
    void put(const QString& url,
            const QByteArray& data,
            const QMap<QString, QString>& headers = QMap<QString, QString>());

    /**
     * @brief 发送DELETE请求
     */
    void del(const QString& url,
            const QMap<QString, QString>& headers = QMap<QString, QString>());

    /**
     * @brief 批量请求
     */
    void batchRequest(const QStringList& urls);

    /**
     * @brief 清除URL缓存
     */
    void clearCache(const QString& urlPattern = QString());

    /**
     * @brief 预热缓存
     */
    void warmupCache(const QStringList& urls);

    /**
     * @brief 获取配置
     */
    NetworkCacheConfig configuration() const;

    /**
     * @brief 设置配置
     */
    void setConfiguration(const NetworkCacheConfig& config);

signals:
    /**
     * @brief 请求完成信号
     */
    void requestFinished(const QString& url, const QByteArray& data, bool fromCache);

    /**
     * @brief 请求失败信号
     */
    void requestFailed(const QString& url, const QString& error);

    /**
     * @brief 缓存命中信号
     */
    void cacheHit(const QString& url);

    /**
     * @brief 缓存未命中信号
     */
    void cacheMiss(const QString& url);

private:
    NetworkCache();
    ~NetworkCache();
    NetworkCache(const NetworkCache&) = delete;
    NetworkCache& operator=(const NetworkCache&) = delete;

    // 内部方法
    bool shouldCache(const QString& url) const;
    QString cacheKey(const QString& url) const;
    void executeRequest(const QString& url, const QString& method, const QByteArray& data, const QMap<QString, QString>& headers);
    void handleReply(QNetworkReply* reply, const QString& url, bool cacheable, int cacheTTL);
    void retryRequest(const QString& url, const QString& method, const QByteArray& data, const QMap<QString, QString>& headers, int retryCount);

    QNetworkAccessManager* m_networkManager;
    NetworkCacheConfig m_config;
    QMap<QString, int> m_pendingRequests;  // URL -> 重试次数
    mutable QMutex m_mutex;
    
    // 请求去重
    QMap<QString, QList<std::function<void(const QByteArray&)>>> m_pendingCallbacks;
};

#endif // NETWORKCACHE_H
