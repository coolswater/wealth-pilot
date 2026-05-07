/**
 * @file DowTheoryAnalyzer.h
 * @brief 道氏理论分析器
 *
 * @details 实现道氏理论的核心分析：
 * 1. 趋势识别（主要、次要、小趋势）
 * 2. 高低点分析
 * 3. 趋势线绘制
 * 4. 趋势反转确认
 * 5. 交易信号生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DOW_THEORY_ANALYZER_H
#define DOW_THEORY_ANALYZER_H

#include "DowTheoryTypes.h"
#include "../IAnalyzer.h"
#include <QObject>
#include <QVector>
#include <memory>

namespace WealthPilot {
namespace DowTheory {

/**
 * @brief 道氏理论分析器
 */
class DowTheoryAnalyzer : public Analysis::IAnalyzer
{
    Q_OBJECT
    Q_INTERFACES(WealthPilot::Analysis::IAnalyzer)

public:
    explicit DowTheoryAnalyzer(QObject* parent = nullptr);
    ~DowTheoryAnalyzer() override;

    // ========== IAnalyzer 接口实现 ==========

    QString name() const override { return QStringLiteral("道氏理论"); }
    Analysis::TheoryType theoryType() const override { return Analysis::TheoryType::DowTheory; }

    Analysis::AnalysisResult analyze(const QVector<Analysis::KLine>& klines) override;
    void clear() override;
    QVector<Analysis::UnifiedSignal> currentSignals() const override;

    // ========== 道氏理论分析接口 ==========

    /**
     * @brief 获取分析结果
     */
    const DowTheoryResult& dowResult() const;

    /**
     * @brief 识别趋势
     */
    Trend identifyTrend(const QVector<Analysis::KLine>& klines, TrendLevel level);

    /**
     * @brief 识别极值点
     */
    QVector<PriceExtremum> identifyExtremums(const QVector<Analysis::KLine>& klines);

    /**
     * @brief 绘制趋势线
     */
    QVector<TrendLine> drawTrendLines(const QVector<Analysis::KLine>& klines,
                                      const QVector<PriceExtremum>& extremums);

    /**
     * @brief 判断趋势反转
     */
    bool detectTrendReversal(const QVector<Analysis::KLine>& klines,
                            const Trend& currentTrend);

    /**
     * @brief 获取当前趋势
     */
    TrendDirection currentTrend() const;

signals:
    void trendChanged(TrendDirection newTrend, TrendDirection oldTrend);
    void trendReversalDetected(const TrendSignal& signal);

private:
    // ========== 内部方法 ==========

    /**
     * @brief 识别更高的高点和更低的低点
     */
    void analyzeHighsAndLows(QVector<PriceExtremum>& extremums);

    /**
     * @brief 确认趋势
     */
    bool confirmTrend(const QVector<Analysis::KLine>& klines,
                     TrendDirection direction,
                     int confirmationBars = 3);

    /**
     * @brief 计算趋势强度
     */
    double calculateTrendStrength(const Trend& trend);

    /**
     * @brief 生成交易信号
     */
    QVector<Analysis::UnifiedSignal> generateSignals(const DowTheoryResult& result);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace DowTheory
} // namespace WealthPilot

#endif // DOW_THEORY_ANALYZER_H
