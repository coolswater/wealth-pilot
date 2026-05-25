/**
 * @file FundPage.h
 * @brief 基金页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 基金列表展示（场内基金ETF、LOF、场外基金）
 * - 基金详情页（净值、持仓、业绩）
 * - 基金K线图（场内基金）
 * - 基金搜索与筛选
 * - 基金对比功能
 * - 自选基金管理
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅基金行情数据
 * - 自动生命周期管理
 * - 实时净值更新
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef FUNDPAGE_H
#define FUNDPAGE_H

#include "presentation/components/DataHubPageBase.h"
#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QDateTime>
#include <memory>

// 前向声明
class KLineChart;

#include "data/market/FundDataSource.h"

/**
 * @brief 基金持仓结构
 */
struct FundHolding {
    QString stockCode;      ///< 股票代码
    QString stockName;      ///< 股票名称
    double ratio = 0.0;     ///< 持仓比例
    double shares = 0.0;    ///< 持仓股数（万股）
    double value = 0.0;     ///< 持仓市值（万元）
    double change = 0.0;    ///< 涨跌幅
};

/**
 * @brief 基金页面类
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅基金行情数据（market:fund:*）
 * - 实时净值更新
 * - 页面销毁时自动取消订阅
 */
class FundPage : public WealthPilot::DataHubPageBase
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit FundPage(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~FundPage() override;

    // ========== 页面信息 ==========

    /**
     * @brief 获取页面ID
     */
    QString pageId() const override { return QStringLiteral("Fund"); }

    /**
     * @brief 获取页面名称
     */
    QString pageName() const override { return QStringLiteral("基金"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 基金数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    /**
     * @brief 刷新数据
     */
    void refresh();

    /**
     * @brief 设置基金代码
     * @param code 基金代码
     */
    void setFund(const QString& code);

signals:
    /**
     * @brief 基金选中信号
     */
    void fundSelected(const QString& code, const QString& name);

private slots:
    // ========== UI 交互槽函数 ==========

    /**
     * @brief 基金类型切换
     */
    void onFundTypeChanged(int index);

    /**
     * @brief 搜索基金
     */
    void onSearchTextChanged(const QString& text);

    /**
     * @brief 基金列表行选中
     */
    void onFundListClicked(int row, int column);

    /**
     * @brief 添加到自选
     */
    void onAddToWatchlist();

    /**
     * @brief 刷新基金数据
     */
    void onRefreshData();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void initToolBar();
    void initFundList();
    void initDetailPanel();
    void initConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 使用 subscribeQuote() 订阅主要基金
     * 2. 使用模式订阅 market:fund:*
     * 3. 回调函数中更新表格显示
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载 ==========

    void loadFundList();
    void updateFundDetail(const FundQuote& quote);
    void loadFundHolding(const QString& code);
    void loadFundKLine(const QString& code);

    /**
     * @brief 格式化基金类型
     */
    static QString formatFundType(FundType type);

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的基金列表
     */
    QStringList m_subscribedFunds;
};

#endif // FUNDPAGE_H