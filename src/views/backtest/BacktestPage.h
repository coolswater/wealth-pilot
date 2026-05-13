/**
 * @file BacktestPage.h
 * @brief 策略回测页面 - 量化策略回测与分析
 *
 * @details 功能：
 * - 策略编写与编辑
 * - 历史数据回测
 * - 回测结果展示（收益曲线、最大回撤、夏普比率）
 * - 策略参数优化
 * - 回测报告导出
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef BACKTESTPAGE_H
#define BACKTESTPAGE_H

#include "ui/components/BasePage.h"
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
 * @brief 回测交易记录结构（用于回测结果展示）
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
 * @brief 策略回测页面�?
 */
class BacktestPage : public WealthPilot::BasePage
{
    Q_OBJECT

public:
    explicit BacktestPage(QWidget *parent = nullptr);
    ~BacktestPage() override;

    QString pageId() const override { return QStringLiteral("Backtest"); }
    QString pageName() const override { return QStringLiteral("量化"); }

    void initializePage() override;
    void refresh();

signals:
    /**
     * @brief 回测完成信号
     */
    void backtestCompleted(const BacktestResult& result);

private slots:
    void onRunBacktest();
    void onStopBacktest();
    void onExportReport();
    void onStrategyChanged(int index);
    void onSymbolChanged(const QString& symbol);
    void onTradeClicked(int row, int column);

private:
    void setupUI();
    void initToolBar();
    void initStrategyEditor();
    void initResultPanel();
    void initTradeHistory();
    void initConnections();
    void initStrategies();
    void updateResult(const BacktestResult& result);
    void updateTradeTable(const QVector<BacktestTradeRecord>& trades);
    void generateMockBacktest();
    void runBacktest(const QString& symbol, const QDate& startDate, const QDate& endDate);
    void exportReport(const QString& filePath);

    struct Impl;
    std::unique_ptr<Impl> d;
};



 // BACKTESTPAGE_H

#endif
