/**
 * @file DashboardPageRefactored.h
 * @brief 金融行情综合看板页面 - 重构版
 *
 * @details 重构要点：
 * 1. 将原 2945 行拆分为多个组件
 * 2. 使用 DataHub 统一数据调度，移除独立 QTimer
 * 3. 职责分离：UI 构建、数据处理、缓存管理
 *
 * @author WealthPilot Team
 * @version 6.0.0 (重构版)
 */

#ifndef DASHBOARDPAGE_REFACTORED_H
#define DASHBOARDPAGE_REFACTORED_H

#include <presentation/components/DataHubPageBase.h>
#include "DashboardTypes.h"
#include <memory>

// 前向声明
class QLabel;

namespace WealthPilot {

// 前向声明组件
class IndexPanel;
class RankGridPanel;
class InfoPanel;

/**
 * @brief 金融行情综合看板页面 - 重构版
 *
 * 架构改进：
 * - 组件化：IndexPanel、RankGridPanel、InfoPanel
 * - 数据流：统一使用 DataHub 订阅
 * - 定时器：移除独立 QTimer，使用 DataHub 调度
 */
class DashboardPage : public DataHubPageBase {
    Q_OBJECT

public:
    explicit DashboardPage(QWidget* parent = nullptr);
    ~DashboardPage() override;

    QString pageId() const override { return QStringLiteral("DashboardPage"); }

    /**
     * @brief 初始化页面
     */
    void initializePage() override;

    /**
     * @brief 刷新数据
     */
    void refreshData();

signals:
    void navigateToStockKLine(const QString& symbol, const QString& name);

private slots:
    // DataHub 数据回调
    void onIndexDataReceived(const QVariant& data);
    void onRankDataReceived(const QVariant& data);
    void onWatchlistDataReceived(const QVariant& data);
    void onNewsDataReceived(const QVariant& data);
    void onMoneyFlowDataReceived(const QVariant& data);

private:
    void setupUI();
    void setupHeader();
    void setupDataHubSubscriptions();
    void setupConnections();
    void updateTheme();

    void loadDataWithFallback();
    bool loadFromCache();
    bool loadFromDatabase();
    void loadFromNetwork();

    void saveToCache();
    void saveToDatabase();

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // DASHBOARDPAGE_REFACTORED_H