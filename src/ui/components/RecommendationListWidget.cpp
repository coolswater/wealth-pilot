/**
 * @file RecommendationListWidget.cpp
 * @brief 推荐列表组件实现
 */

#include "RecommendationListWidget.h"
#include "core/config/Tokens.h"
#include <QHeaderView>

using namespace Tokens;

RecommendationListWidget::RecommendationListWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();

    // 连接推荐系统信号
    auto* recomm = PersonalizedRecommendation::instance();
    connect(recomm, &PersonalizedRecommendation::recommendationsUpdated,
            this, &RecommendationListWidget::onRecommendationsUpdated);
}

RecommendationListWidget::~RecommendationListWidget()
{
}

void RecommendationListWidget::refreshRecommendations()
{
    PersonalizedRecommendation::instance()->updateRecommendationScores();
}

void RecommendationListWidget::onRecommendationsUpdated(const QVector<StockRecommendation>& recommendations)
{
    m_currentRecommendations = recommendations;
    updateRecommendationList(recommendations);
}

void RecommendationListWidget::onStyleFilterChanged(int index)
{
    UserPreference preference;

    switch (index) {
    case 0: // 保守型
        preference.style = InvestmentStyle::Conservative;
        preference.riskTolerance = 30.0;
        break;
    case 1: // 平衡型
        preference.style = InvestmentStyle::Balanced;
        preference.riskTolerance = 50.0;
        break;
    case 2: // 进取型
        preference.style = InvestmentStyle::Aggressive;
        preference.riskTolerance = 70.0;
        break;
    }

    PersonalizedRecommendation::instance()->setUserPreference(preference);
}

void RecommendationListWidget::onRefreshClicked()
{
    refreshRecommendations();
}

void RecommendationListWidget::onStockDoubleClicked(int row, int column)
{
    Q_UNUSED(column);

    if (row >= 0 && row < m_currentRecommendations.size()) {
        emit stockSelected(m_currentRecommendations[row].symbol);
    }
}

void RecommendationListWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 标题栏
    QHBoxLayout* titleLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(QStringLiteral("智能推荐"));
    m_titleLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;")
        .arg(Colors::TextPrimary));
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();

    // 投资风格筛选
    QLabel* styleLabel = new QLabel(QStringLiteral("投资风格:"));
    styleLabel->setStyleSheet(QString("font-size: 12px; color: %1;")
        .arg(Colors::TextSecondary));
    titleLayout->addWidget(styleLabel);

    m_styleFilter = new QComboBox();
    m_styleFilter->addItem(QStringLiteral("保守型"));
    m_styleFilter->addItem(QStringLiteral("平衡型"));
    m_styleFilter->addItem(QStringLiteral("进取型"));
    m_styleFilter->setCurrentIndex(1); // 默认平衡型
    m_styleFilter->setFixedWidth(100);
    connect(m_styleFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RecommendationListWidget::onStyleFilterChanged);
    titleLayout->addWidget(m_styleFilter);

    m_refreshBtn = new QPushButton(QStringLiteral("刷新"));
    m_refreshBtn->setFixedWidth(60);
    connect(m_refreshBtn, &QPushButton::clicked, this, &RecommendationListWidget::onRefreshClicked);
    titleLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(titleLayout);

    // 推荐列表
    m_recommendationTable = new QTableWidget();
    m_recommendationTable->setColumnCount(5);
    m_recommendationTable->setHorizontalHeaderLabels({
        QStringLiteral("股票代码"),
        QStringLiteral("推荐分数"),
        QStringLiteral("风险等级"),
        QStringLiteral("推荐理由"),
        QStringLiteral("投资建议")
    });
    m_recommendationTable->horizontalHeader()->setStretchLastSection(true);
    m_recommendationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recommendationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recommendationTable->setAlternatingRowColors(true);
    m_recommendationTable->verticalHeader()->setVisible(false);
    m_recommendationTable->setStyleSheet(QString(
        "QTableWidget { background-color: %1; border: 1px solid %2; }"
        "QTableWidget::item { padding: 5px; }"
    ).arg(Colors::BgSurface, Colors::Border));
    connect(m_recommendationTable, &QTableWidget::cellDoubleClicked,
            this, &RecommendationListWidget::onStockDoubleClicked);
    mainLayout->addWidget(m_recommendationTable, 1);

    // 提示信息
    QLabel* tipLabel = new QLabel(QStringLiteral("双击股票代码查看详情"));
    tipLabel->setStyleSheet(QString("font-size: 12px; color: %1;")
        .arg(Colors::TextSecondary));
    mainLayout->addWidget(tipLabel);

    setStyleSheet(QString("background-color: %1; border-radius: 8px;")
        .arg(Colors::BgElevated));
}

void RecommendationListWidget::updateRecommendationList(const QVector<StockRecommendation>& recommendations)
{
    m_recommendationTable->setRowCount(0);

    for (const auto& rec : recommendations) {
        int row = m_recommendationTable->rowCount();
        m_recommendationTable->insertRow(row);

        // 股票代码
        m_recommendationTable->setItem(row, 0, new QTableWidgetItem(rec.symbol));

        // 推荐分数
        QTableWidgetItem* scoreItem = new QTableWidgetItem(QString::number(rec.score, 'f', 1));
        if (rec.score >= 70) {
            scoreItem->setForeground(QColor(Colors::Success));
        } else if (rec.score >= 50) {
            scoreItem->setForeground(QColor(Colors::TextPrimary));
        } else {
            scoreItem->setForeground(QColor(Colors::Danger));
        }
        m_recommendationTable->setItem(row, 1, scoreItem);

        // 风险等级
        QString riskText = riskLevelToText(rec.riskLevel);
        QTableWidgetItem* riskItem = new QTableWidgetItem(riskText);
        switch (rec.riskLevel) {
        case RiskLevel::Low:
            riskItem->setForeground(QColor(Colors::Success));
            break;
        case RiskLevel::Medium:
            riskItem->setForeground(QColor(Colors::Warning));
            break;
        case RiskLevel::High:
            riskItem->setForeground(QColor(Colors::WarningLight));
            break;
        case RiskLevel::Critical:
            riskItem->setForeground(QColor(Colors::Danger));
            break;
        }
        m_recommendationTable->setItem(row, 2, riskItem);

        // 推荐理由
        QString reasons;
        for (const auto& reason : rec.reasons) {
            reasons += reason.description + QStringLiteral("；");
        }
        if (!reasons.isEmpty()) {
            reasons.chop(1); // 移除最后的分号
        }
        m_recommendationTable->setItem(row, 3, new QTableWidgetItem(reasons));

        // 投资建议
        m_recommendationTable->setItem(row, 4, new QTableWidgetItem(rec.suggestion));
    }
}

QString RecommendationListWidget::riskLevelToText(RiskLevel level) const
{
    switch (level) {
    case RiskLevel::Low: return QStringLiteral("低风险");
    case RiskLevel::Medium: return QStringLiteral("中风险");
    case RiskLevel::High: return QStringLiteral("高风险");
    case RiskLevel::Critical: return QStringLiteral("极高风险");
    default: return QStringLiteral("未知");
    }
}
