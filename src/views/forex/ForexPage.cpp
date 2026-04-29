/**
 * @file ForexPage.cpp
 * @brief 外汇页面实现 - 外汇行情展示与分析
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ForexPage.h"
#include "ui/components/KLineChart.h"
#include "core/config/Tokens.h"
#include "market/ForexDataSource.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QDoubleSpinBox>

// ========== PIMPL实现 ==========

struct ForexPage::Impl {
    // 外汇列表
    QTableWidget* forexListTable = nullptr;
    
    // 详情面板
    QLabel* pairLabel = nullptr;
    QLabel* rateLabel = nullptr;
    QLabel* bidLabel = nullptr;
    QLabel* askLabel = nullptr;
    QLabel* changeLabel = nullptr;
    QLabel* highLabel = nullptr;
    QLabel* lowLabel = nullptr;
    KLineChart* rateChart = nullptr;
    
    // 汇率换算
    QComboBox* currencyFromCombo = nullptr;
    QComboBox* currencyToCombo = nullptr;
    QDoubleSpinBox* amountSpinBox = nullptr;
    QLabel* resultLabel = nullptr;
    
    // 当前选中
    QString currentPair;
    ForexQuote currentQuote;
    
    // 数据缓存
    QVector<ForexQuote> forexCache;
};

// ========== 构造与析构 ==========

ForexPage::ForexPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

ForexPage::~ForexPage() = default;

// ========== 初始化 ==========

void ForexPage::initializePage()
{
    loadForexList();
}

void ForexPage::refresh()
{
    loadForexList();
    if (!d->currentPair.isEmpty()) {
        loadRateHistory(d->currentPair);
    }
}

// ========== UI初始化 ==========

void ForexPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    initToolBar();
    
    // 主内容区域
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet("QSplitter::handle { background: #2a2a2a; width: 1px; }");
    
    // 左侧：外汇列表
    auto* listWidget = new QWidget();
    listWidget->setStyleSheet("QWidget { background: #0a0a0a; }");
    auto* listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(0, 0, 0, 0);
    
    initForexList();
    listLayout->addWidget(d->forexListTable);
    splitter->addWidget(listWidget);
    
    // 右侧：详情和换算
    auto* rightWidget = new QWidget();
    rightWidget->setStyleSheet("QWidget { background: #0a0a0a; }");
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(12, 12, 12, 12);
    rightLayout->setSpacing(12);
    
    initRateChart();
    rightLayout->addWidget(d->rateChart, 1);
    
    initConverter();
    rightLayout->addStretch();
    
    splitter->addWidget(rightWidget);
    splitter->setSizes({300, 500});
    
    mainLayout->addWidget(splitter, 1);
    
    initConnections();
}

void ForexPage::initToolBar()
{
    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(40);
    toolbar->setStyleSheet("QWidget { background: #0a0a0a; }");
    
    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(12, 6, 12, 6);
    
    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    refreshBtn->setFixedSize(60, 26);
    refreshBtn->setStyleSheet(R"(
        QPushButton {
            background: #2a2a2a;
            color: #ffffff;
            border: none;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover { background: #3a3a3a; }
    )");
    connect(refreshBtn, &QPushButton::clicked, this, &ForexPage::onRefreshData);
    layout->addWidget(refreshBtn);
    
    layout->addStretch();
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(toolbar);
}

void ForexPage::initForexList()
{
    d->forexListTable = new QTableWidget();
    d->forexListTable->setColumnCount(6);
    d->forexListTable->setHorizontalHeaderLabels({
        QStringLiteral("货币对"),
        QStringLiteral("汇率"),
        QStringLiteral("买入价"),
        QStringLiteral("卖出价"),
        QStringLiteral("涨跌幅"),
        QStringLiteral("更新时间")
    });
    
    d->forexListTable->setStyleSheet(R"(
        QTableWidget {
            background: #0a0a0a;
            color: #ffffff;
            border: none;
            gridline-color: #1a1a1a;
            font-size: 12px;
        }
        QTableWidget::item:selected { background: #2a2a2a; }
        QHeaderView::section {
            background: #0d0d0d;
            color: #888888;
            border: none;
            padding: 6px;
            font-size: 11px;
        }
    )");
    
    d->forexListTable->horizontalHeader()->setStretchLastSection(true);
    d->forexListTable->verticalHeader()->setVisible(false);
    d->forexListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->forexListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void ForexPage::initRateChart()
{
    d->rateChart = new KLineChart();
    d->rateChart->setMinimumHeight(300);
}

void ForexPage::initConverter()
{
    auto* converterGroup = new QGroupBox(QStringLiteral("汇率换算"));
    converterGroup->setStyleSheet(R"(
        QGroupBox {
            color: #ffffff;
            font-size: 13px;
            font-weight: bold;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            margin-top: 8px;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
        }
    )");
    
    auto* layout = new QGridLayout(converterGroup);
    
    // 源货币
    layout->addWidget(new QLabel(QStringLiteral("从:")), 0, 0);
    d->currencyFromCombo = new QComboBox();
    d->currencyFromCombo->addItems({"CNY", "USD", "EUR", "GBP", "JPY", "HKD"});
    d->currencyFromCombo->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
    layout->addWidget(d->currencyFromCombo, 0, 1);
    
    // 目标货币
    layout->addWidget(new QLabel(QStringLiteral("到:")), 1, 0);
    d->currencyToCombo = new QComboBox();
    d->currencyToCombo->addItems({"USD", "CNY", "EUR", "GBP", "JPY", "HKD"});
    d->currencyToCombo->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
    layout->addWidget(d->currencyToCombo, 1, 1);
    
    // 金额
    layout->addWidget(new QLabel(QStringLiteral("金额:")), 2, 0);
    d->amountSpinBox = new QDoubleSpinBox();
    d->amountSpinBox->setRange(0, 100000000);
    d->amountSpinBox->setValue(100);
    d->amountSpinBox->setStyleSheet("background: #1a1a1a; color: #ffffff; padding: 4px;");
    layout->addWidget(d->amountSpinBox, 2, 1);
    
    // 结果
    layout->addWidget(new QLabel(QStringLiteral("结果:")), 3, 0);
    d->resultLabel = new QLabel(QStringLiteral("--"));
    d->resultLabel->setStyleSheet("color: #00D4AA; font-size: 16px; font-weight: bold;");
    layout->addWidget(d->resultLabel, 3, 1);
    
    auto* rightLayout = qobject_cast<QVBoxLayout*>(d->rateChart->parentWidget()->layout());
    if (rightLayout) {
        rightLayout->addWidget(converterGroup);
    }
}

void ForexPage::initConnections()
{
    connect(d->forexListTable, &QTableWidget::cellClicked,
            this, &ForexPage::onForexListClicked);
    connect(d->currencyFromCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ForexPage::onCurrencyFromChanged);
    connect(d->currencyToCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ForexPage::onCurrencyToChanged);
    connect(d->amountSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ForexPage::onAmountChanged);
}

// ========== 数据加载 ==========

void ForexPage::loadForexList()
{
    d->forexCache.clear();
    
    // 定义要请求的货币对
    QStringList pairs = {
        "USD/CNY", "EUR/USD", "GBP/USD", "USD/JPY", "EUR/CNY",
        "AUD/USD", "USD/CAD", "USD/CHF", "NZD/USD", "HKD/CNY"
    };
    
    // 使用真实数据源
    ForexDataSource::instance()->requestQuotes(pairs, [this](const QVector<ForexQuote>& quotes) {
        d->forexCache = quotes;
        
        d->forexListTable->setRowCount(d->forexCache.size());
        
        for (int i = 0; i < d->forexCache.size(); ++i) {
            const auto& forex = d->forexCache[i];
            
            d->forexListTable->setItem(i, 0, new QTableWidgetItem(forex.pair));
            d->forexListTable->setItem(i, 1, new QTableWidgetItem(QString::number(forex.rate, 'f', 4)));
            d->forexListTable->setItem(i, 2, new QTableWidgetItem(QString::number(forex.bid, 'f', 4)));
            d->forexListTable->setItem(i, 3, new QTableWidgetItem(QString::number(forex.ask, 'f', 4)));
            
            auto* changeItem = new QTableWidgetItem(QString::number(forex.changePercent, 'f', 2) + "%");
            changeItem->setForeground(forex.changePercent >= 0 ? QColor(Tokens::Colors::Success) : QColor(Tokens::Colors::Danger));
            d->forexListTable->setItem(i, 4, changeItem);
            
            d->forexListTable->setItem(i, 5, new QTableWidgetItem(forex.updateTime.toString("hh:mm:ss")));
        }
        
        // 默认选中第一个
        if (!d->forexCache.isEmpty()) {
            d->currentPair = d->forexCache[0].pair;
            d->currentQuote = d->forexCache[0];
            updateForexDetail(d->forexCache[0]);
        }
    });
    
    // 启动自动刷新
    ForexDataSource::instance()->startAutoRefresh(60000);
}

void ForexPage::updateForexDetail(const ForexQuote& quote)
{
    d->currentQuote = quote;
    // 更新详情面板...
}

void ForexPage::loadRateHistory(const QString& pair)
{
    Q_UNUSED(pair)
    // 加载汇率历史数据...
}

void ForexPage::calculateConversion()
{
    QString from = d->currencyFromCombo->currentText();
    QString to = d->currencyToCombo->currentText();
    double amount = d->amountSpinBox->value();
    
    // 查找汇率
    double rate = 1.0;
    if (from == "CNY" && to == "USD") rate = 1.0 / 7.2456;
    else if (from == "USD" && to == "CNY") rate = 7.2456;
    else if (from == "EUR" && to == "USD") rate = 1.0892;
    else if (from == "USD" && to == "EUR") rate = 1.0 / 1.0892;
    // ... 其他货币对
    
    double result = amount * rate;
    d->resultLabel->setText(QString::number(result, 'f', 2) + " " + to);
}

// ========== 槽函数 ==========

void ForexPage::onForexListClicked(int row, int column)
{
    Q_UNUSED(column)
    if (row >= 0 && row < d->forexCache.size()) {
        const auto& forex = d->forexCache[row];
        d->currentPair = forex.pair;
        updateForexDetail(forex);
        loadRateHistory(forex.pair);
        emit forexPairSelected(forex.pair, forex.rate);
    }
}

void ForexPage::onCurrencyFromChanged(int index)
{
    Q_UNUSED(index)
    calculateConversion();
}

void ForexPage::onCurrencyToChanged(int index)
{
    Q_UNUSED(index)
    calculateConversion();
}

void ForexPage::onAmountChanged(double amount)
{
    Q_UNUSED(amount)
    calculateConversion();
}

void ForexPage::onRefreshData()
{
    refresh();
}
