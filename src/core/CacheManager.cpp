/**
 * @file CacheManager.cpp
 * @brief 缓存管理器实现 - 高性能多级缓存系统
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "CacheManager.h"
#include "../utils/Logger.h"
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
    m_stats = {0, 0, 0, 0, 0, 0, 0, 0.0};
}

CacheManager::~CacheManager()
{
    clearAll();
}

bool CacheManager::initialize(qint64 maxMemorySize, qint64 maxDiskSize)
{
    QElapsedTimer timer;
    timer.start();
    
    m_l1MaxSize = maxMemorySize;
    m_l2MaxSize = maxDiskSize;
    
    // 初始化L2磁盘缓存目录
    QString cachePath = QCoreApplication::applicationDirPath() + "/cache";
    QDir dir(cachePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    m_l2CachePath = cachePath;
    
    // 计算当前磁盘缓存大小
    m_l2CurrentSize = 0;
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
        m_l2CurrentSize += info.size();
    }
    
    // 启动定时清理
    scheduleCleanup();
    
    LOG_INFO(QString("CacheManager initialized in %1ms, L1: %2MB, L2: %3MB")
        .arg(timer.elapsed())
        .arg(maxMemorySize / 1024 / 1024)
        .arg(maxDiskSize / 1024 / 1024));
    
    return true;
}

void CacheManager::set(const QString& key, const QVariant& value, int ttlSeconds, CacheLevel level)
{
    QMutexLocker locker(&m_mutex);
    
    QElapsedTimer timer;
    timer.start();
    
    // 创建缓存项
    CacheItem item;
    item.value = value;
    item.createTime = QDateTime::currentDateTime();
    item.expireTime = item.createTime.addSecs(ttlSeconds);
    item.hitCount = 0;
    item.size = serialize(value).size();
    item.level = level;
    
    // L1内存缓存
    if (level == CacheLevel::L1_Memory) {
        // 检查是否需要淘汰
        if (m_l1CurrentSize + item.size > m_l1MaxSize) {
            evictIfNeeded(CacheLevel::L1_Memory);
        }
        
        // 如果已存在，先删除
        if (m_l1Cache.contains(key)) {
            m_l1CurrentSize -= m_l1Cache[key].size;
        }
        
        m_l1Cache[key] = item;
        m_l1CurrentSize += item.size;
    }
    
    // L2磁盘缓存
    if (level == CacheLevel::L2_Disk) {
        writeToDisk(key, item);
    }
    
    m_stats.totalSets++;
    
    LOG_DEBUG(QString("Cache set: %1, size: %2, time: %3ms")
        .arg(key).arg(item.size).arg(timer.elapsed()));
}

QVariant CacheManager::get(const QString& key, const QVariant& defaultValue)
{
    QMutexLocker locker(&m_mutex);
    
    // 先查L1
    if (m_l1Cache.contains(key)) {
        CacheItem& item = m_l1Cache[key];
        
        // 检查是否过期
        if (item.expireTime.isValid() && QDateTime::currentDateTime() > item.expireTime) {
            m_l1Cache.remove(key);
            m_l1CurrentSize -= item.size;
            m_stats.totalMisses++;
            updateStats(false);
            
            locker.unlock();
            emit cacheExpired(key);
            
            return defaultValue;
        }
        
        // 命中
        item.hitCount++;
        m_stats.totalHits++;
        updateStats(true);
        
        locker.unlock();
        emit cacheHit(key);
        
        return item.value;
    }
    
    // 再查L2
    if (QFile::exists(m_l2CachePath + "/" + key + ".cache")) {
        CacheItem item = readFromDisk(key);
        
        if (item.value.isValid()) {
            // 检查是否过期
            if (item.expireTime.isValid() && QDateTime::currentDateTime() > item.expireTime) {
                QFile::remove(m_l2CachePath + "/" + key + ".cache");
                m_stats.totalMisses++;
                updateStats(false);
                
                locker.unlock();
                emit cacheExpired(key);
                
                return defaultValue;
            }
            
            // 提升到L1
            item.level = CacheLevel::L1_Memory;
            if (m_l1CurrentSize + item.size <= m_l1MaxSize) {
                m_l1Cache[key] = item;
                m_l1CurrentSize += item.size;
            }
            
            m_stats.totalHits++;
            updateStats(true);
            
            locker.unlock();
            emit cacheHit(key);
            
            return item.value;
        }
    }
    
    // 未命中
    m_stats.totalMisses++;
    updateStats(false);
    
    locker.unlock();
    emit cacheMiss(key);
    
    return defaultValue;
}

bool CacheManager::contains(const QString& key) const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_l1Cache.contains(key)) {
        const CacheItem& item = m_l1Cache[key];
        // 检查是否过期
        if (item.expireTime.isValid() && QDateTime::currentDateTime() > item.expireTime) {
            return false;
        }
        return true;
    }
    
    // 检查L2
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
    
    // 从L1删除
    if (m_l1Cache.contains(key)) {
        m_l1CurrentSize -= m_l1Cache[key].size;
        m_l1Cache.remove(key);
    }
    
    // 从L2删除
    QString filePath = m_l2CachePath + "/" + key + ".cache";
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
    }
    
    m_stats.totalDeletes++;
    
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
        for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
            QFile::remove(info.absoluteFilePath());
        }
        m_l2CurrentSize = 0;
    }
    
    LOG_INFO(QString("Cache cleared: level %1").arg(static_cast<int>(level)));
}

void CacheManager::clearAll()
{
    QMutexLocker locker(&m_mutex);
    
    m_l1Cache.clear();
    m_l1CurrentSize = 0;
    
    QDir dir(m_l2CachePath);
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
        QFile::remove(info.absoluteFilePath());
    }
    m_l2CurrentSize = 0;
    
    LOG_INFO("All cache cleared");
}

CacheStats CacheManager::statistics() const
{
    QMutexLocker locker(&m_mutex);
    
    CacheStats stats = m_stats;
    stats.memoryUsage = m_l1CurrentSize;
    stats.diskUsage = m_l2CurrentSize;
    stats.itemCount = m_l1Cache.size();
    
    qint64 total = stats.totalHits + stats.totalMisses;
    stats.hitRate = total > 0 ? (double)stats.totalHits / total : 0.0;
    
    return stats;
}

void CacheManager::setPolicy(CachePolicy policy)
{
    QMutexLocker locker(&m_mutex);
    m_policy = policy;
}

void CacheManager::warmup(const QMap<QString, QVariant>& data)
{
    QElapsedTimer timer;
    timer.start();
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        set(it.key(), it.value(), 3600, CacheLevel::L1_Memory);  // 1小时
    }
    
    LOG_INFO(QString("Cache warmed up: %1 items in %2ms")
        .arg(data.size()).arg(timer.elapsed()));
}

QMap<QString, QVariant> CacheManager::getBatch(const QStringList& keys)
{
    QMap<QString, QVariant> result;
    
    for (const QString& key : keys) {
        QVariant value = get(key);
        if (value.isValid()) {
            result[key] = value;
        }
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
    int expiredCount = 0;
    
    // 清理L1
    QStringList keysToRemove;
    for (auto it = m_l1Cache.begin(); it != m_l1Cache.end(); ++it) {
        if (it.value().expireTime.isValid() && now > it.value().expireTime) {
            keysToRemove.append(it.key());
        }
    }
    
    for (const QString& key : keysToRemove) {
        m_l1CurrentSize -= m_l1Cache[key].size;
        m_l1Cache.remove(key);
        expiredCount++;
    }
    
    // 清理L2
    QDir dir(m_l2CachePath);
    for (const QFileInfo& info : dir.entryInfoList(QDir::Files)) {
        QString key = info.baseName();
        CacheItem item = readFromDisk(key);
        if (item.expireTime.isValid() && now > item.expireTime) {
            QFile::remove(info.absoluteFilePath());
            expiredCount++;
        }
    }
    
    if (expiredCount > 0) {
        LOG_DEBUG(QString("Cleaned up %1 expired cache items").arg(expiredCount));
    }
}

void CacheManager::evictIfNeeded(CacheLevel level)
{
    if (level == CacheLevel::L1_Memory) {
        while (m_l1CurrentSize > m_l1MaxSize * 0.9 && !m_l1Cache.isEmpty()) {
            // 根据策略选择淘汰项
            QString keyToEvict;
            
            switch (m_policy) {
                case CachePolicy::LRU: {
                    // 找到最久未使用的
                    QDateTime oldest = QDateTime::currentDateTime();
                    for (auto it = m_l1Cache.begin(); it != m_l1Cache.end(); ++it) {
                        if (it.value().createTime < oldest) {
                            oldest = it.value().createTime;
                            keyToEvict = it.key();
                        }
                    }
                    break;
                }
                case CachePolicy::LFU: {
                    // 找到最少使用的
                    int minHits = INT_MAX;
                    for (auto it = m_l1Cache.begin(); it != m_l1Cache.end(); ++it) {
                        if (it.value().hitCount < minHits) {
                            minHits = it.value().hitCount;
                            keyToEvict = it.key();
                        }
                    }
                    break;
                }
                default:
                    // 默认删除第一个
                    keyToEvict = m_l1Cache.begin().key();
                    break;
            }
            
            if (!keyToEvict.isEmpty()) {
                m_l1CurrentSize -= m_l1Cache[keyToEvict].size;
                m_l1Cache.remove(keyToEvict);
            }
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
    stream << item.value << item.createTime << item.expireTime << item.hitCount << item.size;
    file.close();
    
    m_l2CurrentSize += item.size;
    
    return true;
}

CacheItem CacheManager::readFromDisk(const QString& key) const
{
    QString filePath = m_l2CachePath + "/" + key + ".cache";
    QFile file(filePath);
    
    CacheItem item;
    if (!file.open(QIODevice::ReadOnly)) {
        return item;
    }
    
    QDataStream stream(&file);
    stream >> item.value >> item.createTime >> item.expireTime >> item.hitCount >> item.size;
    file.close();
    
    item.level = CacheLevel::L2_Disk;
    
    return item;
}

void CacheManager::updateStats(bool hit)
{
    qint64 total = m_stats.totalHits + m_stats.totalMisses;
    m_stats.hitRate = total > 0 ? (double)m_stats.totalHits / total : 0.0;
}

void CacheManager::scheduleCleanup()
{
    QTimer::singleShot(m_cleanupInterval * 1000, this, [this]() {
        cleanupExpired();
        scheduleCleanup();
    });
}
