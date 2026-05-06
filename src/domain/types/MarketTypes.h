/**
 * @file MarketTypes.h
 * @brief 统一的市场数据类型定义
 *
 * @details 整合所有行情相关结构，避免重复定义
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef MARKETTYPES_H
#define MARKETTYPES_H

#include <QString>
#include <QDateTime>
#include <QVariant>

namespace WealthPilot {

/**
 * @brief 市场类型
 */
enum class MarketType {
    Stock,      ///< 股票
    Futures,    ///< 期货
    Forex,      ///< 外汇
    Crypto,     ///< 数字货币
    Fund        ///< 基金
};

/**
 * @brief 行情基类
 */
struct QuoteBase {
    QString symbol;             ///< 代码
    QString name;               ///< 名称
    double price = 0.0;         ///< 最新价
    double change = 0.0;        ///< 涨跌额
    double changePercent = 0.0; ///< 涨跌幅
    qint64 volume = 0;          ///< 成交量
    double amount = 0.0;        ///< 成交额
    QDateTime updateTime;       ///< 更新时间
    
    virtual ~QuoteBase() = default;
    virtual MarketType marketType() const = 0;
};

/**
 * @brief 股票行情
 */
struct StockQuote : QuoteBase {
    StockQuote() = default;
    ~StockQuote() override = default;
    double open = 0.0;          ///< 开盘价
    double high = 0.0;          ///< 最高价
    double low = 0.0;           ///< 最低价
    double prevClose = 0.0;     ///< 昨收
    double bidPrice = 0.0;      ///< 买一价
    double askPrice = 0.0;      ///< 卖一价
    qint64 bidVolume = 0;       ///< 买一量
    qint64 askVolume = 0;       ///< 卖一量
    
    MarketType marketType() const override { return MarketType::Stock; }
};

/**
 * @brief 期货行情
 */
struct FuturesQuote : QuoteBase {
    double open = 0.0;          ///< 开盘价
    double high = 0.0;          ///< 最高价
    double low = 0.0;           ///< 最低价
    double settlement = 0.0;    ///< 结算价
    double prevSettlement = 0.0;///< 昨结算
    qint64 openInterest = 0;    ///< 持仓量
    qint64 preOpenInterest = 0; ///< 昨持仓
    double upperLimit = 0.0;    ///< 涨停价
    double lowerLimit = 0.0;    ///< 跌停价
    
    MarketType marketType() const override { return MarketType::Futures; }
};

/**
 * @brief 外汇行情
 */
struct ForexQuote : QuoteBase {
    double bidPrice = 0.0;      ///< 买入价
    double askPrice = 0.0;      ///< 卖出价
    double high = 0.0;          ///< 最高价
    double low = 0.0;           ///< 最低价
    QString baseCurrency;       ///< 基础货币
    QString quoteCurrency;      ///< 报价货币
    
    MarketType marketType() const override { return MarketType::Forex; }
};

/**
 * @brief 数字货币行情
 */
struct CryptoQuote : QuoteBase {
    double high24h = 0.0;       ///< 24小时最高
    double low24h = 0.0;        ///< 24小时最低
    double marketCap = 0.0;     ///< 市值
    qint64 circulatingSupply = 0; ///< 流通量
    QString exchange;           ///< 交易所
    
    MarketType marketType() const override { return MarketType::Crypto; }
};

/**
 * @brief 基金行情
 */
struct FundQuote : QuoteBase {
    double nav = 0.0;           ///< 净值
    double accNav = 0.0;        ///< 累计净值
    QString fundType;           ///< 基金类型
    QString manager;            ///< 基金经理
    QDate navDate;              ///< 净值日期
    
    MarketType marketType() const override { return MarketType::Fund; }
};

/**
 * @brief K线数据
 */
struct KLine {
    QString symbol;
    QString period;             ///< 周期 (1m, 5m, 1h, 1d, etc.)
    QDateTime time;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    qint64 volume = 0;
    double amount = 0.0;
    qint64 openInterest = 0;    ///< 持仓量（期货）
};

/**
 * @brief 缓存的行情数据
 */
struct CachedQuoteData {
    QString symbol;
    QString name;
    double price = 0.0;
    double change = 0.0;
    double changePercent = 0.0;
    qint64 volume = 0;
    QDateTime updateTime;
    qint64 dbId = 0;            ///< 数据库ID
};

} // namespace WealthPilot

#endif // MARKETTYPES_H
