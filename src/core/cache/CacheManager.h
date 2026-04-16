/**
 * @file CacheManager.h
 * @brief Cache Manager - Multi-level high-performance caching system
 * @author WealthPilot Team
 * @version 2.0.0
 * 
 * @details Features:
 * - Three-level cache: L1 (Memory) -> L2 (Disk) -> L3 (Database)
 * - Cache policies: LRU, LFU, TTL
 * - Automatic expiration cleanup
 * - Thread safe
 * - Performance optimization: pre-allocated memory, batch operations
 * 
 * @thread_safe All public methods are thread-safe
 */
#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include "../base/Singleton.h"
#include <QObject>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QDateTime>
#include <QVariant>
#include <QSettings>
#include <memory>

/**
 * @brief Cache level
 */
enum class CacheLevel {
    L1_Memory,      // Memory cache (fastest)
    L2_Disk,        // Disk cache (fast)
    L3_Database     // Database cache (slower)
};

/**
 * @brief Cache policy
 */
enum class CachePolicy {
    LRU,            // Least Recently Used
    LFU,            // Least Frequently Used
    TTL,            // Time To Live
    FIFO            // First In First Out
};

/**
 * @brief Cache item
 */
struct CacheItem {
    QVariant value;             // Cached value
    QDateTime createTime;       // Creation time
    QDateTime expireTime;       // Expiration time
    int hitCount = 0;           // Hit count
    int size = 0;               // Data size (bytes)
    CacheLevel level = CacheLevel::L1_Memory;  // Cache level
};

/**
 * @brief Cache statistics
 */
struct CacheStats {
    qint64 totalHits = 0;       // Total hits
    qint64 totalMisses = 0;     // Total misses
    qint64 totalSets = 0;       // Total sets
    qint64 totalDeletes = 0;    // Total deletes
    qint64 memoryUsage = 0;     // Memory usage (bytes)
    qint64 diskUsage = 0;       // Disk usage (bytes)
    int itemCount = 0;          // Item count
    double hitRate = 0.0;       // Hit rate
};

/**
 * @brief Cache Manager - High-performance multi-level caching system
 * @thread_safe All public methods are thread-safe
 */
class CacheManager : public QObject, public Singleton<CacheManager>
{
    Q_OBJECT
    friend class Singleton<CacheManager>;

public:
    /**
     * @brief Initialize cache manager
     */
    bool initialize(qint64 maxMemorySize = 100 * 1024 * 1024,  // 100MB
                   qint64 maxDiskSize = 1024 * 1024 * 1024);   // 1GB

    /**
     * @brief Set cache
     */
    void set(const QString& key, 
            const QVariant& value,
            int ttlSeconds = 300,
            CacheLevel level = CacheLevel::L1_Memory);

    /**
     * @brief Get cache
     */
    QVariant get(const QString& key, const QVariant& defaultValue = QVariant());

    /**
     * @brief Check if cache exists
     */
    bool contains(const QString& key) const;

    /**
     * @brief Remove cache
     */
    void remove(const QString& key);

    /**
     * @brief Clear cache
     */
    void clear(CacheLevel level = CacheLevel::L1_Memory);

    /**
     * @brief Clear all caches
     */
    void clearAll();

    /**
     * @brief Get cache statistics
     */
    CacheStats statistics() const;

    /**
     * @brief Set cache policy
     */
    void setPolicy(CachePolicy policy);

    /**
     * @brief Warmup cache (batch set)
     */
    void warmup(const QMap<QString, QVariant>& data);

    /**
     * @brief Batch get
     */
    QMap<QString, QVariant> getBatch(const QStringList& keys);

    /**
     * @brief Batch set
     */
    void setBatch(const QMap<QString, QVariant>& data, int ttlSeconds = 300);

signals:
    /**
     * @brief Cache hit signal
     */
    void cacheHit(const QString& key);

    /**
     * @brief Cache miss signal
     */
    void cacheMiss(const QString& key);

    /**
     * @brief Cache expired signal
     */
    void cacheExpired(const QString& key);

private:
    CacheManager();
    ~CacheManager();

    // L1 memory cache
    QHash<QString, CacheItem> m_l1Cache;
    qint64 m_l1MaxSize;
    qint64 m_l1CurrentSize;
    
    // L2 disk cache
    QString m_l2CachePath;
    qint64 m_l2MaxSize;
    qint64 m_l2CurrentSize;
    
    // Statistics
    mutable CacheStats m_stats;
    
    // Configuration
    CachePolicy m_policy;
    mutable QMutex m_mutex;
    
    // Cleanup interval
    int m_cleanupInterval;
    
    // Internal methods
    void cleanupExpired();
    void evictIfNeeded(CacheLevel level);
    QString serialize(const QVariant& value) const;
    QVariant deserialize(const QString& data) const;
    bool writeToDisk(const QString& key, const CacheItem& item);
    CacheItem readFromDisk(const QString& key) const;
    void updateStats(bool hit);
    void scheduleCleanup();
    int estimateSize(const QVariant& value) const;
};

#endif // CACHEMANAGER_H
