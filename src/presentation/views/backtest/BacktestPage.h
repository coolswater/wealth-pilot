/**
 * @file BacktestPage.h
 * @brief 策略回测页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 策略编写与编辑
 * - 历史数据回测
 * - 回测结果展示（收益曲线、最大回撤、夏普比率）
 * - 策略参数优化
 * - 回测报告导出
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅历史数据
 * - 自动生命周期管理
 * - 回测结果实时更新
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef BACKTESTPAGE_H
#define BACKTESTPAGE_H

#include "presentation/components/DataHubPageBase.h"
#include <QWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QDateTime>
#include <memory>

// 前向声明
class KLineChart;

/**
 * @brief 回测结果数据结构
 */
struct BacktestResult {
    double totalReturn = 0.0;       ///< 总收益率
    double annualReturn = 0.0;      ///< 年化收益率
    double maxDrawdown = 0.0;       ///< 最大回撤
    double sharpeRatio = 0.0;       ///< 夏普比率
    double winRate = 0.0;           ///< 胜率
    double profitFactor = 0.0;      ///< 盈亏比
    int totalTrades = 0;            ///< 总交易次数
    int winTrades = 0;              ///< 盈利次数
    int lossTrades = 0;             ///< 亏损次数
    double maxProfit = 0.0;         ///< 单笔最大盈利
    double maxLoss = 0.0;           ///< 单笔最大亏损
    double avgProfit = 0.0;         ///< 平均盈利
    double avgLoss = 0.0;           ///< 平均亏损
    double avgHoldingDays = 0.0;    ///< 平均持仓天数
};

/**
 * @brief 回测交易记录结构
 */
struct BacktestTradeRecord {
    QDateTime time;                 ///< 交易时间
    QString action;                 ///< 买卖方向
    double price = 0.0;             ///< 成交价格
    int volume = 0;                 ///< 成交数量
    double profit = 0.0;            ///< 盈亏
    double cumProfit = 0.0;         ///< 累计盈亏
};

/**
 * @brief 策略回测页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅历史K线数据（market:kline:*）
 * - 订阅回测进度（backtest:progress）
 * - 页面销毁时自动取消订阅
 */
class BacktestPage : public WealthPilot::DataHubPageBase
{
    Q_OBJECT

public:
    explicit BacktestPage(QWidget *parent = nullptr);
    ~BacktestPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return QStringLiteral("Backtest"); }
    QString pageName() const override { return QStringLiteral("量化"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    /**
     * @brief 刷新数据
     */
    void refresh();

signals:
    /**
     * @brief 回测完成信号
     */
    void backtestCompleted(const BacktestResult& result);

private slots:
    // ========== UI 交互槽函数 ==========

    void onRunBacktest();
    void onStopBacktest();
    void onExportReport();
    void onStrategyChanged(int index);
    void onSymbolChanged(const QString& symbol);
    void onTradeClicked(int row, int column);

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void initToolBar();
    void initStrategyEditor();
    void initResultPanel();
    void initTradeHistory();
    void initConnections();
    void initStrategies();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅回测进度（backtest:progress）
     * 2. 订阅回测结果（backtest:result）
     * 3. 回调函数中更新显示
     */
    void setupDataHubSubscriptions();

    // ========== 数据更新 ==========

    void updateResult(const BacktestResult& result);
    void updateTradeTable(const QVector<BacktestTradeRecord>& trades);
    void generateMockBacktest();
    void runBacktest(const QString& symbol, const QDate& startDate, const QDate& endDate);
    void exportReport(const QString& filePath);

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的回测任务ID
     */
    QString m_currentBacktestId;
};

#endif // BACKTESTPAGE_H