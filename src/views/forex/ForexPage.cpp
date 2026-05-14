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
#include "ui/components/StyleHelper.h"
#include "ui/delegates/ColorDelegates.h"
#include "market/ForexDataSource.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QDoubleSpinBox>

using namespace Tokens;

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
    : DataHubPageBase(parent)
    , d(std::make_unique<Impl>())
{
    setObjectName("ForexPage");
    setupUI();
}

ForexPage::~ForexPage() = default;

// ========== 初始化 ==========

void ForexPage::initializePage()
{
    // ============================================================
    // 1. 设置 DataHub 订阅
    // ============================================================
    setupDataHubSubscriptions();
    
    // ============================================================
    // 2. 加载外汇列表
    // ============================================================
    loadForexList();
}

void ForexPage::setupDataHubSubscriptions()
{
    // 订阅外汇行情
    dataHub().subscribePattern(this, "market:forex:*",
        [this](const QString& topic, const QVariant& value) {
            Q_UNUSED(topic)
            Q_UNUSED(value)
            // 更新外汇行情
        });
    
    // 订阅主要货币对
    QStringList pairs = {"USD/CNY", "EUR/USD", "GBP/USD", "USD/JPY"};
    for (const QString& pair : pairs) {
        dataHub().subscribe(this, QString("market:forex:%1").arg(pair),
            [this, pair](const QString&, const QVariant& value) {
                Q_UNUSED(pair)
                Q_UNUSED(value)
                // 更新汇率
            });
        m_subscribedPairs.append(pair);
    }
    
    LOG_INFO("[ForexPage] DataHub subscriptions setup complete");
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

    // 页面头部
    auto* header = StyleHelper::createPageHeader(this, QStringLiteral("外汇行情"));
    mainLayout->addWidget(header);

    initToolBar();
    
    // 主内容区域
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName("forexSplitter");

    // 左侧：外汇列表
    auto* listWidget = new QWidget();
    listWidget->setObjectName("forexListPanel");
    auto* listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(0, 0, 0, 0);
    
    initForexList();
    listLayout->addWidget(d->forexListTable);
    splitter->addWidget(listWidget);
    
    // 右侧：详情和换算
    auto* rightWidget = new QWidget();
    rightWidget->setObjectName("forexDetailPanel");
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(Spacing::MD, Spacing::MD, Spacing::MD, Spacing::MD);
    rightLayout->setSpacing(Spacing::MD);

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
    toolbar->setObjectName("forexToolbar");
    toolbar->setFixedHeight(40);
    
    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(Spacing::MD, Spacing::XS, Spacing::MD, Spacing::XS);

    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    refreshBtn->setFixedSize(60, 26);
    StyleHelper::setSecondaryButton(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, this, &ForexPage::onRefreshData);
    layout->addWidget(refreshBtn);
    
    layout->addStretch();
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(toolbar);
}

void ForexPage::initForexList()
{
    d->forexListTable = new QTableWidget();
    d->forexListTable->setObjectName("forexListTable");
    d->forexListTable->setColumnCount(6);
    d->forexListTable->setHorizontalHeaderLabels({
        QStringLiteral("货币对"),
        QStringLiteral("汇率"),
        QStringLiteral("买入价"),
        QStringLiteral("卖出价"),
        QStringLiteral("涨跌幅"),
        QStringLiteral("更新时间")
    });
    
    d->forexListTable->horizontalHeader()->setStretchLastSection(true);
    d->forexListTable->verticalHeader()->setVisible(false);
    d->forexListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->forexListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 设置颜色委托（红涨绿跌）
    d->forexListTable->setItemDelegateForColumn(1, new WealthPilot::PriceColorDelegate(this)); // 汇率
    d->forexListTable->setItemDelegateForColumn(2, new WealthPilot::PriceColorDelegate(this)); // 买入价
    d->forexListTable->setItemDelegateForColumn(3, new WealthPilot::PriceColorDelegate(this)); // 卖出价
    d->forexListTable->setItemDelegateForColumn(4, new WealthPilot::ChangeColorDelegate(this)); // 涨跌幅
}

void ForexPage::initRateChart()
{
    d->rateChart = new KLineChart();
    d->rateChart->setMinimumHeight(300);
}

void ForexPage::initConverter()
{
    auto* converterGroup = new QGroupBox(QStringLiteral("汇率换算"));
    
    auto* layout = new QGridLayout(converterGroup);
    
    // 源货币
    QLabel* fromLabel = new QLabel(QStringLiteral("从:"));
    fromLabel->setProperty("dataType", "label");
    layout->addWidget(fromLabel, 0, 0);

    d->currencyFromCombo = new QComboBox();
    d->currencyFromCombo->addItems({"CNY", "USD", "EUR", "GBP", "JPY", "HKD"});
    d->currencyFromCombo->setObjectName("currencyCombo");
    layout->addWidget(d->currencyFromCombo, 0, 1);
    
    // 目标货币
    QLabel* toLabel = new QLabel(QStringLiteral("到:"));
    toLabel->setProperty("dataType", "label");
    layout->addWidget(toLabel, 1, 0);

    d->currencyToCombo = new QComboBox();
    d->currencyToCombo->addItems({"USD", "CNY", "EUR", "GBP", "JPY", "HKD"});
    d->currencyToCombo->setObjectName("currencyCombo");
    layout->addWidget(d->currencyToCombo, 1, 1);
    
    // 金额
    QLabel* amountLabel = new QLabel(QStringLiteral("金额:"));
    amountLabel->setProperty("dataType", "label");
    layout->addWidget(amountLabel, 2, 0);

    d->amountSpinBox = new QDoubleSpinBox();
    d->amountSpinBox->setRange(0, 100000000);
    d->amountSpinBox->setValue(100);
    d->amountSpinBox->setObjectName("amountSpin");
    layout->addWidget(d->amountSpinBox, 2, 1);
    
    // 结果
    QLabel* resultTitleLabel = new QLabel(QStringLiteral("结果:"));
    resultTitleLabel->setProperty("dataType", "label");
    layout->addWidget(resultTitleLabel, 3, 0);

    d->resultLabel = new QLabel(QStringLiteral("--"));
    d->resultLabel->setObjectName("conversionResult");
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
    
    QVector<ForexQuote> defaultQuotes;
    
    ForexQuote q1; q1.pair = "USD/CNY"; q1.baseCurrency = "USD"; q1.quoteCurrency = "CNY";
    q1.rate = 7.2456; q1.bid = 7.2450; q1.ask = 7.2462; q1.changePercent = 0.17;
    q1.high24h = 7.2500; q1.low24h = 7.2400; q1.updateTime = QDateTime::currentDateTime();
    defaultQuotes.append(q1);
    
    ForexQuote q2; q2.pair = "EUR/USD"; q2.baseCurrency = "EUR"; q2.quoteCurrency = "USD";
    q2.rate = 1.0892; q2.bid = 1.0890; q2.ask = 1.0894; q2.changePercent = -0.14;
    q2.high24h = 1.0920; q2.low24h = 1.0860; q2.updateTime = QDateTime::currentDateTime();
    defaultQuotes.append(q2);
    
    ForexQuote q3; q3.pair = "GBP/USD"; q3.baseCurrency = "GBP"; q3.quoteCurrency = "USD";
    q3.rate = 1.2654; q3.bid = 1.2650; q3.ask = 1.2658; q3.changePercent = 0.18;
    q3.high24h = 1.2700; q3.low24h = 1.2600; q3.updateTime = QDateTime::currentDateTime();
    defaultQuotes.append(q3);
    
    ForexQuote q4; q4.pair = "USD/JPY"; q4.baseCurrency = "USD"; q4.quoteCurrency = "JPY";
    q4.rate = 154.32; q4.bid = 154.30; q4.ask = 154.34; q4.changePercent = 0.29;
    q4.high24h = 155.00; q4.low24h = 153.50; q4.updateTime = QDateTime::currentDateTime();
    defaultQuotes.append(q4);
    
    ForexQuote q5; q5.pair = "EUR/CNY"; q5.baseCurrency = "EUR"; q5.quoteCurrency = "CNY";
    q5.rate = 7.8923; q5.bid = 7.8915; q5.ask = 7.8931; q5.changePercent = 0.20;
    q5.high24h = 7.9000; q5.low24h = 7.8800; q5.updateTime = QDateTime::currentDateTime();
    defaultQuotes.append(q5);
    
    d->forexCache = defaultQuotes;
    
    QStringList pairs = {"USD/CNY", "EUR/USD", "GBP/USD", "USD/JPY", "EUR/CNY"};
    ForexDataSource::instance()->requestQuotes(pairs, [this, defaultQuotes](const QVector<ForexQuote>& quotes) {
        if (quotes.isEmpty()) {
            d->forexCache = defaultQuotes;
            LOG_WARNING("Forex API failed, using default data");
        } else {
            d->forexCache = quotes;
        }

        updateForexTable();
    });

    updateForexTable();
}

void ForexPage::updateForexTable()
{
    d->forexListTable->setRowCount(d->forexCache.size());
    for (int i = 0; i < d->forexCache.size(); ++i) {
        const auto& forex = d->forexCache[i];
        
        d->forexListTable->setItem(i, 0, new QTableWidgetItem(forex.pair));
        d->forexListTable->setItem(i, 1, new QTableWidgetItem(QString::number(forex.rate, 'f', 4)));
        d->forexListTable->setItem(i, 2, new QTableWidgetItem(QString::number(forex.bid, 'f', 4)));
        d->forexListTable->setItem(i, 3, new QTableWidgetItem(QString::number(forex.ask, 'f', 4)));
        
        auto* changeItem = new QTableWidgetItem(QString::number(forex.changePercent, 'f', 2) + "%");
        changeItem->setForeground(QColor(forex.changePercent >= 0 ? Tokens::Colors::Danger : Tokens::Colors::Success));
        d->forexListTable->setItem(i, 4, changeItem);
        
        d->forexListTable->setItem(i, 5, new QTableWidgetItem(forex.updateTime.toString("hh:mm:ss")));
    }
    
    if (!d->forexCache.isEmpty()) {
        d->currentPair = d->forexCache[0].pair;
        d->currentQuote = d->forexCache[0];
        updateForexDetail(d->forexCache[0]);
    }
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
