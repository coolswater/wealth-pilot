/**
 * @file RealtimeMarketDataSource.h
 * @brief 实时行情数据源 - 为技术分析提供实时数据
 *
 * @details 功能：
 * - 接入CTP实时行情
 * - 数据缓存管理
 * - K线数据生成
 * - 信号触发机制
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef REALTIME_MARKET_DATA_SOURCE_H
#define REALTIME_MARKET_DATA_SOURCE_H

#include <QObject>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include "shared/types/MarketTypes.h"
#include "analysis/AnalysisTypes.h"

namespace WealthPilot {
namespace Market {

/**
 * @brief K线缓存配置
 */
struct KLineCacheConfig {
    int maxBars = 1000;              // 最大缓存K线数
    int updateIntervalMs = 1000;     // 更新间隔（毫秒）
    bool enableAutoUpdate = true;    // 是否自动更新
};

/**
 * @brief K线缓存
 */
struct KLineCache {
    QString symbol;                          // 标的代码
    KLinePeriod period;                      // K线周期
    QVector<KLineData> klines;               // K线数据
    QDateTime lastUpdateTime;                // 最后更新时间
    bool isComplete = false;                 // 是否完整

    // 当前K线（未完成）
    KLineData currentKLine;
    bool hasCurrentKLine = false;
};

/**
 * @brief 实时行情数据源
 */
class RealtimeMarketDataSource : public QObject
{
    Q_OBJECT

public:
    explicit RealtimeMarketDataSource(QObject* parent = nullptr);
    ~RealtimeMarketDataSource() override;

    // ========== 初始化 ==========

    /**
     * @brief 初始化数据源
     */
    void initialize();

    /**
     * @brief 设置缓存配置
     */
    void setCacheConfig(const KLineCacheConfig& config);

    // ========== 订阅管理 ==========

    /**
     * @brief 订阅行情
     */
    void subscribe(const QString& symbol, KLinePeriod period = KLinePeriod::Minute1);

    /**
     * @brief 取消订阅
     */
    void unsubscribe(const QString& symbol);

    /**
     * @brief 批量订阅
     */
    void subscribeBatch(const QVector<QString>& symbols, KLinePeriod period = KLinePeriod::Minute1);

    /**
     * @brief 获取订阅列表
     */
    QVector<QString> subscriptions() const;

    // ========== 数据获取 ==========

    /**
     * @brief 获取K线数据
     */
    QVector<KLineData> getKLines(const QString& symbol, KLinePeriod period = KLinePeriod::Minute1);

    /**
     * @brief 获取最新K线
     */
    KLineData getLatestKLine(const QString& symbol, KLinePeriod period = KLinePeriod::Minute1);

    /**
     * @brief 获取行情快照
     */
    MarketSnapshot getSnapshot(const QString& symbol);

    /**
     * @brief 获取缓存状态
     */
    KLineCache getCache(const QString& symbol, KLinePeriod period = KLinePeriod::Minute1);

    // ========== 数据更新 ==========

    /**
     * @brief 更新行情数据（由CTP回调调用）
     */
    void updateMarketData(const QString& symbol, const MarketSnapshot& snapshot);

    /**
     * @brief 更新K线数据
     */
    void updateKLineData(const QString& symbol, const QVector<KLineData>& klines, KLinePeriod period);

    /**
     * @brief 手动刷新
     */
    void refresh(const QString& symbol);

    // ========== 缓存管理 ==========

    /**
     * @brief 清空缓存
     */
    void clearCache(const QString& symbol);

    /**
     * @brief 清空所有缓存
     */
    void clearAllCache();

    /**
     * @brief 获取缓存大小
     */
    int cacheSize(const QString& symbol) const;

    /**
     * @brief 获取总缓存大小
     */
    int totalCacheSize() const;

signals:
    /**
     * @brief K线数据更新
     */
    void klineUpdated(const QString& symbol, KLinePeriod period, const QVector<KLineData>& klines);

    /**
     * @brief 行情快照更新
     */
    void snapshotUpdated(const QString& symbol, const MarketSnapshot& snapshot);

    /**
     * @brief 新K线生成
     */
    void newKLineGenerated(const QString& symbol, KLinePeriod period, const KLineData& kline);

    /**
     * @brief 订阅成功
     */
    void subscribed(const QString& symbol);

    /**
     * @brief 取消订阅
     */
    void unsubscribed(const QString& symbol);

    /**
     * @brief 错误发生
     */
    void errorOccurred(const QString& symbol, const QString& error);

private slots:
    /**
     * @brief 定时更新
     */
    void onTimerUpdate();

private:
    // ========== 内部方法 ==========

    /**
     * @brief 生成K线
     */
    void generateKLine(const QString& symbol, const MarketSnapshot& snapshot);

    /**
     * @brief 更新当前K线
     */
    void updateCurrentKLine(const QString& symbol, const MarketSnapshot& snapshot);

    /**
     * @brief 检查K线周期切换
     */
    void checkPeriodChange(const QString& symbol, const QDateTime& time);

    /**
     * @brief 缓存清理
     */
    void cleanupCache(const QString& symbol);

    /**
     * @brief 获取缓存键
     */
    QString getCacheKey(const QString& symbol, KLinePeriod period) const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Market
} // namespace WealthPilot

#endif // REALTIME_MARKET_DATA_SOURCE_H
