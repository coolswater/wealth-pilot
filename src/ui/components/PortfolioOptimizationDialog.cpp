/**
 * @file PortfolioOptimizationDialog.cpp
 * @brief 投资组合优化对话框实现
 */

#include "PortfolioOptimizationDialog.h"
#include "core/config/Tokens.h"
#include <QDateTimeEdit>
#include <QMessageBox>
#include <QHeaderView>

using namespace Tokens;

PortfolioOptimizationDialog::PortfolioOptimizationDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();

    // 连接优化器信号
    auto* optimizer = PortfolioOptimizer::instance();
    connect(optimizer, &PortfolioOptimizer::optimizationCompleted,
            this, &PortfolioOptimizationDialog::onOptimizationCompleted);
    connect(optimizer, &PortfolioOptimizer::backtestCompleted,
            this, &PortfolioOptimizationDialog::onBacktestCompleted);
}

PortfolioOptimizationDialog::~PortfolioOptimizationDialog()
{
}

void PortfolioOptimizationDialog::setCandidateSymbols(const QVector<QString>& symbols)
{
    m_candidateSymbols = symbols;
}

void PortfolioOptimizationDialog::onOptimizeClicked()
{
    if (m_candidateSymbols.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("请先添加候选股票"));
        return;
    }

    // 获取优化目标
    OptimizationObjective objective;
    switch (m_objectiveCombo->currentIndex()) {
    case 0: objective = OptimizationObjective::MaxReturn; break;
    case 1: objective = OptimizationObjective::MinRisk; break;
    case 2: objective = OptimizationObjective::MaxSharpeRatio; break;
    case 3: objective = OptimizationObjective::RiskParity; break;
    }

    // 设置约束条件
    OptimizationConstraint constraint;
    constraint.maxAssets = m_maxAssetsSpin->value();
    constraint.maxWeight = m_maxWeightSpin->value();
    constraint.targetReturn = m_targetReturnSpin->value();
    constraint.maxRisk = m_maxRiskSpin->value();

    // 执行优化
    m_currentPortfolio = PortfolioOptimizer::instance()->optimize(
        m_candidateSymbols, objective, constraint);

    displayPortfolio(m_currentPortfolio);
}

void PortfolioOptimizationDialog::onBacktestClicked()
{
    if (m_currentPortfolio.allocations.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"),
            QStringLiteral("请先执行组合优化"));
        return;
    }

    int days = m_backtestDaysSpin->value();
    QDateTime endDate = QDateTime::currentDateTime();
    QDateTime startDate = endDate.addDays(-days);

    PortfolioOptimizer::instance()->backtest(m_currentPortfolio, startDate, endDate);
}

void PortfolioOptimizationDialog::onOptimizationCompleted(const Portfolio& portfolio)
{
    m_currentPortfolio = portfolio;
    displayPortfolio(portfolio);
}

void PortfolioOptimizationDialog::onBacktestCompleted(const PortfolioBacktestResult& result)
{
    displayBacktestResult(result);
}

void PortfolioOptimizationDialog::setupUI()
{
    setWindowTitle(QStringLiteral("投资组合优化"));
    setMinimumSize(800, 600);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 优化设置组
    QGroupBox* optimizeGroup = new QGroupBox(QStringLiteral("优化设置"));
    QVBoxLayout* optimizeLayout = new QVBoxLayout(optimizeGroup);

    // 第一行：优化目标和最大资产数
    QHBoxLayout* row1 = new QHBoxLayout();
    QLabel* objectiveLabel = new QLabel(QStringLiteral("优化目标:"));
    m_objectiveCombo = new QComboBox();
    m_objectiveCombo->addItem(QStringLiteral("最大收益"));
    m_objectiveCombo->addItem(QStringLiteral("最小风险"));
    m_objectiveCombo->addItem(QStringLiteral("最大夏普比率"));
    m_objectiveCombo->addItem(QStringLiteral("风险平价"));
    row1->addWidget(objectiveLabel);
    row1->addWidget(m_objectiveCombo);

    QLabel* maxAssetsLabel = new QLabel(QStringLiteral("最大资产数:"));
    m_maxAssetsSpin = new QSpinBox();
    m_maxAssetsSpin->setRange(1, 20);
    m_maxAssetsSpin->setValue(10);
    row1->addWidget(maxAssetsLabel);
    row1->addWidget(m_maxAssetsSpin);
    row1->addStretch();
    optimizeLayout->addLayout(row1);

    // 第二行：约束条件
    QHBoxLayout* row2 = new QHBoxLayout();
    QLabel* maxWeightLabel = new QLabel(QStringLiteral("最大权重(%):"));
    m_maxWeightSpin = new QDoubleSpinBox();
    m_maxWeightSpin->setRange(0, 100);
    m_maxWeightSpin->setValue(30);
    row2->addWidget(maxWeightLabel);
    row2->addWidget(m_maxWeightSpin);

    QLabel* targetReturnLabel = new QLabel(QStringLiteral("目标收益(%):"));
    m_targetReturnSpin = new QDoubleSpinBox();
    m_targetReturnSpin->setRange(-100, 100);
    m_targetReturnSpin->setValue(10);
    row2->addWidget(targetReturnLabel);
    row2->addWidget(m_targetReturnSpin);

    QLabel* maxRiskLabel = new QLabel(QStringLiteral("最大风险(%):"));
    m_maxRiskSpin = new QDoubleSpinBox();
    m_maxRiskSpin->setRange(0, 100);
    m_maxRiskSpin->setValue(50);
    row2->addWidget(maxRiskLabel);
    row2->addWidget(m_maxRiskSpin);
    optimizeLayout->addLayout(row2);

    // 优化按钮
    m_optimizeBtn = new QPushButton(QStringLiteral("执行优化"));
    m_optimizeBtn->setFixedHeight(35);
    m_optimizeBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: white; border-radius: 4px; }"
        "QPushButton:hover { background-color: %2; }"
    ).arg(Colors::Primary, Colors::PrimaryHover));
    connect(m_optimizeBtn, &QPushButton::clicked, this, &PortfolioOptimizationDialog::onOptimizeClicked);
    optimizeLayout->addWidget(m_optimizeBtn);

    mainLayout->addWidget(optimizeGroup);

    // 回测设置组
    QGroupBox* backtestGroup = new QGroupBox(QStringLiteral("回测验证"));
    QHBoxLayout* backtestLayout = new QHBoxLayout(backtestGroup);

    QLabel* daysLabel = new QLabel(QStringLiteral("回测天数:"));
    m_backtestDaysSpin = new QSpinBox();
    m_backtestDaysSpin->setRange(30, 730);
    m_backtestDaysSpin->setValue(365);
    backtestLayout->addWidget(daysLabel);
    backtestLayout->addWidget(m_backtestDaysSpin);

    m_backtestBtn = new QPushButton(QStringLiteral("执行回测"));
    m_backtestBtn->setFixedHeight(35);
    connect(m_backtestBtn, &QPushButton::clicked, this, &PortfolioOptimizationDialog::onBacktestClicked);
    backtestLayout->addWidget(m_backtestBtn);
    backtestLayout->addStretch();

    mainLayout->addWidget(backtestGroup);

    // 结果显示组
    QGroupBox* resultGroup = new QGroupBox(QStringLiteral("优化结果"));
    QVBoxLayout* resultLayout = new QVBoxLayout(resultGroup);

    // 资产配置表
    m_allocationTable = new QTableWidget();
    m_allocationTable->setColumnCount(4);
    m_allocationTable->setHorizontalHeaderLabels({
        QStringLiteral("资产代码"),
        QStringLiteral("资产类型"),
        QStringLiteral("权重(%)"),
        QStringLiteral("配置原因")
    });
    m_allocationTable->horizontalHeader()->setStretchLastSection(true);
    m_allocationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_allocationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_allocationTable->verticalHeader()->setVisible(false);
    resultLayout->addWidget(m_allocationTable);

    // 风险收益指标
    QHBoxLayout* metricsLayout = new QHBoxLayout();
    m_riskMetricsLabel = new QLabel(QStringLiteral("风险指标: --"));
    m_returnMetricsLabel = new QLabel(QStringLiteral("收益指标: --"));
    metricsLayout->addWidget(m_riskMetricsLabel);
    metricsLayout->addWidget(m_returnMetricsLabel);
    metricsLayout->addStretch();
    resultLayout->addLayout(metricsLayout);

    mainLayout->addWidget(resultGroup, 1);

    // 回测结果显示
    QGroupBox* backtestResultGroup = new QGroupBox(QStringLiteral("回测结果"));
    QVBoxLayout* backtestResultLayout = new QVBoxLayout(backtestResultGroup);
    m_backtestResultEdit = new QTextEdit();
    m_backtestResultEdit->setReadOnly(true);
    m_backtestResultEdit->setMaximumHeight(150);
    backtestResultLayout->addWidget(m_backtestResultEdit);
    mainLayout->addWidget(backtestResultGroup);

    // 设置样式
    setStyleSheet(QString("QDialog { background-color: %1; }")
        .arg(Colors::BgSurface));
}

void PortfolioOptimizationDialog::displayPortfolio(const Portfolio& portfolio)
{
    m_allocationTable->setRowCount(0);

    for (const auto& allocation : portfolio.allocations) {
        int row = m_allocationTable->rowCount();
        m_allocationTable->insertRow(row);

        m_allocationTable->setItem(row, 0, new QTableWidgetItem(allocation.symbol));

        QString typeText;
        switch (allocation.type) {
        case AssetType::Stock: typeText = QStringLiteral("股票"); break;
        case AssetType::Bond: typeText = QStringLiteral("债券"); break;
        case AssetType::Fund: typeText = QStringLiteral("基金"); break;
        case AssetType::Cash: typeText = QStringLiteral("现金"); break;
        case AssetType::Crypto: typeText = QStringLiteral("数字货币"); break;
        }
        m_allocationTable->setItem(row, 1, new QTableWidgetItem(typeText));
        m_allocationTable->setItem(row, 2, new QTableWidgetItem(
            QString::number(allocation.weight, 'f', 2)));
        m_allocationTable->setItem(row, 3, new QTableWidgetItem(allocation.reason));
    }

    // 显示风险收益指标
    QString riskText = QString("波动率: %1% | 最大回撤: %2% | 夏普比率: %3")
        .arg(portfolio.riskMetrics.volatility, 0, 'f', 2)
        .arg(portfolio.riskMetrics.maxDrawdown, 0, 'f', 2)
        .arg(portfolio.riskMetrics.sharpeRatio, 0, 'f', 2);
    m_riskMetricsLabel->setText(riskText);

    QString returnText = QString("总收益: %1% | 年化收益: %2% | Alpha: %3")
        .arg(portfolio.returnMetrics.totalReturn, 0, 'f', 2)
        .arg(portfolio.returnMetrics.annualizedReturn, 0, 'f', 2)
        .arg(portfolio.returnMetrics.alpha, 0, 'f', 2);
    m_returnMetricsLabel->setText(returnText);
}

void PortfolioOptimizationDialog::displayBacktestResult(const PortfolioBacktestResult& result)
{
    QString text = QString(
        "=== 回测结果 ===\n\n"
        "总收益: %1%\n"
        "年化收益: %2%\n"
        "最大回撤: %3%\n"
        "夏普比率: %4\n"
        "胜率: %5%\n"
        "交易次数: %6\n"
    ).arg(result.totalReturn, 0, 'f', 2)
     .arg(result.annualizedReturn, 0, 'f', 2)
     .arg(result.maxDrawdown, 0, 'f', 2)
     .arg(result.sharpeRatio, 0, 'f', 2)
     .arg(result.winRate, 0, 'f', 2)
     .arg(result.totalTrades);

    m_backtestResultEdit->setText(text);
}
