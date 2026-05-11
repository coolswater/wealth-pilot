/**
 * @file PerformanceManager.h
 * @brief 性能管理器 - 统一管理应用性能优化
 *
 * @details 功能：
 * - 性能监控和统计
 * - 内存池管理
 * - 对象复用
 * - 延迟加载
 * - 性能分析报告
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PERFORMANCEMANAGER_H
#define PERFORMANCEMANAGER_H

#include <QObject>
#include <QElapsedTimer>
#include <QHash>
#include <QMutex>
#include <QVariant>
#include <QTimer>
#include <memory>

namespace WealthPilot {

/**
 * @brief 性能统计项
 */
struct PerformanceMetric {
    QString name;               ///< 指标名称
    qint64 totalTime = 0;       ///< 总耗时（毫秒）
    qint64 callCount = 0;       ///< 调用次数
    qint64 maxTime = 0;         ///< 最大耗时
    qint64 minTime = LLONG_MAX; ///< 最小耗时
    double avgTime = 0.0;       ///< 平均耗时

    void update(qint64 elapsed) {
        totalTime += elapsed;
        callCount++;
        maxTime = qMax(maxTime, elapsed);
        minTime = qMin(minTime, elapsed);
        avgTime = static_cast<double>(totalTime) / callCount;
    }
};

/**
 * @brief 内存池配置
 */
struct MemoryPoolConfig {
    size_t blockSize = 4096;        ///< 块大小
    size_t maxBlocks = 1000;        ///< 最大块数
    bool enableReuse = true;        ///< 启用复用
};

/**
 * @brief 性能管理器
 *
 * 提供统一的性能监控和优化功能：
 * - 性能计时器
 * - 内存池
 * - 对象缓存
 * - 性能报告
 */
class PerformanceManager : public QObject {
    Q_OBJECT

public:
    static PerformanceManager* instance();

    // ========== 性能监控 ==========

    /**
     * @brief 开始性能计时
     * @param name 计时器名称
     */
    void beginTimer(const QString& name);

    /**
     * @brief 结束性能计时
     * @param name 计时器名称
     * @return 耗时（毫秒）
     */
    qint64 endTimer(const QString& name);

    /**
     * @brief 获取性能统计
     * @param name 指标名称
     * @return 性能统计项
     */
    PerformanceMetric getMetric(const QString& name) const;

    /**
     * @brief 获取所有性能统计
     */
    QHash<QString, PerformanceMetric> getAllMetrics() const;

    /**
     * @brief 重置性能统计
     */
    void resetMetrics();

    /**
     * @brief 生成性能报告
     * @return 性能报告文本
     */
    QString generateReport() const;

    // ========== 内存优化 ==========

    /**
     * @brief 配置内存池
     */
    void configureMemoryPool(const MemoryPoolConfig& config);

    /**
     * @brief 分配内存块
     */
    void* allocateBlock();

    /**
     * @brief 释放内存块
     */
    void deallocateBlock(void* block);

    /**
     * @brief 获取内存使用统计
     */
    struct MemoryStats {
        size_t totalAllocated = 0;  ///< 总分配
        size_t totalFreed = 0;      ///< 总释放
        size_t currentUsage = 0;    ///< 当前使用
        size_t peakUsage = 0;       ///< 峰值使用
        size_t poolSize = 0;        ///< 池大小
    };
    MemoryStats getMemoryStats() const;

    // ========== 延迟加载 ==========

    /**
     * @brief 注册延迟加载任务
     * @param id 任务ID
     * @param loader 加载函数
     * @param priority 优先级（越大越先加载）
     */
    void registerLazyLoader(const QString& id,
                           std::function<void()> loader,
                           int priority = 0);

    /**
     * @brief 执行延迟加载
     */
    void executeLazyLoaders();

    // ========== 对象缓存 ==========

    /**
     * @brief 缓存对象
     */
    template<typename T>
    void cacheObject(const QString& key, std::shared_ptr<T> obj) {
        QMutexLocker locker(&m_mutex);
        m_objectCache[key] = std::static_pointer_cast<void>(obj);
    }

    /**
     * @brief 获取缓存对象
     */
    template<typename T>
    std::shared_ptr<T> getCachedObject(const QString& key) {
        QMutexLocker locker(&m_mutex);
        auto it = m_objectCache.find(key);
        if (it != m_objectCache.end()) {
            return std::static_pointer_cast<T>(it.value());
        }
        return nullptr;
    }

    // ========== 性能优化建议 ==========

    /**
     * @brief 分析性能瓶颈
     * @return 优化建议列表
     */
    QStringList analyzeBottlenecks() const;

signals:
    /**
     * @brief 性能警告信号
     */
    void performanceWarning(const QString& message);

    /**
     * @brief 内存警告信号
     */
    void memoryWarning(size_t currentUsage);

private:
    explicit PerformanceManager(QObject* parent = nullptr);
    ~PerformanceManager() override;

    // 性能计时器
    QHash<QString, QElapsedTimer> m_timers;
    QHash<QString, PerformanceMetric> m_metrics;
    mutable QMutex m_metricsMutex;

    // 内存池
    MemoryPoolConfig m_poolConfig;
    QVector<void*> m_memoryPool;
    MemoryStats m_memoryStats;
    mutable QMutex m_poolMutex;

    // 对象缓存
    QHash<QString, std::shared_ptr<void>> m_objectCache;
    mutable QMutex m_mutex;

    // 延迟加载
    struct LazyLoader {
        QString id;
        std::function<void()> loader;
        int priority;
    };
    QVector<LazyLoader> m_lazyLoaders;

    // 定期检查
    QTimer* m_monitorTimer = nullptr;

    void checkPerformance();
    void checkMemory();
};

// ========== 性能计时辅助类 ==========

/**
 * @brief 自动性能计时器
 *
 * 使用 RAII 自动计时：
 * {
 *     PerformanceTimer timer("operation_name");
 *     // ... 执行操作
 * } // 自动结束计时
 */
class PerformanceTimer {
public:
    explicit PerformanceTimer(const QString& name)
        : m_name(name) {
        PerformanceManager::instance()->beginTimer(name);
    }

    ~PerformanceTimer() {
        PerformanceManager::instance()->endTimer(m_name);
    }

    // 禁止拷贝
    PerformanceTimer(const PerformanceTimer&) = delete;
    PerformanceTimer& operator=(const PerformanceTimer&) = delete;

private:
    QString m_name;
};

// ========== 便捷宏 ==========

/**
 * @brief 性能计时宏
 */
#define PERF_TIMER(name) PerformanceTimer _perf_timer_##name(#name)

/**
 * @brief 性能计时开始
 */
#define PERF_BEGIN(name) PerformanceManager::instance()->beginTimer(#name)

/**
 * @brief 性能计时结束
 */
#define PERF_END(name) PerformanceManager::instance()->endTimer(#name)

} // namespace WealthPilot

#endif // PERFORMANCEMANAGER_H
