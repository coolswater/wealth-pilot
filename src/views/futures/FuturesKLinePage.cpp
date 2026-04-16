/**
 * @file FuturesKLinePage.cpp
 * @brief Futures K-Line Page Implementation
 */

#include "FuturesKLinePage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTimer>
#include <QElapsedTimer>
#include <QHeaderView>

#include "core/navigation/PageNavigator.h"
#include "core/di/ServiceLocator.h"
#include "core/cache/CacheManager.h"
#include "plugins/IAIPlugin.h"
#include "utils/Logger.h"
#include "utils/TechnicalIndicators.h"

// ========== FuturesKLinePage::Impl Implementation ==========

struct FuturesKLinePage::Impl {
    // Contract info
    QString instrumentId;
    QString instrumentName;
    KLinePeriod currentPeriod = KLinePeriod::Minute15;
    
    // UI Components
    KLineChart* klineChart = nullptr;
    QComboBox* periodCombo = nullptr;
    QComboBox* indicatorCombo = nullptr;
    TechnicalIndicatorPanel* indicatorPanel = nullptr;
    TradingPanel* tradingPanel = nullptr;
    RealtimeQuoteWidget* quoteWidget = nullptr;
    QTableWidget* depthTable = nullptr;
    
    // Data
    QVector<KLineData> klineData;
    MarketData currentQuote;
    
    // Services
    ICTPPlugin* ctpPlugin = nullptr;
    IAIPlugin* aiPlugin = nullptr;
    
    // Timer
    QTimer* refreshTimer = nullptr;
    
    // Technical indicator states
    QMap<QString, bool> indicatorStates = {
        {"MA5", true},
        {"MA10", true},
        {"MA20", true},
        {"MA60", false},
        {"MACD", false},
        {"RSI", false},
        {"KDJ", false},
        {"BOLL", false}
    };
};

// ========== FuturesKLinePage Constructor and Destructor ==========

FuturesKLinePage::FuturesKLinePage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    LOG_DEBUG("FuturesKLinePage created");
}

FuturesKLinePage::~FuturesKLinePage()
{
    if (d->refreshTimer) {
        d->refreshTimer->stop();
    }
    LOG_DEBUG("FuturesKLinePage destroyed");
}

// ========== BasePage Interface Implementation ==========

void FuturesKLinePage::initializePage()
{
    setupUI();
    setupConnections();
    loadKLineData();
}

void FuturesKLinePage::refresh()
{
    loadKLineData();
}

void FuturesKLinePage::setInstrument(const QString& instrumentId, const QString& instrumentName)
{
    d->instrumentId = instrumentId;
    d->instrumentName = instrumentName;
    
    loadKLineData();
}

void FuturesKLinePage::setPeriod(KLinePeriod period)
{
    if (d->currentPeriod != period) {
        d->currentPeriod = period;
        loadKLineData();
    }
}

void FuturesKLinePage::setIndicatorEnabled(const QString& indicator, bool enabled)
{
    d->indicatorStates[indicator] = enabled;
    calculateIndicators();
}

// ========== Private Methods ==========

void FuturesKLinePage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Top toolbar
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(10, 5, 10, 5);
    
    // Period selector
    d->periodCombo = new QComboBox(this);
    d->periodCombo->addItem("1 Min", static_cast<int>(KLinePeriod::Minute1));
    d->periodCombo->addItem("5 Min", static_cast<int>(KLinePeriod::Minute5));
    d->periodCombo->addItem("15 Min", static_cast<int>(KLinePeriod::Minute15));
    d->periodCombo->addItem("30 Min", static_cast<int>(KLinePeriod::Minute30));
    d->periodCombo->addItem("1 Hour", static_cast<int>(KLinePeriod::Hour1));
    d->periodCombo->addItem("Daily", static_cast<int>(KLinePeriod::Day1));
    d->periodCombo->setCurrentIndex(2);  // Default 15 min
    toolbarLayout->addWidget(new QLabel("Period:", this));
    toolbarLayout->addWidget(d->periodCombo);
    
    // Indicator selector
    d->indicatorCombo = new QComboBox(this);
    d->indicatorCombo->addItem("MA", "MA");
    d->indicatorCombo->addItem("MACD", "MACD");
    d->indicatorCombo->addItem("RSI", "RSI");
    d->indicatorCombo->addItem("KDJ", "KDJ");
    d->indicatorCombo->addItem("BOLL", "BOLL");
    toolbarLayout->addWidget(new QLabel("Indicator:", this));
    toolbarLayout->addWidget(d->indicatorCombo);
    
    toolbarLayout->addStretch();
    mainLayout->addLayout(toolbarLayout);
    
    // Main content area
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    
    // K-Line chart
    d->klineChart = new KLineChart(this);
    splitter->addWidget(d->klineChart);
    
    // Bottom panel: Quote + Trading
    QWidget* bottomPanel = new QWidget(this);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    
    // Quote widget
    d->quoteWidget = new RealtimeQuoteWidget(this);
    bottomLayout->addWidget(d->quoteWidget);
    
    // Depth table
    d->depthTable = new QTableWidget(5, 4, this);
    d->depthTable->setHorizontalHeaderLabels({"Bid Vol", "Bid Price", "Ask Price", "Ask Vol"});
    d->depthTable->verticalHeader()->setVisible(false);
    d->depthTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bottomLayout->addWidget(d->depthTable);
    
    // Trading panel
    d->tradingPanel = new TradingPanel(this);
    bottomLayout->addWidget(d->tradingPanel);
    
    splitter->addWidget(bottomPanel);
    splitter->setSizes({600, 200});
    
    mainLayout->addWidget(splitter);
    
    // Indicator panel
    d->indicatorPanel = new TechnicalIndicatorPanel(this);
    d->indicatorPanel->setVisible(false);
    mainLayout->addWidget(d->indicatorPanel);
    
    // Refresh timer
    d->refreshTimer = new QTimer(this);
    d->refreshTimer->setInterval(1000);  // 1 second
}

void FuturesKLinePage::setupConnections()
{
    // Period changed
    connect(d->periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        int periodValue = d->periodCombo->itemData(index).toInt();
        setPeriod(static_cast<KLinePeriod>(periodValue));
    });
    
    // Indicator changed
    connect(d->indicatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        QString indicator = d->indicatorCombo->itemData(index).toString();
        bool enabled = !d->indicatorStates.value(indicator, false);
        setIndicatorEnabled(indicator, enabled);
    });
    
    // Refresh timer
    connect(d->refreshTimer, &QTimer::timeout, this, &FuturesKLinePage::refresh);
    
    // Get CTP plugin from service locator
    d->ctpPlugin = ServiceLocator::instance().resolve<ICTPPlugin>();
    if (d->ctpPlugin) {
        connect(d->ctpPlugin, &ICTPPlugin::marketDataUpdated,
                this, &FuturesKLinePage::onMarketDataUpdated);
    }
    
    // Get AI plugin
    d->aiPlugin = ServiceLocator::instance().resolve<IAIPlugin>();
}

void FuturesKLinePage::loadKLineData()
{
    if (d->instrumentId.isEmpty()) {
        return;
    }
    
    QElapsedTimer timer;
    timer.start();
    
    // Check cache
    QString cacheKey = QString("kline_%1_%2")
        .arg(d->instrumentId)
        .arg(static_cast<int>(d->currentPeriod));
    
    QVariant cached = CacheManager::instance()->get(cacheKey);
    if (cached.isValid()) {
        QVector<KLineData> data = cached.value<QVector<KLineData>>();
        onKLineDataReceived(data);
        LOG_DEBUG(QString("K-Line loaded from cache: %1").arg(d->instrumentId));
        return;
    }
    
    // Request from CTP plugin
    if (d->ctpPlugin) {
        // TODO: Request K-Line data from CTP
        LOG_DEBUG(QString("Requesting K-Line data: %1").arg(d->instrumentId));
    }
    
    LOG_DEBUG(QString("KLine data loading: %1").arg(d->instrumentId));
}

void FuturesKLinePage::calculateIndicators()
{
    if (d->klineData.isEmpty()) {
        return;
    }
    
    // Extract close prices
    QVector<double> closes;
    for (const auto& kline : d->klineData) {
        closes.append(kline.close);
    }
    
    // Calculate MA
    if (d->indicatorStates["MA5"]) {
        QVector<double> ma5 = TechnicalIndicators::SMA(closes, 5);
        d->klineChart->addIndicator("MA5", ma5, QColor("#FFD700"));
    }
    
    if (d->indicatorStates["MA10"]) {
        QVector<double> ma10 = TechnicalIndicators::SMA(closes, 10);
        d->klineChart->addIndicator("MA10", ma10, QColor("#00CED1"));
    }
    
    if (d->indicatorStates["MA20"]) {
        QVector<double> ma20 = TechnicalIndicators::SMA(closes, 20);
        d->klineChart->addIndicator("MA20", ma20, QColor("#FF6B6B"));
    }
    
    if (d->indicatorStates["MA60"]) {
        QVector<double> ma60 = TechnicalIndicators::SMA(closes, 60);
        d->klineChart->addIndicator("MA60", ma60, QColor("#9B59B6"));
    }
    
    // Calculate MACD
    if (d->indicatorStates["MACD"]) {
        auto macd = TechnicalIndicators::MACD(closes, 12, 26, 9);
        d->klineChart->addIndicator("MACD_DIF", macd.values["DIF"], QColor("#FFD700"));
        d->klineChart->addIndicator("MACD_DEA", macd.values["DEA"], QColor("#00CED1"));
        d->klineChart->addIndicator("MACD_HIST", macd.values["MACD"], QColor("#FF6B6B"));
    }
    
    // Calculate RSI
    if (d->indicatorStates["RSI"]) {
        QVector<double> rsi = TechnicalIndicators::RSI(closes, 14);
        d->klineChart->addIndicator("RSI", rsi, QColor("#9B59B6"));
    }
    
    // Calculate KDJ
    if (d->indicatorStates["KDJ"]) {
        QVector<double> highs, lows;
        for (const auto& kline : d->klineData) {
            highs.append(kline.high);
            lows.append(kline.low);
        }
        
        auto kdj = TechnicalIndicators::KDJ(highs, lows, closes, 9, 3, 3);
        d->klineChart->addIndicator("KDJ_K", kdj.values["K"], QColor("#FFD700"));
        d->klineChart->addIndicator("KDJ_D", kdj.values["D"], QColor("#00CED1"));
        d->klineChart->addIndicator("KDJ_J", kdj.values["J"], QColor("#FF6B6B"));
    }
    
    // Calculate Bollinger Bands
    if (d->indicatorStates["BOLL"]) {
        auto boll = TechnicalIndicators::BollingerBands(closes, 20, 2);
        d->klineChart->addIndicator("BOLL_UPPER", boll.values["upper"], QColor("#FF6B6B"));
        d->klineChart->addIndicator("BOLL_MIDDLE", boll.values["middle"], QColor("#FFD700"));
        d->klineChart->addIndicator("BOLL_LOWER", boll.values["lower"], QColor("#00CED1"));
    }
    
    // Update indicator panel
    if (d->indicatorPanel) {
        QMap<QString, double> indicatorData;
        
        if (!closes.isEmpty()) {
            indicatorData["MA5"] = TechnicalIndicators::SMA(closes, 5).last();
            indicatorData["MA10"] = TechnicalIndicators::SMA(closes, 10).last();
            indicatorData["MA20"] = TechnicalIndicators::SMA(closes, 20).last();
            indicatorData["RSI"] = TechnicalIndicators::RSI(closes, 14).last();
        }
        
        d->indicatorPanel->setIndicatorData("MA", indicatorData);
    }
    
    LOG_DEBUG("Technical indicators calculated");
}

void FuturesKLinePage::updateQuoteDisplay(const MarketData& quote)
{
    if (d->quoteWidget) {
        d->quoteWidget->updateQuote(quote);
    }
    
    // Update depth table
    if (d->depthTable) {
        // Bid level 1
        QTableWidgetItem* bidVolumeItem = d->depthTable->item(0, 0);
        if (!bidVolumeItem) {
            bidVolumeItem = new QTableWidgetItem();
            d->depthTable->setItem(0, 0, bidVolumeItem);
        }
        bidVolumeItem->setText(QString::number(quote.bidVolume1));
        
        QTableWidgetItem* bidPriceItem = d->depthTable->item(0, 1);
        if (!bidPriceItem) {
            bidPriceItem = new QTableWidgetItem();
            d->depthTable->setItem(0, 1, bidPriceItem);
        }
        bidPriceItem->setText(QString::number(quote.bidPrice1, 'f', 2));
        
        // Ask level 1
        QTableWidgetItem* askVolumeItem = d->depthTable->item(0, 2);
        if (!askVolumeItem) {
            askVolumeItem = new QTableWidgetItem();
            d->depthTable->setItem(0, 2, askVolumeItem);
        }
        askVolumeItem->setText(QString::number(quote.askVolume1));
        
        QTableWidgetItem* askPriceItem = d->depthTable->item(0, 3);
        if (!askPriceItem) {
            askPriceItem = new QTableWidgetItem();
            d->depthTable->setItem(0, 3, askPriceItem);
        }
        askPriceItem->setText(QString::number(quote.askPrice1, 'f', 2));
    }
}

// ========== Slots ==========

void FuturesKLinePage::onMarketDataUpdated(const MarketData& data)
{
    if (data.instrumentId == d->instrumentId) {
        d->currentQuote = data;
        updateQuoteDisplay(data);
    }
}

void FuturesKLinePage::onKLineDataReceived(const QVector<KLineData>& data)
{
    d->klineData = data;
    
    // Update chart
    if (d->klineChart) {
        d->klineChart->setData(data);
    }
    
    // Calculate indicators
    calculateIndicators();
    
    // Cache the data
    QString cacheKey = QString("kline_%1_%2")
        .arg(d->instrumentId)
        .arg(static_cast<int>(d->currentPeriod));
    CacheManager::instance()->set(cacheKey, QVariant::fromValue(data), 60);
    
    LOG_DEBUG(QString("K-Line data updated: %1 bars").arg(data.size()));
}

void FuturesKLinePage::onPeriodChanged(int period)
{
    setPeriod(static_cast<KLinePeriod>(period));
}

void FuturesKLinePage::onIndicatorToggled(const QString& indicator, bool enabled)
{
    setIndicatorEnabled(indicator, enabled);
}

// ========== RealtimeQuoteWidget Implementation ==========

struct RealtimeQuoteWidget::Impl {
    QLabel* priceLabel = nullptr;
    QLabel* changeLabel = nullptr;
    QLabel* volumeLabel = nullptr;
    QLabel* highLabel = nullptr;
    QLabel* lowLabel = nullptr;
    
    double lastPrice = 0.0;
    double preClosePrice = 0.0;
};

RealtimeQuoteWidget::RealtimeQuoteWidget(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

RealtimeQuoteWidget::~RealtimeQuoteWidget() = default;

void RealtimeQuoteWidget::setupUI()
{
    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(5);
    
    // Price
    QLabel* priceTitle = new QLabel("Last", this);
    priceTitle->setStyleSheet("color: #9E9E9E;");
    
    d->priceLabel = new QLabel("--", this);
    d->priceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #E0E0E0;");
    
    layout->addWidget(priceTitle, 0, 0);
    layout->addWidget(d->priceLabel, 0, 1);
    
    // Change
    QLabel* changeTitle = new QLabel("Change", this);
    changeTitle->setStyleSheet("color: #9E9E9E;");
    
    d->changeLabel = new QLabel("--", this);
    d->changeLabel->setStyleSheet("color: #E0E0E0;");
    
    layout->addWidget(changeTitle, 1, 0);
    layout->addWidget(d->changeLabel, 1, 1);
    
    // Volume
    QLabel* volumeTitle = new QLabel("Volume", this);
    volumeTitle->setStyleSheet("color: #9E9E9E;");
    
    d->volumeLabel = new QLabel("--", this);
    d->volumeLabel->setStyleSheet("color: #E0E0E0;");
    
    layout->addWidget(volumeTitle, 2, 0);
    layout->addWidget(d->volumeLabel, 2, 1);
    
    // High
    QLabel* highTitle = new QLabel("High", this);
    highTitle->setStyleSheet("color: #9E9E9E;");
    
    d->highLabel = new QLabel("--", this);
    d->highLabel->setStyleSheet("color: #4CAF50;");
    
    layout->addWidget(highTitle, 3, 0);
    layout->addWidget(d->highLabel, 3, 1);
    
    // Low
    QLabel* lowTitle = new QLabel("Low", this);
    lowTitle->setStyleSheet("color: #9E9E9E;");
    
    d->lowLabel = new QLabel("--", this);
    d->lowLabel->setStyleSheet("color: #EF5350;");
    
    layout->addWidget(lowTitle, 4, 0);
    layout->addWidget(d->lowLabel, 4, 1);
}

void RealtimeQuoteWidget::updateQuote(const MarketData& quote)
{
    d->lastPrice = quote.lastPrice;
    d->preClosePrice = quote.openPrice;
    
    // Update price
    d->priceLabel->setText(QString::number(quote.lastPrice, 'f', 2));
    
    // Update change
    double change = quote.lastPrice - quote.openPrice;
    double changePercent = quote.openPrice > 0 ? change / quote.openPrice * 100 : 0;
    
    QString changeText = QString("%1 (%2%)")
        .arg(change, 0, 'f', 2)
        .arg(changePercent, 0, 'f', 2);
    d->changeLabel->setText(changeText);
    
    // Set color based on change
    if (change > 0) {
        d->changeLabel->setStyleSheet("color: #EF5350;");
    } else if (change < 0) {
        d->changeLabel->setStyleSheet("color: #4CAF50;");
    } else {
        d->changeLabel->setStyleSheet("color: #E0E0E0;");
    }
    
    // Update volume
    d->volumeLabel->setText(QString::number(quote.volume));
    
    // Update high/low
    d->highLabel->setText(QString::number(quote.highestPrice, 'f', 2));
    d->lowLabel->setText(QString::number(quote.lowestPrice, 'f', 2));
}

// ========== TradingPanel Implementation ==========

struct TradingPanel::Impl {
    QComboBox* directionCombo = nullptr;
    QComboBox* offsetCombo = nullptr;
    QDoubleSpinBox* priceSpinBox = nullptr;
    QSpinBox* volumeSpinBox = nullptr;
    QPushButton* submitButton = nullptr;
};

TradingPanel::TradingPanel(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

TradingPanel::~TradingPanel() = default;

void TradingPanel::setupUI()
{
    QGridLayout* layout = new QGridLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(5);
    
    // Direction
    layout->addWidget(new QLabel("Direction:", this), 0, 0);
    d->directionCombo = new QComboBox(this);
    d->directionCombo->addItem("Buy", "buy");
    d->directionCombo->addItem("Sell", "sell");
    layout->addWidget(d->directionCombo, 0, 1);
    
    // Offset
    layout->addWidget(new QLabel("Offset:", this), 1, 0);
    d->offsetCombo = new QComboBox(this);
    d->offsetCombo->addItem("Open", "open");
    d->offsetCombo->addItem("Close", "close");
    layout->addWidget(d->offsetCombo, 1, 1);
    
    // Price
    layout->addWidget(new QLabel("Price:", this), 2, 0);
    d->priceSpinBox = new QDoubleSpinBox(this);
    d->priceSpinBox->setDecimals(2);
    d->priceSpinBox->setRange(0, 999999);
    d->priceSpinBox->setValue(0);
    layout->addWidget(d->priceSpinBox, 2, 1);
    
    // Volume
    layout->addWidget(new QLabel("Volume:", this), 3, 0);
    d->volumeSpinBox = new QSpinBox(this);
    d->volumeSpinBox->setRange(1, 9999);
    d->volumeSpinBox->setValue(1);
    layout->addWidget(d->volumeSpinBox, 3, 1);
    
    // Submit button
    d->submitButton = new QPushButton("Submit", this);
    d->submitButton->setStyleSheet("background-color: #2196F3; color: white;");
    layout->addWidget(d->submitButton, 4, 0, 1, 2);
}

// ========== TechnicalIndicatorPanel Implementation ==========

struct TechnicalIndicatorPanel::Impl {
    QMap<QString, QLabel*> labels;
};

TechnicalIndicatorPanel::TechnicalIndicatorPanel(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    
    // Create labels for common indicators
    for (const QString& name : {"MA5", "MA10", "MA20", "RSI", "MACD"}) {
        QLabel* label = new QLabel(QString("%1: --").arg(name), this);
        label->setStyleSheet("color: #E0E0E0;");
        layout->addWidget(label);
        d->labels[name] = label;
    }
}

TechnicalIndicatorPanel::~TechnicalIndicatorPanel() = default;

void TechnicalIndicatorPanel::setIndicatorData(const QString& group, const QMap<QString, double>& data)
{
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (d->labels.contains(it.key())) {
            d->labels[it.key()]->setText(QString("%1: %2").arg(it.key()).arg(it.value(), 0, 'f', 2));
        }
    }
}
void FuturesKLinePage::onPageActivated(const QVariantMap& params)
{
    // Page activated with parameters
    if (params.contains("instrumentId")) {
        QString instrumentId = params["instrumentId"].toString();
        // TODO: Load K-line data for instrument
    }
}