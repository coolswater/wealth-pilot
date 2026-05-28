/**
 * @file IAnalyzer.h
 * @brief 技术分析器接口
 *
 * @details 定义所有技术分析器的统一接口
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef IANALYZER_H
#define IANALYZER_H

#include "AnalysisTypes.h"
#include <QObject>
#include <QVector>

namespace WealthPilot {
namespace Analysis {

/**
 * @brief 技术分析器接口
 *
 * @details 所有技术分析理论（波浪、缠论、道氏、量价）都实现此接口
 */
class IAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit IAnalyzer(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IAnalyzer() = default;

    /**
     * @brief 获取分析器名称
     */
    virtual QString name() const = 0;

    /**
     * @brief 获取理论类型
     */
    virtual TheoryType theoryType() const = 0;

    /**
     * @brief 分析K线数据
     * @param klines K线数据
     * @return 分析结果
     */
    virtual AnalysisResult analyze(const QVector<KLine>& klines) = 0;

    /**
     * @brief 增量更新分析
     * @param newKlines 新增K线
     * @return 分析结果
     */
    virtual AnalysisResult update(const QVector<KLine>& newKlines) {
        Q_UNUSED(newKlines)
        return AnalysisResult();
    }

    /**
     * @brief 清空分析状态
     */
    virtual void clear() = 0;

    /**
     * @brief 获取当前信号列表
     */
    virtual QVector<UnifiedSignal> currentSignals() const = 0;

signals:
    /**
     * @brief 分析完成信号
     */
    void analysisCompleted(const AnalysisResult& result);

    /**
     * @brief 发现新信号
     */
    void signalDetected(const UnifiedSignal& signal);
};

} // namespace Analysis
} // namespace WealthPilot

// 定义接口ID（用于Qt元对象系统）
Q_DECLARE_INTERFACE(WealthPilot::Analysis::IAnalyzer, "com.wealthpilot.IAnalyzer/1.0")

#endif // IANALYZER_H
