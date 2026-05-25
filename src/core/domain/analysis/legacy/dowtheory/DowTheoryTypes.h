/**
 * @file DowTheoryTypes.h
 * @brief 道氏理论类型定义
 *
 * @details 定义道氏理论的核心概念：
 * - 趋势类型
 * - 趋势阶段
 * - 趋势确认信号
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DOW_THEORY_TYPES_H
#define DOW_THEORY_TYPES_H

#include <QString>
#include <QDateTime>
#include <QVector>

namespace WealthPilot {
namespace DowTheory {

/**
 * @brief 趋势方向
 */
enum class TrendDirection {
    Upward,     ///< 上升趋势
    Downward,   ///< 下降趋势
    Sideways    ///< 横盘整理
};

/**
 * @brief 趋势级别
 */
enum class TrendLevel {
    Primary,    ///< 主要趋势（长期）
    Secondary,  ///< 次要趋势（中期）
    Minor       ///< 小趋势（短期）
};

/**
 * @brief 趋势阶段
 */
enum class TrendPhase {
    Accumulation,       ///< 积累阶段
    Markup,             ///< 上涨阶段
    Distribution,       ///< 派发阶段
    Markdown,           ///< 下跌阶段
    Unknown
};

/**
 * @brief 高低点类型
 */
enum class ExtremumType {
    HigherHigh,     ///< 更高的高点
    LowerHigh,      ///< 更低的高点
    HigherLow,      ///< 更高的低点
    LowerLow,       ///< 更低的低点
    Flat            ///< 持平
};

/**
 * @brief 价格极值点
 */
struct PriceExtremum {
    QDateTime time;             ///< 时间
    double price = 0.0;         ///< 价格
    ExtremumType type;          ///< 类型
    int barIndex = -1;          ///< K线索引
    bool isConfirmed = false;   ///< 是否确认
};

/**
 * @brief 趋势线
 */
struct TrendLine {
    QDateTime startTime;        ///< 起始时间
    QDateTime endTime;          ///< 结束时间
    double startPrice = 0.0;    ///< 起始价格
    double endPrice = 0.0;      ///< 结束价格
    TrendDirection direction;   ///< 趋势方向
    int touchCount = 0;         ///< 触及次数
    double slope = 0.0;         ///< 斜率
    bool isValid = true;        ///< 是否有效
    bool isBroken = false;      ///< 是否被突破

    /**
     * @brief 计算指定时间的趋势线价格
     */
    double priceAtTime(const QDateTime& time) const {
        if (!startTime.isValid() || !endTime.isValid()) return 0;
        qint64 totalMs = startTime.msecsTo(endTime);
        qint64 targetMs = startTime.msecsTo(time);
        if (totalMs == 0) return startPrice;
        return startPrice + (endPrice - startPrice) * targetMs / totalMs;
    }
};

/**
 * @brief 趋势结构
 */
struct Trend {
    int id = 0;                         ///< 趋势ID
    TrendDirection direction;           ///< 趋势方向
    TrendLevel level;                   ///< 趋势级别
    TrendPhase phase;                   ///< 趋势阶段

    QDateTime startTime;                ///< 起始时间
    QDateTime endTime;                  ///< 结束时间
    double startPrice = 0.0;            ///< 起始价格
    double endPrice = 0.0;              ///< 结束价格
    double highPrice = 0.0;             ///< 最高价
    double lowPrice = 0.0;              ///< 最低价

    QVector<PriceExtremum> extremums;   ///< 极值点序列
    QVector<TrendLine> trendLines;      ///< 趋势线

    double strength = 0.0;              ///< 趋势强度
    double confidence = 0.0;            ///< 置信度
    bool isActive = true;               ///< 是否活跃

    /**
     * @brief 获取趋势持续时间（秒）
     */
    qint64 duration() const {
        if (!startTime.isValid() || !endTime.isValid()) return 0;
        return startTime.secsTo(endTime);
    }

    /**
     * @brief 获取价格变化
     */
    double priceChange() const {
        return endPrice - startPrice;
    }

    /**
     * @brief 获取价格变化百分比
     */
    double priceChangePercent() const {
        if (startPrice <= 0) return 0;
        return (endPrice - startPrice) / startPrice * 100;
    }
};

/**
 * @brief 道氏理论分析结果
 */
struct DowTheoryResult {
    QString symbol;                     ///< 标的代码
    QDateTime analysisTime;             ///< 分析时间
    bool isValid = false;               ///< 是否有效
    QString errorMessage;               ///< 错误信息

    Trend primaryTrend;                 ///< 主要趋势
    Trend secondaryTrend;               ///< 次要趋势
    Trend minorTrend;                   ///< 小趋势

    QVector<PriceExtremum> extremums;   ///< 所有极值点
    QVector<TrendLine> trendLines;      ///< 所有趋势线

    bool hasTrendReversal = false;      ///< 是否有趋势反转
    TrendDirection previousTrend;       ///< 前一趋势方向

    double confidence = 0.0;            ///< 整体置信度
};

/**
 * @brief 趋势确认信号
 */
struct TrendSignal {
    QDateTime time;                     ///< 信号时间
    TrendDirection direction;           ///< 趋势方向
    TrendLevel level;                   ///< 趋势级别
    QString description;                ///< 信号描述
    double confidence = 0.0;            ///< 置信度
};

} // namespace DowTheory
} // namespace WealthPilot

Q_DECLARE_METATYPE(WealthPilot::DowTheory::TrendDirection)
Q_DECLARE_METATYPE(WealthPilot::DowTheory::TrendLevel)
Q_DECLARE_METATYPE(WealthPilot::DowTheory::Trend)

#endif // DOW_THEORY_TYPES_H
