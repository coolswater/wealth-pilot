/**
 * @file CryptoDataSource.h
 * @brief 数字货币数据源 - 接入第三方加密货币行情API
 *
 * @details 支持数据源：
 * - CoinGecko（免费，无需API Key）
 * - Binance（需要API Key）
 * - OKX（需要API Key）
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef CRYPTODATASOURCE_H
#define CRYPTODATASOURCE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QHash>
#include <QVector>
#include <functional>
#include "core/types/MarketTypes.h"  // 使用统一的类型定义

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::CryptoQuote;

/**
 * @brief 数字货币数据源
 */
class CryptoDataSource : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 数据源类型
     */
    enum class DataSource {
        CoinGecko,  ///< CoinGecko（免费）
        Binance,    ///< 币安
        OKX         ///< 欧易
    };

    static CryptoDataSource* instance();

    /**
     * @brief 设置数据源
     */
    void setDataSource(DataSource source);

    /**
     * @brief 设置API Key（Binance/OKX需要）
     */
    void setApiKey(const QString& apiKey, const QString& apiSecret = QString());

    /**
     * @brief 请求单个币种行情
     * @param symbol 币种代码（如 BTC）
     * @param callback 回调函数
     */
    void requestQuote(const QString& symbol,
                      std::function<void(const CryptoQuote&)> callback);

    /**
     * @brief 批量请求多个币种行情
     * @param symbols 币种代码列表（如 ["BTC", "ETH"]）
     * @param callback 回调函数
     */
    void requestQuotes(const QStringList& symbols,
                       std::function<void(const QVector<CryptoQuote>&)> callback);

    /**
     * @brief 请求排行榜
     * @param limit 数量限制（默认100）
     * @param callback 回调函数
     */
    void requestTopList(int limit,
                        std::function<void(const QVector<CryptoQuote>&)> callback);

    /**
     * @brief 获取缓存的行情
     */
    CryptoQuote cachedQuote(const QString& symbol) const;

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
     * @brief 行情更新信号
     */
    void quoteUpdated(const CryptoQuote& quote);

    /**
     * @brief 批量更新信号
     */
    void quotesUpdated(const QVector<CryptoQuote>& quotes);

    /**
     * @brief 错误信号
     */
    void error(const QString& errorMessage);

private:
    CryptoDataSource(QObject* parent = nullptr);
    ~CryptoDataSource() override;

    // 禁止拷贝
    CryptoDataSource(const CryptoDataSource&) = delete;
    CryptoDataSource& operator=(const CryptoDataSource&) = delete;

    // 数据解析
    CryptoQuote parseCoinGeckoResponse(const QString& symbol, const QByteArray& data);
    CryptoQuote parseBinanceResponse(const QString& symbol, const QByteArray& data);
    CryptoQuote parseOKXResponse(const QString& symbol, const QByteArray& data);
    QVector<CryptoQuote> parseCoinGeckoTopList(const QByteArray& data, int limit);
    QVector<CryptoQuote> parseBinanceTopList(const QByteArray& data, int limit);
    QVector<CryptoQuote> parseOKXTopList(const QByteArray& data, int limit);

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // CRYPTODATASOURCE_H
