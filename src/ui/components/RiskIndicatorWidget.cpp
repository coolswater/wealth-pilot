/**
 * @file RiskIndicatorWidget.cpp
 * @brief 风险指示器组件实现
 */

#include "RiskIndicatorWidget.h"
#include "core/config/Tokens.h"
#include "ui/components/StyleHelper.h"
#include <QHeaderView>
#include <QDateTime>

using namespace Tokens;

RiskIndicatorWidget::RiskIndicatorWidget(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("RiskIndicatorWidget");
    setupUI();

    auto* riskSystem = RiskWarningSystem::instance();
    connect(riskSystem, &RiskWarningSystem::riskAlertTriggered,
            this, &RiskIndicatorWidget::onRiskAlertTriggered);
    connect(riskSystem, &RiskWarningSystem::riskLevelChanged,
            this, &RiskIndicatorWidget::onRiskLevelChanged);
}

RiskIndicatorWidget::~RiskIndicatorWidget()
{
}

void RiskIndicatorWidget::setSymbol(const QString& symbol)
{
    m_currentSymbol = symbol;
    m_symbolLabel->setText(QString("股票: %1").arg(symbol));

    // 启动监控
    RiskWarningSystem::instance()->monitorSymbol(symbol);
}

void RiskIndicatorWidget::updateRiskDisplay(const QString& symbol, RiskLevel level)
{
    if (symbol != m_currentSymbol) return;

    m_currentRiskLevel = level;
    updateRiskLevelIndicator(level);
}

void RiskIndicatorWidget::updateAlertList(const QVector<RiskAlert>& alerts)
{
    m_alertTable->setRowCount(0);

    for (const auto& alert : alerts) {
        int row = m_alertTable->rowCount();
        m_alertTable->insertRow(row);

        // 时间
        m_alertTable->setItem(row, 0, new QTableWidgetItem(
            alert.timestamp.toString("MM-dd hh:mm")));

        // 风险类型
        QString typeText;
        switch (alert.type) {
        case RiskType::PriceDrop: typeText = QStringLiteral("价格下跌"); break;
        case RiskType::VolumeSpike: typeText = QStringLiteral("成交量异常"); break;
        case RiskType::VolatilityHigh: typeText = QStringLiteral("波动率过高"); break;
        case RiskType::DrawdownExceed: typeText = QStringLiteral("回撤超标"); break;
        case RiskType::ConcentrationRisk: typeText = QStringLiteral("持仓集中"); break;
        case RiskType::LiquidityRisk: typeText = QStringLiteral("流动性风险"); break;
        case RiskType::TrendReversal: typeText = QStringLiteral("趋势反转"); break;
        }
        m_alertTable->setItem(row, 1, new QTableWidgetItem(typeText));

        // 风险等级
        QTableWidgetItem* levelItem = new QTableWidgetItem(riskLevelToText(alert.level));
        levelItem->setForeground(QColor(riskLevelToColor(alert.level)));
        m_alertTable->setItem(row, 2, levelItem);

        // 描述
        m_alertTable->setItem(row, 3, new QTableWidgetItem(alert.description));

        // 状态
        QString statusText = alert.acknowledged ? QStringLiteral("已确认") : QStringLiteral("待处理");
        m_alertTable->setItem(row, 4, new QTableWidgetItem(statusText));
    }
}

void RiskIndicatorWidget::onRiskAlertTriggered(const RiskAlert& alert)
{
    if (alert.symbol != m_currentSymbol) return;

    // 更新预警列表
    QVector<RiskAlert> alerts = RiskWarningSystem::instance()->getAlerts(m_currentSymbol);
    updateAlertList(alerts);
}

void RiskIndicatorWidget::onRiskLevelChanged(const QString& symbol, RiskLevel level)
{
    updateRiskDisplay(symbol, level);
}

void RiskIndicatorWidget::onViewDetailsClicked()
{
    emit viewDetailsRequested(m_currentSymbol);
}

void RiskIndicatorWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(Spacing::MD);
    mainLayout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);

    // 标题
    QLabel* titleLabel = new QLabel(QStringLiteral("风险监控"));
    titleLabel->setProperty("dataType", "title");
    mainLayout->addWidget(titleLabel);

    // 股票信息
    QHBoxLayout* symbolLayout = new QHBoxLayout();
    m_symbolLabel = new QLabel(QStringLiteral("股票: --"));
    m_symbolLabel->setProperty("dataType", "label");
    symbolLayout->addWidget(m_symbolLabel);
    symbolLayout->addStretch();
    mainLayout->addLayout(symbolLayout);

    // 风险等级指示器
    QHBoxLayout* riskLayout = new QHBoxLayout();
    QLabel* riskLabel = new QLabel(QStringLiteral("风险等级:"));
    riskLabel->setProperty("dataType", "label");
    riskLayout->addWidget(riskLabel);

    m_riskLevelLabel = new QLabel(QStringLiteral("低风险"));
    m_riskLevelLabel->setObjectName("riskLevelLabel");
    m_riskLevelLabel->setProperty("risk", "low");
    riskLayout->addWidget(m_riskLevelLabel);
    riskLayout->addStretch();

    m_viewDetailsBtn = new QPushButton(QStringLiteral("查看详情"));
    m_viewDetailsBtn->setFixedWidth(80);
    StyleHelper::setSecondaryButton(m_viewDetailsBtn);
    connect(m_viewDetailsBtn, &QPushButton::clicked, this, &RiskIndicatorWidget::onViewDetailsClicked);
    riskLayout->addWidget(m_viewDetailsBtn);

    mainLayout->addLayout(riskLayout);

    // 风险分数进度条
    QHBoxLayout* scoreLayout = new QHBoxLayout();
    QLabel* scoreLabel = new QLabel(QStringLiteral("风险分数:"));
    scoreLabel->setProperty("dataType", "label");
    scoreLayout->addWidget(scoreLabel);

    m_riskScoreBar = new QProgressBar();
    m_riskScoreBar->setRange(0, 100);
    m_riskScoreBar->setValue(0);
    m_riskScoreBar->setFixedHeight(20);
    m_riskScoreBar->setTextVisible(false);
    m_riskScoreBar->setObjectName("riskScoreBar");
    scoreLayout->addWidget(m_riskScoreBar, 1);

    m_riskScoreLabel = new QLabel(QStringLiteral("0"));
    m_riskScoreLabel->setFixedWidth(40);
    m_riskScoreLabel->setProperty("dataType", "value");
    scoreLayout->addWidget(m_riskScoreLabel);

    mainLayout->addLayout(scoreLayout);

    // 预警列表标题
    QLabel* alertTitleLabel = new QLabel(QStringLiteral("预警记录"));
    alertTitleLabel->setProperty("dataType", "title");
    mainLayout->addWidget(alertTitleLabel);

    // 预警列表
    m_alertTable = new QTableWidget();
    m_alertTable->setObjectName("alertTable");
    m_alertTable->setColumnCount(5);
    m_alertTable->setHorizontalHeaderLabels({
        QStringLiteral("时间"),
        QStringLiteral("类型"),
        QStringLiteral("等级"),
        QStringLiteral("描述"),
        QStringLiteral("状态")
    });
    m_alertTable->horizontalHeader()->setStretchLastSection(true);
    m_alertTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_alertTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_alertTable->setAlternatingRowColors(true);
    m_alertTable->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_alertTable, 1);
}

void RiskIndicatorWidget::updateRiskLevelIndicator(RiskLevel level)
{
    QString text = riskLevelToText(level);
    QString riskStatus;

    m_riskLevelLabel->setText(text);

    // 使用属性选择器设置风险等级
    switch (level)
    {
    case RiskLevel::Low:
        riskStatus = "low";
        break;
    case RiskLevel::Medium:
        riskStatus = "medium";
        break;
    case RiskLevel::High:
        riskStatus = "high";
        break;
    case RiskLevel::Critical:
        riskStatus = "critical";
        break;
    default:
        riskStatus = "unknown";
    }

    m_riskLevelLabel->setProperty("risk", riskStatus);
    StyleHelper::refreshStyle(m_riskLevelLabel);

    // 更新进度条
    int score = 0;
    switch (level) {
    case RiskLevel::Low: score = 25; break;
    case RiskLevel::Medium: score = 50; break;
    case RiskLevel::High: score = 75; break;
    case RiskLevel::Critical: score = 100; break;
    }

    m_riskScoreBar->setValue(score);
    m_riskScoreBar->setProperty("riskLevel", riskStatus);
    StyleHelper::refreshStyle(m_riskScoreBar);
    m_riskScoreLabel->setText(QString::number(score));
}

QString RiskIndicatorWidget::riskLevelToColor(RiskLevel level) const
{
    switch (level) {
    case RiskLevel::Low: return Colors::Success;
    case RiskLevel::Medium: return Colors::Warning;
    case RiskLevel::High: return Colors::WarningLight;
    case RiskLevel::Critical: return Colors::Danger;
    default: return Colors::TextSecondary;
    }
}

QString RiskIndicatorWidget::riskLevelToText(RiskLevel level) const
{
    switch (level) {
    case RiskLevel::Low: return QStringLiteral("低风险");
    case RiskLevel::Medium: return QStringLiteral("中风险");
    case RiskLevel::High: return QStringLiteral("高风险");
    case RiskLevel::Critical: return QStringLiteral("极高风险");
    default: return QStringLiteral("未知");
    }
}
