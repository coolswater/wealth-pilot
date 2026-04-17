/**
 * @file FuturesKLinePage.cpp
 * @brief 期货K线页面实现 - 专业级K线图表和技术分析
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
#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QButtonGroup>
#include <QFrame>
#include <QScrollArea>

#include "core/navigation/PageNavigator.h"
#include "core/di/ServiceLocator.h"
#include "core/cache/CacheManager.h"
#include "plugins/IAIPlugin.h"
#include "utils/Logger.h"
#include "utils/TechnicalIndicators.h"
#include "ctp/service/CTPService.h"

// ========== FuturesKLinePage::Impl ==========

struct FuturesKLinePage::Impl {
    // 合约信息
    QString instrumentId;
    QString instrumentName;
    KLinePeriod currentPeriod = KLinePeriod::Minute15;
    AdjustmentType currentAdjustment = AdjustmentType::None;

    // UI组件
    ChartToolBar* toolBar = nullptr;
    KLineChart* klineChart = nullptr;
    MarketDepthWidget* depthWidget = nullptr;
    TickTableView* tickTable = nullptr;
    ChartStatusBar* statusBar = nullptr;
    QSplitter* mainSplitter = nullptr;

    // K线数据
    QVector<KLineData> klineData;
    
    // 当前行情快照
    CTP::MarketData currentQuote;
    bool hasQuoteData = false;

    // CTP服务（直接引用）
    CTP::CTPService* ctpService = nullptr;

    // 服务插件
    ICTPPlugin* ctpPlugin = nullptr;
    IAIPlugin* aiPlugin = nullptr;

    // K线合成相关
    KLineData currentBar;           // 当前未闭合的K线
    bool hasOpenBar = false;        // 是否有未闭合的K线
    QDateTime barOpenTime;          // 当前K线开盘时间
    
    // 技术指标状态
    QMap<QString, bool> indicatorStates = {
        {"MA5", true},
        {"MA10", true},
        {"MA20", true},
        {"MA30", false},
        {"MA60", false},
        {"MACD", false},
        {"RSI", false},
        {"KDJ", false},
        {"BOLL", false},
        {"VOL", true}
    };
    
    // 缓存键前缀
    QString cacheKeyPrefix() const {
        return QString("kline_%1_%2_").arg(instrumentId).arg(static_cast<int>(currentPeriod));
    }
};

// ========== FuturesKLinePage 构造和析构 ==========

FuturesKLinePage::FuturesKLinePage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
    LOG_DEBUG("FuturesKLinePage created");
}

FuturesKLinePage::~FuturesKLinePage()
{
    LOG_DEBUG("FuturesKLinePage destroyed");
}

// ========== BasePage 接口实现 ==========

void FuturesKLinePage::initializePage()
{
    // 页面初始化时不自动加载数据，等待合约设置
}

void FuturesKLinePage::refresh()
{
    if (!d->instrumentId.isEmpty()) {
        requestKLineFromCache();
    }
}

void FuturesKLinePage::setInstrument(const QString& instrumentId, const QString& instrumentName)
{
    d->instrumentId = instrumentId;
    d->instrumentName = instrumentName;

    if (d->depthWidget) {
        d->depthWidget->setInstrument(instrumentId, instrumentName);
    }

    updateWindowTitle();
    
    // 订阅行情
    subscribeMarketData();
    
    // 尝试从缓存加载历史K线
    requestKLineFromCache();
}

QString FuturesKLinePage::instrument() const
{
    return d->instrumentId;
}

void FuturesKLinePage::setPeriod(KLinePeriod period)
{
    if (d->currentPeriod != period) {
        d->currentPeriod = period;
        if (d->toolBar) {
            d->toolBar->setCurrentPeriod(period);
        }
        
        // 重置K线合成状态
        d->hasOpenBar = false;
        d->klineData.clear();
        
        // 重新加载K线数据
        requestKLineFromCache();
    }
}

KLinePeriod FuturesKLinePage::period() const
{
    return d->currentPeriod;
}

void FuturesKLinePage::setIndicatorEnabled(const QString& indicator, bool enabled)
{
    d->indicatorStates[indicator] = enabled;
    calculateIndicators();
}

bool FuturesKLinePage::isIndicatorEnabled(const QString& indicator) const
{
    return d->indicatorStates.value(indicator, false);
}

// ========== 私有方法 ==========

void FuturesKLinePage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ========== 顶部工具栏 ==========
    d->toolBar = new ChartToolBar(this);
    mainLayout->addWidget(d->toolBar);

    // ========== 主内容区域（分割器） ==========
    d->mainSplitter = new QSplitter(Qt::Horizontal, this);
    d->mainSplitter->setHandleWidth(1);
    d->mainSplitter->setStyleSheet(
        "QSplitter::handle { background-color: rgba(255, 255, 255, 0.1); }"
        "QSplitter::handle:hover { background-color: #3B82F6; }"
    );

    // 左侧：K线图区域
    QWidget* chartContainer = new QWidget(d->mainSplitter);
    QVBoxLayout* chartLayout = new QVBoxLayout(chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(0);

    d->klineChart = new KLineChart(chartContainer);
    chartLayout->addWidget(d->klineChart);

    d->mainSplitter->addWidget(chartContainer);

    // 右侧：盘口信息 + 分笔成交
    QWidget* rightPanel = new QWidget(d->mainSplitter);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // 盘口信息组件
    d->depthWidget = new MarketDepthWidget(rightPanel);
    rightLayout->addWidget(d->depthWidget);

    // 分隔线
    QFrame* separator = new QFrame(rightPanel);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: rgba(255, 255, 255, 0.1);");
    separator->setFixedHeight(1);
    rightLayout->addWidget(separator);

    // 分笔成交表格
    d->tickTable = new TickTableView(rightPanel);
    rightLayout->addWidget(d->tickTable, 1);

    d->mainSplitter->addWidget(rightPanel);

    // 设置分割比例
    d->mainSplitter->setStretchFactor(0, 7);
    d->mainSplitter->setStretchFactor(1, 3);

    mainLayout->addWidget(d->mainSplitter, 1);

    // ========== 底部状态栏 ==========
    d->statusBar = new ChartStatusBar(this);
    mainLayout->addWidget(d->statusBar);
}

void FuturesKLinePage::setupConnections()
{
    // 工具栏信号
    connect(d->toolBar, &ChartToolBar::periodChanged,
            this, &FuturesKLinePage::onPeriodChanged);
    connect(d->toolBar, &ChartToolBar::adjustmentChanged,
            this, &FuturesKLinePage::onAdjustmentChanged);
    connect(d->toolBar, &ChartToolBar::indicatorToggled,
            this, &FuturesKLinePage::onIndicatorToggled);
    connect(d->toolBar, &ChartToolBar::drawToolSelected,
            this, &FuturesKLinePage::onDrawToolSelected);
    connect(d->toolBar, &ChartToolBar::chartTypeChanged,
            this, &FuturesKLinePage::onChartTypeChanged);

    // K线图信号
    connect(d->klineChart, &KLineChart::crosshairMoved,
            this, &FuturesKLinePage::onCrosshairMoved);

    // 获取 CTP 服务
    d->ctpService = ServiceLocator::instance().tryResolve<CTP::CTPService>();
    if (d->ctpService) {
        // 连接行情数据信号
        connect(d->ctpService, &CTP::CTPService::marketDataReceived,
                this, &FuturesKLinePage::onCtpMarketDataReceived,
                Qt::QueuedConnection);
        
        connect(d->ctpService, &CTP::CTPService::marketDataBatchReceived,
                this, [this](const QList<CTP::MarketData>& dataList) {
                    for (const auto& data : dataList) {
                        if (data.InstrumentID == d->instrumentId) {
                            onCtpMarketDataReceived(data);
                            break;
                        }
                    }
                }, Qt::QueuedConnection);
        
        LOG_DEBUG("CTPService connected to KLine page");
    } else {
        LOG_WARNING("CTPService not available");
    }

    // CTP 插件（兼容旧接口）
    d->ctpPlugin = ServiceLocator::instance().tryResolve<ICTPPlugin>();
    if (d->ctpPlugin) {
        connect(d->ctpPlugin, &ICTPPlugin::marketDataUpdated,
                this, &FuturesKLinePage::onMarketDataUpdated);
        LOG_DEBUG("CTP plugin connected to KLine page");
    }

    // AI插件
    d->aiPlugin = ServiceLocator::instance().tryResolve<IAIPlugin>();
}

void FuturesKLinePage::resizeEvent(QResizeEvent *event)
{
    BasePage::resizeEvent(event);

    if (width() < 1200) {
        d->mainSplitter->setSizes({static_cast<int>(width() * 0.65), static_cast<int>(width() * 0.35)});
    } else {
        d->mainSplitter->setSizes({static_cast<int>(width() * 0.70), static_cast<int>(width() * 0.30)});
    }
}

void FuturesKLinePage::subscribeMarketData()
{
    if (d->instrumentId.isEmpty()) {
        return;
    }

    if (d->ctpService) {
        d->ctpService->subscribeMarketData({d->instrumentId});
        LOG_INFO(QString("Subscribed market data for: %1").arg(d->instrumentId));
    } else if (d->ctpPlugin) {
        d->ctpPlugin->subscribeMarketData({d->instrumentId});
        LOG_INFO(QString("Subscribed market data via plugin for: %1").arg(d->instrumentId));
    } else {
        LOG_WARNING("No CTP service available for subscription");
    }
}

void FuturesKLinePage::requestKLineFromCache()
{
    if (d->instrumentId.isEmpty()) {
        return;
    }

    // 安全检查 CacheManager
    auto* cacheManager = CacheManager::instance();
    if (!cacheManager) {
        LOG_WARNING("CacheManager not available");
        return;
    }

    // 尝试从缓存加载历史K线
    QString cacheKey = d->cacheKeyPrefix() + "history";
    QVariant cached = cacheManager->get(cacheKey);
    
    if (cached.isValid()) {
        QVector<KLineData> data = cached.value<QVector<KLineData>>();
        if (!data.isEmpty()) {
            d->klineData = data;
            if (d->klineChart) {
                d->klineChart->setData(data);
            }
            calculateIndicators();
            LOG_INFO(QString("Loaded %1 K-Line bars from cache for %2")
                .arg(data.size()).arg(d->instrumentId));
            return;
        }
    }

    // 缓存中没有数据，显示空图表
    LOG_INFO(QString("No cached K-Line data for %1, waiting for real-time data").arg(d->instrumentId));
    
    // 清空图表
    if (d->klineChart) {
        d->klineChart->clearData();
    }
    d->klineData.clear();
}

void FuturesKLinePage::updateKLineFromTick(const CTP::MarketData& tick)
{
    // 安全检查：确保价格有效
    if (tick.lastPrice <= 0) {
        return;
    }
    
    // 根据周期计算K线时间边界
    QDateTime tickTime = tick.UpdateTime;
    if (!tickTime.isValid()) {
        tickTime = QDateTime::currentDateTime();
    }
    
    QDateTime barTime = calculateBarTime(tickTime, d->currentPeriod);
    
    if (!d->hasOpenBar || d->barOpenTime != barTime) {
        // 新K线周期开始
        if (d->hasOpenBar) {
            // 保存上一根K线
            d->klineData.append(d->currentBar);
            
            // 更新图表
            if (d->klineChart) {
                d->klineChart->addData(d->currentBar);
            }
        }
        
        // 开新K线
        d->currentBar = KLineData();
        d->currentBar.time = barTime;
        d->currentBar.open = tick.lastPrice;
        d->currentBar.high = tick.lastPrice;
        d->currentBar.low = tick.lastPrice;
        d->currentBar.close = tick.lastPrice;
        d->currentBar.volume = tick.Volume;
        d->currentBar.turnover = tick.Turnover;
        d->currentBar.openInterest = tick.OpenInterest;
        
        d->barOpenTime = barTime;
        d->hasOpenBar = true;
    } else {
        // 更新当前K线
        d->currentBar.close = tick.lastPrice;
        d->currentBar.high = qMax(d->currentBar.high, tick.lastPrice);
        d->currentBar.low = qMin(d->currentBar.low, tick.lastPrice);
        
        // 累加成交量和持仓量（注意：CTP返回的是累计值，需要计算增量）
        // 这里简化处理，实际需要根据具体需求调整
        d->currentBar.openInterest = tick.OpenInterest;
        
        // 更新图表最后一条
        if (d->klineChart) {
            d->klineChart->updateLastData(d->currentBar);
        }
    }
    
    // 定期计算指标
    static int indicatorUpdateCounter = 0;
    if (++indicatorUpdateCounter % 10 == 0) {  // 每10个tick更新一次指标
        calculateIndicators();
    }
}

QDateTime FuturesKLinePage::calculateBarTime(const QDateTime& tickTime, KLinePeriod period)
{
    QTime time = tickTime.time();
    QDate date = tickTime.date();
    
    switch (period) {
        case KLinePeriod::Minute1: {
            // 1分钟K线
            return QDateTime(date, QTime(time.hour(), time.minute(), 0));
        }
        case KLinePeriod::Minute5: {
            // 5分钟K线
            int minute = (time.minute() / 5) * 5;
            return QDateTime(date, QTime(time.hour(), minute, 0));
        }
        case KLinePeriod::Minute15: {
            // 15分钟K线
            int minute = (time.minute() / 15) * 15;
            return QDateTime(date, QTime(time.hour(), minute, 0));
        }
        case KLinePeriod::Minute30: {
            // 30分钟K线
            int minute = (time.minute() / 30) * 30;
            return QDateTime(date, QTime(time.hour(), minute, 0));
        }
        case KLinePeriod::Hour1: {
            // 60分钟K线
            return QDateTime(date, QTime(time.hour(), 0, 0));
        }
        case KLinePeriod::Day1: {
            // 日线
            return QDateTime(date, QTime(0, 0, 0));
        }
        case KLinePeriod::Week1: {
            // 周线
            int dayOfWeek = date.dayOfWeek();
            QDate weekStart = date.addDays(1 - dayOfWeek);
            return QDateTime(weekStart, QTime(0, 0, 0));
        }
        case KLinePeriod::Month1: {
            // 月线
            QDate monthStart(date.year(), date.month(), 1);
            return QDateTime(monthStart, QTime(0, 0, 0));
        }
        default:
            return tickTime;
    }
}

void FuturesKLinePage::calculateIndicators()
{
    if (d->klineData.isEmpty() || !d->klineChart) {
        return;
    }

    // 提取收盘价
    QVector<double> closes;
    for (const auto& kline : d->klineData) {
        closes.append(kline.close);
    }

    // 清空现有指标
    d->klineChart->clearIndicators();

    // 计算均线
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

    if (d->indicatorStates["MA30"]) {
        QVector<double> ma30 = TechnicalIndicators::SMA(closes, 30);
        d->klineChart->addIndicator("MA30", ma30, QColor("#9B59B6"));
    }

    if (d->indicatorStates["MA60"]) {
        QVector<double> ma60 = TechnicalIndicators::SMA(closes, 60);
        d->klineChart->addIndicator("MA60", ma60, QColor("#3498DB"));
    }

    // MACD
    if (d->indicatorStates["MACD"]) {
        auto macd = TechnicalIndicators::MACD(closes, 12, 26, 9);
        d->klineChart->addIndicator("MACD_DIF", macd.values["DIF"], QColor("#FFD700"));
        d->klineChart->addIndicator("MACD_DEA", macd.values["DEA"], QColor("#00CED1"));
    }

    // RSI
    if (d->indicatorStates["RSI"]) {
        QVector<double> rsi = TechnicalIndicators::RSI(closes, 14);
        d->klineChart->addIndicator("RSI", rsi, QColor("#9B59B6"));
    }

    // KDJ
    if (d->indicatorStates["KDJ"]) {
        QVector<double> highs, lows;
        for (const auto& kline : d->klineData) {
            highs.append(kline.high);
            lows.append(kline.low);
        }
        auto kdj = TechnicalIndicators::KDJ(highs, lows, closes, 9, 3, 3);
        d->klineChart->addIndicator("KDJ_K", kdj.values["K"], QColor("#FFD700"));
        d->klineChart->addIndicator("KDJ_D", kdj.values["D"], QColor("#00CED1"));
    }

    // BOLL
    if (d->indicatorStates["BOLL"]) {
        auto boll = TechnicalIndicators::BollingerBands(closes, 20, 2);
        d->klineChart->addIndicator("BOLL_UPPER", boll.values["upper"], QColor("#FF6B6B"));
        d->klineChart->addIndicator("BOLL_MIDDLE", boll.values["middle"], QColor("#FFD700"));
        d->klineChart->addIndicator("BOLL_LOWER", boll.values["lower"], QColor("#00CED1"));
    }
}

void FuturesKLinePage::updateQuoteDisplay(const MarketData& quote)
{
    if (d->depthWidget) {
        d->depthWidget->updateQuote(quote);
    }
}

void FuturesKLinePage::updateQuoteDisplayFromCtp(const CTP::MarketData& quote)
{
    if (d->depthWidget) {
        // 转换为 MarketData 结构
        MarketData displayQuote;
        displayQuote.instrumentId = quote.InstrumentID;
        displayQuote.lastPrice = quote.lastPrice;
        displayQuote.bidPrice1 = quote.BidPrice1;
        displayQuote.bidVolume1 = quote.BidVolume1;
        displayQuote.askPrice1 = quote.AskPrice1;
        displayQuote.askVolume1 = quote.AskVolume1;
        displayQuote.openPrice = quote.OpenPrice;
        displayQuote.highestPrice = quote.HighestPrice;
        displayQuote.lowestPrice = quote.LowestPrice;
        displayQuote.volume = quote.Volume;
        displayQuote.openInterest = quote.OpenInterest;
        displayQuote.preSettlementPrice = quote.preSettlementPrice;
        displayQuote.upperLimitPrice = quote.UpperLimitPrice;
        displayQuote.lowerLimitPrice = quote.LowerLimitPrice;
        
        d->depthWidget->updateQuote(displayQuote);
    }
}

void FuturesKLinePage::updateStatusBar()
{
    if (d->statusBar) {
        bool connected = d->ctpService && d->ctpService->isLoggedIn();
        d->statusBar->setConnectionStatus(
            connected ? "CTP 已连接" : "CTP 未连接",
            connected ? QColor("#10B981") : QColor("#EF4444")
        );
    }
}

void FuturesKLinePage::updateWindowTitle()
{
    QString title = d->instrumentName.isEmpty() ? d->instrumentId : d->instrumentName;
    emit pageTitleChanged(title);
}

// ========== 槽函数 ==========

void FuturesKLinePage::onCtpMarketDataReceived(const CTP::MarketData& data)
{
    // 安全检查
    if (data.InstrumentID.isEmpty() || data.InstrumentID != d->instrumentId) {
        return;
    }
    
    // 检查价格有效性
    if (data.lastPrice <= 0) {
        return;
    }

    // 更新盘口显示
    updateQuoteDisplayFromCtp(data);

    // 合成K线
    updateKLineFromTick(data);

    // 添加分笔成交记录
    if (d->tickTable && data.lastPrice > 0) {
        QString timeStr = data.UpdateTime.isValid() ? 
            data.UpdateTime.toString("hh:mm:ss") : 
            QDateTime::currentDateTime().toString("hh:mm:ss");
        QString flag = data.BidVolume1 > data.AskVolume1 ? "买" : "卖";
        d->tickTable->addTick(timeStr, data.lastPrice, 
            qAbs(data.BidVolume1 - data.AskVolume1), flag);
    }

    // 更新状态栏
    updateStatusBar();
}

void FuturesKLinePage::onMarketDataUpdated(const MarketData& data)
{
    // 安全检查
    if (data.instrumentId.isEmpty() || data.instrumentId != d->instrumentId) {
        return;
    }
    
    // 检查价格有效性
    if (data.lastPrice <= 0) {
        return;
    }

    // 更新盘口显示
    updateQuoteDisplay(data);

    // 转换为 CTP::MarketData 格式进行K线合成
    CTP::MarketData ctpData;
    ctpData.InstrumentID = data.instrumentId;
    ctpData.ExchangeID = data.exchangeId;
    ctpData.lastPrice = data.lastPrice;
    ctpData.BidPrice1 = data.bidPrice1;
    ctpData.BidVolume1 = data.bidVolume1;
    ctpData.AskPrice1 = data.askPrice1;
    ctpData.AskVolume1 = data.askVolume1;
    ctpData.OpenPrice = data.openPrice;
    ctpData.HighestPrice = data.highestPrice;
    ctpData.LowestPrice = data.lowestPrice;
    ctpData.Volume = data.volume;
    ctpData.OpenInterest = data.openInterest;
    ctpData.preSettlementPrice = data.preSettlementPrice;
    ctpData.UpdateTime = data.updateTime;

    // 合成K线
    updateKLineFromTick(ctpData);

    // 更新状态栏
    updateStatusBar();
}

void FuturesKLinePage::onKLineDataReceived(const QVector<KLineData>& data)
{
    d->klineData = data;
    if (d->klineChart) {
        d->klineChart->setData(data);
    }
    calculateIndicators();
    
    // 缓存数据
    auto* cacheManager = CacheManager::instance();
    if (cacheManager) {
        QString cacheKey = d->cacheKeyPrefix() + "history";
        cacheManager->set(cacheKey, QVariant::fromValue(data), 3600);  // 缓存1小时
    }
}

void FuturesKLinePage::onTickReceived(const QString& time, double price, int volume, const QString& flag)
{
    if (d->tickTable) {
        d->tickTable->addTick(time, price, volume, flag);
    }
}

void FuturesKLinePage::onPeriodChanged(KLinePeriod period)
{
    setPeriod(period);
}

void FuturesKLinePage::onAdjustmentChanged(AdjustmentType type)
{
    d->currentAdjustment = type;
    // TODO: 实现复权计算
    calculateIndicators();
}

void FuturesKLinePage::onIndicatorToggled(const QString& indicator, bool enabled)
{
    setIndicatorEnabled(indicator, enabled);
}

void FuturesKLinePage::onDrawToolSelected(const QString& tool)
{
    LOG_INFO(QString("Draw tool selected: %1").arg(tool));
    // TODO: 实现画线工具
}

void FuturesKLinePage::onChartTypeChanged(const QString& type)
{
    LOG_INFO(QString("Chart type changed: %1").arg(type));
    // TODO: 切换K线/分时图
}

void FuturesKLinePage::onCrosshairMoved(const QDateTime& time, double price)
{
    // 找到对应的K线数据
    int index = -1;
    for (int i = 0; i < d->klineData.size(); ++i) {
        if (d->klineData[i].time == time) {
            index = i;
            break;
        }
    }

    QString info;
    qint64 volume = 0;
    
    if (index >= 0 && index < d->klineData.size()) {
        const KLineData& kline = d->klineData[index];
        info = QString("O:%1 H:%2 L:%3 C:%4 V:%5")
            .arg(kline.open, 0, 'f', 2)
            .arg(kline.high, 0, 'f', 2)
            .arg(kline.low, 0, 'f', 2)
            .arg(kline.close, 0, 'f', 2)
            .arg(kline.volume);
        volume = kline.volume;
    }

    if (d->statusBar) {
        d->statusBar->setCrosshairInfo(time, price, volume);
    }

    emit crosshairMoved(time, price, info);
}

void FuturesKLinePage::onPageActivated(const QVariantMap& params)
{
    LOG_INFO(QString("FuturesKLinePage activated with params: %1").arg(params.size()));

    if (params.contains("instrumentId")) {
        QString instrumentId = params["instrumentId"].toString();
        QString instrumentName = params.value("instrumentName", instrumentId).toString();
        setInstrument(instrumentId, instrumentName);
    }

    updateStatusBar();
}

// ========== ChartToolBar 实现 ==========

struct ChartToolBar::Impl {
    QButtonGroup* periodGroup = nullptr;
    QComboBox* periodCombo = nullptr;
    QToolButton* adjustmentBtn = nullptr;
    QToolButton* indicatorBtn = nullptr;
    QToolButton* drawToolBtn = nullptr;
    QToolButton* chartTypeBtn = nullptr;

    KLinePeriod currentPeriod = KLinePeriod::Minute15;
    AdjustmentType currentAdjustment = AdjustmentType::None;
};

ChartToolBar::ChartToolBar(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

ChartToolBar::~ChartToolBar() = default;

void ChartToolBar::setupUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(8);

    // 周期选择下拉框
    d->periodCombo = new QComboBox(this);
    d->periodCombo->addItem("分时", static_cast<int>(KLinePeriod::Timeline));
    d->periodCombo->addItem("1分钟", static_cast<int>(KLinePeriod::Minute1));
    d->periodCombo->addItem("5分钟", static_cast<int>(KLinePeriod::Minute5));
    d->periodCombo->addItem("15分钟", static_cast<int>(KLinePeriod::Minute15));
    d->periodCombo->addItem("30分钟", static_cast<int>(KLinePeriod::Minute30));
    d->periodCombo->addItem("60分钟", static_cast<int>(KLinePeriod::Hour1));
    d->periodCombo->addItem("日线", static_cast<int>(KLinePeriod::Day1));
    d->periodCombo->addItem("周线", static_cast<int>(KLinePeriod::Week1));
    d->periodCombo->addItem("月线", static_cast<int>(KLinePeriod::Month1));
    d->periodCombo->addItem("更多...", static_cast<int>(KLinePeriod::Custom));
    d->periodCombo->setCurrentIndex(3);
    d->periodCombo->setMinimumWidth(80);

    layout->addWidget(new QLabel("周期:"));
    layout->addWidget(d->periodCombo);

    layout->addWidget(createSeparator());

    // 复权按钮
    d->adjustmentBtn = new QToolButton(this);
    d->adjustmentBtn->setText("不复权");
    d->adjustmentBtn->setPopupMode(QToolButton::MenuButtonPopup);
    d->adjustmentBtn->setToolTip("复权设置");

    QMenu* adjMenu = new QMenu(d->adjustmentBtn);
    adjMenu->addAction("不复权", this, [this]() {
        d->currentAdjustment = AdjustmentType::None;
        d->adjustmentBtn->setText("不复权");
        emit adjustmentChanged(AdjustmentType::None);
    });
    adjMenu->addAction("前复权", this, [this]() {
        d->currentAdjustment = AdjustmentType::Front;
        d->adjustmentBtn->setText("前复权");
        emit adjustmentChanged(AdjustmentType::Front);
    });
    adjMenu->addAction("后复权", this, [this]() {
        d->currentAdjustment = AdjustmentType::Back;
        d->adjustmentBtn->setText("后复权");
        emit adjustmentChanged(AdjustmentType::Back);
    });
    d->adjustmentBtn->setMenu(adjMenu);
    layout->addWidget(d->adjustmentBtn);

    layout->addWidget(createSeparator());

    // 画线工具按钮
    d->drawToolBtn = new QToolButton(this);
    d->drawToolBtn->setText("画线");
    d->drawToolBtn->setPopupMode(QToolButton::MenuButtonPopup);
    d->drawToolBtn->setToolTip("画线工具");

    QMenu* drawMenu = new QMenu(d->drawToolBtn);
    drawMenu->addAction("趋势线", this, [this]() { emit drawToolSelected("trend"); });
    drawMenu->addAction("水平线", this, [this]() { emit drawToolSelected("horizontal"); });
    drawMenu->addAction("射线", this, [this]() { emit drawToolSelected("ray"); });
    drawMenu->addAction("矩形", this, [this]() { emit drawToolSelected("rectangle"); });
    drawMenu->addAction("斐波那契", this, [this]() { emit drawToolSelected("fibonacci"); });
    drawMenu->addSeparator();
    drawMenu->addAction("清除所有", this, [this]() { emit drawToolSelected("clear"); });
    d->drawToolBtn->setMenu(drawMenu);
    layout->addWidget(d->drawToolBtn);

    layout->addWidget(createSeparator());

    // 指标按钮
    d->indicatorBtn = new QToolButton(this);
    d->indicatorBtn->setText("指标");
    d->indicatorBtn->setPopupMode(QToolButton::MenuButtonPopup);
    d->indicatorBtn->setToolTip("技术指标");

    QMenu* indicatorMenu = new QMenu(d->indicatorBtn);
    indicatorMenu->addAction("MA均线", this, [this]() { emit indicatorToggled("MA", true); });
    indicatorMenu->addAction("MACD", this, [this]() { emit indicatorToggled("MACD", true); });
    indicatorMenu->addAction("RSI", this, [this]() { emit indicatorToggled("RSI", true); });
    indicatorMenu->addAction("KDJ", this, [this]() { emit indicatorToggled("KDJ", true); });
    indicatorMenu->addAction("BOLL布林", this, [this]() { emit indicatorToggled("BOLL", true); });
    indicatorMenu->addSeparator();
    indicatorMenu->addAction("指标管理...", this, [this]() { emit indicatorToggled("manage", true); });
    d->indicatorBtn->setMenu(indicatorMenu);
    layout->addWidget(d->indicatorBtn);

    layout->addWidget(createSeparator());

    // 图表类型按钮
    d->chartTypeBtn = new QToolButton(this);
    d->chartTypeBtn->setText("K线");
    d->chartTypeBtn->setPopupMode(QToolButton::MenuButtonPopup);
    d->chartTypeBtn->setToolTip("图表类型");

    QMenu* chartMenu = new QMenu(d->chartTypeBtn);
    chartMenu->addAction("K线图", this, [this]() {
        d->chartTypeBtn->setText("K线");
        emit chartTypeChanged("candle");
    });
    chartMenu->addAction("分时图", this, [this]() {
        d->chartTypeBtn->setText("分时");
        emit chartTypeChanged("timeline");
    });
    chartMenu->addAction("美国线", this, [this]() {
        d->chartTypeBtn->setText("美线");
        emit chartTypeChanged("ohlc");
    });
    d->chartTypeBtn->setMenu(chartMenu);
    layout->addWidget(d->chartTypeBtn);

    layout->addStretch();

    // 连接周期选择信号
    connect(d->periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        int periodValue = d->periodCombo->itemData(index).toInt();
        d->currentPeriod = static_cast<KLinePeriod>(periodValue);
        emit periodChanged(d->currentPeriod);
    });
}

QFrame* ChartToolBar::createSeparator()
{
    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet("background-color: rgba(255, 255, 255, 0.2);");
    sep->setFixedWidth(1);
    return sep;
}

void ChartToolBar::setCurrentPeriod(KLinePeriod period)
{
    d->currentPeriod = period;
    int index = d->periodCombo->findData(static_cast<int>(period));
    if (index >= 0) {
        d->periodCombo->setCurrentIndex(index);
    }
}

void ChartToolBar::setCurrentAdjustment(AdjustmentType type)
{
    d->currentAdjustment = type;
    switch (type) {
        case AdjustmentType::None: d->adjustmentBtn->setText("不复权"); break;
        case AdjustmentType::Front: d->adjustmentBtn->setText("前复权"); break;
        case AdjustmentType::Back: d->adjustmentBtn->setText("后复权"); break;
    }
}

// ========== MarketDepthWidget 实现 ==========

struct MarketDepthWidget::Impl {
    QString instrumentId;
    QString instrumentName;

    QLabel* priceLabel = nullptr;
    QLabel* changeLabel = nullptr;
    QLabel* changePercentLabel = nullptr;

    QLabel* bidPrice1 = nullptr;
    QLabel* bidVolume1 = nullptr;
    QLabel* askPrice1 = nullptr;
    QLabel* askVolume1 = nullptr;
    QLabel* bidPrice2 = nullptr;
    QLabel* bidVolume2 = nullptr;
    QLabel* askPrice2 = nullptr;
    QLabel* askVolume2 = nullptr;

    QLabel* volumeLabel = nullptr;
    QLabel* turnoverLabel = nullptr;
    QLabel* openInterestLabel = nullptr;
    QLabel* highLabel = nullptr;
    QLabel* lowLabel = nullptr;
    QLabel* openLabel = nullptr;
    QLabel* preCloseLabel = nullptr;

    double lastPrice = 0.0;
    double preClosePrice = 0.0;
};

MarketDepthWidget::MarketDepthWidget(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

MarketDepthWidget::~MarketDepthWidget() = default;

void MarketDepthWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // 合约名称
    QLabel* title = new QLabel("盘口信息", this);
    title->setStyleSheet("font-size: 14px; font-weight: bold; color: #FFFFFF;");
    mainLayout->addWidget(title);

    // 最新价区域
    QFrame* priceFrame = new QFrame(this);
    priceFrame->setStyleSheet("background-color: rgba(255, 255, 255, 0.05); border-radius: 6px;");
    QVBoxLayout* priceLayout = new QVBoxLayout(priceFrame);
    priceLayout->setContentsMargins(10, 10, 10, 10);
    priceLayout->setSpacing(4);

    d->priceLabel = new QLabel("--", priceFrame);
    d->priceLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #FFFFFF;");
    d->priceLabel->setAlignment(Qt::AlignCenter);
    priceLayout->addWidget(d->priceLabel);

    QHBoxLayout* changeLayout = new QHBoxLayout();
    d->changeLabel = new QLabel("--", priceFrame);
    d->changeLabel->setStyleSheet("font-size: 14px;");
    d->changePercentLabel = new QLabel("--", priceFrame);
    d->changePercentLabel->setStyleSheet("font-size: 14px;");
    changeLayout->addStretch();
    changeLayout->addWidget(d->changeLabel);
    changeLayout->addSpacing(10);
    changeLayout->addWidget(d->changePercentLabel);
    changeLayout->addStretch();
    priceLayout->addLayout(changeLayout);

    mainLayout->addWidget(priceFrame);

    // 买卖盘口
    QGridLayout* depthGrid = new QGridLayout();
    depthGrid->setSpacing(4);

    depthGrid->addWidget(new QLabel("卖"), 0, 0);
    depthGrid->addWidget(new QLabel("价格"), 0, 1);
    depthGrid->addWidget(new QLabel("量"), 0, 2);
    depthGrid->addWidget(new QLabel("买"), 0, 3);
    depthGrid->addWidget(new QLabel("价格"), 0, 4);
    depthGrid->addWidget(new QLabel("量"), 0, 5);

    d->askVolume2 = new QLabel("--");
    d->askPrice2 = new QLabel("--");
    d->askPrice2->setStyleSheet("color: #10B981;");
    depthGrid->addWidget(new QLabel("卖2"), 1, 0);
    depthGrid->addWidget(d->askPrice2, 1, 1);
    depthGrid->addWidget(d->askVolume2, 1, 2);

    d->askVolume1 = new QLabel("--");
    d->askPrice1 = new QLabel("--");
    d->askPrice1->setStyleSheet("color: #10B981; font-weight: bold;");
    depthGrid->addWidget(new QLabel("卖1"), 2, 0);
    depthGrid->addWidget(d->askPrice1, 2, 1);
    depthGrid->addWidget(d->askVolume1, 2, 2);

    d->bidVolume1 = new QLabel("--");
    d->bidPrice1 = new QLabel("--");
    d->bidPrice1->setStyleSheet("color: #EF4444; font-weight: bold;");
    depthGrid->addWidget(new QLabel("买1"), 2, 3);
    depthGrid->addWidget(d->bidPrice1, 2, 4);
    depthGrid->addWidget(d->bidVolume1, 2, 5);

    d->bidVolume2 = new QLabel("--");
    d->bidPrice2 = new QLabel("--");
    d->bidPrice2->setStyleSheet("color: #EF4444;");
    depthGrid->addWidget(new QLabel("买2"), 1, 3);
    depthGrid->addWidget(d->bidPrice2, 1, 4);
    depthGrid->addWidget(d->bidVolume2, 1, 5);

    mainLayout->addLayout(depthGrid);

    // 分隔线
    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background-color: rgba(255, 255, 255, 0.1);");
    mainLayout->addWidget(sep);

    // 统计信息
    QGridLayout* statsGrid = new QGridLayout();
    statsGrid->setSpacing(4);

    statsGrid->addWidget(new QLabel("成交量:"), 0, 0);
    d->volumeLabel = new QLabel("--");
    statsGrid->addWidget(d->volumeLabel, 0, 1);

    statsGrid->addWidget(new QLabel("持仓量:"), 0, 2);
    d->openInterestLabel = new QLabel("--");
    statsGrid->addWidget(d->openInterestLabel, 0, 3);

    statsGrid->addWidget(new QLabel("最高:"), 1, 0);
    d->highLabel = new QLabel("--");
    d->highLabel->setStyleSheet("color: #10B981;");
    statsGrid->addWidget(d->highLabel, 1, 1);

    statsGrid->addWidget(new QLabel("最低:"), 1, 2);
    d->lowLabel = new QLabel("--");
    d->lowLabel->setStyleSheet("color: #EF4444;");
    statsGrid->addWidget(d->lowLabel, 1, 3);

    statsGrid->addWidget(new QLabel("开盘:"), 2, 0);
    d->openLabel = new QLabel("--");
    statsGrid->addWidget(d->openLabel, 2, 1);

    statsGrid->addWidget(new QLabel("昨收:"), 2, 2);
    d->preCloseLabel = new QLabel("--");
    statsGrid->addWidget(d->preCloseLabel, 2, 3);

    mainLayout->addLayout(statsGrid);
}

void MarketDepthWidget::updateQuote(const MarketData& quote)
{
    d->lastPrice = quote.lastPrice;
    d->preClosePrice = quote.preSettlementPrice;

    // 更新价格
    d->priceLabel->setText(QString::number(quote.lastPrice, 'f', 2));

    // 更新涨跌
    double change = quote.lastPrice - quote.preSettlementPrice;
    double changePercent = quote.preSettlementPrice > 0 ?
        change / quote.preSettlementPrice * 100 : 0;

    d->changeLabel->setText(QString("%1").arg(change, 0, 'f', 2));
    d->changePercentLabel->setText(QString("%1%").arg(changePercent, 0, 'f', 2));

    // 设置颜色
    QColor priceColor = change > 0 ? QColor("#EF4444") :
                        change < 0 ? QColor("#10B981") : QColor("#FFFFFF");
    d->priceLabel->setStyleSheet(QString("font-size: 28px; font-weight: bold; color: %1;").arg(priceColor.name()));
    d->changeLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(priceColor.name()));
    d->changePercentLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(priceColor.name()));

    // 更新买卖盘口
    d->bidPrice1->setText(QString::number(quote.bidPrice1, 'f', 2));
    d->bidVolume1->setText(QString::number(quote.bidVolume1));
    d->askPrice1->setText(QString::number(quote.askPrice1, 'f', 2));
    d->askVolume1->setText(QString::number(quote.askVolume1));

    // 更新统计信息
    d->volumeLabel->setText(QString::number(quote.volume));
    d->openInterestLabel->setText(QString::number(quote.openInterest, 'f', 0));
    d->highLabel->setText(QString::number(quote.highestPrice, 'f', 2));
    d->lowLabel->setText(QString::number(quote.lowestPrice, 'f', 2));
    d->openLabel->setText(QString::number(quote.openPrice, 'f', 2));
    d->preCloseLabel->setText(QString::number(quote.preSettlementPrice, 'f', 2));
}

void MarketDepthWidget::setInstrument(const QString& instrumentId, const QString& instrumentName)
{
    d->instrumentId = instrumentId;
    d->instrumentName = instrumentName;
}

void MarketDepthWidget::clear()
{
    d->priceLabel->setText("--");
    d->changeLabel->setText("--");
    d->changePercentLabel->setText("--");
    d->bidPrice1->setText("--");
    d->bidVolume1->setText("--");
    d->askPrice1->setText("--");
    d->askVolume1->setText("--");
}

// ========== TickTableView 实现 ==========

struct TickTableView::Impl {
    int maxRows = 500;
    bool autoScroll = true;
};

TickTableView::TickTableView(QWidget *parent)
    : QTableWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

TickTableView::~TickTableView() = default;

void TickTableView::setupUI()
{
    setColumnCount(5);
    setHorizontalHeaderLabels({"时间", "价格", "成交量", "仓差", "性质"});

    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    setColumnWidth(0, 80);
    setColumnWidth(1, 70);
    setColumnWidth(2, 60);
    setColumnWidth(3, 60);

    verticalHeader()->setVisible(false);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAlternatingRowColors(true);
    setShowGrid(false);

    setStyleSheet(
        "QTableWidget { background-color: transparent; border: none; }"
        "QTableWidget::item { padding: 4px; }"
        "QHeaderView::section { background-color: #0F1419; color: #9CA3AF; padding: 6px; border: none; }"
    );
}

void TickTableView::addTick(const QString& time, double price, int volume, const QString& flag)
{
    if (rowCount() >= d->maxRows) {
        removeRow(0);
    }

    int row = rowCount();
    insertRow(row);

    setItem(row, 0, new QTableWidgetItem(time));
    setItem(row, 1, new QTableWidgetItem(QString::number(price, 'f', 2)));
    setItem(row, 2, new QTableWidgetItem(QString::number(volume)));
    setItem(row, 3, new QTableWidgetItem("0"));
    setItem(row, 4, new QTableWidgetItem(flag));

    QColor color = flag.contains("买") ? QColor("#EF4444") : QColor("#10B981");
    item(row, 4)->setForeground(color);

    if (d->autoScroll) {
        scrollToBottom();
    }
}

void TickTableView::clearTicks()
{
    setRowCount(0);
}

void TickTableView::setMaxRows(int max)
{
    d->maxRows = max;
}

// ========== ChartStatusBar 实现 ==========

struct ChartStatusBar::Impl {
    QLabel* accountLabel = nullptr;
    QLabel* connectionLabel = nullptr;
    QLabel* coordinateLabel = nullptr;
};

ChartStatusBar::ChartStatusBar(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

ChartStatusBar::~ChartStatusBar() = default;

void ChartStatusBar::setupUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(20);

    setStyleSheet("background-color: #0F1419; border-top: 1px solid rgba(255, 255, 255, 0.05);");

    d->accountLabel = new QLabel("账户: -- | 可用: -- | 保证金: --", this);
    d->accountLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    layout->addWidget(d->accountLabel);

    layout->addStretch();

    d->connectionLabel = new QLabel("CTP: 未连接", this);
    d->connectionLabel->setStyleSheet("color: #EF4444; font-size: 12px;");
    layout->addWidget(d->connectionLabel);

    layout->addStretch();

    d->coordinateLabel = new QLabel("时间: -- | 价格: -- | 成交量: --", this);
    d->coordinateLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    layout->addWidget(d->coordinateLabel);
}

void ChartStatusBar::setAccountInfo(const QString& account, double available, double margin)
{
    d->accountLabel->setText(QString("账户: %1 | 可用: %2 | 保证金: %3")
        .arg(account)
        .arg(available, 0, 'f', 2)
        .arg(margin, 0, 'f', 2));
}

void ChartStatusBar::setConnectionStatus(const QString& status, const QColor& color)
{
    d->connectionLabel->setText(QString("CTP: %1").arg(status));
    d->connectionLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(color.name()));
}

void ChartStatusBar::setCoordinateInfo(const QString& info)
{
    d->coordinateLabel->setText(info);
}

void ChartStatusBar::setCrosshairInfo(const QDateTime& time, double price, double volume)
{
    d->coordinateLabel->setText(QString("时间: %1 | 价格: %2 | 成交量: %3")
        .arg(time.toString("MM-dd hh:mm"))
        .arg(price, 0, 'f', 2)
        .arg(volume));
}
