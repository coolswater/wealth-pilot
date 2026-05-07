/**
 * @file AnalysisManager.h
 * @brief 技术分析管理器
 *
 * @details 统一管理所有技术分析模块：
 * - 波浪理论
 * - 缠论
 * - 道氏理论
 * - 量价形态
 * - 信号过滤
 * - 实时服务
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ANALYSIS_MANAGER_H
#define ANALYSIS_MANAGER_H

#include "AnalysisTypes.h"
#include "IAnalyzer.h"
#include "signal/SignalFilter.h"
#include "signal/SignalService.h"
#include <QObject>
#include <QMap>
#include <memory>

namespace WealthPilot {
namespace Analysis {

/**
 * @brief 技术分析管理器（单例）
 */
class AnalysisManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static AnalysisManager* instance();

    /**
     * @brief 初始化管理器
     */
    void initialize();

    /**
     * @brief 分析指定标的
     */
    CompositeSignal analyze(const QString& symbol, const QVector<KLine>& klines);

    /**
     * @brief 获取指定理论的分析器
     */
    IAnalyzer* getAnalyzer(TheoryType type);

    /**
     * @brief 获取信号过滤器
     */
    SignalFilter* signalFilter();

    /**
     * @brief 获取信号服务
     */
    SignalService* signalService();

    /**
     * @brief 设置分析参数
     */
    void setAnalysisParams(TheoryType type, const QMap<QString, QVariant>& params);

    /**
     * @brief 获取分析结果摘要
     */
    QMap<QString, QVariant> getAnalysisSummary(const QString& symbol);

signals:
    /**
     * @brief 分析完成
     */
    void analysisCompleted(const QString& symbol, const CompositeSignal& signal);

    /**
     * @brief 发现强信号
     */
    void strongSignalFound(const QString& symbol, const CompositeSignal& signal);

private:
    AnalysisManager(QObject* parent = nullptr);
    ~AnalysisManager();

    AnalysisManager(const AnalysisManager&) = delete;
    AnalysisManager& operator=(const AnalysisManager&) = delete;

    void registerAnalyzers();
    void setupSignalConnections();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Analysis
} // namespace WealthPilot

#endif // ANALYSIS_MANAGER_H
