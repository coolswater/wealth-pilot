/**
 * @file MarketDataService.h
 * @brief 市场数据应用服务 - 协调 DataHub 和数据源
 *
 * @details 职责：
 * - 统一市场数据获取接口
 * - 协调 DataHub 发布订阅
 * - 管理数据源生命周期
 * - 提供数据缓存和预加载
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef MARKET_DATA_SERVICE_H
#define MARKET_DATA_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <functional>
#include "shared/base/Singleton.h"

namespace WealthPilot {
namespace Application {

/**
 * @brief 市场数据应用服务
 */
class MarketDataService : public QObject, public Singleton<MarketDataService>
{
    Q_OBJECT
    friend class Singleton<MarketDataService>;

public:
    /**
     * @brief 获取实时行情
     * @param symbol 证券代码
     * @param callback 回调函数
     */
    void getQuote(const QString& symbol, std::function<void(const QVariant&)> callback);

    /**
     * @brief 获取 K 线数据
     * @param symbol 证券代码
     * @param period 周期 (1/5/15/30/60/day/week/month)
     * @param count 数量
     * @param callback 回调函数
     */
    void getKLine(const QString& symbol, int period, int count,
                  std::function<void(const QVariant&)> callback);

    /**
     * @brief 订阅实时行情
     * @param symbol 证券代码
     * @param callback 回调函数
     * @return 订阅 ID
     */
    QString subscribeQuote(const QString& symbol, std::function<void(const QVariant&)> callback);

    /**
     * @brief 取消订阅
     * @param subscriptionId 订阅 ID
     */
    void unsubscribe(const QString& subscriptionId);

    /**
     * @brief 预加载数据
     * @param symbols 证券代码列表
     */
    void preloadData(const QStringList& symbols);

    /**
     * @brief 刷新数据
     */
    void refresh();

signals:
    /**
     * @brief 行情更新信号
     */
    void quoteUpdated(const QString& symbol, const QVariant& quote);

    /**
     * @brief 数据错误信号
     */
    void errorOccurred(const QString& symbol, const QString& error);

private:
    MarketDataService();
    ~MarketDataService();

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Application
} // namespace WealthPilot

#endif // MARKET_DATA_SERVICE_H
