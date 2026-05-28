/**
 * @file ElliottWaveAnalyzer.h
 * @brief 波浪理论分析器
 *
 * @details 实现艾略特波浪理论的核心分析：
 * 1. 波浪识别与计数
 * 2. 波浪级别判断
 * 3. 斐波那契比例验证
 * 4. 波浪规则验证
 * 5. 交易信号生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ELLIOTT_WAVE_ANALYZER_H
#define ELLIOTT_WAVE_ANALYZER_H

#include "ElliottWaveTypes.h"
#include "../IAnalyzer.h"
#include <QObject>
#include <QVector>
#include <memory>

namespace WealthPilot {
namespace ElliottWave {

/**
 * @brief 波浪理论分析器
 *
 * @details 提供完整的波浪分析功能
 */
class ElliottWaveAnalyzer : public Analysis::IAnalyzer
{
    Q_OBJECT
    Q_INTERFACES(WealthPilot::Analysis::IAnalyzer)

public:
    /**
     * @brief 构造函数
     */
    explicit ElliottWaveAnalyzer(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ElliottWaveAnalyzer() override;

    // ========== IAnalyzer 接口实现 ==========

    QString name() const override { return QStringLiteral("波浪理论"); }
    Analysis::TheoryType theoryType() const override { return Analysis::TheoryType::ElliottWave; }

    Analysis::AnalysisResult analyze(const QVector<Analysis::KLine>& klines) override;
    Analysis::AnalysisResult update(const QVector<Analysis::KLine>& newKlines) override;
    void clear() override;
    QVector<Analysis::UnifiedSignal> currentSignals() const override;

    // ========== 波浪分析接口 ==========

    /**
     * @brief 获取波浪分析结果
     */
    const ElliottWaveResult& waveResult() const;

    /**
     * @brief 获取当前波浪计数
     */
    const WaveCount* currentWaveCount() const;

    /**
     * @brief 识别波浪结构
     * @param klines K线数据
     * @return 波浪列表
     */
    QVector<Wave> identifyWaves(const QVector<Analysis::KLine>& klines);

    /**
     * @brief 验证波浪规则
     * @param waves 波浪列表
     * @return 是否符合规则
     */
    bool validateWaveRules(const QVector<Wave>& waves);

    /**
     * @brief 计算斐波那契目标位
     * @param waves 波浪列表
     * @return 目标价格列表
     */
    QVector<double> calculateFibonacciTargets(const QVector<Wave>& waves);

    /**
     * @brief 判断当前所处波浪位置
     * @param klines K线数据
     * @param waves 波浪列表
     * @return 波浪编号
     */
    WaveNumber identifyCurrentPosition(const QVector<Analysis::KLine>& klines,
                                       const QVector<Wave>& waves);

signals:
    /**
     * @brief 波浪计数更新
     */
    void waveCountUpdated(const WaveCount& count);

    /**
     * @brief 发现新的波浪
     */
    void waveIdentified(const Wave& wave);

private:
    // ========== 波浪识别算法 ==========

    /**
     * @brief 识别极值点（波峰波谷）
     */
    QVector<int> findExtremes(const QVector<Analysis::KLine>& klines, int minBars = 3);

    /**
     * @brief 根据极值点构建波浪
     */
    QVector<Wave> buildWavesFromExtremes(const QVector<Analysis::KLine>& klines,
                                         const QVector<int>& extremes);

    /**
     * @brief 判断波浪模式
     */
    WavePattern determinePattern(const QVector<Wave>& waves);

    /**
     * @brief 计算波浪级别
     */
    WaveDegree calculateDegree(const Wave& wave, const Wave& parentWave);

    /**
     * @brief 验证推动浪规则
     */
    bool validateImpulseRules(const Wave& w1, const Wave& w2, const Wave& w3,
                              const Wave& w4, const Wave& w5);

    /**
     * @brief 验证调整浪规则
     */
    bool validateCorrectiveRules(const Wave& wA, const Wave& wB, const Wave& wC);

    /**
     * @brief 检查斐波那契比例
     */
    double checkFibonacciRatios(const QVector<Wave>& waves);

    /**
     * @brief 生成交易信号
     */
    QVector<Analysis::UnifiedSignal> generateSignals(const WaveCount& count);

    /**
     * @brief 计算信号置信度
     */
    double calculateConfidence(const WaveCount& count, const Wave& wave);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace ElliottWave
} // namespace WealthPilot

#endif // ELLIOTT_WAVE_ANALYZER_H
