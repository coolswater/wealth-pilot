/**
 * @file FuturesKLinePage.cpp
 * @brief 期货K线页面实现 - 专业级K线图表和技术分析
 *
 * @details 实现功能：
 * - 多周期K线图表显示
 * - 技术指标叠加计算
 * - CTP实时行情对接
 * - K线实时合成
 * - 盘口深度显示
 * - 分笔成交记录
 */

#include "FuturesKLinePage.h"
#include "ui/components/ChartStyles.h"
#include "core/config/Tokens.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTimer>
#include <QResizeEvent>

#include "core/navigation/PageNavigator.h"
#include "core/di/ServiceLocator.h"
#include "core/cache/CacheManager.h"
#include "plugins/IAIPlugin.h"
#include "utils/Logger.h"
#include "utils/TechnicalIndicators.h"
#include "ctp/service/CTPService.h"

// ============================================================================
// PIMPL 实现
// ============================================================================

struct FuturesKLinePage::Impl {
    // ========== 合约信息 ==========
    QString instrumentId;                   ///< 合约代码
    QString instrumentName;                 ///< 合约名称
    KLinePeriod currentPeriod = KLinePeriod::Minute15;  ///< 当前周期
    AdjustmentType currentAdjustment = AdjustmentType::None;  ///< 复权类型

    // ========== UI组件 ==========
    ChartToolBar* toolBar = nullptr;        ///< 工具栏
    KLineChart* klineChart = nullptr;       ///< K线图
    MarketDepthWidget* depthWidget = nullptr;  ///< 盘口组件
    TickTableView* tickTable = nullptr;     ///< 分笔成交表
    ChartStatusBar* statusBar = nullptr;    ///< 状态栏
    QSplitter* mainSplitter = nullptr;      ///< 主分割器

    // ========== 数据 ==========
    QVector<KLineData> klineData;           ///< K线数据缓存
    CTP::MarketData currentQuote;           ///< 当前行情快照
    bool hasQuoteData = false;              ///< 是否有行情数据

    // ========== CTP服务 ==========
    CTP::CTPService* ctpService = nullptr;  ///< CTP服务（直接引用）
    ICTPPlugin* ctpPlugin = nullptr;        ///< CTP插件
    IAIPlugin* aiPlugin = nullptr;          ///< AI插件

    // ========== K线合成状态 ==========
    KLineData currentBar;                   ///< 当前未闭合的K线
    bool hasOpenBar = false;                ///< 是否有未闭合的K线
    QDateTime barOpenTime;                  ///< 当前K线开盘时间

    // ========== 指标状态 ==========
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

    // ========== 辅助方法 ==========

    /**
     * @brief 生成缓存键前缀
     * @return 缓存键前缀（格式：kline_合约_周期_）
     */
    QString cacheKeyPrefix() const {
        return QString("kline_%1_%2_").arg(instrumentId).arg(static_cast<int>(currentPeriod));
    }
};

// ============================================================================
// 构造与析构
// ============================================================================

FuturesKLinePage::FuturesKLinePage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
    setupServices();

    LOG_DEBUG("FuturesKLinePage created");
}

FuturesKLinePage::~FuturesKLinePage()
{
    LOG_DEBUG("FuturesKLinePage destroyed");
}

// ============================================================================
// BasePage 接口实现
// ============================================================================

void FuturesKLinePage::initializePage()
{
    // 页面初始化时不自动加载数据，等待合约设置
    LOG_DEBUG("FuturesKLinePage initialized");
}

void FuturesKLinePage::refresh()
{
    if (!d->instrumentId.isEmpty()) {
        requestKLineFromCache();
    }
}

// ============================================================================
// 公共接口
// ============================================================================

void FuturesKLinePage::setInstrument(const QString& instrumentId, const QString& instrumentName)
{
    d->instrumentId = instrumentId;
    d->instrumentName = instrumentName;

    // 更新盘口组件
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

        // 更新工具栏
        if (d->toolBar) {
            d->toolBar->setCurrentPeriod(period);
        }

        // 清空当前K线数据
        d->klineData.clear();
        d->hasOpenBar = false;

        // 重新加载数据
        if (!d->instrumentId.isEmpty()) {
            requestKLineFromCache();
        }
    }
}

KLinePeriod FuturesKLinePage::period() const
{
    return d->currentPeriod;
}

void FuturesKLinePage::setIndicatorEnabled(const QString& indicator, bool enabled)
{
    // 已弃用：现在使用 onMainIndicatorChanged/onSubIndicatorChanged
    // 保留此方法以保持向后兼容
    Q_UNUSED(indicator);
    Q_UNUSED(enabled);
}

bool FuturesKLinePage::isIndicatorEnabled(const QString& indicator) const
{
    // 已弃用：现在使用新的指标系统
    Q_UNUSED(indicator);
    return false;
}

void FuturesKLinePage::onPageActivated(const QVariantMap& params)
{
    LOG_INFO(QString("FuturesKLinePage activated with params: %1").arg(params.size()));

    // 从参数获取合约信息
    if (params.contains("instrumentId")) {
        QString instrumentId = params["instrumentId"].toString();
        QString instrumentName = params.value("instrumentName", instrumentId).toString();
        setInstrument(instrumentId, instrumentName);
    }

    updateStatusBar();
}

// ============================================================================
// 初始化方法
// ============================================================================

void FuturesKLinePage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ========== 工具栏 ==========
    d->toolBar = new ChartToolBar(this);
    mainLayout->addWidget(d->toolBar);

    // ========== 主内容区域 ==========
    d->mainSplitter = new QSplitter(Qt::Horizontal, this);
    d->mainSplitter->setHandleWidth(2);
    d->mainSplitter->setStyleSheet(ChartStyles::StyleSheets::splitterStyle());

    // 左侧：K线图
    d->klineChart = new KLineChart(d->mainSplitter);
    d->klineChart->setStyleSheet(ChartStyles::StyleSheets::klineChartStyle());

    // 右侧：盘口 + 分笔成交
    QWidget* rightPanel = new QWidget(d->mainSplitter);
    rightPanel->setStyleSheet("background-color: #0F1419;");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // 盘口组件
    d->depthWidget = new MarketDepthWidget(rightPanel);
    d->depthWidget->setMinimumHeight(220);
    d->depthWidget->setMaximumHeight(320);
    d->depthWidget->setStyleSheet(ChartStyles::StyleSheets::marketDepthStyle());
    rightLayout->addWidget(d->depthWidget);

    // 分笔成交表
    d->tickTable = new TickTableView(rightPanel);
    d->tickTable->setMaxRows(500);
    d->tickTable->setStyleSheet(ChartStyles::StyleSheets::tickTableStyle());
    rightLayout->addWidget(d->tickTable);

    // 添加到分割器
    d->mainSplitter->addWidget(d->klineChart);
    d->mainSplitter->addWidget(rightPanel);

    // 设置分割比例（使用配置的比例）
    int totalWidth = 1000;  // 默认总宽度
    int chartWidth = static_cast<int>(totalWidth * ChartStyles::Layout::ChartRatio);
    int depthWidth = totalWidth - chartWidth;
    d->mainSplitter->setSizes({chartWidth, depthWidth});

    mainLayout->addWidget(d->mainSplitter, 1);

    // ========== 状态栏 ==========
    d->statusBar = new ChartStatusBar(this);
    d->statusBar->setStyleSheet(ChartStyles::StyleSheets::chartStatusBarStyle());
    mainLayout->addWidget(d->statusBar);

    // 设置整体样式
    setStyleSheet(R"(
        FuturesKLinePage {
            background-color: #0F1419;
        }
    )");
}

void FuturesKLinePage::setupConnections()
{
    // ========== 工具栏信号 ==========
    connect(d->toolBar, &ChartToolBar::periodChanged,
            this, &FuturesKLinePage::onPeriodChanged);
    connect(d->toolBar, &ChartToolBar::adjustmentChanged,
            this, &FuturesKLinePage::onAdjustmentChanged);
    connect(d->toolBar, &ChartToolBar::mainIndicatorChanged,
            this, &FuturesKLinePage::onMainIndicatorChanged);
    connect(d->toolBar, &ChartToolBar::subIndicatorChanged,
            this, &FuturesKLinePage::onSubIndicatorChanged);
    connect(d->toolBar, &ChartToolBar::drawToolSelected,
            this, &FuturesKLinePage::onDrawToolSelected);
    connect(d->toolBar, &ChartToolBar::chartTypeChanged,
            this, &FuturesKLinePage::onChartTypeChanged);

    // ========== K线图信号 ==========
    connect(d->klineChart, &KLineChart::crosshairMoved,
            this, &FuturesKLinePage::onCrosshairMoved);

    // ========== 盘口信号 ==========
    connect(d->depthWidget, &MarketDepthWidget::buyClicked,
            this, [this](double price) {
        emit tradeRequested(d->instrumentId, "buy", price, 1);
    });
    connect(d->depthWidget, &MarketDepthWidget::sellClicked,
            this, [this](double price) {
        emit tradeRequested(d->instrumentId, "sell", price, 1);
    });
}

void FuturesKLinePage::setupServices()
{
    // 获取服务定位器
    auto& locator = ServiceLocator::instance();

    // 获取CTP服务
    d->ctpService = locator.tryResolve<CTP::CTPService>();
    if (d->ctpService) {
        // 连接CTP行情信号
        connect(d->ctpService, &CTP::CTPService::marketDataReceived,
                this, &FuturesKLinePage::onCtpMarketDataReceived);
        LOG_DEBUG("CTP service connected");
    }

    // 获取CTP插件
    d->ctpPlugin = locator.tryResolve<ICTPPlugin>();
    if (d->ctpPlugin) {
        connect(d->ctpPlugin, &ICTPPlugin::marketDataUpdated,
                this, &FuturesKLinePage::onMarketDataUpdated);
        LOG_DEBUG("CTP plugin connected");
    }

    // 获取AI插件
    d->aiPlugin = locator.tryResolve<IAIPlugin>();
}

// ============================================================================
// CTP 数据处理
// ============================================================================

void FuturesKLinePage::subscribeMarketData()
{
    if (d->instrumentId.isEmpty()) {
        return;
    }

    // 通过CTP服务订阅（需要列表）
    if (d->ctpService) {
        QList<QString> instruments;
        instruments.append(d->instrumentId);
        d->ctpService->subscribeMarketData(instruments);
        LOG_INFO(QString("Subscribed to market data: %1").arg(d->instrumentId));
    }

    // 通过CTP插件订阅（需要列表）
    if (d->ctpPlugin) {
        QStringList instruments;
        instruments.append(d->instrumentId);
        d->ctpPlugin->subscribeMarketData(instruments);
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

        // 累加成交量和持仓量
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

// ============================================================================
// 指标计算
// ============================================================================

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
        d->klineChart->addIndicator("MA5", ma5, QColor(Tokens::Colors::ChartYellow));
    }

    if (d->indicatorStates["MA10"]) {
        QVector<double> ma10 = TechnicalIndicators::SMA(closes, 10);
        d->klineChart->addIndicator("MA10", ma10, QColor(Tokens::Colors::ChartCyan));
    }

    if (d->indicatorStates["MA20"]) {
        QVector<double> ma20 = TechnicalIndicators::SMA(closes, 20);
        d->klineChart->addIndicator("MA20", ma20, QColor(Tokens::Colors::ChartRed));
    }

    if (d->indicatorStates["MA30"]) {
        QVector<double> ma30 = TechnicalIndicators::SMA(closes, 30);
        d->klineChart->addIndicator("MA30", ma30, QColor(Tokens::Colors::ChartPurple));
    }

    if (d->indicatorStates["MA60"]) {
        QVector<double> ma60 = TechnicalIndicators::SMA(closes, 60);
        d->klineChart->addIndicator("MA60", ma60, QColor(Tokens::Colors::ChartBlue));
    }

    // MACD
    if (d->indicatorStates["MACD"]) {
        auto macd = TechnicalIndicators::MACD(closes, 12, 26, 9);
        d->klineChart->addIndicator("MACD_DIF", macd.values["DIF"], QColor(Tokens::Colors::ChartYellow));
        d->klineChart->addIndicator("MACD_DEA", macd.values["DEA"], QColor(Tokens::Colors::ChartCyan));
    }

    // RSI
    if (d->indicatorStates["RSI"]) {
        QVector<double> rsi = TechnicalIndicators::RSI(closes, 14);
        d->klineChart->addIndicator("RSI", rsi, QColor(Tokens::Colors::ChartPurple));
    }

    // KDJ
    if (d->indicatorStates["KDJ"]) {
        QVector<double> highs, lows;
        for (const auto& kline : d->klineData) {
            highs.append(kline.high);
            lows.append(kline.low);
        }

        auto kdj = TechnicalIndicators::KDJ(highs, lows, closes, 9, 3, 3);
        d->klineChart->addIndicator("KDJ_K", kdj.values["K"], QColor(Tokens::Colors::ChartYellow));
        d->klineChart->addIndicator("KDJ_D", kdj.values["D"], QColor(Tokens::Colors::ChartCyan));
        d->klineChart->addIndicator("KDJ_J", kdj.values["J"], QColor(Tokens::Colors::ChartRed));
    }

    // BOLL
    if (d->indicatorStates["BOLL"]) {
        auto boll = TechnicalIndicators::BollingerBands(closes, 20, 2.0);
        d->klineChart->addIndicator("BOLL_UPPER", boll.values["Upper"], QColor(Tokens::Colors::ChartYellow));
        d->klineChart->addIndicator("BOLL_MIDDLE", boll.values["Middle"], QColor(Tokens::Colors::ChartCyan));
        d->klineChart->addIndicator("BOLL_LOWER", boll.values["Lower"], QColor(Tokens::Colors::ChartRed));
    }
}

// ============================================================================
// 显示更新
// ============================================================================

void FuturesKLinePage::updateQuoteDisplay(const MarketData& quote)
{
    if (d->depthWidget) {
        d->depthWidget->updateQuote(quote);
    }
}

void FuturesKLinePage::updateQuoteDisplayFromCtp(const CTP::MarketData& quote)
{
    if (!d->depthWidget) {
        return;
    }

    // 转换为 MarketData 格式
    MarketData displayQuote;
    displayQuote.instrumentId = quote.InstrumentID;
    displayQuote.exchangeId = quote.ExchangeID;
    displayQuote.lastPrice = quote.lastPrice;
    displayQuote.preSettlementPrice = quote.preSettlementPrice;
    displayQuote.openPrice = quote.OpenPrice;
    displayQuote.highestPrice = quote.HighestPrice;
    displayQuote.lowestPrice = quote.LowestPrice;
    displayQuote.volume = quote.Volume;
    displayQuote.openInterest = quote.OpenInterest;
    displayQuote.bidPrice1 = quote.BidPrice1;
    displayQuote.bidVolume1 = quote.BidVolume1;
    displayQuote.askPrice1 = quote.AskPrice1;
    displayQuote.askVolume1 = quote.AskVolume1;

    d->depthWidget->updateQuote(displayQuote);
}

void FuturesKLinePage::updateStatusBar()
{
    if (d->statusBar) {
        bool connected = d->ctpService && d->ctpService->isLoggedIn();
        d->statusBar->setConnectionStatus(
            connected ? "CTP 已连接" : "CTP 未连接",
            connected ? QColor(Tokens::Colors::Success) : QColor(Tokens::Colors::Danger)
        );
    }
}

void FuturesKLinePage::updateWindowTitle()
{
    QString title = d->instrumentName.isEmpty() ? d->instrumentId : d->instrumentName;
    emit pageTitleChanged(title);
}

// ============================================================================
// 槽函数
// ============================================================================

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
    
    // 实现复权计算
    if (d->klineChart && !d->klineData.isEmpty()) {
        // 期货不需要复权，但保留接口
        // 复权主要用于股票，期货合约换月时需要调整
        // 这里简单实现：根据复权类型调整价格
        
        if (type == AdjustmentType::None) {
            // 不复权，使用原始数据
            LOG_INFO("No adjustment");
        } else if (type == AdjustmentType::Front) {
            // 前复权：以最新价格为基准向前调整
            // 期货场景：保持原样（期货无需复权）
            LOG_INFO("Front adjustment (no change for futures)");
        } else if (type == AdjustmentType::Back) {
            // 后复权：以上市价格为基准向后调整
            // 期货场景：保持原样（期货无需复权）
            LOG_INFO("Back adjustment (no change for futures)");
        }
        
        LOG_INFO(QString("Adjustment type changed: %1").arg(static_cast<int>(type)));
    }
}

void FuturesKLinePage::onMainIndicatorChanged(const QString& indicator)
{
    // 转换为主图指标类型
    MainIndicator mainIndicator = MainIndicator::None;
    if (indicator == "MA") {
        mainIndicator = MainIndicator::MA;
    } else if (indicator == "EMA") {
        mainIndicator = MainIndicator::EMA;
    } else if (indicator == "BOLL") {
        mainIndicator = MainIndicator::BOLL;
    } else if (indicator == "DMI") {
        mainIndicator = MainIndicator::DMI;
    } else if (indicator == "ENE") {
        mainIndicator = MainIndicator::ENE;
    }
    
    if (d->klineChart) {
        d->klineChart->setMainIndicator(mainIndicator);
    }
    
    LOG_INFO(QString("Main indicator changed: %1").arg(indicator));
}

void FuturesKLinePage::onSubIndicatorChanged(const QString& indicator)
{
    // 转换为副图指标类型
    SubIndicator subIndicator = SubIndicator::None;
    if (indicator == "MACD") {
        subIndicator = SubIndicator::MACD;
    } else if (indicator == "KDJ") {
        subIndicator = SubIndicator::KDJ;
    } else if (indicator == "RSI") {
        subIndicator = SubIndicator::RSI;
    } else if (indicator == "EXPMA") {
        subIndicator = SubIndicator::EXPMA;
    }
    
    if (d->klineChart) {
        d->klineChart->setSubIndicator(subIndicator);
    }
    
    LOG_INFO(QString("Sub indicator changed: %1").arg(indicator));
}

void FuturesKLinePage::onDrawToolSelected(const QString& tool)
{
    LOG_INFO(QString("Draw tool selected: %1").arg(tool));
    
    // 画线工具（简化实现）
    // 当前版本仅记录日志，后续可扩展
    if (tool == "clear") {
        // 清除画线
        LOG_INFO("Clear all drawings");
    }
}

void FuturesKLinePage::onChartTypeChanged(const QString& type)
{
    LOG_INFO(QString("Chart type changed: %1").arg(type));
    
    // 切换K线/分时图（简化实现）
    // 当前版本仅记录日志，后续可扩展
    // K线图和分时图需要不同的数据结构和渲染方式
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

// ============================================================================
// 事件处理
// ============================================================================

void FuturesKLinePage::resizeEvent(QResizeEvent *event)
{
    BasePage::resizeEvent(event);

    // 保持分割比例
    if (d->mainSplitter && event->size().width() > 0) {
        int totalWidth = event->size().width();
        int leftWidth = static_cast<int>(totalWidth * 0.7);
        int rightWidth = totalWidth - leftWidth;
        d->mainSplitter->setSizes({leftWidth, rightWidth});
    }
}
