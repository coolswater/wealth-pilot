/**
 * @file NewsPage.h
 * @brief 新闻资讯页面 - 金融新闻展示
 */

#pragma once

#include <QListWidget>
#include <QTextBrowser>
#include <memory>
#include <core/base/BasePage.h>
#include "market/NewsDataSource.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QComboBox;
class QTimer;
QT_END_NAMESPACE

/**
 * @brief 新闻资讯页面
 */
class NewsPage : public BasePage {
    Q_OBJECT

public:
    explicit NewsPage(QWidget* parent = nullptr);
    ~NewsPage() override;

    QString pageId() const override;
    void initializePage() override;
    void onPageActivated(const QVariantMap& params = {}) override;

private slots:
    void onNewsReceived(const QVector<NewsItem>& news);
    void onNewsSelected(QListWidgetItem* item);
    void onFilterChanged(int index);
    void onRefreshNews();
    void onAutoRefresh();

private:
    void setupUI();
    void setupConnections();
    void requestNews();
    void displayNewsDetail(const NewsItem& news);
    void updateNewsList(const QString& category = QString());

    class Impl;
    std::unique_ptr<Impl> d;
};
