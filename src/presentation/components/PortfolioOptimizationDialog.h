/**
 * @file PortfolioOptimizationDialog.h
 * @brief 投资组合优化对话框 - 组合优化和回测界面
 *
 * @details 提供组合优化功能：
 * - 优化目标选择
 * - 约束条件设置
 * - 组合优化执行
 * - 回测验证
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PORTFOLIOOPTIMIZATIONDIALOG_H
#define PORTFOLIOOPTIMIZATIONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include "domain/portfolio/PortfolioOptimizer.h"

/**
 * @brief 投资组合优化对话框
 */
class PortfolioOptimizationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PortfolioOptimizationDialog(QWidget* parent = nullptr);
    ~PortfolioOptimizationDialog() override;

    /**
     * @brief 设置候选股票列表
     */
    void setCandidateSymbols(const QVector<QString>& symbols);

private slots:
    void onOptimizeClicked();
    void onBacktestClicked();
    void onOptimizationCompleted(const Portfolio& portfolio);
    void onBacktestCompleted(const PortfolioBacktestResult& result);

private:
    void setupUI();
    void displayPortfolio(const Portfolio& portfolio);
    void displayBacktestResult(const PortfolioBacktestResult& result);

    // UI组件 - 优化设置
    QComboBox* m_objectiveCombo = nullptr;
    QSpinBox* m_maxAssetsSpin = nullptr;
    QDoubleSpinBox* m_maxWeightSpin = nullptr;
    QDoubleSpinBox* m_targetReturnSpin = nullptr;
    QDoubleSpinBox* m_maxRiskSpin = nullptr;
    QPushButton* m_optimizeBtn = nullptr;

    // UI组件 - 回测设置
    QSpinBox* m_backtestDaysSpin = nullptr;
    QPushButton* m_backtestBtn = nullptr;

    // UI组件 - 结果显示
    QTableWidget* m_allocationTable = nullptr;
    QLabel* m_riskMetricsLabel = nullptr;
    QLabel* m_returnMetricsLabel = nullptr;
    QTextEdit* m_backtestResultEdit = nullptr;

    // 数据
    QVector<QString> m_candidateSymbols;
    Portfolio m_currentPortfolio;
};

#endif // PORTFOLIOOPTIMIZATIONDIALOG_H