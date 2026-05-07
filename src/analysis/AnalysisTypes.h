/**
 * @file AnalysisTypes.h
 * @brief 技术分析统一类型定义
 *
 * @details 定义波浪理论、道氏理论、量价形态、信号过滤等通用类型
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ANALYSIS_TYPES_H
#define ANALYSIS_TYPES_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QVariant>

namespace WealthPilot {
namespace Analysis {

// ============================================================================
// 信号类型定义
// ============================================================================

/**
 * @brief 信号强度等级
 */
enum class SignalStrength {
    Weak = 1,       ///< 弱信号
    Moderate = 2,   ///< 中等信号
    Strong = 3,     ///< 强信号
    VeryStrong = 4  ///< 非常强信号
};

/**
 * @brief 信号方向
 */
enum class SignalDirection {
    Bullish,    ///< 看涨
    Bearish,    ///< 看跌
    Neutral     ///< 中性
};

/**
 * @brief 分析理论类型
 */
enum class TheoryType {
    ElliottWave,    ///< 波浪理论
    ChanLun,        ///< 缠论
    DowTheory,      ///< 道氏理论
    VolumePattern   ///< 量价形态
};

/**
 * @brief 统一信号结构
 */
struct UnifiedSignal {
    QString id;                         ///< 信号ID
    TheoryType source;                  ///< 来源理论
    SignalDirection direction;          ///< 信号方向
    SignalStrength strength;            ///< 信号强度
    QDateTime time;                     ///< 信号时间
    QString symbol;                     ///< 标的代码
    double price = 0.0;                 ///< 信号价格
    double confidence = 0.0;            ///< 置信度 (0-100)
    QString description;                ///< 信号描述
    QMap<QString, QVariant> metadata;   ///< 额外元数据

    /**
     * @brief 获取理论名称
     */
    QString theoryName() const {
        switch (source) {
            case TheoryType::ElliottWave: return QStringLiteral("波浪理论");
            case TheoryType::ChanLun: return QStringLiteral("缠论");
            case TheoryType::DowTheory: return QStringLiteral("道氏理论");
            case TheoryType::VolumePattern: return QStringLiteral("量价形态");
            default: return QStringLiteral("未知");
        }
    }

    /**
     * @brief 获取方向描述
     */
    QString directionText() const {
        switch (direction) {
            case SignalDirection::Bullish: return QStringLiteral("看涨");
            case SignalDirection::Bearish: return QStringLiteral("看跌");
            default: return QStringLiteral("中性");
        }
    }

    /**
     * @brief 获取强度描述
     */
    QString strengthText() const {
        switch (strength) {
            case SignalStrength::Weak: return QStringLiteral("弱");
            case SignalStrength::Moderate: return QStringLiteral("中");
            case SignalStrength::Strong: return QStringLiteral("强");
            case SignalStrength::VeryStrong: return QStringLiteral("极强");
            default: return QStringLiteral("未知");
        }
    }
};

/**
 * @brief 多层过滤后的综合信号
 */
struct CompositeSignal {
    QString id;                             ///< 信号ID
    QDateTime time;                         ///< 信号时间
    QString symbol;                         ///< 标的代码
    double price = 0.0;                     ///< 信号价格
    SignalDirection direction;              ///< 综合方向
    double confidence = 0.0;                ///< 综合置信度
    int theoryCount = 0;                    ///< 支持理论数量
    QVector<UnifiedSignal> sourceSignals;   ///< 来源信号列表
    QString description;                    ///< 综合描述
    QMap<QString, QVariant> metadata;       ///< 额外元数据

    /**
     * @brief 计算综合得分
     */
    double score() const {
        // 理论数量权重 + 置信度权重
        double theoryWeight = theoryCount * 25.0;  // 每个理论25分
        double confidenceWeight = confidence * 0.5; // 置信度占50%
        return qMin(100.0, theoryWeight + confidenceWeight);
    }

    /**
     * @brief 判断是否为强信号
     */
    bool isStrongSignal() const {
        return theoryCount >= 3 && confidence >= 70.0;
    }
};

/**
 * @brief 分析结果基类
 */
struct AnalysisResult {
    QString symbol;                     ///< 标的代码
    QDateTime analysisTime;             ///< 分析时间
    bool isValid = false;               ///< 是否有效
    QString errorMessage;               ///< 错误信息
    QVector<UnifiedSignal> generatedSignals;     ///< 产生的信号
};

// ============================================================================
// K线数据结构（复用 MarketTypes）
// ============================================================================

/**
 * @brief K线数据（简化版，用于分析）
 */
struct KLine {
    QDateTime time;
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    qint64 volume = 0;
    double turnover = 0.0;

    KLine() = default;
    KLine(const QDateTime& t, double o, double h, double l, double c, qint64 v = 0)
        : time(t), open(o), high(h), low(l), close(c), volume(v) {}

    bool isValid() const { return time.isValid() && high >= low && high >= open && high >= close; }
    bool isUp() const { return close > open; }
    bool isDown() const { return close < open; }
    double body() const { return qAbs(close - open); }
    double upperShadow() const { return high - qMax(open, close); }
    double lowerShadow() const { return qMin(open, close) - low; }
    double range() const { return high - low; }
};

} // namespace Analysis
} // namespace WealthPilot

Q_DECLARE_METATYPE(WealthPilot::Analysis::UnifiedSignal)
Q_DECLARE_METATYPE(WealthPilot::Analysis::CompositeSignal)
Q_DECLARE_METATYPE(WealthPilot::Analysis::SignalDirection)
Q_DECLARE_METATYPE(WealthPilot::Analysis::SignalStrength)
Q_DECLARE_METATYPE(WealthPilot::Analysis::TheoryType)

#endif // ANALYSIS_TYPES_H
