/**
 * @file NetworkCache.cpp
 * @brief 网络请求缓存实现 - 优化网络请求性能
 *
 * @details 实现功能：
 * - HTTP请求缓存：减少重复请求
 * - 请求去重：合并相同请求
 * - 批量请求合并：提高效率
 * - 失败重试：提高可靠性
 * - 离线支持：缓存优先
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "NetworkCache.h"
#include "../core/CacheManager.h"
#include "../utils/Logger.h"
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>

// ========== NetworkCache 实现 ==========

/**
 * @brief 获取单例实例
 */
NetworkCache& NetworkCache::instance()
{
    static NetworkCache instance;
    return instance;
}

/**
 * @brief 构造函数
 */
NetworkCache::NetworkCache()
    : m_networkManager(new QNetworkAccessManager(this))
{
    LOG_DEBUG("NetworkCache created");
}

/**
 * @brief 析构函数
 */
NetworkCache::~NetworkCache()
{
    LOG_DEBUG("NetworkCache destroyed");
}

/**
 * @brief 初始化网络缓存
 * @param config 缓存配置
 * @return 是否成功
 */
bool NetworkCache::initialize(const NetworkCacheConfig& config)
{
    m_config = config;
    
    LOG_INFO(QString("NetworkCache initialized: enabled=%1, defaultTTL=%2s")
        .arg(config.enabled).arg(config.defaultTTL));
    
    return true;
}

/**
 * @brief 发送GET请求（带缓存）
 * @param url 请求URL
 * @param headers 请求头
 * @param cacheTTL 缓存时间（-1使用默认值）
 */
void NetworkCache::get(const QString& url,
                      const QMap<QString, QString>& headers,
                      int cacheTTL)
{
    QElapsedTimer timer;
    timer.start();
    
    // 检查是否应该缓存
    bool cacheable = shouldCache(url);
    
    // 检查缓存
    if (m_config.enabled && cacheable) {
        QString cacheKey = this->cacheKey(url);
        QVariant cached = CacheManager::instance()->get(cacheKey);
        
        if (cached.isValid()) {
            // 缓存命中
            LOG_DEBUG(QString("Cache hit for: %1").arg(url));
            emit cacheHit(url);
            emit requestFinished(url, cached.toByteArray(), true);
            return;
        }
        
        // 缓存未命中
        LOG_DEBUG(QString("Cache miss for: %1").arg(url));
        emit cacheMiss(url);
    }
    
    // 检查请求去重
    {
        QMutexLocker locker(&m_mutex);
        if (m_pendingRequests.contains(url)) {
            LOG_DEBUG(QString("Request already pending, waiting: %1").arg(url));
            // 等待正在进行的请求完成
            return;
        }
        m_pendingRequests[url] = 0;
    }
    
    // 执行实际请求
    executeRequest(url, "GET", QByteArray(), headers);
    
    LOG_DEBUG(QString("GET request sent: %1 (%2ms)")
        .arg(url).arg(timer.elapsed()));
}

/**
 * @brief 发送POST请求
 * @param url 请求URL
 * @param data 请求数据
 * @param headers 请求头
 */
void NetworkCache::post(const QString& url,
                       const QByteArray& data,
                       const QMap<QString, QString>& headers)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_DEBUG(QString("POST request: %1").arg(url));
    
    // POST请求通常不缓存
    executeRequest(url, "POST", data, headers);
    
    LOG_DEBUG(QString("POST request sent: %1 (%2ms)")
        .arg(url).arg(timer.elapsed()));
}

/**
 * @brief 发送PUT请求
 * @param url 请求URL
 * @param data 请求数据
 * @param headers 请求头
 */
void NetworkCache::put(const QString& url,
                      const QByteArray& data,
                      const QMap<QString, QString>& headers)
{
    executeRequest(url, "PUT", data, headers);
}

/**
 * @brief 发送DELETE请求
 * @param url 请求URL
 * @param headers 请求头
 */
void NetworkCache::del(const QString& url,
                      const QMap<QString, QString>& headers)
{
    executeRequest(url, "DELETE", QByteArray(), headers);
}

/**
 * @brief 批量请求
 * @param urls URL列表
 */
void NetworkCache::batchRequest(const QStringList& urls)
{
    LOG_DEBUG(QString("Batch request: %1 URLs").arg(urls.size()));
    
    for (const QString& url : urls) {
        get(url);
    }
}

/**
 * @brief 清除URL缓存
 * @param urlPattern URL模式（为空则清除所有）
 */
void NetworkCache::clearCache(const QString& urlPattern)
{
    if (urlPattern.isEmpty()) {
        CacheManager::instance()->clear(CacheLevel::L1_Memory);
        CacheManager::instance()->clear(CacheLevel::L2_Disk);
        LOG_INFO("All network cache cleared");
    } else {
        // 根据模式清除（简化实现：清除包含该模式的所有缓存）
        // TODO: 实现模式匹配
        LOG_INFO(QString("Cache cleared for pattern: %1").arg(urlPattern));
    }
}

/**
 * @brief 预热缓存
 * @param urls URL列表
 */
void NetworkCache::warmupCache(const QStringList& urls)
{
    LOG_INFO(QString("Warming up cache: %1 URLs").arg(urls.size()));
    
    for (const QString& url : urls) {
        get(url);
    }
}

/**
 * @brief 获取配置
 */
NetworkCacheConfig NetworkCache::configuration() const
{
    return m_config;
}

/**
 * @brief 设置配置
 */
void NetworkCache::setConfiguration(const NetworkCacheConfig& config)
{
    m_config = config;
}

/**
 * @brief 判断URL是否应该缓存
 * @param url URL
 * @return 是否应该缓存
 */
bool NetworkCache::shouldCache(const QString& url) const
{
    // 检查排除路径
    for (const QString& exclude : m_config.excludePaths) {
        if (url.contains(exclude)) {
            return false;
        }
    }
    
    // 检查可缓存路径
    if (m_config.cacheablePaths.isEmpty()) {
        // 默认：GET请求缓存
        return true;
    }
    
    for (const QString& cacheable : m_config.cacheablePaths) {
        if (url.contains(cacheable)) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 生成缓存键
 * @param url URL
 * @return 缓存键
 */
QString NetworkCache::cacheKey(const QString& url) const
{
    return QString("network_%1").arg(QString::number(qHash(url)));
}

/**
 * @brief 执行请求
 * @param url URL
 * @param method HTTP方法
 * @param data 请求数据
 * @param headers 请求头
 */
void NetworkCache::executeRequest(const QString& url,
                                  const QString& method,
                                  const QByteArray& data,
                                  const QMap<QString, QString>& headers)
{
    QNetworkRequest request{QUrl(url)};
    
    // 设置请求头
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    
    // 设置超时
    request.setTransferTimeout(m_config.timeout);
    
    // 发送请求
    QNetworkReply* reply = nullptr;
    
    if (method == "GET") {
        reply = m_networkManager->get(request);
    } else if (method == "POST") {
        reply = m_networkManager->post(request, data);
    } else if (method == "PUT") {
        reply = m_networkManager->put(request, data);
    } else if (method == "DELETE") {
        reply = m_networkManager->deleteResource(request);
    }
    
    if (reply) {
        // 连接完成信号
        connect(reply, &QNetworkReply::finished, this, [this, reply, url, method, data, headers]() {
            handleReply(reply, url, shouldCache(url), m_config.defaultTTL);
            
            // 移除待处理请求
            QMutexLocker locker(&m_mutex);
            m_pendingRequests.remove(url);
        });
    }
}

/**
 * @brief 处理响应
 * @param reply 网络响应
 * @param url URL
 * @param cacheable 是否可缓存
 * @param cacheTTL 缓存时间
 */
void NetworkCache::handleReply(QNetworkReply* reply,
                              const QString& url,
                              bool cacheable,
                              int cacheTTL)
{
    QElapsedTimer timer;
    timer.start();
    
    if (reply->error() == QNetworkReply::NoError) {
        // 成功
        QByteArray data = reply->readAll();
        
        // 缓存响应
        if (m_config.enabled && cacheable) {
            QString key = cacheKey(url);
            CacheManager::instance()->set(key, data, cacheTTL, CacheLevel::L1_Memory);
            
            LOG_DEBUG(QString("Response cached: %1").arg(url));
        }
        
        emit requestFinished(url, data, false);
    } else {
        // 失败
        QString error = reply->errorString();
        
        LOG_ERROR(QString("Request failed: %1, error: %2").arg(url).arg(error));
        
        // 检查是否需要重试
        int retryCount = m_pendingRequests.value(url, 0);
        
        if (retryCount < m_config.maxRetries) {
            LOG_INFO(QString("Retrying request: %1 (attempt %2/%3)")
                .arg(url).arg(retryCount + 1).arg(m_config.maxRetries));
            
            // 重试
            QTimer::singleShot(m_config.retryDelay, this, [this, url, retryCount]() {
                QMutexLocker locker(&m_mutex);
                m_pendingRequests[url] = retryCount + 1;
                locker.unlock();
                
                // 重新执行请求
                executeRequest(url, "GET", QByteArray(), QMap<QString, QString>());
            });
        } else {
            // 重试次数用尽
            emit requestFailed(url, error);
        }
    }
    
    reply->deleteLater();
    
    LOG_DEBUG(QString("Request completed: %1 (%2ms)")
        .arg(url).arg(timer.elapsed()));
}

/**
 * @brief 重试请求
 * @param url URL
 * @param method HTTP方法
 * @param data 请求数据
 * @param headers 请求头
 * @param retryCount 重试次数
 */
void NetworkCache::retryRequest(const QString& url,
                               const QString& method,
                               const QByteArray& data,
                               const QMap<QString, QString>& headers,
                               int retryCount)
{
    if (retryCount < m_config.maxRetries) {
        LOG_DEBUG(QString("Retrying request: %1 (attempt %2)").arg(url).arg(retryCount + 1));
        
        QTimer::singleShot(m_config.retryDelay, this, [this, url, method, data, headers, retryCount]() {
            executeRequest(url, method, data, headers);
        });
    } else {
        LOG_ERROR(QString("Max retries exceeded: %1").arg(url));
        emit requestFailed(url, "Max retries exceeded");
    }
}
