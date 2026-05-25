/**
 * @file NewsPage.h
 * @brief 新闻资讯页面 - 使用 DataHub 数据中心
 *
 * @details 布局结构：
 * - 顶部：分类过滤栏
 * - 中间：滚动卡片列表
 * - 无标题栏/状态栏
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅新闻数据
 * - 自动生命周期管理
 * - 支持按股票代码订阅相关新闻
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef NEWSPAGE_H
#define NEWSPAGE_H

#pragma once

#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <memory>
#include <presentation/components/DataHubPageBase.h>
#include "data/market/NewsDataSource.h"

class QVBoxLayout;

/**
 * @brief 新闻卡片数据
 */
struct NewsCardData {
    QString id;                     ///< 新闻ID
    QString title;                  ///< 标题
    QString summary;                ///< 摘要
    QString source;                 ///< 来源
    QString category;               ///< 分类
    QDateTime publishTime;          ///< 发布时间
    QStringList highlightNumbers;   ///< 需要高亮的数字列表
    QString fullContent;            ///< 完整内容
};

/**
 * @brief 新闻卡片 Widget
 *
 * @details 特性：
 * - 鼠标悬停效果
 * - 点击展开详情
 * - 数字高亮显示
 */
class NewsCardWidget : public QFrame {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param data 新闻数据
     * @param parent 父窗口
     */
    explicit NewsCardWidget(const NewsCardData& data, QWidget* parent = nullptr);

    /**
     * @brief 设置是否为最后一张卡片（控制分隔线显示）
     */
    void setLastCard(bool isLast);

signals:
    /**
     * @brief 卡片点击信号
     */
    void clicked();

    /**
     * @brief 请求详情信号
     */
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
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅新闻数据（news:*）
 * - 支持按分类过滤
 * - 页面销毁时自动取消订阅
 */
class NewsPage : public WealthPilot::DataHubPageBase {
    Q_OBJECT

public:
    explicit NewsPage(QWidget* parent = nullptr);
    ~NewsPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override;
    QString pageName() const override { return QStringLiteral("新闻"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 新闻数据
     * 3. 加载初始数据
     */
    void initializePage() override;

private slots:
    // ========== UI 交互槽函数 ==========

    /**
     * @brief 分类点击
     */
    void onCategoryClicked(const QString& category);

    /**
     * @brief 卡片点击
     */
    void onCardClicked();

    /**
     * @brief 请求详情
     */
    void onDetailRequested(const NewsCardData& data);

    // ========== 数据接收槽函数 ==========

    /**
     * @brief 新闻数据接收
     */
    void onNewsReceived(const QString& symbol, const QVector<NewsItem>& news);

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupCategoryBar();
    void setupScrollArea();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 使用模式订阅 news:*
     * 2. 支持按股票代码订阅相关新闻
     * 3. 回调函数中更新卡片列表
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载 ==========

    void loadDemoData();

    /**
     * @brief 更新卡片列表
     * @param filter 分类过滤（空表示全部）
     */
    void updateCards(const QString& filter = QString());

    /**
     * @brief 显示详情对话框
     */
    void showDetailDialog(const NewsCardData& data);

    // ========== 私有实现类（PIMPL） ==========
    class Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的新闻分类
     */
    QStringList m_subscribedCategories;
};

#endif // NEWSPAGE_H