/**
 * @file BacktestReportWidget.cpp
 * @brief 回测报告面板实现
 */

#include "BacktestReportWidget.h"
#include "shared/utils/Logger.h"

#include <QScrollArea>
#include <QHeaderView>
#include <QGroupBox>
#include <QGridLayout>

namespace WealthPilot {

struct BacktestReportWidget::Impl {
    BacktestStats stats;
    QVector<BacktestDataPoint> equityCurve;
    QVector<TradeMarker> trades;
    
    // UI 组件
    BacktestChartWidget* equityChart = nullptr;
    QTableWidget* tradeTable = nullptr;
    QTableWidget* monthlyTable = nullptr;
    
    // 指标标签
    QLabel* totalReturnLabel = nullptr;
    QLabel* annualizedReturnLabel = nullptr;
    QLabel* maxDrawdownLabel = nullptr;
    QLabel* sharpeRatioLabel = nullptr;
    QLabel* winRateLabel = nullptr;
    QLabel* profitFactorLabel = nullptr;
    QLabel* totalTradesLabel = nullptr;
    QLabel* avgHoldingDaysLabel = nullptr;
};

BacktestReportWidget::BacktestReportWidget(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

BacktestReportWidget::~BacktestReportWidget() = default;

void BacktestReportWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // 顶部：关键指标
    auto* statsGroup = new QGroupBox("关键指标");
    auto* statsLayout = new QGridLayout(statsGroup);
    statsLayout->setSpacing(15);
    
    auto createStatLabel = [](const QString& title, const QString& value) {
        auto* layout = new QVBoxLayout();
        auto* titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("color: #888; font-size: 11px;");
        auto* valueLabel = new QLabel(value);
        valueLabel->setStyleSheet("color: #fff; font-size: 16px; font-weight: bold;");
        layout->addWidget(titleLabel);
        layout->addWidget(valueLabel);
        return layout;
    };
    
    statsLayout->addLayout(createStatLabel("总收益率", "0.00%"), 0, 0);
    statsLayout->addLayout(createStatLabel("年化收益", "0.00%"), 0, 1);
    statsLayout->addLayout(createStatLabel("最大回撤", "0.00%"), 0, 2);
    statsLayout->addLayout(createStatLabel("夏普比率", "0.00"), 0, 3);
    statsLayout->addLayout(createStatLabel("胜率", "0.00%"), 1, 0);
    statsLayout->addLayout(createStatLabel("盈亏比", "0.00"), 1, 1);
    statsLayout->addLayout(createStatLabel("交易次数", "0"), 1, 2);
    statsLayout->addLayout(createStatLabel("平均持仓", "0天"), 1, 3);
    
    // 保存标签引用以便更新
    auto labels = statsGroup->findChildren<QLabel*>();
    d->totalReturnLabel = labels[1];
    d->annualizedReturnLabel = labels[3];
    d->maxDrawdownLabel = labels[5];
    d->sharpeRatioLabel = labels[7];
    d->winRateLabel = labels[9];
    d->profitFactorLabel = labels[11];
    d->totalTradesLabel = labels[13];
    d->avgHoldingDaysLabel = labels[15];
    
    mainLayout->addWidget(statsGroup);
    
    // 中部：资金曲线图
    auto* chartGroup = new QGroupBox("资金曲线");
    auto* chartLayout = new QVBoxLayout(chartGroup);
    
    d->equityChart = new BacktestChartWidget();
    chartLayout->addWidget(d->equityChart);
    
    mainLayout->addWidget(chartGroup, 2);  // stretch=2
    
    // 底部：交易列表和月度收益
    auto* bottomLayout = new QHBoxLayout();
    
    // 交易列表
    auto* tradesGroup = new QGroupBox("交易记录");
    auto* tradesLayout = new QVBoxLayout(tradesGroup);
    
    d->tradeTable = new QTableWidget();
    d->tradeTable->setColumnCount(6);
    d->tradeTable->setHorizontalHeaderLabels({"日期", "股票", "方向", "价格", "数量", "盈亏"});
    d->tradeTable->horizontalHeader()->setStretchLastSection(true);
    d->tradeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->tradeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    tradesLayout->addWidget(d->tradeTable);
    bottomLayout->addWidget(tradesGroup);
    
    // 月度收益
    auto* monthlyGroup = new QGroupBox("月度收益");
    auto* monthlyLayout = new QVBoxLayout(monthlyGroup);
    
    d->monthlyTable = new QTableWidget();
    d->monthlyTable->setColumnCount(3);
    d->monthlyTable->setHorizontalHeaderLabels({"月份", "收益", "交易次数"});
    d->monthlyTable->horizontalHeader()->setStretchLastSection(true);
    d->monthlyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    monthlyLayout->addWidget(d->monthlyTable);
    bottomLayout->addWidget(monthlyGroup);
    
    mainLayout->addLayout(bottomLayout, 1);  // stretch=1
}

void BacktestReportWidget::setBacktestResult(const BacktestStats& stats,
                                              const QVector<BacktestDataPoint>& equityCurve,
                                              const QVector<TradeMarker>& trades)
{
    d->stats = stats;
    d->equityCurve = equityCurve;
    d->trades = trades;
    
    updateStatsDisplay();
    d->equityChart->setData(equityCurve, trades);
    updateTradeTable();
    updateMonthlyReturns();
}

void BacktestReportWidget::clear()
{
    d->stats = BacktestStats();
    d->equityCurve.clear();
    d->trades.clear();
    
    updateStatsDisplay();
    d->equityChart->clearData();
    d->tradeTable->setRowCount(0);
    d->monthlyTable->setRowCount(0);
}

void BacktestReportWidget::updateStatsDisplay()
{
    auto formatPercent = [](double value) -> QString {
        QString text = QString::number(value, 'f', 2) + "%";
        if (value > 0) {
            return "<span style='color: #00c853'>" + text + "</span>";
        } else if (value < 0) {
            return "<span style='color: #ff1744'>" + text + "</span>";
        }
        return text;
    };
    
    d->totalReturnLabel->setText(formatPercent(d->stats.totalReturn));
    d->annualizedReturnLabel->setText(formatPercent(d->stats.annualizedReturn));
    d->maxDrawdownLabel->setText(formatPercent(d->stats.maxDrawdown));
    d->sharpeRatioLabel->setText(QString::number(d->stats.sharpeRatio, 'f', 2));
    d->winRateLabel->setText(QString::number(d->stats.winRate, 'f', 2) + "%");
    d->profitFactorLabel->setText(QString::number(d->stats.profitFactor, 'f', 2));
    d->totalTradesLabel->setText(QString::number(d->stats.totalTrades));
    d->avgHoldingDaysLabel->setText(QString::number(d->stats.avgHoldingDays, 'f', 1) + "天");
}

void BacktestReportWidget::updateTradeTable()
{
    d->tradeTable->setRowCount(d->trades.size());
    
    for (int i = 0; i < d->trades.size(); ++i) {
        const auto& trade = d->trades[i];
        
        d->tradeTable->setItem(i, 0, new QTableWidgetItem(trade.date.toString("yyyy-MM-dd")));
        d->tradeTable->setItem(i, 1, new QTableWidgetItem(trade.symbol));
        d->tradeTable->setItem(i, 2, new QTableWidgetItem(trade.action));
        d->tradeTable->setItem(i, 3, new QTableWidgetItem(QString::number(trade.price, 'f', 2)));
        d->tradeTable->setItem(i, 4, new QTableWidgetItem(QString::number(trade.quantity)));
        
        auto* profitItem = new QTableWidgetItem(QString::number(trade.profit, 'f', 2));
        profitItem->setForeground(trade.profit >= 0 ? QColor("#00c853") : QColor("#ff1744"));
        d->tradeTable->setItem(i, 5, profitItem);
    }
    
    d->tradeTable->resizeColumnsToContents();
}

void BacktestReportWidget::updateMonthlyReturns()
{
    // 按月份统计收益
    QMap<QString, double> monthlyReturns;
    QMap<QString, int> monthlyTrades;
    
    for (const auto& trade : d->trades) {
        QString month = trade.date.toString("yyyy-MM");
        monthlyReturns[month] += trade.profit;
        monthlyTrades[month]++;
    }
    
    d->monthlyTable->setRowCount(monthlyReturns.size());
    
    int row = 0;
    for (auto it = monthlyReturns.begin(); it != monthlyReturns.end(); ++it, ++row) {
        d->monthlyTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        
        auto* returnItem = new QTableWidgetItem(QString::number(it.value(), 'f', 2));
        returnItem->setForeground(it.value() >= 0 ? QColor("#00c853") : QColor("#ff1744"));
        d->monthlyTable->setItem(row, 1, returnItem);
        
        d->monthlyTable->setItem(row, 2, new QTableWidgetItem(QString::number(monthlyTrades[it.key()])));
    }
    
    d->monthlyTable->resizeColumnsToContents();
}

} // namespace WealthPilot