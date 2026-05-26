/**
 * @file BacktestReportWidget.h
 * @brief 回测报告面板
 *
 * @details 显示：
 * - 关键指标摘要
 * - 收益分布图
 * - 交易列表
 * - 月度/年度收益表
 */

#ifndef BACKTESTREPORTWIDGET_H
#define BACKTESTREPORTWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTableWidget>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <memory>

#include "BacktestChartWidget.h"

namespace WealthPilot {

/**
 * @brief 回测报告面板
 */
class BacktestReportWidget : public QWidget {
    Q_OBJECT

public:
    explicit BacktestReportWidget(QWidget* parent = nullptr);
    ~BacktestReportWidget() override = default;

    /**
     * @brief 设置回测结果
     */
    void setBacktestResult(const BacktestStats& stats,
                            const QVector<BacktestDataPoint>& equityCurve,
                            const QVector<TradeMarker>& trades);

    /**
     * @brief 清空结果
     */
    void clear();

private:
    void setupUI();
    void updateStatsDisplay();
    void updateTradeTable();
    void updateMonthlyReturns();

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // BACKTESTREPORTWIDGET_H