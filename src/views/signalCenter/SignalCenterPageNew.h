/**
 * @file SignalCenterPageNew.h
 * @brief 信号中心页面 - 集成实时行情信号服务
 *
 * @details 功能：
 * - 实时信号展示
 * - K线图信号标记
 * - 信号详情面板
 * - 订阅管理
 * - 历史信号查询
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef SIGNAL_CENTER_PAGE_NEW_H
#define SIGNAL_CENTER_PAGE_NEW_H

#include "ui/components/BasePage.h"
#include "ui/components/KLineChartWithSignals.h"
#include "ui/components/SignalDetailPanel.h"
#include "analysis/AnalysisManager.h"
#include "market/RealtimeMarketDataSource.h"
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

namespace WealthPilot {
namespace UI {

/**
 * @brief 信号中心页面
 */
class SignalCenterPageNew : public BasePage
{
    Q_OBJECT

public:
    explicit SignalCenterPageNew(QWidget* parent = nullptr);
    ~SignalCenterPageNew() override;

    QString pageTitle() const override { return QStringLiteral("信号中心"); }
    QString pageIcon() const override { return ":/icons/signal.png"; }

protected:
    void setupUI() override;
    void setupConnections() override;
    void onDataLoaded() override;

private:
    // ========== UI组件 ==========

    QWidget* createToolbar();
    QWidget* createSignalListPanel();
    QWidget* createChartPanel();
    QWidget* createDetailPanel();

    // ========== 数据管理 ==========

    void loadSignals();
    void updateSignalList();
    void selectSignal(const QString& symbol);
    void refreshCurrentSymbol();

    // ========== 信号处理 ==========

    void onSignalGenerated(const Analysis::CompositeSignal& signal);
    void onSignalClicked(const Analysis::UnifiedSignal& signal);
    void onSymbolChanged(const QString& symbol);
    void onPeriodChanged(int period);
    void onTheoryFilterChanged(int theory);
    void onSubscribeClicked();
    void onRefreshClicked();

private:
    // UI组件
    QComboBox* m_symbolCombo = nullptr;
    QComboBox* m_periodCombo = nullptr;
    QComboBox* m_theoryFilterCombo = nullptr;
    QPushButton* m_subscribeBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;

    QTableWidget* m_signalTable = nullptr;
    KLineChartWithSignals* m_chartWidget = nullptr;
    SignalDetailPanel* m_detailPanel = nullptr;

    QLabel* m_statusLabel = nullptr;

    // 数据
    Analysis::AnalysisManager* m_analysisManager = nullptr;
    Market::RealtimeMarketDataSource* m_dataSource = nullptr;

    QString m_currentSymbol;
    Analysis::KLinePeriod m_currentPeriod = Analysis::KLinePeriod::Minute1;
    QVector<Analysis::CompositeSignal> m_signals;
};

} // namespace UI
} // namespace WealthPilot

#endif // SIGNAL_CENTER_PAGE_NEW_H
