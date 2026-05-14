/**
 * @file ForexPage.h
 * @brief 外汇页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 主要货币对行情展示（USD/CNY, EUR/USD等）
 * - 汇率走势图
 * - 外汇新闻资讯
 * - 汇率换算工具
 * - 历史汇率查询
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅外汇行情数据
 * - 自动生命周期管理
 * - 实时汇率更新
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef FOREXPAGE_H
#define FOREXPAGE_H

#include "ui/components/DataHubPageBase.h"
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QDateTime>
#include <memory>

// 前向声明
class KLineChart;

#include "market/ForexDataSource.h"

/**
 * @brief 外汇页面类
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅外汇行情数据（market:forex:*）
 * - 实时汇率更新
 * - 页面销毁时自动取消订阅
 */
class ForexPage : public WealthPilot::DataHubPageBase
{
    Q_OBJECT

public:
    explicit ForexPage(QWidget *parent = nullptr);
    ~ForexPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return QStringLiteral("Forex"); }
    QString pageName() const override { return QStringLiteral("外汇"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 外汇数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    /**
     * @brief 刷新数据
     */
    void refresh();

signals:
    /**
     * @brief 外汇对选中信号
     */
    void forexPairSelected(const QString& pair, double rate);

private slots:
    // ========== UI 交互槽函数 ==========

    void onForexListClicked(int row, int column);
    void onCurrencyFromChanged(int index);
    void onCurrencyToChanged(int index);
    void onAmountChanged(double amount);
    void onRefreshData();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void initToolBar();
    void initForexList();
    void initRateChart();
    void initConverter();
    void initConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 使用 subscribeQuote() 订阅主要货币对
     * 2. 使用模式订阅 market:forex:*
     * 3. 回调函数中更新表格显示
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载 ==========

    void loadForexList();
    void updateForexTable();
    void updateForexDetail(const ForexQuote& quote);
    void loadRateHistory(const QString& pair);
    void calculateConversion();

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的货币对列表
     */
    QStringList m_subscribedPairs;
};

#endif // FOREXPAGE_H