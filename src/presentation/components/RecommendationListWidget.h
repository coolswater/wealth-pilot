/**
 * @file RecommendationListWidget.h
 * @brief 推荐列表组件 - 显示个性化推荐股票
 *
 * @details 提供推荐展示功能：
 * - 推荐股票列表
 * - 推荐分数显示
 * - 推荐理由展示
 * - 投资风格筛选
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef RECOMMENDATIONLISTWIDGET_H
#define RECOMMENDATIONLISTWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "core/domain/recommendation/PersonalizedRecommendation.h"

/**
 * @brief 推荐列表组件
 */
class RecommendationListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecommendationListWidget(QWidget* parent = nullptr);
    ~RecommendationListWidget() override;

    /**
     * @brief 刷新推荐列表
     */
    void refreshRecommendations();

signals:
    /**
     * @brief 股票选中信号
     */
    void stockSelected(const QString& symbol);

private slots:
    void onRecommendationsUpdated(const QVector<StockRecommendation>& recommendations);
    void onStyleFilterChanged(int index);
    void onRefreshClicked();
    void onStockDoubleClicked(int row, int column);

private:
    void setupUI();
    void updateRecommendationList(const QVector<StockRecommendation>& recommendations);
    QString riskLevelToText(RiskLevel level) const;

    // UI组件
    QLabel* m_titleLabel = nullptr;
    QComboBox* m_styleFilter = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QTableWidget* m_recommendationTable = nullptr;

    QVector<StockRecommendation> m_currentRecommendations;
};

#endif // RECOMMENDATIONLISTWIDGET_H