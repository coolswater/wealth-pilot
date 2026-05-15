/**
 * @file ForexDataSource.h
 * @brief 外汇数据源 - 接入第三方外汇汇率API
 *
 * @details 支持数据源：
 * - 中国银行外汇牌价（免费）
 * - 新浪外汇（免费）
 * - 东方财富外汇（免费）
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FOREXDATASOURCE_H
#define FOREXDATASOURCE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QHash>
#include <QVector>
#include <functional>
#include "core/types/MarketTypes.h"  // 使用统一的类型定义

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::ForexQuote;

/**
 * @brief 外汇数据源
 */
class ForexDataSource : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 数据源类型
     */
    enum class DataSource {
        Sina,       ///< 新浪外汇
        EastMoney,  ///< 东方财富
        BOC         ///< 中国银行
    };

    static ForexDataSource* instance();

    /**
     * @brief 设置数据源
     */
    void setDataSource(DataSource source);

    /**
     * @brief 请求货币对汇率
     * @param baseCurrency 基础货币（如 USD）
     * @param quoteCurrency 报价货币（如 CNY）
     * @param callback 回调函数
     */
    void requestQuote(const QString& baseCurrency, const QString& quoteCurrency,
                      std::function<void(const ForexQuote&)> callback);

    /**
     * @brief 批量请求多个货币对
     * @param pairs 货币对列表（如 ["USD/CNY", "EUR/USD"]）
     * @param callback 回调函数
     */
    void requestQuotes(const QStringList& pairs,
                       std::function<void(const QVector<ForexQuote>&)> callback);

    /**
     * @brief 获取缓存的汇率
     */
    ForexQuote cachedQuote(const QString& pair) const;

    /**
     * @brief 货币兑换
     * @param amount 金额
     * @param from 源货币
     * @param to 目标货币
     * @return 兑换结果
     */
    double convert(double amount, const QString& from, const QString& to);

    /**
     * @brief 启动自动刷新
     * @param intervalMs 刷新间隔（毫秒）
     */
    void startAutoRefresh(int intervalMs = 60000);

    /**
     * @brief 停止自动刷新
     */
    void stopAutoRefresh();

signals:
    /**
     * @brief 汇率更新信号
     */
    void quoteUpdated(const ForexQuote& quote);

    /**
     * @brief 批量更新信号
     */
    void quotesUpdated(const QVector<ForexQuote>& quotes);

    /**
     * @brief 错误信号
     */
    void error(const QString& errorMessage);

private:
    ForexDataSource(QObject* parent = nullptr);
    ~ForexDataSource() override;

    // 禁止拷贝
    ForexDataSource(const ForexDataSource&) = delete;
    ForexDataSource& operator=(const ForexDataSource&) = delete;

    // 数据解析
    ForexQuote parseSinaResponse(const QString& pair, const QByteArray& data);
    ForexQuote parseEastMoneyResponse(const QString& pair, const QByteArray& data);
    ForexQuote parseBOCResponse(const QString& pair, const QByteArray& data);

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FOREXDATASOURCE_H
