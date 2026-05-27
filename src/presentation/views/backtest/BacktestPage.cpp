/**
 * @file BacktestPage.cpp
 * @brief 策略回测页面实现 - 量化策略回测与分析
 *
 * @details 实现功能：
 * - 策略编写与回测执行
 * - 回测结果可视化展示
 * - 交易记录查看
 * - 报告导出
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "BacktestPage.h"
#include "presentation/components/KLineChart.h"
#include "presentation/components/BacktestChartWidget.h"
#include "presentation/components/BacktestReportWidget.h"
#include "infrastructure/config/Tokens.h"
#include "presentation/components/StyleHelper.h"
#include "presentation/styles/ButtonStyles.h"
#include "shared/utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QProgressBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QSplitter>
#include <QGroupBox>
#include <QRandomGenerator>
#include <QThread>
#include <QCoreApplication>

using namespace Tokens;

struct BacktestPage::Impl {
    // 工具栏组件
    QComboBox* strategyCombo = nullptr;     ///< 策略选择
    QLineEdit* symbolEdit = nullptr;        ///< 标的代码
    QDateEdit* startDateEdit = nullptr;     ///< 开始日期
    QDateEdit* endDateEdit = nullptr;       ///< 结束日期
    QPushButton* runBtn = nullptr;          ///< 运行按钮
    QPushButton* stopBtn = nullptr;         ///< 停止按钮
    QPushButton* exportBtn = nullptr;       ///< 导出按钮
    QProgressBar* progressBar = nullptr;    ///< 进度条
    
    // 策略编辑器
    QTextEdit* strategyEditor = nullptr;    ///< 策略代码编辑器
    
    // 结果面板
        QLabel* totalReturnLabel = nullptr;     ///< 总收益率
        QLabel* annualReturnLabel = nullptr;    ///< 年化收益率
        QLabel* maxDrawdownLabel = nullptr;     ///< 最大回撤
        QLabel* sharpeRatioLabel = nullptr;     ///< 夏普比率
        QLabel* winRateLabel = nullptr;         ///< 胜率
        QLabel* profitFactorLabel = nullptr;    ///< 盈亏比
        QLabel* totalTradesLabel = nullptr;     ///< 总交易次数
        KLineChart* equityCurve = nullptr;      ///< 权益曲线（旧）
    
        // 新增：回测可视化组件
        WealthPilot::BacktestChartWidget* chartWidget = nullptr;    ///< 回测图表（资金曲线+回撤）
        WealthPilot::BacktestReportWidget* reportWidget = nullptr;  ///< 回测报告面板
    
    // 交易记录
    QTableWidget* tradeTable = nullptr;     ///< 交易记录表格
    
    // 回测数据
    BacktestResult result;                  ///< 回测结果
    QVector<BacktestTradeRecord> trades;            ///< 交易记录
    bool isRunning = false;                 ///< 是否正在运行
};

// ========== 构造与析构 ==========

BacktestPage::BacktestPage(QWidget *parent)
    : DataHubPageBase(parent)
    , d(std::make_unique<Impl>())
{
    setObjectName("BacktestPage");
    setupUI();
}

BacktestPage::~BacktestPage() = default;

// ========== 初始化 ==========

void BacktestPage::initializePage()
{
    // ============================================================
    // 1. 设置 DataHub 订阅
    // ============================================================
    setupDataHubSubscriptions();
    
    // ============================================================
    // 2. 初始化默认策略代码
    // ============================================================
    d->strategyEditor->setPlainText(R"(
// 简单双均线策略
// 参数: shortPeriod=5, longPeriod=20

function onBar(bar) {
    var shortMA = SMA(close, shortPeriod);
    var longMA = SMA(close, longPeriod);
    
    if (shortMA > longMA && position <= 0) {
        buy(1, bar.close);
    } else if (shortMA < longMA && position >= 0) {
        sell(1, bar.close);
    }
}
)");
}

void BacktestPage::setupDataHubSubscriptions()
{
    // 订阅回测进度
    dataHub().subscribe(this, "backtest:progress",
        [this](const QVariant& value) {
            Q_UNUSED(value)
            // 更新回测进度
        });
    
    // 订阅回测结果
    dataHub().subscribe(this, "backtest:result",
        [this](const QVariant& value) {
            Q_UNUSED(value)
            // 更新回测结果
        });
    
    // 订阅历史K线数据
    dataHub().subscribePattern(this, "market:kline:*",
        [this](const QString& topic, const QVariant& value) {
            Q_UNUSED(topic)
            Q_UNUSED(value)
            // K线数据更新
        });
    
    LOG_INFO("[BacktestPage] DataHub subscriptions setup complete");
}

void BacktestPage::refresh()
{
    // 刷新回测结果
}

// ========== UI初始化 ==========

void BacktestPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 页面头部
    auto* header = StyleHelper::createPageHeader(this, QStringLiteral("策略回测"));
    mainLayout->addWidget(header);

    initToolBar();
    
    // 主内容区域
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName("backtestSplitter");

    // 左侧：策略编辑器
    auto* leftWidget = new QWidget();
    leftWidget->setObjectName("strategyPanel");
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(Spacing::SM, Spacing::SM, Spacing::SM, Spacing::SM);

    initStrategyEditor();
    leftLayout->addWidget(d->strategyEditor);
    splitter->addWidget(leftWidget);
    
    // 右侧：结果面板
    auto* rightWidget = new QWidget();
    rightWidget->setObjectName("resultPanel");
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(Spacing::SM, Spacing::SM, Spacing::SM, Spacing::SM);
    rightLayout->setSpacing(Spacing::SM);

    initResultPanel();
    initTradeHistory();
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({400, 600});
    
    mainLayout->addWidget(splitter, 1);
    
    initConnections();
}

void BacktestPage::initToolBar()
{
    auto* toolbar = new QWidget(this);
    toolbar->setObjectName("backtestToolbar");
    toolbar->setFixedHeight(50);
    
    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(Spacing::SM, Spacing::XS, Spacing::SM, Spacing::XS);
    layout->setSpacing(Spacing::XS);

    // 策略选择
    QLabel* strategyLabel = new QLabel(QStringLiteral("策略:"));
    strategyLabel->setProperty("dataType", "label");
    layout->addWidget(strategyLabel);

    d->strategyCombo = new QComboBox();
    d->strategyCombo->addItems({
        QStringLiteral("双均线策略"),
        QStringLiteral("MACD策略"),
        QStringLiteral("RSI策略"),
        QStringLiteral("布林带策略"),
        QStringLiteral("自定义策略")
    });
    d->strategyCombo->setFixedWidth(120);
    d->strategyCombo->setObjectName("strategyCombo");
    layout->addWidget(d->strategyCombo);
    
    // 标的代码
    QLabel* symbolLabel = new QLabel(QStringLiteral("标的:"));
    symbolLabel->setProperty("dataType", "label");
    layout->addWidget(symbolLabel);

    d->symbolEdit = new QLineEdit(QStringLiteral("600519"));
    d->symbolEdit->setFixedWidth(80);
    d->symbolEdit->setObjectName("symbolInput");
    layout->addWidget(d->symbolEdit);
    
    // 开始日期
    QLabel* startLabel = new QLabel(QStringLiteral("开始:"));
    startLabel->setProperty("dataType", "label");
    layout->addWidget(startLabel);

    d->startDateEdit = new QDateEdit(QDate::currentDate().addYears(-1));
    d->startDateEdit->setCalendarPopup(true);
    d->startDateEdit->setFixedWidth(100);
    layout->addWidget(d->startDateEdit);
    
    // 结束日期
    QLabel* endLabel = new QLabel(QStringLiteral("结束:"));
    endLabel->setProperty("dataType", "label");
    layout->addWidget(endLabel);

    d->endDateEdit = new QDateEdit(QDate::currentDate());
    d->endDateEdit->setCalendarPopup(true);
    d->endDateEdit->setFixedWidth(100);
    layout->addWidget(d->endDateEdit);
    
    // 进度条
    d->progressBar = new QProgressBar();
    d->progressBar->setFixedWidth(100);
    d->progressBar->setVisible(false);
    layout->addWidget(d->progressBar);
    
    layout->addStretch();
    
    // 运行按钮
    d->runBtn = new QPushButton(QStringLiteral("运行回测"));
    d->runBtn->setFixedSize(80, 28);
    ButtonStyles::setSuccess(d->runBtn);
    layout->addWidget(d->runBtn);
    
    // 停止按钮
    d->stopBtn = new QPushButton(QStringLiteral("停止"));
    d->stopBtn->setFixedSize(60, 28);
    d->stopBtn->setEnabled(false);
    ButtonStyles::setDanger(d->stopBtn);
    layout->addWidget(d->stopBtn);
    
    // 导出按钮
    d->exportBtn = new QPushButton(QStringLiteral("导出报告"));
    d->exportBtn->setFixedSize(80, 28);
    ButtonStyles::setExport(d->exportBtn);
    layout->addWidget(d->exportBtn);
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(toolbar);
}

void BacktestPage::initStrategyEditor()
{
    auto* group = new QGroupBox(QStringLiteral("策略代码"));
    
    auto* layout = new QVBoxLayout(group);
    layout->setContentsMargins(Spacing::SM, Spacing::SM, Spacing::SM, Spacing::SM);

    d->strategyEditor = new QTextEdit();
    d->strategyEditor->setObjectName("strategyEditor");
    d->strategyEditor->setFont(QFont("Consolas", 10));
    
    layout->addWidget(d->strategyEditor);
}

void BacktestPage::initResultPanel()
{
    // 结果指标面板
    auto* resultGroup = new QGroupBox(QStringLiteral("回测结果"));
    
    auto* resultLayout = new QGridLayout(resultGroup);
    resultLayout->setSpacing(Spacing::XS);

    int row = 0;
    auto createResultRow = [&](const QString& label, QLabel*& valueLabel) {
        auto* lbl = new QLabel(label);
        lbl->setProperty("dataType", "label");
        resultLayout->addWidget(lbl, row, 0);
        valueLabel = new QLabel(QStringLiteral("--"));
        valueLabel->setProperty("dataType", "value");
        resultLayout->addWidget(valueLabel, row, 1);
        row++;
    };
    
    createResultRow(QStringLiteral("总收益率:"), d->totalReturnLabel);
    createResultRow(QStringLiteral("年化收益率:"), d->annualReturnLabel);
    createResultRow(QStringLiteral("最大回撤:"), d->maxDrawdownLabel);
    createResultRow(QStringLiteral("夏普比率:"), d->sharpeRatioLabel);
    createResultRow(QStringLiteral("胜率:"), d->winRateLabel);
    createResultRow(QStringLiteral("盈亏比:"), d->profitFactorLabel);
    createResultRow(QStringLiteral("总交易次数:"), d->totalTradesLabel);
    
    // 权益曲线 - 使用新的可视化组件
    d->chartWidget = new WealthPilot::BacktestChartWidget();
    d->chartWidget->setMinimumHeight(250);
    resultLayout->addWidget(d->chartWidget, row, 0, 1, 2);

    // 回测报告面板 - 添加到结果布局
    d->reportWidget = new WealthPilot::BacktestReportWidget();
    d->reportWidget->setMinimumHeight(150);
    row++;
    resultLayout->addWidget(d->reportWidget, row, 0, 1, 2);
}

void BacktestPage::initTradeHistory()
{
    auto* tradeGroup = new QGroupBox(QStringLiteral("交易记录"));
    
    auto* layout = new QVBoxLayout(tradeGroup);
    
    d->tradeTable = new QTableWidget();
    d->tradeTable->setObjectName("tradeTable");
    d->tradeTable->setColumnCount(6);
    d->tradeTable->setHorizontalHeaderLabels({
        QStringLiteral("时间"),
        QStringLiteral("方向"),
        QStringLiteral("价格"),
        QStringLiteral("数量"),
        QStringLiteral("盈亏"),
        QStringLiteral("累计盈亏")
    });
    
    d->tradeTable->horizontalHeader()->setStretchLastSection(true);
    d->tradeTable->verticalHeader()->setVisible(false);
    d->tradeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    layout->addWidget(d->tradeTable);
}

void BacktestPage::initConnections()
{
    connect(d->runBtn, &QPushButton::clicked, this, &BacktestPage::onRunBacktest);
    connect(d->stopBtn, &QPushButton::clicked, this, &BacktestPage::onStopBacktest);
    connect(d->exportBtn, &QPushButton::clicked, this, &BacktestPage::onExportReport);
    connect(d->strategyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BacktestPage::onStrategyChanged);
    connect(d->symbolEdit, &QLineEdit::textChanged, this, &BacktestPage::onSymbolChanged);
}

// ========== 回测执行 ==========

void BacktestPage::runBacktest(const QString& symbol, const QDate& startDate, const QDate& endDate)
{
    Q_UNUSED(symbol)
    Q_UNUSED(startDate)
    Q_UNUSED(endDate)
    
    // 模拟回测结果
    d->result.totalReturn = 45.67;
    d->result.annualReturn = 38.92;
    d->result.maxDrawdown = -15.23;
    d->result.sharpeRatio = 1.85;
    d->result.winRate = 62.5;
    d->result.profitFactor = 1.78;
    d->result.totalTrades = 48;
    d->result.winTrades = 30;
    d->result.lossTrades = 18;
    
    // 生成模拟交易记录
    d->trades.clear();
    QDateTime baseTime(startDate.startOfDay());
    double cumProfit = 0;
    
    for (int i = 0; i < 20; ++i) {
        BacktestTradeRecord trade;
        trade.time = baseTime.addDays(i * 10);
        trade.action = (i % 2 == 0) ? QStringLiteral("买入") : QStringLiteral("卖出");
        trade.price = 1800.0 + QRandomGenerator::global()->bounded(-100, 100);
        trade.volume = 100;
        trade.profit = QRandomGenerator::global()->bounded(-500, 800);
        cumProfit += trade.profit;
        trade.cumProfit = cumProfit;
        d->trades.append(trade);
    }
    
    updateResult(d->result);
}

void BacktestPage::updateResult(const BacktestResult& result)
{
    // 更新结果标签
    d->totalReturnLabel->setText(QString::number(result.totalReturn, 'f', 2) + "%");
    d->annualReturnLabel->setText(QString::number(result.annualReturn, 'f', 2) + "%");
    d->maxDrawdownLabel->setText(QString::number(result.maxDrawdown, 'f', 2) + "%");
    d->sharpeRatioLabel->setText(QString::number(result.sharpeRatio, 'f', 2));
    d->winRateLabel->setText(QString::number(result.winRate, 'f', 1) + "%");
    d->profitFactorLabel->setText(QString::number(result.profitFactor, 'f', 2));
    d->totalTradesLabel->setText(QString::number(result.totalTrades));

    // 使用属性选择器设置颜色
    d->totalReturnLabel->setProperty("trend", result.totalReturn > 0 ? "up" : "down");
    StyleHelper::refreshStyle(d->totalReturnLabel);

    // 更新回测可视化组件
    if (d->chartWidget) {
        // 生成模拟资金曲线数据
        QVector<WealthPilot::BacktestDataPoint> equityCurve;
        QDateTime baseDate = d->startDateEdit->date().startOfDay();
        double baseEquity = 100000.0;  // 初始资金 10万
        
        for (int i = 0; i < 250; ++i) {  // 一年约250个交易日
            WealthPilot::BacktestDataPoint point;
            point.date = baseDate.addDays(i);
            
            // 模拟资金增长曲线
            double dailyReturn = 0.001 + (result.totalReturn / 100.0 / 250.0);  // 日收益率
            double randomFactor = (QRandomGenerator::global()->bounded(100) - 50) / 1000.0;
            point.equity = baseEquity * (1.0 + dailyReturn * i + randomFactor);
            point.returnRate = (point.equity - baseEquity) / baseEquity * 100.0;
            
            // 模拟回撤
            if (i > 50) {
                point.drawdown = std::max(0.0, -point.returnRate * 0.3 + randomFactor * 5);
            }
            
            equityCurve.append(point);
        }
        
        // 生成交易标记数据
        QVector<WealthPilot::TradeMarker> trades;
        for (const auto& trade : d->trades) {
            WealthPilot::TradeMarker marker;
            marker.date = trade.time;
            marker.action = trade.action.contains(QStringLiteral("买")) ? "buy" : "sell";
            marker.price = trade.price;
            marker.quantity = trade.volume;
            marker.profit = trade.profit;
            marker.isWin = trade.profit > 0;
            trades.append(marker);
        }
        
        d->chartWidget->setData(equityCurve, trades);
    }
    
    // 更新回测报告组件
    if (d->reportWidget) {
        // 使用 BacktestStats 直接更新
        BacktestStats stats;
        stats.totalReturn = result.totalReturn;
        stats.annualizedReturn = result.annualizedReturn;
        stats.maxDrawdown = result.maxDrawdown;
        stats.sharpeRatio = result.sharpeRatio;
        stats.winRate = result.winRate;
        stats.profitFactor = result.profitFactor;
        stats.totalTrades = result.totalTrades;
        
        // 转换 equityCurve: QVector<QPointF> -> QVector<BacktestDataPoint>
        QVector<WealthPilot::BacktestDataPoint> equityData;
        equityData.reserve(result.equityCurve.size());
        for (const auto& point : result.equityCurve) {
            WealthPilot::BacktestDataPoint dp;
            dp.date = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x()));
            dp.equity = point.y();
            dp.returnRate = (point.y() > 0 && !result.equityCurve.isEmpty()) 
                ? (point.y() / result.equityCurve.first().y() - 1.0) * 100.0 : 0.0;
            equityData.append(dp);
        }
        
        // 转换 trades: QVector<BacktestTradeRecord> -> QVector<TradeMarker>
        QVector<WealthPilot::TradeMarker> tradeMarkers;
        tradeMarkers.reserve(d->trades.size());
        for (const auto& trade : d->trades) {
            WealthPilot::TradeMarker marker;
            marker.date = trade.time;
            marker.action = trade.action;
            marker.price = trade.price;
            marker.quantity = trade.volume;
            marker.profit = trade.profit;
            marker.isWin = trade.profit > 0;
            tradeMarkers.append(marker);
        }
        
        d->reportWidget->setBacktestResult(stats, equityData, tradeMarkers);
    }

    // 更新交易记录表格
    d->tradeTable->setRowCount(d->trades.size());
    for (int i = 0; i < d->trades.size(); ++i) {
        const auto& trade = d->trades[i];
        
        d->tradeTable->setItem(i, 0, new QTableWidgetItem(trade.time.toString("yyyy-MM-dd")));
        
        auto* actionItem = new QTableWidgetItem(trade.action);
        actionItem->setForeground(QColor(trade.action == QStringLiteral("买入")
                                             ? Tokens::Colors::Success
                                             : Tokens::Colors::Danger));
        d->tradeTable->setItem(i, 1, actionItem);
        
        d->tradeTable->setItem(i, 2, new QTableWidgetItem(QString::number(trade.price, 'f', 2)));
        d->tradeTable->setItem(i, 3, new QTableWidgetItem(QString::number(trade.volume)));
        
        auto* profitItem = new QTableWidgetItem(QString::number(trade.profit, 'f', 2));
        profitItem->setForeground(QColor(trade.profit >= 0 ? Tokens::Colors::Danger : Tokens::Colors::Success));
        d->tradeTable->setItem(i, 4, profitItem);
        
        auto* cumItem = new QTableWidgetItem(QString::number(trade.cumProfit, 'f', 2));
        cumItem->setForeground(QColor(trade.cumProfit >= 0 ? Tokens::Colors::Danger : Tokens::Colors::Success));
        d->tradeTable->setItem(i, 5, cumItem);
    }
    
    emit backtestCompleted(result);
}

void BacktestPage::exportReport(const QString& filePath)
{
    // 导出报告（简化版：导出为文本格式）
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), 
            QStringLiteral("无法打开文件：") + filePath);
        return;
    }
    
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    
    // 写入报告内容
    out << QStringLiteral("========== 回测报告 ==========\n\n");
    out << QStringLiteral("策略名称: ") << d->strategyCombo->currentText() << "\n";
    out << QStringLiteral("回测时间: ") << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n\n";
    
    // 统计指标
    out << QStringLiteral("---------- 统计指标 ----------\n");
    out << QStringLiteral("总收益率: ") << d->totalReturnLabel->text() << "\n";
    out << QStringLiteral("年化收益率: ") << d->annualReturnLabel->text() << "\n";
    out << QStringLiteral("最大回撤: ") << d->maxDrawdownLabel->text() << "\n";
    out << QStringLiteral("夏普比率: ") << d->sharpeRatioLabel->text() << "\n";
    out << QStringLiteral("胜率: ") << d->winRateLabel->text() << "\n";
    out << QStringLiteral("盈亏比: ") << d->profitFactorLabel->text() << "\n\n";
    
    // 交易记录
    out << QStringLiteral("---------- 交易记录 ----------\n");
    out << QStringLiteral("总交易次数: ") << d->tradeTable->rowCount() << "\n\n";;
    
    file.close();
    
    QMessageBox::information(this, QStringLiteral("导出成功"), 
        QStringLiteral("回测报告已导出到：") + filePath);
    LOG_INFO(QStringLiteral("Export backtest report to: ") + filePath);
}

// ========== 槽函数 ==========

void BacktestPage::onRunBacktest()
{
    d->isRunning = true;
    d->runBtn->setEnabled(false);
    d->stopBtn->setEnabled(true);
    d->progressBar->setVisible(true);
    d->progressBar->setRange(0, 100);
    
    // 模拟进度
    for (int i = 0; i <= 100; i += 10) {
        d->progressBar->setValue(i);
        QCoreApplication::processEvents();
        QThread::msleep(100);
    }
    
    // 执行回测
    runBacktest(d->symbolEdit->text(), d->startDateEdit->date(), d->endDateEdit->date());
    
    d->isRunning = false;
    d->runBtn->setEnabled(true);
    d->stopBtn->setEnabled(false);
    d->progressBar->setVisible(false);
}

void BacktestPage::onStopBacktest()
{
    d->isRunning = false;
    d->runBtn->setEnabled(true);
    d->stopBtn->setEnabled(false);
    d->progressBar->setVisible(false);
}

void BacktestPage::onExportReport()
{
    QString filePath = QFileDialog::getSaveFileName(this,
        QStringLiteral("导出报告"),
        QStringLiteral("backtest_report.pdf"),
        QStringLiteral("PDF Files (*.pdf)"));
    
    if (!filePath.isEmpty()) {
        exportReport(filePath);
    }
}

void BacktestPage::onStrategyChanged(int index)
{
    // 根据策略类型更新代码模板
    Q_UNUSED(index)
}

void BacktestPage::onSymbolChanged(const QString& symbol)
{
    Q_UNUSED(symbol)
    // 更新标的信息
}

void BacktestPage::onTradeClicked(int row, int column)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
    // 显示交易详情
}
