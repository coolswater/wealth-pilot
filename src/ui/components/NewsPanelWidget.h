/**
 * @file NewsPanelWidget.h
 * @brief 新闻资讯面板 - 显示新闻和舆情分析
 *
 * @details 提供新闻展示功能：
 * - 新闻列表展示
 * - 情感分析显示
 * - 社交热度监控
 * - 新闻筛选
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef NEWSPANELWIDGET_H
#define NEWSPANELWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include "market/NewsDataSource.h"

/**
 * @brief 新闻资讯面板
 */
class NewsPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NewsPanelWidget(QWidget* parent = nullptr);
    ~NewsPanelWidget() override;

    /**
     * @brief 设置股票代码
     */
    void setSymbol(const QString& symbol);

    /**
     * @brief 刷新新闻
     */
    void refreshNews();

signals:
    void newsSelected(const NewsItem& news);

private slots:
    void onNewsUpdated(const QString& symbol, const QVector<NewsItem>& news);
    void onSocialHeatUpdated(const QString& symbol, const SocialHeatData& heat);
    void onCategoryFilterChanged(int index);
    void onRefreshClicked();
    void onNewsDoubleClicked(int row, int column);

private:
    void setupUI();
    void updateNewsList(const QVector<NewsItem>& news);
    void updateSocialHeat(const SocialHeatData& heat);
    QString sentimentToColor(SentimentType sentiment) const;
    QString sentimentToText(SentimentType sentiment) const;

    // UI组件
    QLabel* m_titleLabel = nullptr;
    QComboBox* m_categoryFilter = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QTableWidget* m_newsTable = nullptr;
    QLabel* m_socialHeatLabel = nullptr;

    QString m_currentSymbol;
    QVector<NewsItem> m_currentNews;
};

#endif // NEWSPANELWIDGET_H