/**
 * @file VolumePatternAnalyzer.cpp
 * @brief 量价形态分析器实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "VolumePatternAnalyzer.h"
#include <QDebug>
#include <QtMath>
#include <QUuid>

namespace WealthPilot {
namespace VolumePattern {

struct VolumePatternAnalyzer::Impl {
    QVector<Analysis::KLine> klines;
    VolumePatternResult result;
    QVector<Analysis::UnifiedSignal> signals;

    // 参数
    int volumeMAPeriod = 20;        // 成交量均线周期
    double volumeSpikeThreshold = 2.0; // 量能突增阈值
    double priceChangeThreshold = 0.03; // 价格变化阈值
};

VolumePatternAnalyzer::VolumePatternAnalyzer(QObject* parent)
    : IAnalyzer(parent)
    , d(std::make_unique<Impl>())
{
}

VolumePatternAnalyzer::~VolumePatternAnalyzer() = default;

Analysis::AnalysisResult VolumePatternAnalyzer::analyze(const QVector<Analysis::KLine>& klines)
{
    Analysis::AnalysisResult result;
    result.analysisTime = QDateTime::currentDateTime();

    if (klines.size() < 20) {
        result.isValid = false;
        result.errorMessage = QStringLiteral("K线数据不足");
        return result;
    }

    d->klines = klines;

    // 1. 计算平均成交量
    qint64 totalVolume = 0;
    int count = qMin(d->volumeMAPeriod, klines.size());
    for (int i = klines.size() - count; i < klines.size(); ++i) {
        totalVolume += klines[i].volume;
    }
    d->result.avgVolume = totalVolume / count;

    // 2. 分析每根K线的量价特征
    QVector<VolumePriceBar> bars;
    for (int i = 0; i < klines.size(); ++i) {
        auto bar = analyzeBar(klines[i], d->result.avgVolume);

        // 判断量价组合形态
        if (i > 0) {
            bar.pattern = determinePattern(bar, &bars.last());
        }

        bars.append(bar);
    }
    d->result.bars = bars;

    // 3. 计算OBV
    auto obvData = calculateOBV(klines);
    if (!obvData.isEmpty()) {
        d->result.obv = obvData.last();
    }

    // 4. 识别形态组合
    d->result.formations = identifyFormations(bars);

    // 5. 检测背离
    d->result.hasDivergence = detectDivergence(klines, obvData);

    // 6. 检测突破
    d->result.hasBreakout = detectBreakout(klines);

    d->result.isValid = true;
    d->result.analysisTime = QDateTime::currentDateTime();

    // 7. 生成信号
    d->signals = generateSignals(d->result);
    result.signals = d->signals;
    result.isValid = true;

    emit analysisCompleted(result);

    return result;
}

void VolumePatternAnalyzer::clear()
{
    d->klines.clear();
    d->result = VolumePatternResult();
    d->signals.clear();
}

QVector<Analysis::UnifiedSignal> VolumePatternAnalyzer::currentSignals() const
{
    return d->signals;
}

const VolumePatternResult& VolumePatternAnalyzer::volumeResult() const
{
    return d->result;
}

QVector<OBVData> VolumePatternAnalyzer::calculateOBV(
    const QVector<Analysis::KLine>& klines, int maPeriod)
{
    QVector<OBVData> result;

    if (klines.isEmpty()) return result;

    double obv = 0.0;
    QVector<double> obvValues;

    for (int i = 0; i < klines.size(); ++i) {
        OBVData data;
        data.time = klines[i].time;

        // 计算OBV
        if (i == 0) {
            obv = klines[i].volume;
        } else {
            if (klines[i].close > klines[i - 1].close) {
                obv += klines[i].volume;
            } else if (klines[i].close < klines[i - 1].close) {
                obv -= klines[i].volume;
            }
        }

        data.obv = obv;
        obvValues.append(obv);

        // 计算OBV均线
        if (obvValues.size() >= maPeriod) {
            double sum = 0;
            for (int j = obvValues.size() - maPeriod; j < obvValues.size(); ++j) {
                sum += obvValues[j];
            }
            data.obvMA = sum / maPeriod;
        }

        result.append(data);
    }

    return result;
}

QVector<VolumePriceFormation> VolumePatternAnalyzer::identifyFormations(
    const QVector<VolumePriceBar>& bars)
{
    QVector<VolumePriceFormation> formations;

    if (bars.size() < 3) return formations;

    // 寻找连续的量价形态
    for (int i = 2; i < bars.size(); ++i) {
        const auto& bar = bars[i];

        // 检测放量突破
        if (bar.pattern == VolumePricePattern::VolumeBreakout) {
            VolumePriceFormation formation;
            formation.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            formation.pattern = VolumePricePattern::VolumeBreakout;
            formation.startTime = bar.time;
            formation.endTime = bar.time;
            formation.startPrice = bar.open;
            formation.endPrice = bar.close;
            formation.barCount = 1;
            formation.totalVolume = bar.volume;
            formation.confidence = 70.0;
            formation.description = QStringLiteral("放量突破");
            formation.bars.append(bar);

            formations.append(formation);
        }

        // 检测量价背离
        if (bar.pattern == VolumePricePattern::TopDivergence ||
            bar.pattern == VolumePricePattern::BottomDivergence) {
            VolumePriceFormation formation;
            formation.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            formation.pattern = bar.pattern;
            formation.startTime = bars[i - 2].time;
            formation.endTime = bar.time;
            formation.startPrice = bars[i - 2].open;
            formation.endPrice = bar.close;
            formation.barCount = 3;
            formation.confidence = 65.0;
            formation.description = bar.pattern == VolumePricePattern::TopDivergence
                ? QStringLiteral("顶部背离")
                : QStringLiteral("底部背离");

            for (int j = i - 2; j <= i; ++j) {
                formation.bars.append(bars[j]);
                formation.totalVolume += bars[j].volume;
            }

            formations.append(formation);
        }
    }

    return formations;
}

bool VolumePatternAnalyzer::detectDivergence(
    const QVector<Analysis::KLine>& klines,
    const QVector<OBVData>& obvData)
{
    if (klines.size() < 10 || obvData.size() < 10) return false;

    int lookback = 10;

    // 检查最近10根K线
    double priceHigh = 0, priceLow = std::numeric_limits<double>::max();
    double obvHigh = 0, obvLow = std::numeric_limits<double>::max();
    int priceHighIdx = -1, priceLowIdx = -1;
    int obvHighIdx = -1, obvLowIdx = -1;

    for (int i = klines.size() - lookback; i < klines.size(); ++i) {
        // 价格极值
        if (klines[i].high > priceHigh) {
            priceHigh = klines[i].high;
            priceHighIdx = i;
        }
        if (klines[i].low < priceLow) {
            priceLow = klines[i].low;
            priceLowIdx = i;
        }

        // OBV极值
        int obvIdx = i - (klines.size() - obvData.size());
        if (obvIdx >= 0 && obvIdx < obvData.size()) {
            if (obvData[obvIdx].obv > obvHigh) {
                obvHigh = obvData[obvIdx].obv;
                obvHighIdx = obvIdx;
            }
            if (obvData[obvIdx].obv < obvLow) {
                obvLow = obvData[obvIdx].obv;
                obvLowIdx = obvIdx;
            }
        }
    }

    // 顶部背离：价格创新高，OBV未创新高
    if (priceHighIdx >= 0 && obvHighIdx >= 0 && priceHighIdx != obvHighIdx) {
        return true;
    }

    // 底部背离：价格创新低，OBV未创新低
    if (priceLowIdx >= 0 && obvLowIdx >= 0 && priceLowIdx != obvLowIdx) {
        return true;
    }

    return false;
}

bool VolumePatternAnalyzer::detectBreakout(const QVector<Analysis::KLine>& klines)
{
    if (klines.size() < 20) return false;

    const auto& lastBar = klines.last();

    // 计算最近20根K线的最高价和最低价
    double highest = 0, lowest = std::numeric_limits<double>::max();
    for (int i = klines.size() - 20; i < klines.size() - 1; ++i) {
        highest = qMax(highest, klines[i].high);
        lowest = qMin(lowest, klines[i].low);
    }

    // 判断是否突破
    bool isBreakout = lastBar.close > highest && lastBar.volume > d->result.avgVolume * 1.5;

    return isBreakout;
}

VolumePriceBar VolumePatternAnalyzer::analyzeBar(const Analysis::KLine& kline, double avgVolume)
{
    VolumePriceBar bar;
    bar.time = kline.time;
    bar.open = kline.open;
    bar.high = kline.high;
    bar.low = kline.low;
    bar.close = kline.close;
    bar.volume = kline.volume;

    bar.volumeShape = determineVolumeShape(kline.volume, avgVolume);
    bar.priceShape = determinePriceShape(kline);
    bar.volumeRatio = calculateVolumeRatio(kline.volume, avgVolume);
    bar.priceChange = kline.close > 0 ? (kline.close - kline.open) / kline.open : 0;
    bar.amplitude = kline.low > 0 ? (kline.high - kline.low) / kline.low : 0;

    return bar;
}

VolumeShape VolumePatternAnalyzer::determineVolumeShape(qint64 volume, double avgVolume)
{
    if (avgVolume <= 0) return VolumeShape::Stable;

    double ratio = volume / avgVolume;

    if (ratio >= d->volumeSpikeThreshold) {
        return VolumeShape::VolumeSpike;
    } else if (ratio >= 1.5) {
        return VolumeShape::Increasing;
    } else if (ratio <= 0.5) {
        return VolumeShape::Decreasing;
    } else {
        return VolumeShape::Stable;
    }
}

PriceShape VolumePatternAnalyzer::determinePriceShape(const Analysis::KLine& kline)
{
    double change = kline.close > 0 ? (kline.close - kline.open) / kline.open : 0;
    double amplitude = kline.low > 0 ? (kline.high - kline.low) / kline.low : 0;

    if (change >= d->priceChangeThreshold) {
        return PriceShape::BigUp;
    } else if (change <= -d->priceChangeThreshold) {
        return PriceShape::BigDown;
    } else if (change > 0.005) {
        return PriceShape::SmallUp;
    } else if (change < -0.005) {
        return PriceShape::SmallDown;
    } else {
        return PriceShape::Flat;
    }
}

VolumePricePattern VolumePatternAnalyzer::determinePattern(
    const VolumePriceBar& bar,
    const VolumePriceBar* prevBar)
{
    if (!prevBar) return VolumePricePattern::Unknown;

    bool priceUp = bar.close > prevBar->close;
    bool volumeUp = bar.volume > prevBar->volume;

    // 价涨量增
    if (priceUp && volumeUp) {
        return VolumePricePattern::PriceUpVolumeUp;
    }

    // 价涨量缩
    if (priceUp && !volumeUp) {
        return VolumePricePattern::PriceUpVolumeDown;
    }

    // 价跌量增
    if (!priceUp && volumeUp) {
        return VolumePricePattern::PriceDownVolumeUp;
    }

    // 价跌量缩
    if (!priceUp && !volumeUp) {
        return VolumePricePattern::PriceDownVolumeDown;
    }

    // 放量突破
    if (bar.volumeShape == VolumeShape::VolumeSpike && bar.priceShape == PriceShape::BigUp) {
        return VolumePricePattern::VolumeBreakout;
    }

    return VolumePricePattern::Unknown;
}

double VolumePatternAnalyzer::calculateVolumeRatio(qint64 volume, double avgVolume)
{
    if (avgVolume <= 0) return 1.0;
    return volume / avgVolume;
}

QVector<Analysis::UnifiedSignal> VolumePatternAnalyzer::generateSignals(const VolumePatternResult& result)
{
    QVector<Analysis::UnifiedSignal> signals;

    if (result.bars.isEmpty()) return signals;

    const auto& lastBar = result.bars.last();

    Analysis::UnifiedSignal signal;
    signal.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    signal.source = Analysis::TheoryType::VolumePattern;
    signal.time = QDateTime::currentDateTime();
    signal.price = lastBar.close;

    // 根据量价形态设置信号
    switch (lastBar.pattern) {
        case VolumePricePattern::PriceUpVolumeUp:
            signal.direction = Analysis::SignalDirection::Bullish;
            signal.strength = Analysis::SignalStrength::Strong;
            signal.confidence = 75.0;
            signal.description = QStringLiteral("价涨量增，健康上涨");
            break;

        case VolumePricePattern::PriceUpVolumeDown:
            signal.direction = Analysis::SignalDirection::Neutral;
            signal.strength = Analysis::SignalStrength::Weak;
            signal.confidence = 50.0;
            signal.description = QStringLiteral("价涨量缩，上涨乏力");
            break;

        case VolumePricePattern::PriceDownVolumeUp:
            signal.direction = Analysis::SignalDirection::Bullish;
            signal.strength = Analysis::SignalStrength::Moderate;
            signal.confidence = 65.0;
            signal.description = QStringLiteral("价跌量增，可能见底");
            break;

        case VolumePricePattern::PriceDownVolumeDown:
            signal.direction = Analysis::SignalDirection::Bearish;
            signal.strength = Analysis::SignalStrength::Weak;
            signal.confidence = 55.0;
            signal.description = QStringLiteral("价跌量缩，下跌动能减弱");
            break;

        case VolumePricePattern::VolumeBreakout:
            signal.direction = Analysis::SignalDirection::Bullish;
            signal.strength = Analysis::SignalStrength::VeryStrong;
            signal.confidence = 85.0;
            signal.description = QStringLiteral("放量突破");
            break;

        case VolumePricePattern::BottomDivergence:
            signal.direction = Analysis::SignalDirection::Bullish;
            signal.strength = Analysis::SignalStrength::Strong;
            signal.confidence = 70.0;
            signal.description = QStringLiteral("底部背离，可能反转");
            break;

        case VolumePricePattern::TopDivergence:
            signal.direction = Analysis::SignalDirection::Bearish;
            signal.strength = Analysis::SignalStrength::Strong;
            signal.confidence = 70.0;
            signal.description = QStringLiteral("顶部背离，注意风险");
            break;

        default:
            signal.direction = Analysis::SignalDirection::Neutral;
            signal.strength = Analysis::SignalStrength::Weak;
            signal.confidence = 40.0;
            signal.description = QStringLiteral("量价形态中性");
            break;
    }

    // 添加元数据
    signal.metadata["volumeRatio"] = lastBar.volumeRatio;
    signal.metadata["priceChange"] = lastBar.priceChange;
    signal.metadata["amplitude"] = lastBar.amplitude;

    signals.append(signal);

    return signals;
}

} // namespace VolumePattern
} // namespace WealthPilot
