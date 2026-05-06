/**
 * @file ChanLunIntegration.h
 * @brief 缠论指标集成到K线页面
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#pragma once

#include "analysis/chanlun/ChanLunAnalyzer.h"
#include "analysis/chanlun/ChanLunIndicator.h"
#include "core/types/MarketTypes.h"
#include <QObject>
#include <QVector>
#include <memory>

class KLineChart;

namespace WealthPilot {
namespace ChanLun {

/**
 * @brief 缠论指标集成管理器
 * 
 * 负责将缠论分析结果集成到K线图表中
 */
class ChanLunIntegration : public QObject
{
    Q_OBJECT

public:
    explicit ChanLunIntegration(KLineChart* chart, QObject* parent = nullptr);
    ~ChanLunIntegration() override;

    /**
     * @brief 设置K线数据并进行分析
     */
    void setKLineData(const QVector<KLineData>& klines);

    /**
     * @brief 启用/禁用缠论指标
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief 设置显示选项
     */
    void setShowPens(bool show);
    void setShowSegments(bool show);
    void setShowPivots(bool show);
    void setShowSignals(bool show);
    void setShowFractals(bool show);

    /**
     * @brief 获取分析结果
     */
    const ChanLunResult& result() const { return m_result; }

signals:
    /**
     * @brief 分析完成信号
     */
    void analysisCompleted(const ChanLunResult& result);

    /**
     * @brief 买卖点信号
     */
    void signalDetected(const TradeSignal& signal);

private:
    void performAnalysis();
    void updateChartOverlay();

    KLineChart* m_chart = nullptr;
    ChanLunAnalyzer m_analyzer;
    ChanLunResult m_result;
    QVector<KLineData> m_klines;
    bool m_enabled = false;

    // 显示选项
    bool m_showPens = true;
    bool m_showSegments = true;
    bool m_showPivots = true;
    bool m_showSignals = true;
    bool m_showFractals = false;

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace ChanLun
} // namespace WealthPilot
