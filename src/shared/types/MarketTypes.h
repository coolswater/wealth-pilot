/**
 * @file MarketTypes.h
 * @brief 统一的市场数据类型定义
 *
 * @details 整合所有行情相关结构，包括：
 * - 枚举类型：MarketType, KLinePeriod, AdjustmentType 等
 * - 行情数据：StockQuote, FuturesQuote, MarketSnapshot
 * - K线数据：KLineData, TimeShareData
 *
 * @author WealthPilot Team
 * @version 3.0.0
 */

#ifndef WEALTHPILOT_MARKETTYPES_H
#define WEALTHPILOT_MARKETTYPES_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QVariant>

namespace WealthPilot {

// ============================================================================
// 枚举定义
// ============================================================================

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
 * @brief K线周期枚举
 */
enum class KLinePeriod {
    Timeline = 0,   ///< 分时图
    Minute1,        ///< 1分钟
    Minute5,        ///< 5分钟
    Minute15,       ///< 15分钟
    Minute30,       ///< 30分钟
    Hour1,          ///< 60分钟
    Day1,           ///< 日线
    Week1,          ///< 周线
    Month1,         ///< 月线
    Custom          ///< 自定义周期
};

/**
 * @brief 复权类型枚举
 */
enum class AdjustmentType {
    None = 0,       ///< 不复权
    Front,          ///< 前复权
    Back            ///< 后复权
};

/**
 * @brief 买卖方向枚举
 */
enum class TradeDirection {
    Buy = 0,            ///< 买入/开多
    Sell = 1,           ///< 卖出/开空
    BuyClose = 2,       ///< 平空（买入平仓）
    SellClose = 3,      ///< 平多（卖出平仓）
    Unknown = 99        ///< 未知
};

/**
 * @brief 订单状态枚举
 */
enum class OrderStatus {
    Pending = 0,        ///< 待提交
    Submitted = 1,      ///< 已提交
    Accepted = 2,       ///< 已接受
    Rejected = 3,       ///< 已拒绝
    PartialFilled = 4,  ///< 部分成交
    Filled = 5,         ///< 全部成交
    Cancelled = 6,      ///< 已撤销
    Unknown = 99        ///< 未知
};

/**
 * @brief 市场状态枚举
 */
enum class MarketStatus {
    PreMarket,      ///< 盘前
    Trading,        ///< 交易中
    Paused,         ///< 休市
    Closed,         ///< 收盘
    Auction         ///< 集合竞价
};

/**
 * @brief 基金类型枚举
 */
enum class FundType {
    ETF,        ///< 交易所交易基金
    LOF,        ///< 上市开放式基金
    OpenEnd,    ///< 开放式基金
    ClosedEnd,  ///< 封闭式基金
    Money,      ///< 货币基金
    Bond,       ///< 债券基金
    Mixed,      ///< 混合基金
    Stock,      ///< 股票基金
    Index,      ///< 指数基金
    QDII,       ///< QDII基金
    Unknown     ///< 未知类型
};

// ============================================================================
// 行情基类
// ============================================================================

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

    /**
     * @brief 判断是否有效
     */
    bool isValid() const { return !symbol.isEmpty() && price > 0; }
};

// ============================================================================
// 股票行情
// ============================================================================

/**
 * @brief 股票行情数据
 */
struct StockQuote : QuoteBase {
    // ========== 价格信息 ==========
    double lastPrice = 0.0;     ///< 最新价（兼容别名）
    double open = 0.0;          ///< 开盘价
    double openPrice = 0.0;     ///< 开盘价（兼容别名）
    double high = 0.0;          ///< 最高价
    double highPrice = 0.0;     ///< 最高价（兼容别名）
    double low = 0.0;           ///< 最低价
    double lowPrice = 0.0;      ///< 最低价（兼容别名）
    double prevClose = 0.0;     ///< 昨收价
    double preClose = 0.0;      ///< 昨收价（兼容别名）
    double turnover = 0.0;      ///< 成交额（兼容别名）
    double changeAmount = 0.0;  ///< 涨跌额（兼容别名）
    qint64 orderDiff = 0;       ///< 委差
    double orderRatio = 0.0;    ///< 委比

    // ========== 五档行情 ==========
    double bidPrice[5] = {};    ///< 买价[1-5]
    qint64 bidVolume[5] = {};   ///< 买量[1-5]
    double askPrice[5] = {};    ///< 卖价[1-5]
    qint64 askVolume[5] = {};   ///< 卖量[1-5]

    // ========== 涨跌停 ==========
    double upperLimit = 0.0;    ///< 涨停价
    double lowerLimit = 0.0;    ///< 跌停价

    // ========== 构造函数 ==========
    StockQuote() = default;
    ~StockQuote() override = default;

    // ========== 虚函数实现 ==========
    MarketType marketType() const override { return MarketType::Stock; }

    // ========== 辅助方法 ==========

    /**
     * @brief 获取买一价
     */
    double bidPrice1() const { return bidPrice[0]; }

    /**
     * @brief 获取卖一价
     */
    double askPrice1() const { return askPrice[0]; }

    /**
     * @brief 获取买一量
     */
    qint64 bidVolume1() const { return bidVolume[0]; }

    /**
     * @brief 获取卖一量
     */
    qint64 askVolume1() const { return askVolume[0]; }

    /**
     * @brief 判断是否涨停
     */
    bool isUpperLimit() const {
        return upperLimit > 0 && price >= upperLimit * 0.998;
    }

    /**
     * @brief 判断是否跌停
     */
    bool isLowerLimit() const {
        return lowerLimit > 0 && price <= lowerLimit * 1.002;
    }
};

// ============================================================================
// 期货行情
// ============================================================================

/**
 * @brief 期货行情数据
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

// ============================================================================
// 外汇行情
// ============================================================================

/**
 * @brief 外汇行情数据
 */
struct ForexQuote : QuoteBase {
    QString pair;               ///< 货币对（如 USD/CNY）
    double rate = 0.0;          ///< 当前汇率
    double bid = 0.0;           ///< 买入价
    double ask = 0.0;           ///< 卖出价
    double high = 0.0;          ///< 最高价
    double low = 0.0;           ///< 最低价
    double high24h = 0.0;       ///< 24小时最高
    double low24h = 0.0;        ///< 24小时最低
    QString baseCurrency;       ///< 基础货币
    QString quoteCurrency;      ///< 报价货币

    MarketType marketType() const override { return MarketType::Forex; }
};

// ============================================================================
// 数字货币行情
// ============================================================================

/**
 * @brief 数字货币行情数据
 */
struct CryptoQuote : QuoteBase {
    double priceUsd = 0.0;      ///< USD价格
    double priceCny = 0.0;      ///< CNY价格
    double change24h = 0.0;     ///< 24小时涨跌幅
    double volume24h = 0.0;     ///< 24小时成交量
    double high24h = 0.0;       ///< 24小时最高
    double low24h = 0.0;        ///< 24小时最低
    double marketCap = 0.0;     ///< 市值
    qint64 circulatingSupply = 0; ///< 流通量
    int rank = 0;               ///< 排名
    QString exchange;           ///< 交易所

    MarketType marketType() const override { return MarketType::Crypto; }
};

// ============================================================================
// 基金行情
// ============================================================================

/**
 * @brief 基金行情数据
 */
struct FundQuote : QuoteBase {
    QString code;               ///< 基金代码
    double nav = 0.0;           ///< 净值
    double accNav = 0.0;        ///< 累计净值
    double lastPrice = 0.0;     ///< 最新价格（ETF/LOF）
    double changeAmount = 0.0;  ///< 涨跌额
    FundType type = FundType::Unknown;  ///< 基金类型
    QString manager;            ///< 基金经理
    QString company;            ///< 基金公司
    double scale = 0.0;         ///< 基金规模（亿元）
    QDate navDate;              ///< 净值日期

    MarketType marketType() const override { return MarketType::Fund; }
};

// ============================================================================
// K线数据
// ============================================================================

/**
 * @brief K线数据结构
 */
struct KLineData {
    QDateTime time;             ///< 时间
    double open = 0.0;          ///< 开盘价
    double high = 0.0;          ///< 最高价
    double low = 0.0;           ///< 最低价
    double close = 0.0;         ///< 收盘价
    qint64 volume = 0;          ///< 成交量
    double turnover = 0.0;      ///< 成交额
    double openInterest = 0.0;  ///< 持仓量（期货）

    KLineData() = default;
    KLineData(const QDateTime& t, double o, double h, double l, double c, qint64 v = 0)
        : time(t), open(o), high(h), low(l), close(c), volume(v) {}

    bool isValid() const {
        return time.isValid() && open > 0 && high > 0 && low > 0 && close > 0;
    }

    bool isUp() const { return close > open; }
    bool isDown() const { return close < open; }
    double body() const { return qAbs(close - open); }
    double upperShadow() const { return high - qMax(open, close); }
    double lowerShadow() const { return qMin(open, close) - low; }
    double amplitude() const {
        return low > 0 ? (high - low) / low * 100 : 0;
    }
    double changePercent(double preClose) const {
        return preClose > 0 ? (close - preClose) / preClose * 100 : 0;
    }
};

// ============================================================================
// 行情快照
// ============================================================================

/**
 * @brief 行情快照数据
 */
struct MarketSnapshot {
    // 合约信息
    QString instrumentId;           ///< 合约代码
    QString exchangeId;             ///< 交易所代码
    QString instrumentName;         ///< 合约名称

    // 价格信息
    double lastPrice = 0.0;         ///< 最新价
    double preClose = 0.0;          ///< 昨收价
    double preSettlement = 0.0;     ///< 昨结算价
    double openPrice = 0.0;         ///< 开盘价
    double highestPrice = 0.0;      ///< 最高价
    double lowestPrice = 0.0;       ///< 最低价
    double upperLimit = 0.0;        ///< 涨停价
    double lowerLimit = 0.0;        ///< 跌停价

    // 成交信息
    qint64 volume = 0;              ///< 成交量
    double turnover = 0.0;          ///< 成交额
    qint64 openInterest = 0;        ///< 持仓量

    // 买卖盘口
    double bidPrice[5] = {};        ///< 买价[1-5]
    int bidVolume[5] = {};          ///< 买量[1-5]
    double askPrice[5] = {};        ///< 卖价[1-5]
    int askVolume[5] = {};          ///< 卖量[1-5]

    // 时间信息
    QDateTime updateTime;           ///< 更新时间
    QDateTime tradingDay;           ///< 交易日

    // 辅助方法
    double change() const {
        double base = preSettlement > 0 ? preSettlement : preClose;
        return base > 0 ? lastPrice - base : 0;
    }

    double changePercent() const {
        double base = preSettlement > 0 ? preSettlement : preClose;
        return base > 0 ? (lastPrice - base) / base * 100 : 0;
    }

    bool isUpperLimit() const {
        return upperLimit > 0 && lastPrice >= upperLimit;
    }

    bool isLowerLimit() const {
        return lowerLimit > 0 && lastPrice <= lowerLimit;
    }

    double bidPrice1() const { return bidPrice[0]; }
    int bidVolume1() const { return bidVolume[0]; }
    double askPrice1() const { return askPrice[0]; }
    int askVolume1() const { return askVolume[0]; }
};

// ============================================================================
// 分时数据
// ============================================================================

/**
 * @brief 分时数据结构
 */
struct TimeShareData {
    QDateTime time;             ///< 时间
    double price = 0.0;         ///< 当前价格
    double avgPrice = 0.0;      ///< 均价
    qint64 volume = 0;          ///< 成交量
    double turnover = 0.0;      ///< 成交额
    double changePercent = 0.0; ///< 涨跌幅

    TimeShareData() = default;
    TimeShareData(const QDateTime& t, double p, double avg = 0.0, qint64 v = 0)
        : time(t), price(p), avgPrice(avg), volume(v) {}

    bool isValid() const {
        return time.isValid() && price > 0;
    }
};

// ============================================================================
// 分笔成交数据
// ============================================================================

/**
 * @brief 分笔成交数据
 */
struct TickData {
    QDateTime time;             ///< 成交时间
    double price = 0.0;         ///< 成交价格
    int volume = 0;             ///< 成交数量
    TradeDirection direction = TradeDirection::Unknown;  ///< 成交方向

    QString directionString() const {
        switch (direction) {
            case TradeDirection::Buy: return QStringLiteral("买");
            case TradeDirection::Sell: return QStringLiteral("卖");
            default: return QStringLiteral("--");
        }
    }
};

// ============================================================================
// 工具函数
// ============================================================================

namespace MarketUtils {

/**
 * @brief K线周期转字符串
 */
inline QString periodToString(KLinePeriod period) {
    switch (period) {
        case KLinePeriod::Timeline: return QStringLiteral("分时");
        case KLinePeriod::Minute1: return QStringLiteral("1分");
        case KLinePeriod::Minute5: return QStringLiteral("5分");
        case KLinePeriod::Minute15: return QStringLiteral("15分");
        case KLinePeriod::Minute30: return QStringLiteral("30分");
        case KLinePeriod::Hour1: return QStringLiteral("60分");
        case KLinePeriod::Day1: return QStringLiteral("日线");
        case KLinePeriod::Week1: return QStringLiteral("周线");
        case KLinePeriod::Month1: return QStringLiteral("月线");
        default: return QStringLiteral("自定义");
    }
}

/**
 * @brief 字符串转K线周期
 */
inline KLinePeriod stringToPeriod(const QString& str) {
    if (str == QStringLiteral("分时")) return KLinePeriod::Timeline;
    if (str == QStringLiteral("1分")) return KLinePeriod::Minute1;
    if (str == QStringLiteral("5分")) return KLinePeriod::Minute5;
    if (str == QStringLiteral("15分")) return KLinePeriod::Minute15;
    if (str == QStringLiteral("30分")) return KLinePeriod::Minute30;
    if (str == QStringLiteral("60分")) return KLinePeriod::Hour1;
    if (str == QStringLiteral("日线")) return KLinePeriod::Day1;
    if (str == QStringLiteral("周线")) return KLinePeriod::Week1;
    if (str == QStringLiteral("月线")) return KLinePeriod::Month1;
    return KLinePeriod::Custom;
}

/**
 * @brief 格式化价格
 */
inline QString formatPrice(double price, int precision = 2) {
    if (price <= 0) return "--";
    return QString::number(price, 'f', precision);
}

/**
 * @brief 格式化成交量
 */
inline QString formatVolume(qint64 volume) {
    if (volume <= 0) return "--";
    if (volume >= 100000000) {
        return QString("%1亿").arg(volume / 100000000.0, 0, 'f', 2);
    }
    if (volume >= 10000) {
        return QString("%1万").arg(volume / 10000.0, 0, 'f', 2);
    }
    return QString::number(volume);
}

/**
 * @brief 格式化金额
 */
inline QString formatMoney(double value) {
    if (qAbs(value) >= 100000000.0) {
        return QString::number(value / 100000000.0, 'f', 2) + "亿";
    }
    if (qAbs(value) >= 10000.0) {
        return QString::number(value / 10000.0, 'f', 2) + "万";
    }
    return QString::number(value, 'f', 2);
}

} // namespace MarketUtils

// ============================================================================
// QMetaType 注册
// ============================================================================

} // namespace WealthPilot

// 注册 QMetaType 以支持 QVariant
Q_DECLARE_METATYPE(WealthPilot::StockQuote)
Q_DECLARE_METATYPE(WealthPilot::FuturesQuote)
Q_DECLARE_METATYPE(WealthPilot::ForexQuote)
Q_DECLARE_METATYPE(WealthPilot::CryptoQuote)
Q_DECLARE_METATYPE(WealthPilot::FundQuote)
Q_DECLARE_METATYPE(WealthPilot::KLineData)
Q_DECLARE_METATYPE(WealthPilot::MarketSnapshot)
Q_DECLARE_METATYPE(WealthPilot::TimeShareData)
Q_DECLARE_METATYPE(WealthPilot::TickData)

#endif // WEALTHPILOT_MARKETTYPES_H
