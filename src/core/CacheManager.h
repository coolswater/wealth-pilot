/**
 * @file CacheManager.h
 * @brief 缓存管理器 - 多级高性能缓存系统
 *
 * @details 功能：
 * - 三级缓存：L1（内存）-> L2（磁盘）-> L3（数据库）
 * - 缓存策略：LRU、LFU、TTL
 * - 自动过期清理
 * - 线程安全
 * - 性能优化：预分配内存、批量操作
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include "Singleton.h"
#include <QObject>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QDateTime>
#include <QVariant>
#include <QSettings>
#include <memory>

/**
 * @brief 缓存级别
 */
enum class CacheLevel {
    L1_Memory,      // 内存缓存（最快）
    L2_Disk,        // 磁盘缓存（较快）
    L3_Database     // 数据库缓存（较慢）
};

/**
 * @brief 缓存策略
 */
enum class CachePolicy {
    LRU,            // 最近最少使用
    LFU,            // 最不经常使用
    TTL,            // 时间过期
    FIFO           // 先进先出
};

/**
 * @brief 缓存项
 */
struct CacheItem {
    QVariant value;             // 缓存值
    QDateTime createTime;       // 创建时间
    QDateTime expireTime;       // 过期时间
    int hitCount;               // 命中次数
    int size;                   // 数据大小（字节）
    CacheLevel level;           // 缓存级别
};

/**
 * @brief 缓存统计
 */
struct CacheStats {
    qint64 totalHits;           // 总命中次数
    qint64 totalMisses;         // 总未命中次数
    qint64 totalSets;           // 总设置次数
    qint64 totalDeletes;        // 总删除次数
    qint64 memoryUsage;         // 内存使用量（字节）
    qint64 diskUsage;           // 磁盘使用量（字节）
    int itemCount;              // 缓存项数量
    double hitRate;             // 命中率
};

/**
 * @brief 缓存管理器 - 高性能多级缓存系统
 */
class CacheManager : public QObject, public Singleton<CacheManager>
{
    Q_OBJECT
    friend class Singleton<CacheManager>;

public:
    /**
     * @brief 初始化缓存管理器
     */
    bool initialize(qint64 maxMemorySize = 100 * 1024 * 1024,  // 100MB
                   qint64 maxDiskSize = 1024 * 1024 * 1024);   // 1GB

    /**
     * @brief 设置缓存
     */
    void set(const QString& key, 
            const QVariant& value,
            int ttlSeconds = 300,
            CacheLevel level = CacheLevel::L1_Memory);

    /**
     * @brief 获取缓存
     */
    QVariant get(const QString& key, const QVariant& defaultValue = QVariant());

    /**
     * @brief 检查缓存是否存在
     */
    bool contains(const QString& key) const;

    /**
     * @brief 删除缓存
     */
    void remove(const QString& key);

    /**
     * @brief 清空缓存
     */
    void clear(CacheLevel level = CacheLevel::L1_Memory);

    /**
     * @brief 清空所有缓存
     */
    void clearAll();

    /**
     * @brief 获取缓存统计
     */
    CacheStats statistics() const;

    /**
     * @brief 设置缓存策略
     */
    void setPolicy(CachePolicy policy);

    /**
     * @brief 预热缓存（批量设置）
     */
    void warmup(const QMap<QString, QVariant>& data);

    /**
     * @brief 批量获取
     */
    QMap<QString, QVariant> getBatch(const QStringList& keys);

    /**
     * @brief 批量设置
     */
    void setBatch(const QMap<QString, QVariant>& data, int ttlSeconds = 300);

signals:
    /**
     * @brief 缓存命中信号
     */
    void cacheHit(const QString& key);

    /**
     * @brief 缓存未命中信号
     */
    void cacheMiss(const QString& key);

    /**
     * @brief 缓存过期信号
     */
    void cacheExpired(const QString& key);

private:
    CacheManager();
    ~CacheManager();

    // L1内存缓存
    QHash<QString, CacheItem> m_l1Cache;
    qint64 m_l1MaxSize;
    qint64 m_l1CurrentSize;
    
    // L2磁盘缓存
    QString m_l2CachePath;
    qint64 m_l2MaxSize;
    qint64 m_l2CurrentSize;
    
    // 统计信息
    mutable CacheStats m_stats;
    
    // 配置
    CachePolicy m_policy;
    mutable QMutex m_mutex;
    
    // 定时清理
    int m_cleanupInterval;
    
    // 内部方法
    void cleanupExpired();
    void evictIfNeeded(CacheLevel level);
    QString serialize(const QVariant& value) const;
    QVariant deserialize(const QString& data) const;
    bool writeToDisk(const QString& key, const CacheItem& item);
    CacheItem readFromDisk(const QString& key) const;
    void updateStats(bool hit);
    void scheduleCleanup();
};

#endif // CACHEMANAGER_H
