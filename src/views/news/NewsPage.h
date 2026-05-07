#ifndef NEWSPAGE_H
#define NEWSPAGE_H

/**
 * @file NewsPage.h
 * @brief 新闻资讯页面 - 垂直滚动卡片列表设计
 * 
 * @details 布局结构：
 * - 顶部：分类过滤栏
 * - 中间：滚动卡片列表
 * - 无标题栏/状态栏
 */

#pragma once

#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <memory>
#include <ui/components/BasePage.h>
#include "market/NewsDataSource.h"

class QVBoxLayout;

/**
 * @brief 新闻卡片数据
 */
struct NewsCardData {
    QString id;
    QString title;
    QString summary;
    QString source;
    QString category;
    QDateTime publishTime;
    QStringList highlightNumbers;  ///< 需要高亮的数字列表
    QString fullContent;           ///< 完整内容
};

/**
 * @brief 新闻卡片 Widget
 */
class NewsCardWidget : public QFrame {
    Q_OBJECT

public:
    explicit NewsCardWidget(const NewsCardData& data, QWidget* parent = nullptr);
    
    void setLastCard(bool isLast);

signals:
    void clicked();
    void detailRequested(const NewsCardData& data);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setupUI();
    QString formatTime(const QDateTime& time) const;
    QString highlightNumbersInText(const QString& text, const QStringList& numbers) const;

    NewsCardData m_data;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_timeLabel = nullptr;
    QLabel* m_sourceLabel = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QPushButton* m_detailBtn = nullptr;
    QFrame* m_separator = nullptr;
};

/**
 * @brief 新闻资讯页面
 */
class NewsPage : public WealthPilot::BasePage {
    Q_OBJECT

public:
    explicit NewsPage(QWidget* parent = nullptr);
    ~NewsPage() override;

    QString pageId() const override;
    void initializePage() override;

private slots:
    void onCategoryClicked(const QString& category);
    void onCardClicked();
    void onDetailRequested(const NewsCardData& data);
    void onNewsReceived(const QString& symbol, const QVector<NewsItem>& news);

private:
    void setupUI();
    void setupCategoryBar();
    void setupScrollArea();
    void loadDemoData();
    void updateCards(const QString& filter = QString());
    void showDetailDialog(const NewsCardData& data);

    class Impl;
    std::unique_ptr<Impl> d;
};

#endif
