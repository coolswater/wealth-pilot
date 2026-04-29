/**
 * @file FundPage.h
 * @brief 基金页面 - 基金行情展示与分析
 *
 * @details 功能：
 * - 基金列表展示（场内基金ETF、LOF、场外基金）
 * - 基金详情页（净值、持仓、业绩）
 * - 基金K线图（场内基金）
 * - 基金搜索与筛选
 * - 基金对比功能
 * - 自选基金管理
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FUNDPAGE_H
#define FUNDPAGE_H

#include "core/base/BasePage.h"
#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QDateTime>
#include <memory>

// 前向声明
class KLineChart;

#include "market/FundDataSource.h"

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
 */
class FundPage : public BasePage
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

    /**
     * @brief 获取页面ID
     */
    QString pageId() const override { return QStringLiteral("Fund"); }
    
    /**
     * @brief 获取页面名称
     */
    QString pageName() const override { return QStringLiteral("基金"); }

    /**
     * @brief 初始化页面（BasePage接口）
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
    /**
     * @brief 初始化UI
     */
    void setupUI();
    
    /**
     * @brief 初始化工具栏
     */
    void initToolBar();
    
    /**
     * @brief 初始化基金列表
     */
    void initFundList();
    
    /**
     * @brief 初始化详情面板
     */
    void initDetailPanel();
    
    /**
     * @brief 初始化连接
     */
    void initConnections();
    
    /**
     * @brief 加载基金列表
     */
    void loadFundList();
    
    /**
     * @brief 更新基金详情
     */
    void updateFundDetail(const FundQuote& quote);
    
    /**
     * @brief 加载基金持仓
     */
    void loadFundHolding(const QString& code);
    
    /**
     * @brief 加载基金K线
     */
    void loadFundKLine(const QString& code);
    
    /**
     * @brief 格式化基金类型
     */
    static QString formatFundType(FundType type);

    // PIMPL实现
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FUNDPAGE_H
