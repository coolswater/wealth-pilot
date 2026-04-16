/**
 * @file CacheManager.h
 * @brief 缓存管理器 - 多级高性能缓存系统
 * @author WealthPilot Team
 * @version 2.0.0
 * 
 * @details 主要功能：
 * - 三级缓存：L1（内存）-> L2（磁盘）-> L3（数据库）
 * - 缓存策略：LRU、LFU、TTL
 * - 自动过期清理
 * - 线程安全
 * - 性能优化：预分配内存、批量操作
 * 
 * @thread_safe 所有公共方法都是线程安全的
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
 * @brief 缓存级别
 */
enum class CacheLevel {
    L1_Memory,      ///< 内存缓存（最快）
    L2_Disk,        ///< 磁盘缓存（较快）
    L3_Database     ///< 数据库缓存（较慢）
};

/**
 * @brief 缓存策略
 */
enum class CachePolicy {
    LRU,            ///< 最近最少使用
    LFU,            ///< 最不经常使用
    TTL,            ///< 生存时间
    FIFO            ///< 先进先出
};

/**
 * @brief 缓存项
 */
struct CacheItem {
    QVariant value;             ///< 缓存值
    QDateTime createTime;       ///< 创建时间
    QDateTime expireTime;       ///< 过期时间
    int hitCount = 0;           ///< 命中次数
    int size = 0;               ///< 数据大小（字节）
    CacheLevel level = CacheLevel::L1_Memory;  ///< 缓存级别
};

/**
 * @brief 缓存统计信息
 */
struct CacheStats {
    qint64 totalHits = 0;       ///< 总命中次数
    qint64 totalMisses = 0;     ///< 总未命中次数
    qint64 totalSets = 0;       ///< 总设置次数
    qint64 totalDeletes = 0;    ///< 总删除次数
    qint64 memoryUsage = 0;     ///< 内存使用量（字节）
    qint64 diskUsage = 0;       ///< 磁盘使用量（字节）
    int itemCount = 0;          ///< 缓存项数量
    double hitRate = 0.0;       ///< 命中率
};

/**
 * @brief 缓存管理器 - 高性能多级缓存系统
 * @thread_safe 所有公共方法都是线程安全的
 */
class CacheManager : public QObject, public Singleton<CacheManager>
{
    Q_OBJECT
    friend class Singleton<CacheManager>;

public:
    /**
     * @brief 初始化缓存管理器
     * @param maxMemorySize 最大内存大小（默认100MB）
     * @param maxDiskSize 最大磁盘大小（默认1GB）
     */
    bool initialize(qint64 maxMemorySize = 100 * 1024 * 1024,
                   qint64 maxDiskSize = 1024 * 1024 * 1024);

    /**
     * @brief 设置缓存
     * @param key 缓存键
     * @param value 缓存值
     * @param ttlSeconds 生存时间（秒）
     * @param level 缓存级别
     */
    void set(const QString& key, 
            const QVariant& value,
            int ttlSeconds = 300,
            CacheLevel level = CacheLevel::L1_Memory);

    /**
     * @brief 获取缓存
     * @param key 缓存键
     * @param defaultValue 默认值
     * @return 缓存值
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
     * @param level 缓存级别
     */
    void clear(CacheLevel level = CacheLevel::L1_Memory);

    /**
     * @brief 清空所有缓存
     */
    void clearAll();

    /**
     * @brief 获取缓存统计信息
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

    // L1 内存缓存
    QHash<QString, CacheItem> m_l1Cache;   ///< 内存缓存
    qint64 m_l1MaxSize;                    ///< 最大内存大小
    qint64 m_l1CurrentSize;                ///< 当前内存大小
    
    // L2 磁盘缓存
    QString m_l2CachePath;                 ///< 磁盘缓存路径
    qint64 m_l2MaxSize;                    ///< 最大磁盘大小
    qint64 m_l2CurrentSize;                ///< 当前磁盘大小
    
    // 统计信息
    mutable CacheStats m_stats;            ///< 缓存统计
    
    // 配置
    CachePolicy m_policy;                  ///< 缓存策略
    mutable QMutex m_mutex;                ///< 线程安全锁
    
    // 清理间隔
    int m_cleanupInterval;                 ///< 清理间隔
    
    // 内部方法
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