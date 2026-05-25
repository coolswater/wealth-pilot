/**
 * @file RiskIndicatorWidget.h
 * @brief 风险指示器组件 - 显示风险等级和预警信息
 *
 * @details 提供风险可视化功能：
 * - 风险等级指示器
 * - 风险分数显示
 * - 预警列表展示
 * - 风险详情查看
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef RISKINDICATORWIDGET_H
#define RISKINDICATORWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include "core/risk/RiskWarningSystem.h"

/**
 * @brief 风险指示器组件
 */
class RiskIndicatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RiskIndicatorWidget(QWidget* parent = nullptr);
    ~RiskIndicatorWidget() override;

    /**
     * @brief 设置股票代码
     */
    void setSymbol(const QString& symbol);

    /**
     * @brief 更新风险显示
     */
    void updateRiskDisplay(const QString& symbol, RiskLevel level);

    /**
     * @brief 更新预警列表
     */
    void updateAlertList(const QVector<RiskAlert>& alerts);

signals:
    /**
     * @brief 查看详情信号
     */
    void viewDetailsRequested(const QString& symbol);

private slots:
    void onRiskAlertTriggered(const RiskAlert& alert);
    void onRiskLevelChanged(const QString& symbol, RiskLevel level);
    void onViewDetailsClicked();

private:
    void setupUI();
    void updateRiskLevelIndicator(RiskLevel level);
    QString riskLevelToColor(RiskLevel level) const;
    QString riskLevelToText(RiskLevel level) const;

    // UI组件
    QLabel* m_symbolLabel = nullptr;
    QLabel* m_riskLevelLabel = nullptr;
    QProgressBar* m_riskScoreBar = nullptr;
    QLabel* m_riskScoreLabel = nullptr;
    QPushButton* m_viewDetailsBtn = nullptr;
    QTableWidget* m_alertTable = nullptr;

    QString m_currentSymbol;
    RiskLevel m_currentRiskLevel = RiskLevel::Low;
};

#endif // RISKINDICATORWIDGET_H