/**
 * @file KLineChartWithSignals.h
 * @brief 带信号标记的K线图组件
 *
 * @details 集成K线图和信号标记，提供完整的技术分析可视化
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef KLINE_CHART_WITH_SIGNALS_H
#define KLINE_CHART_WITH_SIGNALS_H

#include "KLineChart.h"
#include "SignalMarker.h"
#include "SignalDetailPanel.h"
#include "analysis/AnalysisTypes.h"
#include <QWidget>
#include <QSplitter>

namespace WealthPilot {
namespace UI {

/**
 * @brief 带信号标记的K线图组件
 */
class KLineChartWithSignals : public QWidget
{
    Q_OBJECT

public:
    explicit KLineChartWithSignals(QWidget* parent = nullptr);
    ~KLineChartWithSignals() override;

    // ========== K线数据 ==========

    /**
     * @brief 设置K线数据
     */
    void setKLineData(const QVector<KLineData>& data);

    /**
     * @brief 添加K线数据
     */
    void addKLineData(const KLineData& data);

    /**
     * @brief 清空数据
     */
    void clearData();

    // ========== 信号数据 ==========

    /**
     * @brief 设置综合信号
     */
    void setCompositeSignal(const Analysis::CompositeSignal& signal);

    /**
     * @brief 设置信号列表
     */
    void setSignals(const QVector<Analysis::UnifiedSignal>& signalList);

    /**
     * @brief 清空信号
     */
    void clearSignals();

    // ========== 显示控制 ==========

    /**
     * @brief 设置是否显示信号标记
     */
    void setShowSignalMarkers(bool show);

    /**
     * @brief 设置是否显示信号详情面板
     */
    void setShowDetailPanel(bool show);

    /**
     * @brief 设置信号过滤理论
     */
    void setSignalFilterTheory(Analysis::TheoryType theory);

    // ========== 指标叠加 ==========

    /**
     * @brief 设置主图指标
     */
    void setMainIndicator(MainIndicator indicator);

    /**
     * @brief 设置副图指标
     */
    void setSubIndicator(SubIndicator indicator);

    /**
     * @brief 添加自定义指标
     */
    void addCustomIndicator(const QString& name, const QVector<double>& values, const QColor& color);

    // ========== 视图控制 ==========

    /**
     * @brief 缩放
     */
    void zoom(double factor);

    /**
     * @brief 重置视图
     */
    void resetView();

    /**
     * @brief 显示最新数据
     */
    void showLatest(int count = 100);

signals:
    /**
     * @brief 信号被点击
     */
    void signalClicked(const Analysis::UnifiedSignal& signal);

    /**
     * @brief K线信息变化
     */
    void klineInfoChanged(const KLineData& kline, int index);

    /**
     * @brief 订阅请求
     */
    void subscribeRequested(const QString& symbol);

private:
    // ========== 内部方法 ==========

    void setupUI();
    void setupConnections();
    void updateSignalPositions();
    void onSignalMarkerClicked(const SignalMarkerData& marker);
    void onKLineCrosshairMoved(const QDateTime& time, double price);
    void onVisibleRangeChanged(int startIndex, int count);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace UI
} // namespace WealthPilot

#endif // KLINE_CHART_WITH_SIGNALS_H