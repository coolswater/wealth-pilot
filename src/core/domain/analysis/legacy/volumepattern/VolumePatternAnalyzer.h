/**
 * @file VolumePatternAnalyzer.h
 * @brief 量价形态分析器
 *
 * @details 实现量价分析的核心功能：
 * 1. 成交量形态识别
 * 2. 价格形态识别
 * 3. 量价组合分析
 * 4. OBV指标计算
 * 5. 背离检测
 * 6. 交易信号生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef VOLUME_PATTERN_ANALYZER_H
#define VOLUME_PATTERN_ANALYZER_H

#include "VolumePatternTypes.h"
#include "../IAnalyzer.h"
#include <QObject>
#include <QVector>
#include <memory>

namespace WealthPilot {
namespace VolumePattern {

/**
 * @brief 量价形态分析器
 */
class VolumePatternAnalyzer : public Analysis::IAnalyzer
{
    Q_OBJECT
    Q_INTERFACES(WealthPilot::Analysis::IAnalyzer)

public:
    explicit VolumePatternAnalyzer(QObject* parent = nullptr);
    ~VolumePatternAnalyzer() override;

    // ========== IAnalyzer 接口实现 ==========

    QString name() const override { return QStringLiteral("量价形态"); }
    Analysis::TheoryType theoryType() const override { return Analysis::TheoryType::VolumePattern; }

    Analysis::AnalysisResult analyze(const QVector<Analysis::KLine>& klines) override;
    void clear() override;
    QVector<Analysis::UnifiedSignal> currentSignals() const override;

    // ========== 量价分析接口 ==========

    /**
     * @brief 获取分析结果
     */
    const VolumePatternResult& volumeResult() const;

    /**
     * @brief 计算OBV指标
     */
    QVector<OBVData> calculateOBV(const QVector<Analysis::KLine>& klines, int maPeriod = 20);

    /**
     * @brief 识别量价形态
     */
    QVector<VolumePriceFormation> identifyFormations(const QVector<VolumePriceBar>& bars);

    /**
     * @brief 检测量价背离
     */
    bool detectDivergence(const QVector<Analysis::KLine>& klines,
                         const QVector<OBVData>& obvData);

    /**
     * @brief 检测放量突破
     */
    bool detectBreakout(const QVector<Analysis::KLine>& klines);

signals:
    void formationDetected(const VolumePriceFormation& formation);
    void divergenceDetected(bool isTopDivergence);

private:
    // ========== 内部方法 ==========

    /**
     * @brief 分析单根K线的量价特征
     */
    VolumePriceBar analyzeBar(const Analysis::KLine& kline, double avgVolume);

    /**
     * @brief 判断成交量形态
     */
    VolumeShape determineVolumeShape(qint64 volume, double avgVolume);

    /**
     * @brief 判断价格形态
     */
    PriceShape determinePriceShape(const Analysis::KLine& kline);

    /**
     * @brief 判断量价组合形态
     */
    VolumePricePattern determinePattern(const VolumePriceBar& bar,
                                        const VolumePriceBar* prevBar);

    /**
     * @brief 计算量比
     */
    double calculateVolumeRatio(qint64 volume, double avgVolume);

    /**
     * @brief 生成交易信号
     */
    QVector<Analysis::UnifiedSignal> generateSignals(const VolumePatternResult& result);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace VolumePattern
} // namespace WealthPilot

#endif // VOLUME_PATTERN_ANALYZER_H
