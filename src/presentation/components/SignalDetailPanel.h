/**
 * @file SignalDetailPanel.h
 * @brief 信号详情面板 - 显示交易信号的详细信息
 *
 * @details 功能：
 * - 显示综合信号详情
 * - 显示各理论分析结果
 * - 显示信号强度和置信度
 * - 支持信号历史记录
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SIGNAL_DETAIL_PANEL_H
#define SIGNAL_DETAIL_PANEL_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTableWidget>
#include <QPushButton>
#include <QTextEdit>
#include "analysis/AnalysisTypes.h"

namespace WealthPilot {
namespace UI {

/**
 * @brief 信号详情面板
 */
class SignalDetailPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SignalDetailPanel(QWidget* parent = nullptr);
    ~SignalDetailPanel() override;

    /**
     * @brief 设置综合信号
     */
    void setCompositeSignal(const Analysis::CompositeSignal& signal);

    /**
     * @brief 设置单个信号
     */
    void setUnifiedSignal(const Analysis::UnifiedSignal& signal);

    /**
     * @brief 清空显示
     */
    void clear();

    /**
     * @brief 设置是否紧凑模式
     */
    void setCompactMode(bool compact);

signals:
    /**
     * @brief 订阅信号
     */
    void subscribeRequested(const QString& symbol);

    /**
     * @brief 查看历史信号
     */
    void historyRequested(const QString& symbol);

private:
    // ========== UI创建 ==========

    void setupUI();
    QWidget* createSummaryWidget();
    QWidget* createTheoryDetailsWidget();
    QWidget* createRiskWidget();
    QWidget* createActionWidget();

    // ========== 更新方法 ==========

    void updateSummary();
    void updateTheoryDetails();
    void updateRiskInfo();
    void updateActions();

    // ========== 样式 ==========

    QString getDirectionIcon(Analysis::SignalDirection direction);
    QString getStrengthIcon(Analysis::SignalStrength strength);
    QColor getDirectionColor(Analysis::SignalDirection direction);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace UI
} // namespace WealthPilot

#endif // SIGNAL_DETAIL_PANEL_H
