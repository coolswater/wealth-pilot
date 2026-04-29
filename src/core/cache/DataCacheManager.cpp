/**
 * @file DataCacheManager.cpp
 * @brief 数据缓存管理器实现
 */

#include "DataCacheManager.h"
#include "utils/Logger.h"

#include <QVariant>
#include <QMutexLocker>

struct DataCacheManager::Impl {
    QHash<QString, QVariant> cache;
    QHash<QString, QDateTime> expireTimes;
    mutable QMutex mutex;
    QTimer* cleanupTimer = nullptr;
    
    DataCacheStats stats;
    int maxSize = 1000;
    bool enabled = true;
};

DataCacheManager* DataCacheManager::instance()
{
    static DataCacheManager instance;
    return &instance;
}

DataCacheManager::DataCacheManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->cleanupTimer = new QTimer(this);
    connect(d->cleanupTimer, &QTimer::timeout, this, &DataCacheManager::cleanupExpired);
    d->cleanupTimer->start(60000); // 每分钟清理一次过期缓存
    
    LOG_DEBUG("DataCacheManager initialized");
}

DataCacheManager::~DataCacheManager()
{
    clear();
}

template<typename T>
void DataCacheManager::set(const QString& key, const T& data, qint64 ttlMs)
{
    if (!d->enabled) return;
    
    QMutexLocker locker(&d->mutex);
    
    // 设置过期时间
    QDateTime expireTime;
    if (ttlMs > 0) {
        expireTime = QDateTime::currentDateTime().addMSecs(ttlMs);
    }
    
    // 存储数据
    d->cache[key] = QVariant::fromValue(data);
    d->expireTimes[key] = expireTime;
    d->stats.totalSets++;
    
    // 检查是否需要淘汰
    evictIfNeeded();
    
    LOG_DEBUG(QString("Cache set: %1, TTL: %2ms").arg(key).arg(ttlMs));
}

template<typename T>
bool DataCacheManager::get(const QString& key, T& outData)
{
    if (!d->enabled) {
        d->stats.totalMisses++;
        return false;
    }
    
    QMutexLocker locker(&d->mutex);
    
    if (!d->cache.contains(key)) {
        d->stats.totalMisses++;
        emit cacheMiss(key);
        return false;
    }
    
    // 检查是否过期
    if (d->expireTimes.contains(key) && d->expireTimes[key].isValid()) {
        if (QDateTime::currentDateTime() > d->expireTimes[key]) {
            d->cache.remove(key);
            d->expireTimes.remove(key);
            d->stats.totalMisses++;
            d->stats.totalEvictions++;
            emit cacheMiss(key);
            emit cacheEvicted(key);
            return false;
        }
    }
    
    // 获取数据
    outData = d->cache[key].value<T>();
    d->stats.totalHits++;
    emit cacheHit(key);
    
    return true;
}

bool DataCacheManager::contains(const QString& key) const
{
    QMutexLocker locker(&d->mutex);
    return d->cache.contains(key);
}

void DataCacheManager::remove(const QString& key)
{
    QMutexLocker locker(&d->mutex);
    d->cache.remove(key);
    d->expireTimes.remove(key);
}

void DataCacheManager::clear()
{
    QMutexLocker locker(&d->mutex);
    d->cache.clear();
    d->expireTimes.clear();
    LOG_INFO("Cache cleared");
}

DataCacheStats DataCacheManager::stats() const
{
    QMutexLocker locker(&d->mutex);
    return d->stats;
}

void DataCacheManager::setMaxSize(int maxSize)
{
    QMutexLocker locker(&d->mutex);
    d->maxSize = maxSize;
    evictIfNeeded();
}

void DataCacheManager::setEnabled(bool enabled)
{
    QMutexLocker locker(&d->mutex);
    d->enabled = enabled;
}

bool DataCacheManager::isEnabled() const
{
    QMutexLocker locker(&d->mutex);
    return d->enabled;
}

void DataCacheManager::cleanupExpired()
{
    QMutexLocker locker(&d->mutex);
    
    QDateTime now = QDateTime::currentDateTime();
    QStringList expiredKeys;
    
    for (auto it = d->expireTimes.begin(); it != d->expireTimes.end(); ++it) {
        if (it->isValid() && now > *it) {
            expiredKeys.append(it.key());
        }
    }
    
    for (const QString& key : expiredKeys) {
        d->cache.remove(key);
        d->expireTimes.remove(key);
        d->stats.totalEvictions++;
        emit cacheEvicted(key);
    }
    
    if (!expiredKeys.isEmpty()) {
        LOG_DEBUG(QString("Cleaned up %1 expired cache items").arg(expiredKeys.size()));
    }
}

void DataCacheManager::evictIfNeeded()
{
    // 注意：调用此函数时已经持有锁
    if (d->cache.size() <= d->maxSize) {
        return;
    }
    
    // 简单的LRU策略：移除最早过期的项
    int toRemove = d->cache.size() - d->maxSize;
    
    // 按过期时间排序
    QVector<QPair<QString, QDateTime>> items;
    for (auto it = d->expireTimes.begin(); it != d->expireTimes.end(); ++it) {
        items.append({it.key(), it.value()});
    }
    
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        if (!a.second.isValid()) return false;
        if (!b.second.isValid()) return true;
        return a.second < b.second;
    });
    
    for (int i = 0; i < toRemove && i < items.size(); ++i) {
        const QString& key = items[i].first;
        d->cache.remove(key);
        d->expireTimes.remove(key);
        d->stats.totalEvictions++;
        emit cacheEvicted(key);
    }
    
    LOG_DEBUG(QString("Evicted %1 cache items").arg(toRemove));
}

// 显式实例化常用类型
template void DataCacheManager::set<QString>(const QString&, const QString&, qint64);
template void DataCacheManager::set<double>(const QString&, const double&, qint64);
template void DataCacheManager::set<int>(const QString&, const int&, qint64);
template bool DataCacheManager::get<QString>(const QString&, QString&);
template bool DataCacheManager::get<double>(const QString&, double&);
template bool DataCacheManager::get<int>(const QString&, int&);
