/**
 * @file ElliottWaveAnalyzer.cpp
 * @brief 波浪理论分析器实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ElliottWaveAnalyzer.h"
#include <QDebug>
#include <QtMath>
#include <QUuid>

namespace WealthPilot {
namespace ElliottWave {

// ============================================================================
// 内部实现
// ============================================================================

struct ElliottWaveAnalyzer::Impl {
    QVector<Analysis::KLine> klines;        // K线缓存
    ElliottWaveResult result;               // 分析结果
    QVector<Analysis::UnifiedSignal> currentSignalList; // 当前信号

    // 波浪识别参数
    int minWaveBars = 3;                    // 最小波浪K线数
    double minWaveAmplitude = 0.01;         // 最小波浪振幅比例
    double fibTolerance = 0.02;             // 斐波那契容差

    Impl() = default;
};

// ============================================================================
// 构造/析构
// ============================================================================

ElliottWaveAnalyzer::ElliottWaveAnalyzer(QObject* parent)
    : IAnalyzer(parent)
    , d(std::make_unique<Impl>())
{
}

ElliottWaveAnalyzer::~ElliottWaveAnalyzer() = default;

// ============================================================================
// IAnalyzer 接口实现
// ============================================================================

Analysis::AnalysisResult ElliottWaveAnalyzer::analyze(const QVector<Analysis::KLine>& klines)
{
    Analysis::AnalysisResult result;
    result.symbol = "Unknown";
    result.analysisTime = QDateTime::currentDateTime();

    if (klines.size() < 50) {
        result.isValid = false;
        result.errorMessage = QStringLiteral("K线数据不足，至少需要50根K线");
        return result;
    }

    // 保存K线数据
    d->klines = klines;

    // 识别波浪
    auto waves = identifyWaves(klines);

    if (waves.isEmpty()) {
        result.isValid = false;
        result.errorMessage = QStringLiteral("未能识别出有效的波浪结构");
        return result;
    }

    // 构建波浪计数方案
    WaveCount count;
    count.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    count.analysisTime = QDateTime::currentDateTime();
    count.waves = waves;
    count.confidence = checkFibonacciRatios(waves);

    // 确定当前波浪位置
    if (!waves.isEmpty()) {
        count.currentWaveIndex = waves.size() - 1;
        count.currentPattern = determinePattern(waves);

        // 预测下一波浪
        const auto& lastWave = waves.last();
        if (lastWave.number == WaveNumber::Wave1) {
            count.nextExpectedWave = WaveNumber::Wave2;
        } else if (lastWave.number == WaveNumber::Wave2) {
            count.nextExpectedWave = WaveNumber::Wave3;
        } else if (lastWave.number == WaveNumber::Wave3) {
            count.nextExpectedWave = WaveNumber::Wave4;
        } else if (lastWave.number == WaveNumber::Wave4) {
            count.nextExpectedWave = WaveNumber::Wave5;
        } else if (lastWave.number == WaveNumber::Wave5) {
            count.nextExpectedWave = WaveNumber::WaveA;
        } else if (lastWave.number == WaveNumber::WaveA) {
            count.nextExpectedWave = WaveNumber::WaveB;
        } else if (lastWave.number == WaveNumber::WaveB) {
            count.nextExpectedWave = WaveNumber::WaveC;
        }
    }

    // 保存结果
    d->result.waveCounts.clear();
    d->result.waveCounts.append(count);
    d->result.preferredCountIndex = 0;
    d->result.isValid = true;

    // 生成信号
    d->currentSignalList = generateSignals(count);
    result.generatedSignals = d->currentSignalList;
    result.isValid = true;

    emit analysisCompleted(result);

    return result;
}

Analysis::AnalysisResult ElliottWaveAnalyzer::update(const QVector<Analysis::KLine>& newKlines)
{
    // 增量更新：添加新K线并重新分析
    d->klines.append(newKlines);
    return analyze(d->klines);
}

void ElliottWaveAnalyzer::clear()
{
    d->klines.clear();
    d->result = ElliottWaveResult();
    d->currentSignalList.clear();
}

QVector<Analysis::UnifiedSignal> ElliottWaveAnalyzer::currentSignals() const
{
    return d->currentSignalList;
}

// ============================================================================
// 波浪分析接口
// ============================================================================

const ElliottWaveResult& ElliottWaveAnalyzer::waveResult() const
{
    return d->result;
}

const WaveCount* ElliottWaveAnalyzer::currentWaveCount() const
{
    return d->result.preferredCount();
}

// ============================================================================
// 波浪识别算法
// ============================================================================

QVector<Wave> ElliottWaveAnalyzer::identifyWaves(const QVector<Analysis::KLine>& klines)
{
    QVector<Wave> waves;

    // 1. 找出极值点
    auto extremes = findExtremes(klines, d->minWaveBars);
    if (extremes.size() < 3) {
        return waves;
    }

    // 2. 根据极值点构建波浪
    waves = buildWavesFromExtremes(klines, extremes);

    // 3. 标记波浪编号
    if (waves.size() >= 5) {
        // 假设是推动浪结构
        for (int i = 0; i < waves.size(); ++i) {
            if (i % 2 == 0) {
                // 推动浪：1, 3, 5
                waves[i].type = WaveType::Impulse;
                waves[i].direction = (i % 4 == 0) ? WaveDirection::Up : WaveDirection::Down;

                if (i == 0) waves[i].number = WaveNumber::Wave1;
                else if (i == 2) waves[i].number = WaveNumber::Wave3;
                else if (i == 4) waves[i].number = WaveNumber::Wave5;
            } else {
                // 调整浪：2, 4
                waves[i].type = WaveType::Corrective;
                waves[i].direction = (i % 4 == 1) ? WaveDirection::Down : WaveDirection::Up;

                if (i == 1) waves[i].number = WaveNumber::Wave2;
                else if (i == 3) waves[i].number = WaveNumber::Wave4;
            }
        }
    } else if (waves.size() >= 3) {
        // 可能是调整浪结构 A-B-C
        for (int i = 0; i < waves.size(); ++i) {
            waves[i].type = (i % 2 == 0) ? WaveType::Impulse : WaveType::Corrective;

            if (i == 0) {
                waves[i].number = WaveNumber::WaveA;
                waves[i].direction = WaveDirection::Down;
            } else if (i == 1) {
                waves[i].number = WaveNumber::WaveB;
                waves[i].direction = WaveDirection::Up;
            } else if (i == 2) {
                waves[i].number = WaveNumber::WaveC;
                waves[i].direction = WaveDirection::Down;
            }
        }
    }

    return waves;
}

QVector<int> ElliottWaveAnalyzer::findExtremes(const QVector<Analysis::KLine>& klines, int minBars)
{
    QVector<int> extremes;

    if (klines.size() < minBars * 2 + 1) {
        return extremes;
    }

    // 使用滑动窗口识别局部极值
    for (int i = minBars; i < klines.size() - minBars; ++i) {
        bool isHigh = true;
        bool isLow = true;

        // 检查是否为局部高点
        for (int j = i - minBars; j <= i + minBars; ++j) {
            if (j == i) continue;
            if (klines[j].high >= klines[i].high) {
                isHigh = false;
            }
            if (klines[j].low <= klines[i].low) {
                isLow = false;
            }
        }

        // 添加极值点（避免连续同类型极值）
        if (isHigh || isLow) {
            if (!extremes.isEmpty()) {
                int lastIdx = extremes.last();
                bool lastWasHigh = klines[lastIdx].high > klines[lastIdx].low;

                // 如果上一个极值是高点，这次必须是低点，反之亦然
                if ((isHigh && lastWasHigh) || (isLow && !lastWasHigh)) {
                    // 选择更极端的点
                    if (isHigh) {
                        if (klines[i].high > klines[lastIdx].high) {
                            extremes.removeLast();
                            extremes.append(i);
                        }
                    } else {
                        if (klines[i].low < klines[lastIdx].low) {
                            extremes.removeLast();
                            extremes.append(i);
                        }
                    }
                    continue;
                }
            }
            extremes.append(i);
        }
    }

    return extremes;
}

QVector<Wave> ElliottWaveAnalyzer::buildWavesFromExtremes(
    const QVector<Analysis::KLine>& klines,
    const QVector<int>& extremes)
{
    QVector<Wave> waves;

    for (int i = 0; i < extremes.size() - 1; ++i) {
        int startIdx = extremes[i];
        int endIdx = extremes[i + 1];

        Wave wave;
        wave.id = i;
        wave.startTime = klines[startIdx].time;
        wave.endTime = klines[endIdx].time;
        wave.startPrice = klines[startIdx].close;
        wave.endPrice = klines[endIdx].close;
        wave.highPrice = 0;
        wave.lowPrice = std::numeric_limits<double>::max();
        wave.barCount = endIdx - startIdx + 1;

        // 计算波浪内的最高最低价
        for (int j = startIdx; j <= endIdx; ++j) {
            wave.highPrice = qMax(wave.highPrice, klines[j].high);
            wave.lowPrice = qMin(wave.lowPrice, klines[j].low);
        }

        // 计算振幅
        wave.amplitude = wave.highPrice - wave.lowPrice;

        // 判断方向
        wave.direction = (klines[endIdx].close > klines[startIdx].close)
            ? WaveDirection::Up : WaveDirection::Down;

        // 设置默认类型和级别
        wave.type = WaveType::Impulse;
        wave.degree = WaveDegree::Minor;

        waves.append(wave);
    }

    return waves;
}

bool ElliottWaveAnalyzer::validateWaveRules(const QVector<Wave>& waves)
{
    if (waves.size() < 5) {
        return false;
    }

    // 验证推动浪规则
    // 规则1: 浪2不能回撤超过浪1的起点
    // 规则2: 浪3不能是最短的
    // 规则3: 浪4不能进入浪1的价格区间

    const Wave& w1 = waves[0];
    const Wave& w2 = waves[1];
    const Wave& w3 = waves[2];
    const Wave& w4 = waves[3];
    const Wave& w5 = waves[4];

    return validateImpulseRules(w1, w2, w3, w4, w5);
}

bool ElliottWaveAnalyzer::validateImpulseRules(
    const Wave& w1, const Wave& w2, const Wave& w3,
    const Wave& w4, const Wave& w5)
{
    // 规则1: 浪2回撤不能超过浪1起点
    if (w2.direction == WaveDirection::Down) {
        if (w2.lowPrice < w1.startPrice) {
            return false;
        }
    }

    // 规则2: 浪3不能是最短的
    double w1Size = qAbs(w1.endPrice - w1.startPrice);
    double w3Size = qAbs(w3.endPrice - w3.startPrice);
    double w5Size = qAbs(w5.endPrice - w5.startPrice);

    if (w3Size < w1Size && w3Size < w5Size) {
        return false;
    }

    // 规则3: 浪4不能进入浪1区间
    if (w4.direction == WaveDirection::Down) {
        if (w4.lowPrice < w1.highPrice) {
            return false;
        }
    }

    return true;
}

bool ElliottWaveAnalyzer::validateCorrectiveRules(
    const Wave& wA, const Wave& wB, const Wave& wC)
{
    // 调整浪规则相对宽松
    // 主要检查比例关系

    Q_UNUSED(wA)
    Q_UNUSED(wB)
    Q_UNUSED(wC)

    return true;
}

WavePattern ElliottWaveAnalyzer::determinePattern(const QVector<Wave>& waves)
{
    if (waves.size() >= 5) {
        // 检查是否为推动浪
        bool allImpulse = true;
        for (int i = 0; i < 5; ++i) {
            if (i % 2 == 0 && waves[i].type != WaveType::Impulse) {
                allImpulse = false;
                break;
            }
        }
        if (allImpulse && validateWaveRules(waves)) {
            return WavePattern::Impulse5;
        }
    }

    if (waves.size() >= 3) {
        // 检查是否为锯齿形
        return WavePattern::ZigZag;
    }

    return WavePattern::Unknown;
}

WaveDegree ElliottWaveAnalyzer::calculateDegree(const Wave& wave, const Wave& parentWave)
{
    Q_UNUSED(wave)
    Q_UNUSED(parentWave)
    // 简化实现：根据波浪大小判断级别
    return WaveDegree::Minor;
}

QVector<double> ElliottWaveAnalyzer::calculateFibonacciTargets(const QVector<Wave>& waves)
{
    QVector<double> targets;

    if (waves.isEmpty()) {
        return targets;
    }

    const Wave& lastWave = waves.last();

    // 根据当前波浪计算目标位
    if (lastWave.number == WaveNumber::Wave1) {
        // 浪2回撤目标
        targets.append(Fibonacci::calculateRetracement(
            lastWave.startPrice, lastWave.endPrice, Fibonacci::RETRACEMENT_382));
        targets.append(Fibonacci::calculateRetracement(
            lastWave.startPrice, lastWave.endPrice, Fibonacci::RETRACEMENT_500));
        targets.append(Fibonacci::calculateRetracement(
            lastWave.startPrice, lastWave.endPrice, Fibonacci::RETRACEMENT_618));
    } else if (lastWave.number == WaveNumber::Wave3) {
        // 浪4回撤目标
        targets.append(Fibonacci::calculateRetracement(
            lastWave.startPrice, lastWave.endPrice, Fibonacci::RETRACEMENT_382));
        targets.append(Fibonacci::calculateRetracement(
            lastWave.startPrice, lastWave.endPrice, Fibonacci::RETRACEMENT_500));
    }

    return targets;
}

WaveNumber ElliottWaveAnalyzer::identifyCurrentPosition(
    const QVector<Analysis::KLine>& klines,
    const QVector<Wave>& waves)
{
    Q_UNUSED(klines)

    if (waves.isEmpty()) {
        return WaveNumber::Unknown;
    }

    return waves.last().number;
}

double ElliottWaveAnalyzer::checkFibonacciRatios(const QVector<Wave>& waves)
{
    if (waves.size() < 3) {
        return 0.5;
    }

    double score = 0.0;
    int checks = 0;

    // 检查浪2回撤比例
    if (waves.size() >= 2) {
        double w1Size = qAbs(waves[0].endPrice - waves[0].startPrice);
        double w2Size = qAbs(waves[1].endPrice - waves[1].startPrice);

        if (w1Size > 0) {
            double retracement = w2Size / w1Size;

            // 理想回撤比例在 38.2% - 61.8%
            if (retracement >= 0.382 && retracement <= 0.618) {
                score += 1.0;
            } else if (retracement >= 0.236 && retracement <= 0.786) {
                score += 0.5;
            }
            checks++;
        }
    }

    // 检查浪3与浪1的比例
    if (waves.size() >= 3) {
        double w1Size = qAbs(waves[0].endPrice - waves[0].startPrice);
        double w3Size = qAbs(waves[2].endPrice - waves[2].startPrice);

        if (w1Size > 0) {
            double ratio = w3Size / w1Size;

            // 理想比例在 1.618 或 2.618
            if (qAbs(ratio - 1.618) < 0.2 || qAbs(ratio - 2.618) < 0.3) {
                score += 1.0;
            } else if (ratio >= 1.0 && ratio <= 3.0) {
                score += 0.5;
            }
            checks++;
        }
    }

    return checks > 0 ? score / checks : 0.5;
}

QVector<Analysis::UnifiedSignal> ElliottWaveAnalyzer::generateSignals(const WaveCount& count)
{
    QVector<Analysis::UnifiedSignal> result;

    if (count.waves.isEmpty()) {
        return result;
    }

    const Wave& lastWave = count.waves.last();

    // 根据波浪位置生成信号
    Analysis::UnifiedSignal signal;
    signal.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    signal.source = Analysis::TheoryType::ElliottWave;
    signal.time = QDateTime::currentDateTime();
    signal.price = lastWave.endPrice;
    signal.confidence = calculateConfidence(count, lastWave);

    // 设置信号方向
    if (lastWave.number == WaveNumber::Wave2 || lastWave.number == WaveNumber::Wave4) {
        // 调整浪结束，预期推动浪
        signal.direction = lastWave.isUp() ? Analysis::SignalDirection::Bearish
                                           : Analysis::SignalDirection::Bullish;
        signal.description = QStringLiteral("调整浪结束，预期%1")
            .arg(count.nextExpectedWave == WaveNumber::Wave3 ? "浪3上涨" : "浪5上涨");
    } else if (lastWave.number == WaveNumber::Wave3) {
        // 浪3是最强的推动浪
        signal.direction = Analysis::SignalDirection::Bullish;
        signal.description = QStringLiteral("浪3进行中，强势上涨");
    } else if (lastWave.number == WaveNumber::Wave5) {
        // 浪5可能是顶部
        signal.direction = Analysis::SignalDirection::Neutral;
        signal.description = QStringLiteral("浪5完成，注意顶部风险");
    } else if (lastWave.number == WaveNumber::WaveC) {
        // C浪结束可能是底部
        signal.direction = Analysis::SignalDirection::Bullish;
        signal.description = QStringLiteral("C浪调整结束，可能见底");
    }

    // 设置信号强度
    if (signal.confidence >= 80) {
        signal.strength = Analysis::SignalStrength::VeryStrong;
    } else if (signal.confidence >= 60) {
        signal.strength = Analysis::SignalStrength::Strong;
    } else if (signal.confidence >= 40) {
        signal.strength = Analysis::SignalStrength::Moderate;
    } else {
        signal.strength = Analysis::SignalStrength::Weak;
    }

    // 添加元数据
    signal.metadata["waveNumber"] = static_cast<int>(lastWave.number);
    signal.metadata["wavePattern"] = static_cast<int>(count.currentPattern);
    signal.metadata["fibonacciScore"] = count.confidence;

    result.append(signal);

    return result;
}

double ElliottWaveAnalyzer::calculateConfidence(const WaveCount& count, const Wave& wave)
{
    double confidence = 50.0;

    // 基于斐波那契比例得分
    confidence += count.confidence * 30;

    // 基于波浪规则验证
    if (validateWaveRules(count.waves)) {
        confidence += 20;
    }

    // 限制在0-100范围
    return qBound(0.0, confidence, 100.0);
}

} // namespace ElliottWave
} // namespace WealthPilot
