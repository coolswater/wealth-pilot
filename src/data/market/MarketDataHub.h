/**
 * @file MarketDataHub.h
 * @brief 行情数据中心 - 统一行情分发
 *
 * @details 功能：
 * - 整合 CTP 行情和网络行情
 * - 订阅/发布模式
 * - 行情数据缓存
 * - 高频数据优化
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef MARKETDATAHUB_H
#define MARKETDATAHUB_H

#include <QObject>
#include <QHash>
#include <QSet>
#include <QMutex>
#include <QTimer>
#include <memory>
#include "shared/types/MarketTypes.h"

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::MarketSnapshot;
using WealthPilot::KLinePeriod;

// 前向声明
namespace CTP {
    struct MarketData;
}

/**
 * @brief 行情订阅者接口
 */
class IMarketDataSubscriber {
public:
    virtual ~IMarketDataSubscriber() = default;
    virtual void onMarketData(const QString& instrumentId, const MarketSnapshot& data) = 0;
};

/**
 * @brief 行情数据中心
 * @details 单例模式，统一管理所有行情数据
 */
class MarketDataHub : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static MarketDataHub& instance();

    /**
     * @brief 初始化
     */
    bool initialize();

    /**
     * @brief 关闭
     */
    void shutdown();

    // ========== 订阅管理 ==========

    /**
     * @brief 订阅行情
     * @param instrumentId 合约代码
     * @param subscriber 订阅者
     */
    void subscribe(const QString& instrumentId, IMarketDataSubscriber* subscriber);

    /**
     * @brief 取消订阅
     * @param instrumentId 合约代码
     * @param subscriber 订阅者
     */
    void unsubscribe(const QString& instrumentId, IMarketDataSubscriber* subscriber);

    /**
     * @brief 批量订阅
     * @param instrumentIds 合约代码列表
     */
    void subscribeBatch(const QStringList& instrumentIds);

    // ========== 数据获取 ==========

    /**
     * @brief 获取最新行情
     * @param instrumentId 合约代码
     * @return 行情数据，不存在返回空
     */
    std::optional<MarketSnapshot> getQuote(const QString& instrumentId) const;

    /**
     * @brief 获取所有行情
     * @return 行情列表
     */
    QVector<MarketSnapshot> getAllQuotes() const;

    /**
     * @brief 获取行情快照（线程安全）
     */
    MarketSnapshot getSnapshot(const QString& instrumentId) const;

    // ========== 数据更新 ==========

    /**
     * @brief 更新CTP行情数据
     * @param data CTP行情数据
     */
    void updateCtpMarketData(const CTP::MarketData& data);

    /**
     * @brief 批量更新行情（高频优化）
     * @param dataList 行情列表
     */
    void updateMarketDataBatch(const QVector<MarketSnapshot>& dataList);

    /**
     * @brief 更新单个行情
     * @param instrumentId 合约代码
     * @param data 行情数据
     */
    void updateMarketData(const QString& instrumentId, const MarketSnapshot& data);

signals:
    /**
     * @brief 行情更新信号
     * @param instrumentId 合约代码
     * @param data 行情数据
     */
    void marketDataUpdated(const QString& instrumentId, const MarketSnapshot& data);

    /**
     * @brief 批量行情更新信号（高频优化）
     * @param dataList 行情列表
     */
    void marketDataBatchUpdated(const QVector<MarketSnapshot>& dataList);

    /**
     * @brief 订阅成功信号
     * @param instrumentId 合约代码
     */
    void subscribed(const QString& instrumentId);

    /**
     * @brief 订阅失败信号
     * @param instrumentId 合约代码
     * @param reason 失败原因
     */
    void subscribeFailed(const QString& instrumentId, const QString& reason);

private:
    MarketDataHub(QObject* parent = nullptr);
    ~MarketDataHub() override;
    Q_DISABLE_COPY(MarketDataHub)

    // 内部方法
    void notifySubscribers(const QString& instrumentId, const MarketSnapshot& data);
    void checkAndEmitSignals();

    // PIMPL
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // MARKETDATAHUB_H
