/**
 * @file CacheManager.cpp
 * @brief Cache Manager Implementation - High-performance multi-level caching system
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "CacheManager.h"
#include "shared/utils/Logger.h"
#include <QFile>
#include <QDir>
#include <QDataStream>
#include <QElapsedTimer>
#include <QTimer>
#include <QCoreApplication>

CacheManager::CacheManager()
    : m_l1MaxSize(100 * 1024 * 1024)
    , m_l1CurrentSize(0)
    , m_l2MaxSize(1024 * 1024 * 1024)
    , m_l2CurrentSize(0)
    , m_policy(CachePolicy::LRU)
    , m_cleanupInterval(60)
{
}

CacheManager::~CacheManager()
{
    clearAll();
}

bool CacheManager::initialize(qint64 maxMemorySize, qint64 maxDiskSize)
{
    QMutexLocker locker(&m_mutex);
    
    m_l1MaxSize = maxMemorySize;
    m_l2MaxSize = maxDiskSize;
    
    // Setup L2 cache path
    m_l2CachePath = QCoreApplication::applicationDirPath() + "/cache";
    QDir dir(m_l2CachePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Schedule periodic cleanup
    scheduleCleanup();
    
    LOG_INFO(QString("CacheManager initialized (L1: %1MB, L2: %2MB)")
        .arg(maxMemorySize / 1024 / 1024)
        .arg(maxDiskSize / 1024 / 1024));
    
    return true;
}

void CacheManager::set(const QString& key, const QVariant& value, int ttlSeconds, CacheLevel level)
{
    QMutexLocker locker(&m_mutex);
    
    CacheItem item;
    item.value = value;
    item.createTime = QDateTime::currentDateTime();
    item.expireTime = QDateTime::currentDateTime().addSecs(ttlSeconds);
    item.hitCount = 0;
    item.size = estimateSize(value);
    item.level = level;
    
    if (level == CacheLevel::L1_Memory) {
        // Evict if needed
        evictIfNeeded(level);
        
        // Remove old if exists
        if (m_l1Cache.contains(key)) {
            m_l1CurrentSize -= m_l1Cache[key].size;
        }
        
        m_l1Cache[key] = item;
        m_l1CurrentSize += item.size;
    } else if (level == CacheLevel::L2_Disk) {
        writeToDisk(key, item);
    }
    
    ++m_stats.totalSets;
    LOG_DEBUG(QString("Cache set: %1 (size: %2, ttl: %3s)")
        .arg(key).arg(item.size).arg(ttlSeconds));
}

QVariant CacheManager::get(const QString& key, const QVariant& defaultValue)
{
    QMutexLocker locker(&m_mutex);
    
    // Check L1 cache
    if (m_l1Cache.contains(key)) {
        CacheItem& item = m_l1Cache[key];
        
        // Check expiration
        if (item.expireTime.isValid() && QDateTime::currentDateTime() > item.expireTime) {
            m_l1CurrentSize -= item.size;
            m_l1Cache.remove(key);
            emit cacheExpired(key);
            updateStats(false);
            return defaultValue;
        }
        
        ++item.hitCount;
        updateStats(true);
        emit cacheHit(key);
        return item.value;
    }
    
    // Check L2 cache
    CacheItem diskItem = readFromDisk(key);
    if (diskItem.value.isValid()) {
        // Check expiration
        if (diskItem.expireTime.isValid() && QDateTime::currentDateTime() > diskItem.expireTime) {
            QFile::remove(m_l2CachePath + "/" + key + ".cache");
            emit cacheExpired(key);
            updateStats(false);
            return defaultValue;
        }
        
        // Promote to L1
        set(key, diskItem.value, 
            QDateTime::currentDateTime().secsTo(diskItem.expireTime),
            CacheLevel::L1_Memory);
        
        updateStats(true);
        emit cacheHit(key);
        return diskItem.value;
    }
    
    updateStats(false);
    emit cacheMiss(key);
    return defaultValue;
}

bool CacheManager::contains(const QString& key) const
{
    QMutexLocker locker(&m_mutex);
    
    // Check L1
    if (m_l1Cache.contains(key)) {
        const CacheItem& item = m_l1Cache[key];
        if (item.expireTime.isValid() && QDateTime::currentDateTime() > item.expireTime) {
            return false;
        }
        return true;
    }
    
    // Check L2
    QString filePath = m_l2CachePath + "/" + key + ".cache";
    if (QFile::exists(filePath)) {
        CacheItem item = readFromDisk(key);
        if (item.expireTime.isValid() && QDateTime::currentDateTime() > item.expireTime) {
            return false;
        }
        return item.value.isValid();
    }
    
    return false;
}

void CacheManager::remove(const QString& key)
{
    QMutexLocker locker(&m_mutex);
    
    // Remove from L1
    if (m_l1Cache.contains(key)) {
        m_l1CurrentSize -= m_l1Cache[key].size;
        m_l1Cache.remove(key);
    }
    
    // Remove from L2
    QString filePath = m_l2CachePath + "/" + key + ".cache";
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
    }
    
    ++m_stats.totalDeletes;
    LOG_DEBUG(QString("Cache removed: %1").arg(key));
}

void CacheManager::clear(CacheLevel level)
{
    QMutexLocker locker(&m_mutex);
    
    if (level == CacheLevel::L1_Memory) {
        m_l1Cache.clear();
        m_l1CurrentSize = 0;
    } else if (level == CacheLevel::L2_Disk) {
        QDir dir(m_l2CachePath);
        QStringList files = dir.entryList(QStringList() << "*.cache");
        for (const QString& file : files) {
            QFile::remove(m_l2CachePath + "/" + file);
        }
        m_l2CurrentSize = 0;
    }
    
    LOG_INFO(QString("Cache cleared (level: %1)").arg(static_cast<int>(level)));
}

void CacheManager::clearAll()
{
    clear(CacheLevel::L1_Memory);
    clear(CacheLevel::L2_Disk);
}

CacheStats CacheManager::statistics() const
{
    QMutexLocker locker(&m_mutex);
    
    CacheStats stats = m_stats;
    stats.memoryUsage = m_l1CurrentSize;
    stats.diskUsage = m_l2CurrentSize;
    stats.itemCount = m_l1Cache.size();
    
    qint64 total = stats.totalHits + stats.totalMisses;
    stats.hitRate = total > 0 ? static_cast<double>(stats.totalHits) / total : 0.0;
    
    return stats;
}

void CacheManager::setPolicy(CachePolicy policy)
{
    QMutexLocker locker(&m_mutex);
    m_policy = policy;
    LOG_INFO(QString("Cache policy changed to: %1").arg(static_cast<int>(policy)));
}

void CacheManager::warmup(const QMap<QString, QVariant>& data)
{
    for (auto it = data.begin(); it != data.end(); ++it) {
        set(it.key(), it.value(), 3600, CacheLevel::L1_Memory);
    }
    LOG_INFO(QString("Cache warmed up with %1 items").arg(data.size()));
}

QMap<QString, QVariant> CacheManager::getBatch(const QStringList& keys)
{
    QMap<QString, QVariant> result;
    for (const QString& key : keys) {
        result[key] = get(key);
    }
    return result;
}

void CacheManager::setBatch(const QMap<QString, QVariant>& data, int ttlSeconds)
{
    for (auto it = data.begin(); it != data.end(); ++it) {
        set(it.key(), it.value(), ttlSeconds, CacheLevel::L1_Memory);
    }
}

void CacheManager::cleanupExpired()
{
    QMutexLocker locker(&m_mutex);
    
    QDateTime now = QDateTime::currentDateTime();
    
    // Cleanup L1
    QStringList expiredKeys;
    for (auto it = m_l1Cache.begin(); it != m_l1Cache.end(); ++it) {
        if (it->expireTime.isValid() && now > it->expireTime) {
            expiredKeys.append(it.key());
        }
    }
    
    for (const QString& key : expiredKeys) {
        m_l1CurrentSize -= m_l1Cache[key].size;
        m_l1Cache.remove(key);
        emit cacheExpired(key);
    }
    
    // Cleanup L2
    QDir dir(m_l2CachePath);
    QStringList files = dir.entryList(QStringList() << "*.cache");
    for (const QString& file : files) {
        QString key = file.left(file.length() - 6);  // Remove ".cache"
        CacheItem item = readFromDisk(key);
        if (item.expireTime.isValid() && now > item.expireTime) {
            QFile::remove(m_l2CachePath + "/" + file);
            emit cacheExpired(key);
        }
    }
    
    if (!expiredKeys.isEmpty()) {
        LOG_DEBUG(QString("Cleaned up %1 expired cache items").arg(expiredKeys.size()));
    }
}

void CacheManager::evictIfNeeded(CacheLevel level)
{
    if (level != CacheLevel::L1_Memory) {
        return;
    }
    
    while (m_l1CurrentSize > m_l1MaxSize && !m_l1Cache.isEmpty()) {
        QString keyToEvict;
        
        if (m_policy == CachePolicy::LRU) {
            // Find oldest by create time
            QDateTime oldest = QDateTime::currentDateTime();
            for (auto it = m_l1Cache.begin(); it != m_l1Cache.end(); ++it) {
                if (it->createTime < oldest) {
                    oldest = it->createTime;
                    keyToEvict = it.key();
                }
            }
        } else if (m_policy == CachePolicy::LFU) {
            // Find lowest hit count
            int minHits = INT_MAX;
            for (auto it = m_l1Cache.begin(); it != m_l1Cache.end(); ++it) {
                if (it->hitCount < minHits) {
                    minHits = it->hitCount;
                    keyToEvict = it.key();
                }
            }
        } else if (m_policy == CachePolicy::FIFO) {
            // Find oldest by create time (same as LRU for simplicity)
            QDateTime oldest = QDateTime::currentDateTime();
            for (auto it = m_l1Cache.begin(); it != m_l1Cache.end(); ++it) {
                if (it->createTime < oldest) {
                    oldest = it->createTime;
                    keyToEvict = it.key();
                }
            }
        }
        
        if (!keyToEvict.isEmpty()) {
            m_l1CurrentSize -= m_l1Cache[keyToEvict].size;
            m_l1Cache.remove(keyToEvict);
            LOG_DEBUG(QString("Evicted cache item: %1").arg(keyToEvict));
        } else {
            break;
        }
    }
}

QString CacheManager::serialize(const QVariant& value) const
{
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream << value;
    return data.toBase64();
}

QVariant CacheManager::deserialize(const QString& data) const
{
    QByteArray bytes = QByteArray::fromBase64(data.toUtf8());
    QDataStream stream(&bytes, QIODevice::ReadOnly);
    QVariant value;
    stream >> value;
    return value;
}

bool CacheManager::writeToDisk(const QString& key, const CacheItem& item)
{
    QString filePath = m_l2CachePath + "/" + key + ".cache";
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to write cache to disk: %1").arg(key));
        return false;
    }
    
    QDataStream stream(&file);
    stream << item.value << item.createTime << item.expireTime << item.hitCount;
    file.close();
    
    return true;
}

CacheItem CacheManager::readFromDisk(const QString& key) const
{
    CacheItem item;
    QString filePath = m_l2CachePath + "/" + key + ".cache";
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        return item;
    }
    
    QDataStream stream(&file);
    stream >> item.value >> item.createTime >> item.expireTime >> item.hitCount;
    file.close();
    
    item.level = CacheLevel::L2_Disk;
    return item;
}

void CacheManager::updateStats(bool hit)
{
    if (hit) {
        ++m_stats.totalHits;
    } else {
        ++m_stats.totalMisses;
    }
}

void CacheManager::scheduleCleanup()
{
    QTimer::singleShot(m_cleanupInterval * 1000, this, [this]() {
        cleanupExpired();
        scheduleCleanup();
    });
}

int CacheManager::estimateSize(const QVariant& value) const
{
    // Rough estimate of memory size
    if (value.typeId() == QMetaType::QString) {
        return value.toString().size() * 2;  // UTF-16
    } else if (value.typeId() == QMetaType::QByteArray) {
        return value.toByteArray().size();
    } else if (value.typeId() == QMetaType::Int || value.typeId() == QMetaType::Double) {
        return 8;
    }
    return 64;  // Default estimate
}
