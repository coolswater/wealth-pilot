/**
 * @file DowTheoryAnalyzer.cpp
 * @brief 道氏理论分析器实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "DowTheoryAnalyzer.h"
#include <QDebug>
#include <QtMath>
#include <QUuid>

namespace WealthPilot {
namespace DowTheory {

struct DowTheoryAnalyzer::Impl {
    QVector<Analysis::KLine> klines;
    DowTheoryResult result;
    QVector<Analysis::UnifiedSignal> signals;

    // 参数
    int minTrendBars = 5;           // 最小趋势K线数
    double minTrendChange = 0.02;   // 最小趋势变化比例
    int confirmationBars = 3;       // 确认K线数
};

DowTheoryAnalyzer::DowTheoryAnalyzer(QObject* parent)
    : IAnalyzer(parent)
    , d(std::make_unique<Impl>())
{
}

DowTheoryAnalyzer::~DowTheoryAnalyzer() = default;

Analysis::AnalysisResult DowTheoryAnalyzer::analyze(const QVector<Analysis::KLine>& klines)
{
    Analysis::AnalysisResult result;
    result.analysisTime = QDateTime::currentDateTime();

    if (klines.size() < 30) {
        result.isValid = false;
        result.errorMessage = QStringLiteral("K线数据不足");
        return result;
    }

    d->klines = klines;

    // 1. 识别极值点
    auto extremums = identifyExtremums(klines);

    // 2. 分析高低点模式
    analyzeHighsAndLows(extremums);

    // 3. 绘制趋势线
    auto trendLines = drawTrendLines(klines, extremums);

    // 4. 识别各级别趋势
    d->result.primaryTrend = identifyTrend(klines, TrendLevel::Primary);
    d->result.secondaryTrend = identifyTrend(klines, TrendLevel::Secondary);
    d->result.minorTrend = identifyTrend(klines, TrendLevel::Minor);

    // 5. 检测趋势反转
    d->result.hasTrendReversal = detectTrendReversal(klines, d->result.primaryTrend);

    d->result.extremums = extremums;
    d->result.trendLines = trendLines;
    d->result.isValid = true;
    d->result.analysisTime = QDateTime::currentDateTime();

    // 6. 生成信号
    d->signals = generateSignals(d->result);
    result.signals = d->signals;
    result.isValid = true;

    emit analysisCompleted(result);

    return result;
}

void DowTheoryAnalyzer::clear()
{
    d->klines.clear();
    d->result = DowTheoryResult();
    d->signals.clear();
}

QVector<Analysis::UnifiedSignal> DowTheoryAnalyzer::currentSignals() const
{
    return d->signals;
}

const DowTheoryResult& DowTheoryAnalyzer::dowResult() const
{
    return d->result;
}

Trend DowTheoryAnalyzer::identifyTrend(const QVector<Analysis::KLine>& klines, TrendLevel level)
{
    Trend trend;
    trend.level = level;

    if (klines.size() < d->minTrendBars) {
        trend.direction = TrendDirection::Sideways;
        return trend;
    }

    // 根据级别确定分析窗口
    int windowSize = 0;
    switch (level) {
        case TrendLevel::Primary:
            windowSize = qMin(200, klines.size());
            break;
        case TrendLevel::Secondary:
            windowSize = qMin(50, klines.size());
            break;
        case TrendLevel::Minor:
            windowSize = qMin(20, klines.size());
            break;
    }

    int startIdx = klines.size() - windowSize;

    // 计算价格变化
    double startPrice = klines[startIdx].close;
    double endPrice = klines.last().close;
    double change = (endPrice - startPrice) / startPrice;

    // 判断趋势方向
    if (change > d->minTrendChange) {
        trend.direction = TrendDirection::Upward;
        trend.phase = TrendPhase::Markup;
    } else if (change < -d->minTrendChange) {
        trend.direction = TrendDirection::Downward;
        trend.phase = TrendPhase::Markdown;
    } else {
        trend.direction = TrendDirection::Sideways;
        trend.phase = TrendPhase::Unknown;
    }

    trend.startPrice = startPrice;
    trend.endPrice = endPrice;
    trend.startTime = klines[startIdx].time;
    trend.endTime = klines.last().time;
    trend.strength = calculateTrendStrength(trend);

    return trend;
}

QVector<PriceExtremum> DowTheoryAnalyzer::identifyExtremums(const QVector<Analysis::KLine>& klines)
{
    QVector<PriceExtremum> extremums;

    if (klines.size() < 5) {
        return extremums;
    }

    // 使用滑动窗口识别局部极值
    int window = 3;

    for (int i = window; i < klines.size() - window; ++i) {
        bool isHigh = true;
        bool isLow = true;

        for (int j = i - window; j <= i + window; ++j) {
            if (j == i) continue;
            if (klines[j].high >= klines[i].high) isHigh = false;
            if (klines[j].low <= klines[i].low) isLow = false;
        }

        if (isHigh || isLow) {
            PriceExtremum ext;
            ext.time = klines[i].time;
            ext.price = isHigh ? klines[i].high : klines[i].low;
            ext.type = isHigh ? ExtremumType::HigherHigh : ExtremumType::LowerLow;
            ext.barIndex = i;
            extremums.append(ext);
        }
    }

    return extremums;
}

void DowTheoryAnalyzer::analyzeHighsAndLows(QVector<PriceExtremum>& extremums)
{
    if (extremums.size() < 2) return;

    for (int i = 1; i < extremums.size(); ++i) {
        double prevPrice = extremums[i - 1].price;
        double currPrice = extremums[i].price;

        // 判断是高点还是低点
        bool isHigh = extremums[i].type == ExtremumType::HigherHigh ||
                     extremums[i].type == ExtremumType::LowerHigh;

        if (isHigh) {
            if (currPrice > prevPrice) {
                extremums[i].type = ExtremumType::HigherHigh;
            } else if (currPrice < prevPrice) {
                extremums[i].type = ExtremumType::LowerHigh;
            } else {
                extremums[i].type = ExtremumType::Flat;
            }
        } else {
            if (currPrice > prevPrice) {
                extremums[i].type = ExtremumType::HigherLow;
            } else if (currPrice < prevPrice) {
                extremums[i].type = ExtremumType::LowerLow;
            } else {
                extremums[i].type = ExtremumType::Flat;
            }
        }
    }
}

QVector<TrendLine> DowTheoryAnalyzer::drawTrendLines(
    const QVector<Analysis::KLine>& klines,
    const QVector<PriceExtremum>& extremums)
{
    QVector<TrendLine> lines;

    if (extremums.size() < 2) return lines;

    // 寻找上升趋势线（连接低点）
    QVector<const PriceExtremum*> lows;
    for (const auto& ext : extremums) {
        if (ext.type == ExtremumType::HigherLow || ext.type == ExtremumType::LowerLow) {
            lows.append(&ext);
        }
    }

    if (lows.size() >= 2) {
        TrendLine line;
        line.direction = TrendDirection::Upward;
        line.startTime = lows.first()->time;
        line.startPrice = lows.first()->price;
        line.endTime = lows.last()->time;
        line.endPrice = lows.last()->price;
        line.touchCount = lows.size();

        // 计算斜率
        qint64 duration = line.startTime.msecsTo(line.endTime);
        if (duration > 0) {
            line.slope = (line.endPrice - line.startPrice) / duration;
        }

        lines.append(line);
    }

    // 寻找下降趋势线（连接高点）
    QVector<const PriceExtremum*> highs;
    for (const auto& ext : extremums) {
        if (ext.type == ExtremumType::HigherHigh || ext.type == ExtremumType::LowerHigh) {
            highs.append(&ext);
        }
    }

    if (highs.size() >= 2) {
        TrendLine line;
        line.direction = TrendDirection::Downward;
        line.startTime = highs.first()->time;
        line.startPrice = highs.first()->price;
        line.endTime = highs.last()->time;
        line.endPrice = highs.last()->price;
        line.touchCount = highs.size();

        qint64 duration = line.startTime.msecsTo(line.endTime);
        if (duration > 0) {
            line.slope = (line.endPrice - line.startPrice) / duration;
        }

        lines.append(line);
    }

    return lines;
}

bool DowTheoryAnalyzer::detectTrendReversal(
    const QVector<Analysis::KLine>& klines,
    const Trend& currentTrend)
{
    if (klines.size() < 10) return false;

    // 简化的反转检测：
    // 上升趋势：出现更低的高点和更低的低点
    // 下降趋势：出现更高的低点和更高的高点

    int recentBars = 10;
    auto recentExtremums = identifyExtremums(klines.mid(klines.size() - recentBars));

    if (recentExtremums.size() < 2) return false;

    int higherHighs = 0;
    int lowerHighs = 0;
    int higherLows = 0;
    int lowerLows = 0;

    for (const auto& ext : recentExtremums) {
        switch (ext.type) {
            case ExtremumType::HigherHigh: higherHighs++; break;
            case ExtremumType::LowerHigh: lowerHighs++; break;
            case ExtremumType::HigherLow: higherLows++; break;
            case ExtremumType::LowerLow: lowerLows++; break;
            default: break;
        }
    }

    if (currentTrend.direction == TrendDirection::Upward) {
        // 上升趋势中，出现更低的高点和更低的低点 = 反转信号
        return lowerHighs > 0 && lowerLows > 0;
    } else if (currentTrend.direction == TrendDirection::Downward) {
        // 下降趋势中，出现更高的低点和更高的高点 = 反转信号
        return higherLows > 0 && higherHighs > 0;
    }

    return false;
}

bool DowTheoryAnalyzer::confirmTrend(
    const QVector<Analysis::KLine>& klines,
    TrendDirection direction,
    int confirmationBars)
{
    if (klines.size() < confirmationBars) return false;

    int confirmCount = 0;

    for (int i = klines.size() - confirmationBars; i < klines.size(); ++i) {
        if (direction == TrendDirection::Upward && klines[i].close > klines[i].open) {
            confirmCount++;
        } else if (direction == TrendDirection::Downward && klines[i].close < klines[i].open) {
            confirmCount++;
        }
    }

    return confirmCount >= confirmationBars / 2;
}

double DowTheoryAnalyzer::calculateTrendStrength(const Trend& trend)
{
    double strength = 0.0;

    // 基于价格变化幅度
    double changePercent = qAbs(trend.priceChangePercent());
    strength += qMin(changePercent / 10.0, 1.0) * 40; // 最多40分

    // 基于趋势持续时间
    qint64 duration = trend.duration();
    double durationScore = qMin(duration / 86400.0 / 30.0, 1.0); // 最多30天
    strength += durationScore * 30; // 最多30分

    // 基于极值点确认
    int confirmedCount = 0;
    for (const auto& ext : trend.extremums) {
        if (ext.isConfirmed) confirmedCount++;
    }
    if (!trend.extremums.isEmpty()) {
        strength += (confirmedCount * 1.0 / trend.extremums.size()) * 30; // 最多30分
    }

    return strength;
}

TrendDirection DowTheoryAnalyzer::currentTrend() const
{
    return d->result.primaryTrend.direction;
}

QVector<Analysis::UnifiedSignal> DowTheoryAnalyzer::generateSignals(const DowTheoryResult& result)
{
    QVector<Analysis::UnifiedSignal> signals;

    // 主要趋势信号
    Analysis::UnifiedSignal primarySignal;
    primarySignal.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    primarySignal.source = Analysis::TheoryType::DowTheory;
    primarySignal.time = QDateTime::currentDateTime();
    primarySignal.confidence = result.primaryTrend.strength;

    switch (result.primaryTrend.direction) {
        case TrendDirection::Upward:
            primarySignal.direction = Analysis::SignalDirection::Bullish;
            primarySignal.description = QStringLiteral("主要趋势向上");
            break;
        case TrendDirection::Downward:
            primarySignal.direction = Analysis::SignalDirection::Bearish;
            primarySignal.description = QStringLiteral("主要趋势向下");
            break;
        default:
            primarySignal.direction = Analysis::SignalDirection::Neutral;
            primarySignal.description = QStringLiteral("主要趋势横盘");
            break;
    }

    // 设置信号强度
    if (primarySignal.confidence >= 70) {
        primarySignal.strength = Analysis::SignalStrength::Strong;
    } else if (primarySignal.confidence >= 50) {
        primarySignal.strength = Analysis::SignalStrength::Moderate;
    } else {
        primarySignal.strength = Analysis::SignalStrength::Weak;
    }

    signals.append(primarySignal);

    // 趋势反转信号
    if (result.hasTrendReversal) {
        Analysis::UnifiedSignal reversalSignal;
        reversalSignal.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        reversalSignal.source = Analysis::TheoryType::DowTheory;
        reversalSignal.time = QDateTime::currentDateTime();
        reversalSignal.direction = result.primaryTrend.direction == TrendDirection::Upward
            ? Analysis::SignalDirection::Bearish
            : Analysis::SignalDirection::Bullish;
        reversalSignal.strength = Analysis::SignalStrength::Strong;
        reversalSignal.confidence = 75.0;
        reversalSignal.description = QStringLiteral("检测到趋势反转信号");

        signals.append(reversalSignal);
    }

    return signals;
}

} // namespace DowTheory
} // namespace WealthPilot
