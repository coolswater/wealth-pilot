/**
 * @file PerformanceConfig.h
 * @brief 性能优化配置
 *
 * @details 定义性能相关的配置参数
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef WEALTHPILOT_PERFORMANCECONFIG_H
#define WEALTHPILOT_PERFORMANCECONFIG_H

#include <QtGlobal>

namespace WealthPilot {
namespace Performance {

// ============================================================================
// 缓存配置
// ============================================================================
namespace Cache {
    // 内存缓存大小 (字节)
    constexpr qint64 DefaultMemorySize = 10 * 1024 * 1024;     // 10MB
    constexpr qint64 MaxMemorySize = 100 * 1024 * 1024;        // 100MB

    // 磁盘缓存大小 (字节)
    constexpr qint64 DefaultDiskSize = 100 * 1024 * 1024;      // 100MB
    constexpr qint64 MaxDiskSize = 1024 * 1024 * 1024;         // 1GB

    // 默认过期时间 (毫秒)
    constexpr int DefaultTTL = 60000;          // 1分钟
    constexpr int QuoteTTL = 5000;             // 行情5秒
    constexpr int KLineTTL = 60000;            // K线1分钟
    constexpr int NewsTTL = 300000;            // 新闻5分钟
}

// ============================================================================
// 网络配置
// ============================================================================
namespace Network {
    // 请求超时 (毫秒)
    constexpr int DefaultTimeout = 10000;      // 10秒
    constexpr int LongTimeout = 30000;         // 30秒
    constexpr int ShortTimeout = 5000;         // 5秒

    // 重试配置
    constexpr int MaxRetries = 3;
    constexpr int RetryDelay = 1000;           // 1秒

    // 连接池
    constexpr int MaxConnections = 10;
    constexpr int ConnectionTimeout = 5000;
}

// ============================================================================
// 数据刷新
// ============================================================================
namespace Refresh {
    // 刷新间隔 (毫秒)
    constexpr int QuoteInterval = 3000;        // 行情3秒
    constexpr int KLineInterval = 60000;       // K线1分钟
    constexpr int NewsInterval = 60000;        // 新闻1分钟
    constexpr int AccountInterval = 30000;     // 账户30秒
    constexpr int PositionInterval = 10000;    // 持仓10秒
}

// ============================================================================
// 批量处理
// ============================================================================
namespace Batch {
    // 批量订阅数量
    constexpr int SubscribeBatchSize = 50;

    // 批量请求延迟 (毫秒)
    constexpr int BatchDelay = 100;

    // 最大批量大小
    constexpr int MaxBatchSize = 200;
}

// ============================================================================
// 线程池
// ============================================================================
namespace ThreadPool {
    // 核心线程数
    constexpr int CoreThreads = 4;

    // 最大线程数
    constexpr int MaxThreads = 8;

    // 空闲线程存活时间 (秒)
    constexpr int IdleTimeout = 60;

    // 任务队列大小
    constexpr int QueueSize = 100;
}

// ============================================================================
// 内存管理
// ============================================================================
namespace Memory {
    // 对象池大小
    constexpr int ObjectPoolSize = 100;

    // 内存警告阈值 (MB)
    constexpr int WarningThreshold = 500;

    // 内存清理间隔 (毫秒)
    constexpr int CleanupInterval = 60000;
}

// ============================================================================
// 渲染优化
// ============================================================================
namespace Render {
    // FPS 限制
    constexpr int MaxFPS = 60;
    constexpr int MinFPS = 30;

    // 图表数据点限制
    constexpr int MaxDataPoints = 1000;
    constexpr int DefaultDataPoints = 500;

    // 渲染延迟 (毫秒)
    constexpr int RenderDelay = 16;            // ~60fps
}

} // namespace Performance
} // namespace WealthPilot

#endif // WEALTHPILOT_PERFORMANCECONFIG_H