/**
 * @file VolumePatternTypes.h
 * @brief 量价形态类型定义
 *
 * @details 定义量价分析的核心形态：
 * - 成交量形态
 * - 价格形态
 * - 量价组合形态
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef VOLUME_PATTERN_TYPES_H
#define VOLUME_PATTERN_TYPES_H

#include <QString>
#include <QDateTime>
#include <QVector>

namespace WealthPilot {
namespace VolumePattern {

/**
 * @brief 成交量形态类型
 */
enum class VolumeShape {
    Increasing,         ///< 放量
    Decreasing,         ///< 缩量
    Stable,             ///< 平量
    Abnormal,           ///< 异常量
    VolumeSpike         ///< 量能突增
};

/**
 * @brief 价格形态类型
 */
enum class PriceShape {
    BigUp,              ///< 大涨
    BigDown,            ///< 大跌
    SmallUp,            ///< 小涨
    SmallDown,          ///< 小跌
    Flat,               ///< 横盘
    GapUp,              ///< 跳空高开
    GapDown             ///< 跳空低开
};

/**
 * @brief 量价组合形态
 */
enum class VolumePricePattern {
    // 看涨形态
    PriceUpVolumeUp,            ///< 价涨量增（健康上涨）
    PriceUpVolumeDown,          ///< 价涨量缩（上涨乏力）
    PriceDownVolumeUp,          ///< 价跌量增（可能见底）
    PriceDownVolumeDown,        ///< 价跌量缩（下跌动能减弱）

    // 反转形态
    BottomDivergence,           ///< 底部背离
    TopDivergence,              ///< 顶部背离
    VolumeBreakout,             ///< 放量突破
    VolumeBreakdown,            ///< 放量破位

    // 经典形态
    OBVUp,                      ///< OBV上升
    OBVDown,                    ///< OBV下降
    VolumeClimax,               ///< 量能高潮
    SellingClimax,              ///< 抛售高潮
    BuyingClimax,               ///< 买入高潮

    Unknown
};

/**
 * @brief 单根K线量价特征
 */
struct VolumePriceBar {
    QDateTime time;                 ///< 时间
    double open = 0.0;              ///< 开盘价
    double high = 0.0;              ///< 最高价
    double low = 0.0;               ///< 最低价
    double close = 0.0;             ///< 收盘价
    qint64 volume = 0;              ///< 成交量

    VolumeShape volumeShape;        ///< 成交量形态
    PriceShape priceShape;          ///< 价格形态
    VolumePricePattern pattern;     ///< 组合形态

    double volumeRatio = 1.0;       ///< 量比（相对均量）
    double priceChange = 0.0;       ///< 价格变化
    double amplitude = 0.0;         ///< 振幅

    /**
     * @brief 判断是否为看涨信号
     */
    bool isBullish() const {
        return pattern == VolumePricePattern::PriceUpVolumeUp ||
               pattern == VolumePricePattern::VolumeBreakout ||
               pattern == VolumePricePattern::BottomDivergence;
    }

    /**
     * @brief 判断是否为看跌信号
     */
    bool isBearish() const {
        return pattern == VolumePricePattern::PriceDownVolumeUp ||
               pattern == VolumePricePattern::VolumeBreakdown ||
               pattern == VolumePricePattern::TopDivergence;
    }
};

/**
 * @brief 量价形态组合
 */
struct VolumePriceFormation {
    QString id;                         ///< 形态ID
    VolumePricePattern pattern;         ///< 形态类型
    QDateTime startTime;                ///< 起始时间
    QDateTime endTime;                  ///< 结束时间
    double startPrice = 0.0;            ///< 起始价格
    double endPrice = 0.0;              ///< 结束价格

    int barCount = 0;                   ///< K线数量
    qint64 totalVolume = 0;             ///< 总成交量
    double avgVolumeRatio = 1.0;        ///< 平均量比

    double confidence = 0.0;            ///< 置信度
    QString description;                ///< 形态描述

    QVector<VolumePriceBar> bars;       ///< 包含的K线
};

/**
 * @brief OBV指标数据
 */
struct OBVData {
    QDateTime time;
    double obv = 0.0;           ///< OBV值
    double obvMA = 0.0;         ///< OBV均线
    double divergence = 0.0;    ///< 背离程度
};

/**
 * @brief 量价分析结果
 */
struct VolumePatternResult {
    QString symbol;                     ///< 标的代码
    QDateTime analysisTime;             ///< 分析时间
    bool isValid = false;               ///< 是否有效
    QString errorMessage;               ///< 错误信息

    QVector<VolumePriceBar> bars;       ///< 量价K线
    QVector<VolumePriceFormation> formations; ///< 识别出的形态

    OBVData obv;                        ///< OBV指标
    double avgVolume = 0.0;             ///< 平均成交量
    double volumeTrend = 0.0;           ///< 成交量趋势

    bool hasDivergence = false;         ///< 是否有背离
    bool hasBreakout = false;           ///< 是否有突破
    double confidence = 0.0;            ///< 整体置信度
};

/**
 * @brief 量价信号
 */
struct VolumeSignal {
    QDateTime time;
    VolumePricePattern pattern;
    QString description;
    double confidence;
    bool isBullish;
};

} // namespace VolumePattern
} // namespace WealthPilot

Q_DECLARE_METATYPE(WealthPilot::VolumePattern::VolumePricePattern)
Q_DECLARE_METATYPE(WealthPilot::VolumePattern::VolumePriceBar)

#endif // VOLUME_PATTERN_TYPES_H
