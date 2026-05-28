/**
 * @file DataCacheManager.h
 * @brief 数据缓存管理器 - 统一管理各类数据缓存
 *
 * @details 功能：
 * - 内存缓存：快速访问热点数据
 * - 磁盘缓存：持久化存储历史数据
 * - 缓存过期：自动清理过期数据
 * - 缓存统计：监控缓存命中率
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATACACHEMANAGER_H
#define DATACACHEMANAGER_H

#include <QObject>
#include <QHash>
#include <QDateTime>
#include <QMutex>
#include <QTimer>
#include <memory>

/**
 * @brief 缓存项
 */
template<typename T>
struct DataCacheItem {
    T data;
    QDateTime expireTime;
    QDateTime createTime;
    int hitCount = 0;
};

/**
 * @brief 缓存统计
 */
struct DataCacheStats {
    qint64 totalHits = 0;       ///< 总命中次数
    qint64 totalMisses = 0;     ///< 总未命中次数
    qint64 totalSets = 0;       ///< 总设置次数
    qint64 totalEvictions = 0;  ///< 总淘汰次数
    
    double hitRate() const {
        qint64 total = totalHits + totalMisses;
        return total > 0 ? static_cast<double>(totalHits) / total : 0.0;
    }
};

/**
 * @brief 数据缓存管理器
 */
class DataCacheManager : public QObject
{
    Q_OBJECT

public:
    static DataCacheManager* instance();

    /**
     * @brief 设置缓存
     * @param key 缓存键
     * @param data 缓存数据
     * @param ttlMs 过期时间（毫秒），0表示永不过期
     */
    template<typename T>
    void set(const QString& key, const T& data, qint64 ttlMs = 60000);

    /**
     * @brief 获取缓存
     * @param key 缓存键
     * @param outData 输出数据
     * @return 是否命中
     */
    template<typename T>
    bool get(const QString& key, T& outData);

    /**
     * @brief 检查缓存是否存在
     */
    bool contains(const QString& key) const;

    /**
     * @brief 移除缓存
     */
    void remove(const QString& key);

    /**
     * @brief 清空所有缓存
     */
    void clear();

    /**
     * @brief 获取缓存统计
     */
    DataCacheStats stats() const;

    /**
     * @brief 设置最大缓存大小
     */
    void setMaxSize(int maxSize);

    /**
     * @brief 启用/禁用缓存
     */
    void setEnabled(bool enabled);

    /**
     * @brief 是否启用缓存
     */
    bool isEnabled() const;

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
     * @brief 缓存淘汰信号
     */
    void cacheEvicted(const QString& key);

private:
    DataCacheManager(QObject* parent = nullptr);
    ~DataCacheManager() override;

    // 禁止拷贝
    DataCacheManager(const DataCacheManager&) = delete;
    DataCacheManager& operator=(const DataCacheManager&) = delete;

    void cleanupExpired();
    void evictIfNeeded();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // DATACACHEMANAGER_H
