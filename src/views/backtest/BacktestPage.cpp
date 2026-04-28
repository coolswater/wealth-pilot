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
#include "ui/components/KLineChart.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QFileDialog>
#include <QProgressBar>
#include <QDateTime>
#include <QRandomGenerator>

// ========== PIMPL实现 ==========

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
    KLineChart* equityCurve = nullptr;      ///< 权益曲线
    
    // 交易记录
    QTableWidget* tradeTable = nullptr;     ///< 交易记录表格
    
    // 回测数据
    BacktestResult result;                  ///< 回测结果
    QVector<BacktestTradeRecord> trades;            ///< 交易记录
    bool isRunning = false;                 ///< 是否正在运行
};

// ========== 构造与析构 ==========

BacktestPage::BacktestPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

BacktestPage::~BacktestPage() = default;

// ========== 初始化 ==========

void BacktestPage::initializePage()
{
    // 初始化默认策略代码
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
    
    initToolBar();
    
    // 主内容区域
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet("QSplitter::handle { background: #2a2a2a; width: 1px; }");
    
    // 左侧：策略编辑器
    auto* leftWidget = new QWidget();
    leftWidget->setStyleSheet("QWidget { background: #0a0a0a; }");
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(8, 8, 8, 8);
    
    initStrategyEditor();
    leftLayout->addWidget(d->strategyEditor);
    splitter->addWidget(leftWidget);
    
    // 右侧：结果面板
    auto* rightWidget = new QWidget();
    rightWidget->setStyleSheet("QWidget { background: #0a0a0a; }");
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(8, 8, 8, 8);
    rightLayout->setSpacing(8);
    
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
    toolbar->setFixedHeight(50);
    toolbar->setStyleSheet("QWidget { background: #0a0a0a; }");
    
    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);
    
    // 策略选择
    layout->addWidget(new QLabel(QStringLiteral("策略:")));
    d->strategyCombo = new QComboBox();
    d->strategyCombo->addItems({
        QStringLiteral("双均线策略"),
        QStringLiteral("MACD策略"),
        QStringLiteral("RSI策略"),
        QStringLiteral("布林带策略"),
        QStringLiteral("自定义策略")
    });
    d->strategyCombo->setFixedWidth(120);
    d->strategyCombo->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
    layout->addWidget(d->strategyCombo);
    
    // 标的代码
    layout->addWidget(new QLabel(QStringLiteral("标的:")));
    d->symbolEdit = new QLineEdit(QStringLiteral("600519"));
    d->symbolEdit->setFixedWidth(80);
    d->symbolEdit->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
    layout->addWidget(d->symbolEdit);
    
    // 开始日期
    layout->addWidget(new QLabel(QStringLiteral("开始:")));
    d->startDateEdit = new QDateEdit(QDate::currentDate().addYears(-1));
    d->startDateEdit->setCalendarPopup(true);
    d->startDateEdit->setFixedWidth(100);
    d->startDateEdit->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
    layout->addWidget(d->startDateEdit);
    
    // 结束日期
    layout->addWidget(new QLabel(QStringLiteral("结束:")));
    d->endDateEdit = new QDateEdit(QDate::currentDate());
    d->endDateEdit->setCalendarPopup(true);
    d->endDateEdit->setFixedWidth(100);
    d->endDateEdit->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
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
    d->runBtn->setStyleSheet(R"(
        QPushButton {
            background: #00D4AA;
            color: #ffffff;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: #00B894; }
    )");
    layout->addWidget(d->runBtn);
    
    // 停止按钮
    d->stopBtn = new QPushButton(QStringLiteral("停止"));
    d->stopBtn->setFixedSize(60, 28);
    d->stopBtn->setEnabled(false);
    d->stopBtn->setStyleSheet(R"(
        QPushButton {
            background: #FF3366;
            color: #ffffff;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: #E91E63; }
    )");
    layout->addWidget(d->stopBtn);
    
    // 导出按钮
    d->exportBtn = new QPushButton(QStringLiteral("导出报告"));
    d->exportBtn->setFixedSize(80, 28);
    d->exportBtn->setStyleSheet(R"(
        QPushButton {
            background: #2a2a2a;
            color: #ffffff;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: #3a3a3a; }
    )");
    layout->addWidget(d->exportBtn);
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(toolbar);
}

void BacktestPage::initStrategyEditor()
{
    auto* group = new QGroupBox(QStringLiteral("策略代码"));
    group->setStyleSheet(R"(
        QGroupBox {
            color: #ffffff;
            font-size: 13px;
            font-weight: bold;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; }
    )");
    
    auto* layout = new QVBoxLayout(group);
    layout->setContentsMargins(8, 8, 8, 8);
    
    d->strategyEditor = new QTextEdit();
    d->strategyEditor->setStyleSheet(R"(
        QTextEdit {
            background: #0d0d0d;
            color: #00D4AA;
            border: 1px solid #2a2a2a;
            font-family: Consolas, Monaco, monospace;
            font-size: 12px;
        }
    )");
    d->strategyEditor->setFont(QFont("Consolas", 10));
    
    layout->addWidget(d->strategyEditor);
    
    auto* parentLayout = qobject_cast<QVBoxLayout*>(d->strategyEditor->parentWidget()->layout());
    if (parentLayout) {
        parentLayout->addWidget(group);
    }
}

void BacktestPage::initResultPanel()
{
    auto* rightLayout = qobject_cast<QVBoxLayout*>(d->tradeTable ? d->tradeTable->parentWidget()->layout() : nullptr);
    if (!rightLayout) {
        rightLayout = qobject_cast<QVBoxLayout*>(d->runBtn->parentWidget()->layout());
    }
    
    // 结果指标面板
    auto* resultGroup = new QGroupBox(QStringLiteral("回测结果"));
    resultGroup->setStyleSheet(R"(
        QGroupBox {
            color: #ffffff;
            font-size: 13px;
            font-weight: bold;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; }
    )");
    
    auto* resultLayout = new QGridLayout(resultGroup);
    resultLayout->setSpacing(8);
    
    int row = 0;
    auto createResultRow = [&](const QString& label, QLabel*& valueLabel) {
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet("color: #888888; font-size: 12px;");
        resultLayout->addWidget(lbl, row, 0);
        valueLabel = new QLabel(QStringLiteral("--"));
        valueLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold;");
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
    
    // 权益曲线
    d->equityCurve = new KLineChart();
    d->equityCurve->setMinimumHeight(200);
    resultLayout->addWidget(d->equityCurve, row, 0, 1, 2);
    
    // 添加到右侧布局
    auto* parent = d->runBtn->parentWidget()->parentWidget();
    if (parent) {
        auto* layout = qobject_cast<QVBoxLayout*>(parent->layout());
        if (layout) {
            layout->addWidget(resultGroup);
        }
    }
}

void BacktestPage::initTradeHistory()
{
    auto* tradeGroup = new QGroupBox(QStringLiteral("交易记录"));
    tradeGroup->setStyleSheet(R"(
        QGroupBox {
            color: #ffffff;
            font-size: 13px;
            font-weight: bold;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; }
    )");
    
    auto* layout = new QVBoxLayout(tradeGroup);
    
    d->tradeTable = new QTableWidget();
    d->tradeTable->setColumnCount(6);
    d->tradeTable->setHorizontalHeaderLabels({
        QStringLiteral("时间"),
        QStringLiteral("方向"),
        QStringLiteral("价格"),
        QStringLiteral("数量"),
        QStringLiteral("盈亏"),
        QStringLiteral("累计盈亏")
    });
    
    d->tradeTable->setStyleSheet(R"(
        QTableWidget {
            background: #0a0a0a;
            color: #ffffff;
            border: none;
            gridline-color: #1a1a1a;
            font-size: 11px;
        }
        QHeaderView::section {
            background: #0d0d0d;
            color: #888888;
            border: none;
            padding: 4px;
        }
    )");
    
    d->tradeTable->horizontalHeader()->setStretchLastSection(true);
    d->tradeTable->verticalHeader()->setVisible(false);
    d->tradeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    layout->addWidget(d->tradeTable);
    
    // 添加到右侧布局
    auto* parent = d->runBtn->parentWidget()->parentWidget();
    if (parent) {
        auto* layout = qobject_cast<QVBoxLayout*>(parent->layout());
        if (layout) {
            layout->addWidget(tradeGroup);
        }
    }
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
    QDateTime baseTime = QDateTime(startDate);
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
    
    // 颜色设置
    if (result.totalReturn > 0) {
        d->totalReturnLabel->setStyleSheet("color: #00D4AA; font-size: 14px; font-weight: bold;");
    } else {
        d->totalReturnLabel->setStyleSheet("color: #FF3366; font-size: 14px; font-weight: bold;");
    }
    
    // 更新交易记录表格
    d->tradeTable->setRowCount(d->trades.size());
    for (int i = 0; i < d->trades.size(); ++i) {
        const auto& trade = d->trades[i];
        
        d->tradeTable->setItem(i, 0, new QTableWidgetItem(trade.time.toString("yyyy-MM-dd")));
        
        auto* actionItem = new QTableWidgetItem(trade.action);
        actionItem->setForeground(trade.action == QStringLiteral("买入") ? QColor("#FF3366") : QColor("#00D4AA"));
        d->tradeTable->setItem(i, 1, actionItem);
        
        d->tradeTable->setItem(i, 2, new QTableWidgetItem(QString::number(trade.price, 'f', 2)));
        d->tradeTable->setItem(i, 3, new QTableWidgetItem(QString::number(trade.volume)));
        
        auto* profitItem = new QTableWidgetItem(QString::number(trade.profit, 'f', 2));
        profitItem->setForeground(trade.profit >= 0 ? QColor("#00D4AA") : QColor("#FF3366"));
        d->tradeTable->setItem(i, 4, profitItem);
        
        auto* cumItem = new QTableWidgetItem(QString::number(trade.cumProfit, 'f', 2));
        cumItem->setForeground(trade.cumProfit >= 0 ? QColor("#00D4AA") : QColor("#FF3366"));
        d->tradeTable->setItem(i, 5, cumItem);
    }
    
    emit backtestCompleted(result);
}

void BacktestPage::exportReport(const QString& filePath)
{
    Q_UNUSED(filePath)
    // TODO: 导出PDF报告
    LOG_INFO(QStringLiteral("Export backtest report"));
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
