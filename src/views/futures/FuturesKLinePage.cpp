/**
 * @file FuturesKLinePage.cpp
 * @brief 期货K线详情页实现 - 高性能K线图和技术分析
 *
 * @details 实现功能：
 * - 实时K线图表（支持多周期切换）
 * - 技术指标分析（MA、MACD、RSI、KDJ等）
 * - 成交量分析
 * - 实时行情数据展示
 * - 交易操作面板
 * - AI智能分析建议
 *
 * @author WealthPilot Team
 * @version 2.0.0
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

#include "core/PageNavigator.h"
#include "core/ServiceLocator.h"
#include "core/CacheManager.h"
#include "plugins/IAIPlugin.h"
#include "utils/Logger.h"
#include "utils/TechnicalIndicators.h"

// ========== FuturesKLinePage::Impl 实现 ==========

/**
 * @brief PIMPL实现结构
 */
struct FuturesKLinePage::Impl {
    // 合约信息
    QString instrumentId;           // 合约代码
    QString instrumentName;         // 合约名称
    KLinePeriod currentPeriod = KLinePeriod::Minute15;  // 当前周期
    
    // UI组件
    KLineChart* klineChart = nullptr;               // K线图
    QComboBox* periodCombo = nullptr;               // 周期选择
    QComboBox* indicatorCombo = nullptr;            // 指标选择
    TechnicalIndicatorPanel* indicatorPanel = nullptr;  // 指标面板
    TradingPanel* tradingPanel = nullptr;           // 交易面板
    RealtimeQuoteWidget* quoteWidget = nullptr;     // 行情组件
    QTableWidget* depthTable = nullptr;             // 盘口表格
    
    // 数据
    QVector<KLineData> klineData;   // K线数据
    MarketData currentQuote;        // 当前行情
    
    // 服务
    ICTPPlugin* ctpPlugin = nullptr;
    IAIPlugin* aiPlugin = nullptr;
    
    // 定时器
    QTimer* refreshTimer = nullptr;
    
    // 技术指标开关
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

// ========== FuturesKLinePage 构造和析构 ==========

/**
 * @brief 构造函数
 * @param parent 父控件
 */
FuturesKLinePage::FuturesKLinePage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    LOG_DEBUG("FuturesKLinePage created");
}

/**
 * @brief 析构函数
 */
FuturesKLinePage::~FuturesKLinePage()
{
    if (d->refreshTimer) {
        d->refreshTimer->stop();
    }
    
    LOG_DEBUG("FuturesKLinePage destroyed");
}

/**
 * @brief 初始化页面
 */
void FuturesKLinePage::initializePage()
{
    setupUI();
    connectSignals();
    LOG_DEBUG("FuturesKLinePage initialized");
}

/**
 * @brief 页面激活（重写）
 * @param params 导航参数
 */
void FuturesKLinePage::onPageActivated(const QVariantMap& params)
{
    LOG_INFO(QString("FuturesKLinePage activated with params: %1").arg(params.size()));
    
    // 处理导航参数
    if (params.contains(NavParam::INSTRUMENT_ID)) {
        QString instrumentId = params[NavParam::INSTRUMENT_ID].toString();
        setInstrument(instrumentId);
    }
    
    if (params.contains(NavParam::PERIOD)) {
        int periodValue = params[NavParam::PERIOD].toInt();
        setPeriod(static_cast<KLinePeriod>(periodValue));
    }
}

// ========== 初始化 ==========

/**
 * @brief 初始化UI
 */
void FuturesKLinePage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 顶部工具栏
    QWidget* toolbar = createToolbar();
    mainLayout->addWidget(toolbar);
    
    // 主内容区域（分割器）
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet("QSplitter::handle { background: #2A2A3E; }");
    
    // 左侧：K线图表区域
    QWidget* chartArea = createChartArea();
    splitter->addWidget(chartArea);
    
    // 右侧：信息面板
    QWidget* rightPanel = createRightPanel();
    splitter->addWidget(rightPanel);
    
    // 设置分割比例
    splitter->setStretchFactor(0, 3);  // K线图占3份
    splitter->setStretchFactor(1, 1);  // 右侧面板占1份
    
    mainLayout->addWidget(splitter, 1);
    
    // 底部面板
    QWidget* bottomPanel = createBottomPanel();
    mainLayout->addWidget(bottomPanel);
    
    // 初始化定时器
    d->refreshTimer = new QTimer(this);
    connect(d->refreshTimer, &QTimer::timeout, this, &FuturesKLinePage::refresh);
    
    // 获取服务
    d->ctpPlugin = ServiceLocator::instance().resolve<ICTPPlugin>();
    d->aiPlugin = ServiceLocator::instance().resolve<IAIPlugin>();
    
    LOG_DEBUG("FuturesKLinePage UI setup completed");
}

/**
 * @brief 连接信号
 */
void FuturesKLinePage::connectSignals()
{
    // 周期切换
    connect(d->periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FuturesKLinePage::onPeriodChanged);
    
    // K线图交互
    connect(d->klineChart, &KLineChart::crosshairMoved,
            this, [this](const QDateTime& time, double price) {
        // 更新价格显示
        if (d->tradingPanel) {
            d->tradingPanel->setPrice(price);
        }
    });
    
    LOG_DEBUG("FuturesKLinePage signals connected");
}

// ========== 创建UI组件 ==========

/**
 * @brief 创建顶部工具栏
 */
QWidget* FuturesKLinePage::createToolbar()
{
    QWidget* toolbar = new QWidget(this);
    toolbar->setFixedHeight(50);
    toolbar->setStyleSheet(R"(
        QWidget {
            background: #1E1E2E;
            border-bottom: 1px solid #2A2A3E;
        }
        QLabel {
            color: #E0E0E0;
            font-size: 14px;
        }
        QComboBox {
            background: #2A2A3E;
            border: 1px solid #3A3A4E;
            border-radius: 4px;
            padding: 5px 10px;
            color: #E0E0E0;
            min-width: 80px;
        }
        QComboBox:hover {
            border-color: #4A90E2;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QPushButton {
            background: #2A2A3E;
            border: 1px solid #3A3A4E;
            border-radius: 4px;
            padding: 5px 15px;
            color: #E0E0E0;
            font-size: 13px;
        }
        QPushButton:hover {
            background: #3A3A4E;
            border-color: #4A90E2;
        }
        QPushButton:pressed {
            background: #4A90E2;
        }
    )");
    
    QHBoxLayout* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(15, 0, 15, 0);
    layout->setSpacing(10);
    
    // 合约名称
    QLabel* contractLabel = new QLabel("合约:", toolbar);
    layout->addWidget(contractLabel);
    
    QLabel* instrumentLabel = new QLabel("选择合约", toolbar);
    instrumentLabel->setStyleSheet("font-weight: bold; color: #4A90E2;");
    layout->addWidget(instrumentLabel);
    
    layout->addSpacing(20);
    
    // 分隔线
    QFrame* separator1 = new QFrame(toolbar);
    separator1->setFrameShape(QFrame::VLine);
    separator1->setStyleSheet("color: #3A3A4E;");
    layout->addWidget(separator1);
    
    // 周期选择
    QLabel* periodLabel = new QLabel("周期:", toolbar);
    layout->addWidget(periodLabel);
    
    d->periodCombo = new QComboBox(toolbar);
    d->periodCombo->addItem("1分钟", static_cast<int>(KLinePeriod::Minute1));
    d->periodCombo->addItem("5分钟", static_cast<int>(KLinePeriod::Minute5));
    d->periodCombo->addItem("15分钟", static_cast<int>(KLinePeriod::Minute15));
    d->periodCombo->addItem("30分钟", static_cast<int>(KLinePeriod::Minute30));
    d->periodCombo->addItem("1小时", static_cast<int>(KLinePeriod::Hour1));
    d->periodCombo->addItem("4小时", static_cast<int>(KLinePeriod::Hour4));
    d->periodCombo->addItem("日线", static_cast<int>(KLinePeriod::Day1));
    d->periodCombo->addItem("周线", static_cast<int>(KLinePeriod::Week1));
    d->periodCombo->addItem("月线", static_cast<int>(KLinePeriod::Month1));
    d->periodCombo->setCurrentIndex(2);  // 默认15分钟
    layout->addWidget(d->periodCombo);
    
    layout->addSpacing(10);
    
    // 分隔线
    QFrame* separator2 = new QFrame(toolbar);
    separator2->setFrameShape(QFrame::VLine);
    separator2->setStyleSheet("color: #3A3A4E;");
    layout->addWidget(separator2);
    
    // 技术指标选择
    QLabel* indicatorLabel = new QLabel("指标:", toolbar);
    layout->addWidget(indicatorLabel);
    
    d->indicatorCombo = new QComboBox(toolbar);
    d->indicatorCombo->addItem("MA均线", "MA");
    d->indicatorCombo->addItem("MACD", "MACD");
    d->indicatorCombo->addItem("RSI", "RSI");
    d->indicatorCombo->addItem("KDJ", "KDJ");
    d->indicatorCombo->addItem("布林带", "BOLL");
    d->indicatorCombo->addItem("成交量", "VOL");
    layout->addWidget(d->indicatorCombo);
    
    layout->addStretch();
    
    // 刷新按钮
    QPushButton* refreshBtn = new QPushButton("🔄 刷新", toolbar);
    connect(refreshBtn, &QPushButton::clicked, this, &FuturesKLinePage::refresh);
    layout->addWidget(refreshBtn);
    
    // AI分析按钮
    QPushButton* aiBtn = new QPushButton("🤖 AI分析", toolbar);
    connect(aiBtn, &QPushButton::clicked, this, &FuturesKLinePage::onAIAnalysisRequested);
    layout->addWidget(aiBtn);
    
    return toolbar;
}

/**
 * @brief 创建K线图表区域
 */
QWidget* FuturesKLinePage::createChartArea()
{
    QWidget* chartArea = new QWidget(this);
    chartArea->setStyleSheet("background: #1A1A2E;");
    
    QVBoxLayout* layout = new QVBoxLayout(chartArea);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    
    // K线图组件
    d->klineChart = new KLineChart(chartArea);
    d->klineChart->setMinimumHeight(400);
    layout->addWidget(d->klineChart, 1);
    
    return chartArea;
}

/**
 * @brief 创建右侧面板
 */
QWidget* FuturesKLinePage::createRightPanel()
{
    QWidget* panel = new QWidget(this);
    panel->setFixedWidth(300);
    panel->setStyleSheet(R"(
        QWidget {
            background: #1E1E2E;
        }
        QGroupBox {
            color: #E0E0E0;
            font-weight: bold;
            border: 1px solid #2A2A3E;
            border-radius: 4px;
            margin-top: 10px;
            padding-top: 10px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px;
        }
    )");
    
    QVBoxLayout* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);
    
    // 实时行情组件
    QGroupBox* quoteGroup = new QGroupBox("实时行情", panel);
    QVBoxLayout* quoteLayout = new QVBoxLayout(quoteGroup);
    d->quoteWidget = new RealtimeQuoteWidget(quoteGroup);
    quoteLayout->addWidget(d->quoteWidget);
    layout->addWidget(quoteGroup);
    
    // 交易面板
    QGroupBox* tradeGroup = new QGroupBox("交易下单", panel);
    QVBoxLayout* tradeLayout = new QVBoxLayout(tradeGroup);
    d->tradingPanel = new TradingPanel(tradeGroup);
    connect(d->tradingPanel, &TradingPanel::buyClicked, this, &FuturesKLinePage::onBuyClicked);
    connect(d->tradingPanel, &TradingPanel::sellClicked, this, &FuturesKLinePage::onSellClicked);
    tradeLayout->addWidget(d->tradingPanel);
    layout->addWidget(tradeGroup);
    
    // 技术指标面板
    QGroupBox* indicatorGroup = new QGroupBox("技术指标", panel);
    QVBoxLayout* indicatorLayout = new QVBoxLayout(indicatorGroup);
    d->indicatorPanel = new TechnicalIndicatorPanel(indicatorGroup);
    connect(d->indicatorPanel, &TechnicalIndicatorPanel::indicatorToggled,
            this, &FuturesKLinePage::onIndicatorToggled);
    indicatorLayout->addWidget(d->indicatorPanel);
    layout->addWidget(indicatorGroup);
    
    return panel;
}

/**
 * @brief 创建底部面板
 */
QWidget* FuturesKLinePage::createBottomPanel()
{
    QWidget* panel = new QWidget(this);
    panel->setFixedHeight(150);
    panel->setStyleSheet(R"(
        QWidget {
            background: #1E1E2E;
            border-top: 1px solid #2A2A3E;
        }
        QTableWidget {
            background: transparent;
            color: #E0E0E0;
            border: none;
            gridline-color: #2A2A3E;
        }
        QTableWidget::item {
            padding: 5px;
        }
        QHeaderView::section {
            background: #2A2A3E;
            color: #E0E0E0;
            border: none;
            padding: 5px;
        }
    )");
    
    QHBoxLayout* layout = new QHBoxLayout(panel);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);
    
    // 五档盘口
    QGroupBox* depthGroup = new QGroupBox("五档盘口", panel);
    QVBoxLayout* depthLayout = new QVBoxLayout(depthGroup);
    
    d->depthTable = new QTableWidget(5, 4, depthGroup);
    d->depthTable->setHorizontalHeaderLabels({"买量", "价格", "卖量", "价格"});
    d->depthTable->horizontalHeader()->setStretchLastSection(true);
    d->depthTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->depthTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    depthLayout->addWidget(d->depthTable);
    
    layout->addWidget(depthGroup, 1);
    
    // 成交记录
    QGroupBox* tradeGroup = new QGroupBox("最新成交", panel);
    QVBoxLayout* tradeLayout = new QVBoxLayout(tradeGroup);
    
    QTableWidget* tradeTable = new QTableWidget(5, 3, tradeGroup);
    tradeTable->setHorizontalHeaderLabels({"时间", "价格", "成交量"});
    tradeTable->horizontalHeader()->setStretchLastSection(true);
    tradeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tradeLayout->addWidget(tradeTable);
    
    layout->addWidget(tradeGroup, 1);
    
    return panel;
}

// ========== 公共方法 ==========

/**
 * @brief 设置合约
 * @param instrumentId 合约代码
 */
void FuturesKLinePage::setInstrument(const QString& instrumentId)
{
    d->instrumentId = instrumentId;
    
    // 更新UI
    if (d->quoteWidget) {
        d->quoteWidget->setInstrument(instrumentId);
    }
    if (d->tradingPanel) {
        d->tradingPanel->setInstrument(instrumentId);
    }
    
    // 加载K线数据
    loadKLineData();
    
    // 订阅行情
    if (d->ctpPlugin) {
        d->ctpPlugin->subscribeMarketData({instrumentId});
    }
    
    LOG_INFO(QString("Instrument set: %1").arg(instrumentId));
}

/**
 * @brief 获取合约代码
 */
QString FuturesKLinePage::instrument() const
{
    return d->instrumentId;
}

/**
 * @brief 设置K线周期
 */
void FuturesKLinePage::setPeriod(KLinePeriod period)
{
    d->currentPeriod = period;
    
    // 更新下拉框
    int index = d->periodCombo->findData(static_cast<int>(period));
    if (index >= 0) {
        d->periodCombo->setCurrentIndex(index);
    }
    
    // 重新加载数据
    loadKLineData();
}

/**
 * @brief 刷新数据
 */
void FuturesKLinePage::refresh()
{
    loadKLineData();
}

// ========== 私槽函数 ==========

/**
 * @brief 周期切换
 */
void FuturesKLinePage::onPeriodChanged(int index)
{
    KLinePeriod period = static_cast<KLinePeriod>(d->periodCombo->itemData(index).toInt());
    d->currentPeriod = period;
    
    LOG_DEBUG(QString("Period changed to: %1").arg(periodText(period)));
    
    loadKLineData();
}

/**
 * @brief 技术指标切换
 */
void FuturesKLinePage::onIndicatorToggled(const QString& indicator, bool enabled)
{
    d->indicatorStates[indicator] = enabled;
    
    if (enabled) {
        // 计算并显示指标
        calculateIndicators();
    } else {
        // 移除指标
        d->klineChart->removeIndicator(indicator);
    }
    
    LOG_DEBUG(QString("Indicator toggled: %1 = %2").arg(indicator).arg(enabled));
}

/**
 * @brief K线数据更新
 */
void FuturesKLinePage::onKLineDataReceived(const QVector<KLineData>& data)
{
    d->klineData = data;
    d->klineChart->setData(data);
    
    // 计算技术指标
    calculateIndicators();
    
    LOG_DEBUG(QString("KLine data received: %1 items").arg(data.size()));
}

/**
 * @brief 实时行情更新
 */
void FuturesKLinePage::onQuoteUpdated(const MarketData& quote)
{
    d->currentQuote = quote;
    
    // 更新行情显示
    updateQuoteDisplay(quote);
    
    // 更新交易面板价格
    if (d->tradingPanel) {
        d->tradingPanel->setPrice(quote.lastPrice);
    }
    
    // 更新K线（如果是当前周期）
    if (!d->klineData.isEmpty()) {
        KLineData lastKline = d->klineData.last();
        
        // 更新最高价、最低价、收盘价
        lastKline.high = qMax(lastKline.high, quote.lastPrice);
        lastKline.low = qMin(lastKline.low, quote.lastPrice);
        lastKline.close = quote.lastPrice;
        lastKline.volume = quote.volume;
        
        d->klineChart->updateLastData(lastKline);
    }
}

/**
 * @brief 买入按钮点击
 */
void FuturesKLinePage::onBuyClicked()
{
    emit tradeRequested(d->instrumentId, "BUY", d->currentQuote.lastPrice, 1);
    
    LOG_INFO(QString("Buy requested: %1 @ %2")
        .arg(d->instrumentId).arg(d->currentQuote.lastPrice));
}

/**
 * @brief 卖出按钮点击
 */
void FuturesKLinePage::onSellClicked()
{
    emit tradeRequested(d->instrumentId, "SELL", d->currentQuote.lastPrice, 1);
    
    LOG_INFO(QString("Sell requested: %1 @ %2")
        .arg(d->instrumentId).arg(d->currentQuote.lastPrice));
}

/**
 * @brief AI分析请求
 */
void FuturesKLinePage::onAIAnalysisRequested()
{
    if (!d->aiPlugin) {
        LOG_WARNING("AI plugin not available");
        return;
    }
    
    // 构建分析请求
    QString prompt = QString("请分析期货合约 %1 的走势，当前价格: %2，"
                            "最高价: %3，最低价: %4，成交量: %5。"
                            "请给出技术分析和交易建议。")
        .arg(d->instrumentId)
        .arg(d->currentQuote.lastPrice)
        .arg(d->currentQuote.highestPrice)
        .arg(d->currentQuote.lowestPrice)
        .arg(d->currentQuote.volume);
    
    // 异步请求AI分析
    QJsonObject context;
    context["instrumentId"] = d->instrumentId;
    d->aiPlugin->sendMessageAsync(prompt, context);
}

// ========== 私有方法 ==========

/**
 * @brief 加载K线数据
 */
void FuturesKLinePage::loadKLineData()
{
    if (d->instrumentId.isEmpty()) {
        return;
    }
    
    QElapsedTimer timer;
    timer.start();
    
    // 检查缓存
    QString cacheKey = QString("kline_%1_%2")
        .arg(d->instrumentId)
        .arg(static_cast<int>(d->currentPeriod));
    
    QVariant cached = CacheManager::instance()->get(cacheKey);
    if (cached.isValid()) {
        // 使用缓存数据
        QVector<KLineData> data = cached.value<QVector<KLineData>>();
        onKLineDataReceived(data);
        
        LOG_DEBUG(QString("KLine data loaded from cache: %1ms").arg(timer.elapsed()));
        return;
    }
    
    // 从CTP服务获取数据（简化实现：使用模拟数据）
    if (d->ctpPlugin) {
        // TODO: 实际实现需要CTP API支持K线数据订阅
        // 目前使用空数据触发UI更新
        QVector<KLineData> emptyData;
        onKLineDataReceived(emptyData);
        LOG_DEBUG(QString("KLine data request sent for %1").arg(d->instrumentId));
    }
    
    LOG_DEBUG(QString("KLine data loading: %1").arg(d->instrumentId));
}

/**
 * @brief 计算技术指标
 */
void FuturesKLinePage::calculateIndicators()
{
    if (d->klineData.isEmpty()) {
        return;
    }
    
    // 提取收盘价
    QVector<double> closes;
    for (const auto& kline : d->klineData) {
        closes.append(kline.close);
    }
    
    // 计算MA均线
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
    
    // 计算MACD
    if (d->indicatorStates["MACD"]) {
        auto macd = TechnicalIndicators::MACD(closes, 12, 26, 9);
        d->klineChart->addIndicator("MACD_DIF", macd.values["DIF"], QColor("#FFD700"));
        d->klineChart->addIndicator("MACD_DEA", macd.values["DEA"], QColor("#00CED1"));
        d->klineChart->addIndicator("MACD_HIST", macd.values["MACD"], QColor("#FF6B6B"));
    }
    
    // 计算RSI
    if (d->indicatorStates["RSI"]) {
        QVector<double> rsi = TechnicalIndicators::RSI(closes, 14);
        d->klineChart->addIndicator("RSI", rsi, QColor("#9B59B6"));
    }
    
    // 计算KDJ
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
    
    // 计算布林带
    if (d->indicatorStates["BOLL"]) {
        auto boll = TechnicalIndicators::BollingerBands(closes, 20, 2);
        d->klineChart->addIndicator("BOLL_UPPER", boll.values["upper"], QColor("#FF6B6B"));
        d->klineChart->addIndicator("BOLL_MIDDLE", boll.values["middle"], QColor("#FFD700"));
        d->klineChart->addIndicator("BOLL_LOWER", boll.values["lower"], QColor("#00CED1"));
    }
    
    // 更新指标面板
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

/**
 * @brief 更新行情显示
 */
void FuturesKLinePage::updateQuoteDisplay(const MarketData& quote)
{
    if (d->quoteWidget) {
        d->quoteWidget->updateQuote(quote);
    }
    
    // 更新五档盘口（MarketData只有一档数据，简化显示）
    if (d->depthTable) {
        // 买盘第一档
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
        
        // 卖盘第一档
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

/**
 * @brief 获取周期文本
 */
QString FuturesKLinePage::periodText(KLinePeriod period) const
{
    switch (period) {
        case KLinePeriod::Minute1: return "1分钟";
        case KLinePeriod::Minute5: return "5分钟";
        case KLinePeriod::Minute15: return "15分钟";
        case KLinePeriod::Minute30: return "30分钟";
        case KLinePeriod::Hour1: return "1小时";
        case KLinePeriod::Hour4: return "4小时";
        case KLinePeriod::Day1: return "日线";
        case KLinePeriod::Week1: return "周线";
        case KLinePeriod::Month1: return "月线";
        default: return "未知";
    }
}

/**
 * @brief 获取周期分钟数
 */
int FuturesKLinePage::periodMinutes(KLinePeriod period) const
{
    switch (period) {
        case KLinePeriod::Minute1: return 1;
        case KLinePeriod::Minute5: return 5;
        case KLinePeriod::Minute15: return 15;
        case KLinePeriod::Minute30: return 30;
        case KLinePeriod::Hour1: return 60;
        case KLinePeriod::Hour4: return 240;
        case KLinePeriod::Day1: return 1440;
        case KLinePeriod::Week1: return 10080;
        case KLinePeriod::Month1: return 43200;
        default: return 15;
    }
}

// ========== TechnicalIndicatorPanel 实现 ==========

struct TechnicalIndicatorPanel::Impl {
    QMap<QString, QCheckBox*> checkboxes;
    QMap<QString, QLabel*> valueLabels;
};

TechnicalIndicatorPanel::TechnicalIndicatorPanel(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

TechnicalIndicatorPanel::~TechnicalIndicatorPanel() = default;

void TechnicalIndicatorPanel::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(5);
    
    // MA均线
    QGridLayout* maLayout = new QGridLayout();
    
    QStringList indicators = {"MA5", "MA10", "MA20", "MA60"};
    QStringList colors = {"#FFD700", "#00CED1", "#FF6B6B", "#9B59B6"};
    
    for (int i = 0; i < indicators.size(); ++i) {
        QCheckBox* checkbox = new QCheckBox(indicators[i], this);
        checkbox->setChecked(i < 3);  // 默认显示前3个
        checkbox->setStyleSheet(QString("color: %1;").arg(colors[i]));
        
        QLabel* valueLabel = new QLabel("--", this);
        valueLabel->setStyleSheet("color: #E0E0E0;");
        
        maLayout->addWidget(checkbox, i, 0);
        maLayout->addWidget(valueLabel, i, 1);
        
        d->checkboxes[indicators[i]] = checkbox;
        d->valueLabels[indicators[i]] = valueLabel;
        
        connect(checkbox, &QCheckBox::toggled, this, [this, indicator = indicators[i]](bool checked) {
            emit indicatorToggled(indicator, checked);
        });
    }
    
    layout->addLayout(maLayout);
    layout->addStretch();
}

void TechnicalIndicatorPanel::setIndicatorData(const QString& name, const QMap<QString, double>& data)
{
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (d->valueLabels.contains(it.key())) {
            d->valueLabels[it.key()]->setText(QString::number(it.value(), 'f', 2));
        }
    }
}

void TechnicalIndicatorPanel::clearData()
{
    for (auto label : d->valueLabels) {
        label->setText("--");
    }
}

// ========== TradingPanel 实现 ==========

struct TradingPanel::Impl {
    QString instrumentId;
    QDoubleSpinBox* priceSpinBox = nullptr;
    QSpinBox* volumeSpinBox = nullptr;
    QLabel* availableLabel = nullptr;
    QPushButton* buyBtn = nullptr;
    QPushButton* sellBtn = nullptr;
};

TradingPanel::TradingPanel(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

TradingPanel::~TradingPanel() = default;

void TradingPanel::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(10);
    
    // 价格输入
    QHBoxLayout* priceLayout = new QHBoxLayout();
    QLabel* priceLabel = new QLabel("价格:", this);
    priceLabel->setStyleSheet("color: #E0E0E0;");
    
    d->priceSpinBox = new QDoubleSpinBox(this);
    d->priceSpinBox->setDecimals(2);
    d->priceSpinBox->setRange(0, 999999);
    d->priceSpinBox->setSingleStep(1);
    d->priceSpinBox->setStyleSheet(R"(
        QDoubleSpinBox {
            background: #2A2A3E;
            border: 1px solid #3A3A4E;
            border-radius: 4px;
            padding: 5px;
            color: #E0E0E0;
        }
    )");
    
    priceLayout->addWidget(priceLabel);
    priceLayout->addWidget(d->priceSpinBox);
    layout->addLayout(priceLayout);
    
    // 数量输入
    QHBoxLayout* volumeLayout = new QHBoxLayout();
    QLabel* volumeLabel = new QLabel("数量:", this);
    volumeLabel->setStyleSheet("color: #E0E0E0;");
    
    d->volumeSpinBox = new QSpinBox(this);
    d->volumeSpinBox->setRange(1, 10000);
    d->volumeSpinBox->setValue(1);
    d->volumeSpinBox->setStyleSheet(R"(
        QSpinBox {
            background: #2A2A3E;
            border: 1px solid #3A3A4E;
            border-radius: 4px;
            padding: 5px;
            color: #E0E0E0;
        }
    )");
    
    volumeLayout->addWidget(volumeLabel);
    volumeLayout->addWidget(d->volumeSpinBox);
    layout->addLayout(volumeLayout);
    
    // 可用资金
    d->availableLabel = new QLabel("可用: --", this);
    d->availableLabel->setStyleSheet("color: #E0E0E0;");
    layout->addWidget(d->availableLabel);
    
    // 买卖按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    d->buyBtn = new QPushButton("买入", this);
    d->buyBtn->setStyleSheet(R"(
        QPushButton {
            background: #26A69A;
            border: none;
            border-radius: 4px;
            padding: 10px;
            color: white;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #2BBBAD;
        }
    )");
    
    d->sellBtn = new QPushButton("卖出", this);
    d->sellBtn->setStyleSheet(R"(
        QPushButton {
            background: #EF5350;
            border: none;
            border-radius: 4px;
            padding: 10px;
            color: white;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #F44336;
        }
    )");
    
    btnLayout->addWidget(d->buyBtn);
    btnLayout->addWidget(d->sellBtn);
    layout->addLayout(btnLayout);
    
    // 连接信号
    connect(d->buyBtn, &QPushButton::clicked, this, &TradingPanel::buyClicked);
    connect(d->sellBtn, &QPushButton::clicked, this, &TradingPanel::sellClicked);
}

void TradingPanel::setInstrument(const QString& instrumentId)
{
    d->instrumentId = instrumentId;
}

void TradingPanel::setPrice(double price)
{
    d->priceSpinBox->setValue(price);
}

void TradingPanel::setAvailable(double available)
{
    d->availableLabel->setText(QString("可用: %1").arg(available, 0, 'f', 2));
}

// ========== RealtimeQuoteWidget 实现 ==========

struct RealtimeQuoteWidget::Impl {
    QString instrumentId;
    QLabel* priceLabel = nullptr;
    QLabel* changeLabel = nullptr;
    QLabel* highLabel = nullptr;
    QLabel* lowLabel = nullptr;
    QLabel* volumeLabel = nullptr;
    double lastPrice = 0;
    double preClosePrice = 0;
};

RealtimeQuoteWidget::RealtimeQuoteWidget(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

RealtimeQuoteWidget::~RealtimeQuoteWidget() = default;

void RealtimeQuoteWidget::setupUI()
{
    QGridLayout* layout = new QGridLayout(this);
    layout->setSpacing(5);
    
    // 最新价
    QLabel* priceTitle = new QLabel("最新价:", this);
    priceTitle->setStyleSheet("color: #9E9E9E;");
    
    d->priceLabel = new QLabel("--", this);
    d->priceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #E0E0E0;");
    
    layout->addWidget(priceTitle, 0, 0);
    layout->addWidget(d->priceLabel, 0, 1);
    
    // 涨跌
    QLabel* changeTitle = new QLabel("涨跌:", this);
    changeTitle->setStyleSheet("color: #9E9E9E;");
    
    d->changeLabel = new QLabel("--", this);
    d->changeLabel->setStyleSheet("font-size: 14px; color: #E0E0E0;");
    
    layout->addWidget(changeTitle, 1, 0);
    layout->addWidget(d->changeLabel, 1, 1);
    
    // 最高价
    QLabel* highTitle = new QLabel("最高:", this);
    highTitle->setStyleSheet("color: #9E9E9E;");
    
    d->highLabel = new QLabel("--", this);
    d->highLabel->setStyleSheet("color: #26A69A;");
    
    layout->addWidget(highTitle, 2, 0);
    layout->addWidget(d->highLabel, 2, 1);
    
    // 最低价
    QLabel* lowTitle = new QLabel("最低:", this);
    lowTitle->setStyleSheet("color: #9E9E9E;");
    
    d->lowLabel = new QLabel("--", this);
    d->lowLabel->setStyleSheet("color: #EF5350;");
    
    layout->addWidget(lowTitle, 3, 0);
    layout->addWidget(d->lowLabel, 3, 1);
    
    // 成交量
    QLabel* volumeTitle = new QLabel("成交量:", this);
    volumeTitle->setStyleSheet("color: #9E9E9E;");
    
    d->volumeLabel = new QLabel("--", this);
    d->volumeLabel->setStyleSheet("color: #E0E0E0;");
    
    layout->addWidget(volumeTitle, 4, 0);
    layout->addWidget(d->volumeLabel, 4, 1);
}

void RealtimeQuoteWidget::updateQuote(const MarketData& quote)
{
    d->lastPrice = quote.lastPrice;
    d->preClosePrice = quote.openPrice;  // 使用开盘价作为昨收价
    
    // 更新价格
    d->priceLabel->setText(QString::number(quote.lastPrice, 'f', 2));
    
    // 更新涨跌
    double change = quote.lastPrice - quote.openPrice;
    double changePercent = quote.openPrice > 0 ? change / quote.openPrice * 100 : 0;
    
    QString changeText = QString("%1 (%2%)")
        .arg(change >= 0 ? "+" : "")
        .arg(change, 0, 'f', 2)
        .arg(changePercent, 0, 'f', 2);
    
    d->changeLabel->setText(changeText);
    
    // 更新颜色
    if (change > 0) {
        d->priceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #26A69A;");
        d->changeLabel->setStyleSheet("font-size: 14px; color: #26A69A;");
    } else if (change < 0) {
        d->priceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #EF5350;");
        d->changeLabel->setStyleSheet("font-size: 14px; color: #EF5350;");
    } else {
        d->priceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #E0E0E0;");
        d->changeLabel->setStyleSheet("font-size: 14px; color: #E0E0E0;");
    }
    
    // 更新最高最低
    d->highLabel->setText(QString::number(quote.highestPrice, 'f', 2));
    d->lowLabel->setText(QString::number(quote.lowestPrice, 'f', 2));
    
    // 更新成交量
    d->volumeLabel->setText(QString::number(quote.volume));
}

void RealtimeQuoteWidget::setInstrument(const QString& instrumentId)
{
    d->instrumentId = instrumentId;
}
