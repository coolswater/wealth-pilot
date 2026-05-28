/**
 * @file SignalFilter.h
 * @brief 多层过滤信号系统
 *
 * @details 整合波浪理论、缠论、道氏理论、量价形态的分析结果，
 * 通过多层过滤产生高质量的综合交易信号。
 *
 * 过滤策略：
 * 1. 单理论信号强度过滤
 * 2. 多理论一致性过滤
 * 3. 趋势方向确认
 * 4. 风险收益比评估
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SIGNAL_FILTER_H
#define SIGNAL_FILTER_H

#include "../AnalysisTypes.h"
#include <QObject>
#include <QVector>
#include <QMap>
#include <memory>

namespace WealthPilot {
namespace Analysis {

/**
 * @brief 信号过滤器配置
 */
struct SignalFilterConfig {
    // 最小信号强度
    SignalStrength minStrength = SignalStrength::Moderate;

    // 最小置信度
    double minConfidence = 50.0;

    // 最少支持理论数量
    int minTheoryCount = 2;

    // 是否要求趋势确认
    bool requireTrendConfirmation = true;

    // 最小风险收益比
    double minRiskRewardRatio = 2.0;

    // 各理论权重
    QMap<TheoryType, double> theoryWeights = {
        {TheoryType::ElliottWave, 1.0},
        {TheoryType::ChanLun, 1.2},
        {TheoryType::DowTheory, 0.9},
        {TheoryType::VolumePattern, 0.8}
    };
};

/**
 * @brief 多层过滤信号系统
 */
class SignalFilter : public QObject
{
    Q_OBJECT

public:
    explicit SignalFilter(QObject* parent = nullptr);
    ~SignalFilter() override;

    /**
     * @brief 设置过滤配置
     */
    void setConfig(const SignalFilterConfig& config);

    /**
     * @brief 获取当前配置
     */
    const SignalFilterConfig& config() const;

    /**
     * @brief 过滤信号
     * @param inputSignals 来自各理论的信号列表
     * @return 过滤后的综合信号
     */
    CompositeSignal filter(const QVector<UnifiedSignal>& inputSignals);

    /**
     * @brief 批量过滤信号
     */
    QVector<CompositeSignal> filterBatch(const QVector<UnifiedSignal>& inputSignals);

    /**
     * @brief 计算信号得分
     */
    double calculateScore(const UnifiedSignal& signal);

    /**
     * @brief 检查信号一致性
     */
    bool checkConsistency(const QVector<UnifiedSignal>& inputSignals);

    /**
     * @brief 获取统计信息
     */
    QMap<QString, QVariant> statistics() const;

signals:
    /**
     * @brief 发现高质量信号
     */
    void highQualitySignalFound(const CompositeSignal& signal);

    /**
     * @brief 过滤完成
     */
    void filteringCompleted(const QVector<CompositeSignal>& resultSignals);

private:
    // ========== 内部方法 ==========

    /**
     * @brief 按理论分组信号
     */
    QMap<TheoryType, QVector<UnifiedSignal>> groupByTheory(const QVector<UnifiedSignal>& inputSignals);

    /**
     * @brief 计算综合置信度
     */
    double calculateCompositeConfidence(const QVector<UnifiedSignal>& inputSignals);

    /**
     * @brief 确定综合方向
     */
    SignalDirection determineCompositeDirection(const QVector<UnifiedSignal>& inputSignals);

    /**
     * @brief 评估风险收益比
     */
    double evaluateRiskReward(const CompositeSignal& signal);

    /**
     * @brief 生成信号描述
     */
    QString generateDescription(const CompositeSignal& signal);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Analysis
} // namespace WealthPilot

#endif // SIGNAL_FILTER_H
