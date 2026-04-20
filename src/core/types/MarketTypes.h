/**
 * @file MarketTypes.h
 * @brief 市场数据类型定义 - 统一管理行情相关数据结构
 *
 * @details 定义：
 * - K线数据结构
 * - 行情数据结构
 * - 枚举类型定义
 * - 数据转换工具
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef MARKETTYPES_H
#define MARKETTYPES_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QVariant>

// ============================================================================
// 枚举定义
// ============================================================================

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

// ============================================================================
// 数据结构
// ============================================================================

/**
 * @brief K线数据结构
 *
 * @details 存储单根K线的所有信息，包括：
 * - 时间、开高低收
 * - 成交量、成交额
 * - 持仓量（期货专用）
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

    /**
     * @brief 默认构造函数
     */
    KLineData() = default;

    /**
     * @brief 构造函数
     */
    KLineData(const QDateTime& t, double o, double h, double l, double c, qint64 v = 0)
        : time(t), open(o), high(h), low(l), close(c), volume(v) {}

    /**
     * @brief 判断是否有效
     */
    bool isValid() const {
        return time.isValid() && open > 0 && high > 0 && low > 0 && close > 0;
    }

    /**
     * @brief 判断是否上涨
     */
    bool isUp() const { return close > open; }

    /**
     * @brief 判断是否下跌
     */
    bool isDown() const { return close < open; }

    /**
     * @brief 获取实体长度
     */
    double body() const { return qAbs(close - open); }

    /**
     * @brief 获取上影线长度
     */
    double upperShadow() const { return high - qMax(open, close); }

    /**
     * @brief 获取下影线长度
     */
    double lowerShadow() const { return qMin(open, close) - low; }

    /**
     * @brief 获取振幅
     */
    double amplitude() const {
        return low > 0 ? (high - low) / low * 100 : 0;
    }

    /**
     * @brief 获取涨跌幅
     * @param preClose 昨收价
     */
    double changePercent(double preClose) const {
        return preClose > 0 ? (close - preClose) / preClose * 100 : 0;
    }
};

/**
 * @brief 行情快照数据
 *
 * @details 存储实时行情的完整信息，包括：
 * - 合约信息
 * - 价格信息
 * - 买卖盘口
 * - 成交统计
 */
struct MarketSnapshot {
    // ========== 合约信息 ==========
    QString instrumentId;           ///< 合约代码
    QString exchangeId;             ///< 交易所代码
    QString instrumentName;         ///< 合约名称

    // ========== 价格信息 ==========
    double lastPrice = 0.0;         ///< 最新价
    double preClose = 0.0;          ///< 昨收价
    double preSettlement = 0.0;     ///< 昨结算价
    double openPrice = 0.0;         ///< 开盘价
    double highestPrice = 0.0;      ///< 最高价
    double lowestPrice = 0.0;       ///< 最低价
    double upperLimit = 0.0;        ///< 涨停价
    double lowerLimit = 0.0;        ///< 跌停价

    // ========== 成交信息 ==========
    qint64 volume = 0;              ///< 成交量
    double turnover = 0.0;          ///< 成交额
    qint64 openInterest = 0;        ///< 持仓量

    // ========== 买卖盘口 ==========
    double bidPrice[5] = {};        ///< 买价[1-5]
    int bidVolume[5] = {};          ///< 买量[1-5]
    double askPrice[5] = {};        ///< 卖价[1-5]
    int askVolume[5] = {};          ///< 卖量[1-5]

    // ========== 时间信息 ==========
    QDateTime updateTime;           ///< 更新时间
    QDateTime tradingDay;           ///< 交易日

    // ========== 辅助方法 ==========

    /**
     * @brief 获取涨跌额
     */
    double change() const {
        double base = preSettlement > 0 ? preSettlement : preClose;
        return base > 0 ? lastPrice - base : 0;
    }

    /**
     * @brief 获取涨跌幅
     */
    double changePercent() const {
        double base = preSettlement > 0 ? preSettlement : preClose;
        return base > 0 ? (lastPrice - base) / base * 100 : 0;
    }

    /**
     * @brief 判断是否涨停
     */
    bool isUpperLimit() const {
        return upperLimit > 0 && lastPrice >= upperLimit;
    }

    /**
     * @brief 判断是否跌停
     */
    bool isLowerLimit() const {
        return lowerLimit > 0 && lastPrice <= lowerLimit;
    }

    /**
     * @brief 获取买一价
     */
    double bidPrice1() const { return bidPrice[0]; }

    /**
     * @brief 获取买一量
     */
    int bidVolume1() const { return bidVolume[0]; }

    /**
     * @brief 获取卖一价
     */
    double askPrice1() const { return askPrice[0]; }

    /**
     * @brief 获取卖一量
     */
    int askVolume1() const { return askVolume[0]; }
};

/**
 * @brief 分笔成交数据
 */
struct TickData {
    QDateTime time;             ///< 成交时间
    double price = 0.0;         ///< 成交价格
    int volume = 0;             ///< 成交数量
    TradeDirection direction = TradeDirection::Unknown;  ///< 成交方向

    /**
     * @brief 获取方向字符串
     */
    QString directionString() const {
        switch (direction) {
            case TradeDirection::Buy: return QStringLiteral("买");
            case TradeDirection::Sell: return QStringLiteral("卖");
            default: return QStringLiteral("--");
        }
    }
};

/**
 * @brief 订单数据（CTP专用）
 * @note 此结构体在 ICTPPlugin.h 中定义，此处仅做前向声明
 */
// struct OrderData; // 使用 ICTPPlugin.h 中的定义

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

/**
 * @brief 格式化涨跌幅
 */
inline QString formatChangePercent(double percent, bool showSign = true) {
    QString result = QString::number(qAbs(percent), 'f', 2) + "%";
    if (showSign) {
        if (percent > 0) return "+" + result;
        if (percent < 0) return "-" + result;
    }
    return result;
}

} // namespace MarketUtils

// ============================================================================
// Qt 元类型注册
// ============================================================================

Q_DECLARE_METATYPE(KLineData)
Q_DECLARE_METATYPE(MarketSnapshot)
Q_DECLARE_METATYPE(TickData)
// Q_DECLARE_METATYPE(OrderData) // 在 ICTPPlugin.h 中定义
Q_DECLARE_METATYPE(KLinePeriod)
Q_DECLARE_METATYPE(AdjustmentType)
Q_DECLARE_METATYPE(TradeDirection)
Q_DECLARE_METATYPE(OrderStatus)

#endif // MARKETTYPES_H
