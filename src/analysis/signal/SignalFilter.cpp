/**
 * @file SignalFilter.cpp
 * @brief 多层过滤信号系统实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "SignalFilter.h"
#include <QDebug>
#include <QUuid>
#include <QtMath>

namespace WealthPilot {
namespace Analysis {

struct SignalFilter::Impl {
    SignalFilterConfig config;
    QMap<QString, QVariant> stats;

    // 统计计数
    int totalSignalsProcessed = 0;
    int signalsPassed = 0;
    int signalsRejected = 0;

    Impl() = default;
};

SignalFilter::SignalFilter(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
}

SignalFilter::~SignalFilter() = default;

void SignalFilter::setConfig(const SignalFilterConfig& config)
{
    d->config = config;
}

const SignalFilterConfig& SignalFilter::config() const
{
    return d->config;
}

CompositeSignal SignalFilter::filter(const QVector<UnifiedSignal>& signals)
{
    CompositeSignal result;
    result.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    result.time = QDateTime::currentDateTime();

    d->totalSignalsProcessed += signals.size();

    if (signals.isEmpty()) {
        return result;
    }

    // 1. 按理论分组
    auto groups = groupByTheory(signals);

    // 2. 过滤弱信号
    QVector<UnifiedSignal> strongSignals;
    for (const auto& signal : signals) {
        // 强度过滤
        if (signal.strength < d->config.minStrength) {
            d->signalsRejected++;
            continue;
        }

        // 置信度过滤
        if (signal.confidence < d->config.minConfidence) {
            d->signalsRejected++;
            continue;
        }

        strongSignals.append(signal);
        d->signalsPassed++;
    }

    if (strongSignals.isEmpty()) {
        return result;
    }

    // 3. 检查理论数量
    auto strongGroups = groupByTheory(strongSignals);
    result.theoryCount = strongGroups.size();

    if (result.theoryCount < d->config.minTheoryCount) {
        // 理论数量不足，降低信号质量
        result.confidence = 30.0;
        result.direction = SignalDirection::Neutral;
        result.description = QStringLiteral("支持理论数量不足");
        return result;
    }

    // 4. 计算综合方向
    result.direction = determineCompositeDirection(strongSignals);

    // 5. 计算综合置信度
    result.confidence = calculateCompositeConfidence(strongSignals);

    // 6. 设置价格和时间
    result.price = strongSignals.last().price;
    result.symbol = strongSignals.last().symbol;
    result.sourceSignals = strongSignals;

    // 7. 评估风险收益比
    double riskReward = evaluateRiskReward(result);
    result.metadata["riskRewardRatio"] = riskReward;

    // 8. 生成描述
    result.description = generateDescription(result);

    // 9. 检查是否为高质量信号
    if (result.isStrongSignal()) {
        emit highQualitySignalFound(result);
    }

    return result;
}

QVector<CompositeSignal> SignalFilter::filterBatch(const QVector<UnifiedSignal>& signals)
{
    QVector<CompositeSignal> results;

    // 按时间和标的分组
    QMap<QString, QVector<UnifiedSignal>> groupedSignals;

    for (const auto& signal : signals) {
        QString key = signal.symbol + "_" + signal.time.toString("yyyyMMdd");
        groupedSignals[key].append(signal);
    }

    // 对每组信号进行过滤
    for (auto it = groupedSignals.begin(); it != groupedSignals.end(); ++it) {
        auto composite = filter(it.value());
        if (composite.confidence > 0) {
            results.append(composite);
        }
    }

    emit filteringCompleted(results);

    return results;
}

double SignalFilter::calculateScore(const UnifiedSignal& signal)
{
    double score = 0.0;

    // 基础分：置信度
    score += signal.confidence * 0.4;

    // 强度分
    double strengthScore = static_cast<int>(signal.strength) * 10.0;
    score += strengthScore * 0.3;

    // 理论权重分
    double weight = d->config.theoryWeights.value(signal.source, 1.0);
    score += weight * 20.0 * 0.3;

    return score;
}

bool SignalFilter::checkConsistency(const QVector<UnifiedSignal>& signals)
{
    if (signals.size() < 2) return true;

    int bullishCount = 0;
    int bearishCount = 0;
    int neutralCount = 0;

    for (const auto& signal : signals) {
        switch (signal.direction) {
            case SignalDirection::Bullish: bullishCount++; break;
            case SignalDirection::Bearish: bearishCount++; break;
            case SignalDirection::Neutral: neutralCount++; break;
        }
    }

    // 如果超过70%的信号方向一致，则认为一致
    int total = signals.size();
    double maxRatio = qMax(bullishCount, bearishCount) * 1.0 / total;

    return maxRatio >= 0.7;
}

QMap<QString, QVariant> SignalFilter::statistics() const
{
    QMap<QString, QVariant> stats;
    stats["totalProcessed"] = d->totalSignalsProcessed;
    stats["passed"] = d->signalsPassed;
    stats["rejected"] = d->signalsRejected;
    stats["passRate"] = d->totalSignalsProcessed > 0
        ? d->signalsPassed * 100.0 / d->totalSignalsProcessed
        : 0.0;

    return stats;
}

QMap<TheoryType, QVector<UnifiedSignal>> SignalFilter::groupByTheory(
    const QVector<UnifiedSignal>& signals)
{
    QMap<TheoryType, QVector<UnifiedSignal>> groups;

    for (const auto& signal : signals) {
        groups[signal.source].append(signal);
    }

    return groups;
}

double SignalFilter::calculateCompositeConfidence(const QVector<UnifiedSignal>& signals)
{
    if (signals.isEmpty()) return 0.0;

    double weightedSum = 0.0;
    double totalWeight = 0.0;

    for (const auto& signal : signals) {
        double weight = d->config.theoryWeights.value(signal.source, 1.0);
        weightedSum += signal.confidence * weight;
        totalWeight += weight;
    }

    double confidence = totalWeight > 0 ? weightedSum / totalWeight : 0.0;

    // 一致性加成
    if (checkConsistency(signals)) {
        confidence *= 1.2; // 20%加成
    }

    return qMin(100.0, confidence);
}

SignalDirection SignalFilter::determineCompositeDirection(const QVector<UnifiedSignal>& signals)
{
    if (signals.isEmpty()) return SignalDirection::Neutral;

    int bullishScore = 0;
    int bearishScore = 0;

    for (const auto& signal : signals) {
        double weight = d->config.theoryWeights.value(signal.source, 1.0);
        double score = static_cast<int>(signal.strength) * weight;

        if (signal.direction == SignalDirection::Bullish) {
            bullishScore += score;
        } else if (signal.direction == SignalDirection::Bearish) {
            bearishScore += score;
        }
    }

    if (bullishScore > bearishScore * 1.5) {
        return SignalDirection::Bullish;
    } else if (bearishScore > bullishScore * 1.5) {
        return SignalDirection::Bearish;
    } else {
        return SignalDirection::Neutral;
    }
}

double SignalFilter::evaluateRiskReward(const CompositeSignal& signal)
{
    // 简化的风险收益比评估
    // 实际应用中应该基于支撑阻力位计算

    double baseRatio = 2.0;

    // 根据信号强度调整
    switch (signal.direction) {
        case SignalDirection::Bullish:
        case SignalDirection::Bearish:
            if (signal.confidence >= 80) {
                baseRatio = 3.0;
            } else if (signal.confidence >= 60) {
                baseRatio = 2.5;
            }
            break;
        default:
            baseRatio = 1.0;
            break;
    }

    // 根据理论数量调整
    baseRatio *= (1.0 + (signal.theoryCount - 2) * 0.1);

    return baseRatio;
}

QString SignalFilter::generateDescription(const CompositeSignal& signal)
{
    QString desc;

    // 方向描述
    QString directionText;
    switch (signal.direction) {
        case SignalDirection::Bullish:
            directionText = QStringLiteral("看涨");
            break;
        case SignalDirection::Bearish:
            directionText = QStringLiteral("看跌");
            break;
        default:
            directionText = QStringLiteral("中性");
            break;
    }

    // 理论支持描述
    QStringList theoryNames;
    for (const auto& srcSignal : signal.sourceSignals) {
        theoryNames << srcSignal.theoryName();
    }
    theoryNames.removeDuplicates();

    desc = QStringLiteral("%1信号，置信度%2%，由%3支持")
        .arg(directionText)
        .arg(QString::number(signal.confidence, 'f', 1))
        .arg(theoryNames.join("、"));

    // 添加强度描述
    if (signal.isStrongSignal()) {
        desc += QStringLiteral("（强信号）");
    }

    return desc;
}

} // namespace Analysis
} // namespace WealthPilot
