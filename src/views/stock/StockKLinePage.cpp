/**
 * @file StockKLinePage.cpp
 * @brief 股票K线图页面实现
 */

#include "StockKLinePage.h"
#include "core/config/Tokens.h"
#include "core/cache/CacheManager.h"
#include "data/DataStorageService.h"
#include "utils/Logger.h"
#include "ui/components/StockInfoPanel.h"
#include "ui/components/TimeShareChart.h"
#include "ui/components/QmlKLineWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>
#include <QSplitter>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QDateTime>
#include <QVariant>
#include <QElapsedTimer>
#include <QCoreApplication>

// 使用 ui/components/TimeShareChart.h 中定义的 TimeShareChart 组件

// ============================================================================
// StockKLinePage 实现
// ============================================================================

struct StockKLinePage::Impl {
    QTimer* refreshTimer = nullptr;
    QVector<KLineData> klineData;
};

StockKLinePage::StockKLinePage(QWidget* parent)
    : WealthPilot::DataHubPageBase(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
}

StockKLinePage::~StockKLinePage() = default;

void StockKLinePage::initializePage()
{
    if (isInitialized())
    {
        return;
    }
    
    // 设置默认股票代码（贵州茅台）
    if (m_stockCode.isEmpty()) {
        m_stockCode = "sh600519";
        m_stockName = QStringLiteral("贵州茅台");
        if (m_stockNameLabel) {
            m_stockNameLabel->setText(QStringLiteral("%1 (%2)").arg(m_stockName, m_stockCode));
        }
    }
    
    // 设置 DataHub 订阅
    setupDataHubSubscriptions();
    
    // 加载初始数据
    loadDataWithFallback();
    
    setInitialized(true);
    LOG_INFO("StockKLinePage initialized");
}

void StockKLinePage::setupDataHubSubscriptions()
{
    // 订阅K线数据
    if (!m_stockCode.isEmpty()) {
        QString topic = QString("market:kline:%1:%2").arg(m_stockCode, QString::number(static_cast<int>(m_period)));
        dataHub().subscribe(this, topic,
            [this](const QVariant& value) {
                Q_UNUSED(value)
                // K线数据更新
            });
    }
    
    // 订阅实时行情
    if (!m_stockCode.isEmpty()) {
        QString quoteTopic = QString("market:quote:%1").arg(m_stockCode);
        dataHub().subscribe(this, quoteTopic,
            [this](const QVariant& value) {
                Q_UNUSED(value)
                // 实时行情更新
            });
    }
    
    LOG_INFO("StockKLinePage DataHub subscriptions setup complete");
}

void StockKLinePage::setStock(const QString& stockCode, const QString& stockName)
{
    m_stockCode = stockCode;
    m_stockName = stockName.isEmpty() ? stockCode : stockName;
    
    // 更新显示
    if (m_stockNameLabel) {
        m_stockNameLabel->setText(QStringLiteral("%1 (%2)").arg(m_stockName, m_stockCode));
    }
    
    // 更新右侧信息面板
    if (m_infoPanel) {
        m_infoPanel->setStock(stockCode, stockName);
    }
    
    // 加载数据
    loadDataWithFallback();
    
    emit stockChanged(stockCode);
    LOG_DEBUG(QString("Stock set: %1 (%2)").arg(m_stockCode, m_stockName));
}

void StockKLinePage::setPeriod(StockKLinePeriod period)
{
    m_period = period;
    if (m_periodCombo) {
        m_periodCombo->setCurrentIndex(static_cast<int>(period));
    }
    emit periodChanged(static_cast<int>(period));
    loadDataWithFallback();
}

void StockKLinePage::setChartType(ChartType type)
{
    m_chartType = type;
    if (m_chartTypeTab) {
        m_chartTypeTab->setCurrentIndex(static_cast<int>(type));
    }
}

void StockKLinePage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 顶部工具栏
    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(48);
    toolbar->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(Tokens::Colors::BgElevated, Tokens::Colors::Border));
    
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(16, 0, 16, 0);
    toolbarLayout->setSpacing(12);

    // 股票名称标签
    m_stockNameLabel = new QLabel(QStringLiteral("未选择股票"), toolbar);
    m_stockNameLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;")
        .arg(Tokens::Colors::TextPrimary));
    toolbarLayout->addWidget(m_stockNameLabel);

    toolbarLayout->addSpacing(20);

    // 图表类型切换
    m_chartTypeTab = new QTabWidget(toolbar);
    m_chartTypeTab->addTab(new QWidget(), QStringLiteral("分时"));
    m_chartTypeTab->addTab(new QWidget(), QStringLiteral("K线"));
    m_chartTypeTab->setStyleSheet(QString(R"(
        QTabWidget::pane { border: none; background: transparent; }
        QTabBar::tab {
            background: transparent;
            color: %1;
            padding: 6px 16px;
            border-radius: 4px;
        }
        QTabBar::tab:selected {
            background: %2;
            color: white;
        }
        QTabBar::tab:hover:!selected {
            background: %3;
        }
    )").arg(Tokens::Colors::TextSecondary, Tokens::Colors::Primary, Tokens::Colors::BgHover));
    m_chartTypeTab->setFixedHeight(36);
    toolbarLayout->addWidget(m_chartTypeTab);

    toolbarLayout->addSpacing(10);

    // 周期选择
    toolbarLayout->addWidget(new QLabel(QStringLiteral("周期:"), toolbar));
    m_periodCombo = new QComboBox(toolbar);
    m_periodCombo->addItem(QStringLiteral("1分钟"), static_cast<int>(StockKLinePeriod::Min1));
    m_periodCombo->addItem(QStringLiteral("5分钟"), static_cast<int>(StockKLinePeriod::Min5));
    m_periodCombo->addItem(QStringLiteral("15分钟"), static_cast<int>(StockKLinePeriod::Min15));
    m_periodCombo->addItem(QStringLiteral("30分钟"), static_cast<int>(StockKLinePeriod::Min30));
    m_periodCombo->addItem(QStringLiteral("60分钟"), static_cast<int>(StockKLinePeriod::Min60));
    m_periodCombo->addItem(QStringLiteral("日线"), static_cast<int>(StockKLinePeriod::Day));
    m_periodCombo->addItem(QStringLiteral("周线"), static_cast<int>(StockKLinePeriod::Week));
    m_periodCombo->addItem(QStringLiteral("月线"), static_cast<int>(StockKLinePeriod::Month));
    m_periodCombo->setCurrentIndex(static_cast<int>(StockKLinePeriod::Day));
    toolbarLayout->addWidget(m_periodCombo);

    toolbarLayout->addSpacing(10);

    // 主图指标
    toolbarLayout->addWidget(new QLabel(QStringLiteral("主图:"), toolbar));
    m_mainIndicatorCombo = new QComboBox(toolbar);
    m_mainIndicatorCombo->addItem(QStringLiteral("MA"), static_cast<int>(MainIndicator::MA));
    m_mainIndicatorCombo->addItem(QStringLiteral("EMA"), static_cast<int>(MainIndicator::EMA));
    m_mainIndicatorCombo->addItem(QStringLiteral("BOLL"), static_cast<int>(MainIndicator::BOLL));
    m_mainIndicatorCombo->addItem(QStringLiteral("缠论"), static_cast<int>(MainIndicator::CHANLUN));
    m_mainIndicatorCombo->addItem(QStringLiteral("无"), static_cast<int>(MainIndicator::None));
    toolbarLayout->addWidget(m_mainIndicatorCombo);

    toolbarLayout->addSpacing(10);

    // 副图指标
    toolbarLayout->addWidget(new QLabel(QStringLiteral("副图:"), toolbar));
    m_subIndicatorCombo = new QComboBox(toolbar);
    m_subIndicatorCombo->addItem(QStringLiteral("MACD"), static_cast<int>(SubIndicator::MACD));
    m_subIndicatorCombo->addItem(QStringLiteral("KDJ"), static_cast<int>(SubIndicator::KDJ));
    m_subIndicatorCombo->addItem(QStringLiteral("RSI"), static_cast<int>(SubIndicator::RSI));
    m_subIndicatorCombo->addItem(QStringLiteral("VOL"), static_cast<int>(SubIndicator::VOLUME));
    m_subIndicatorCombo->addItem(QStringLiteral("无"), static_cast<int>(SubIndicator::None));
    toolbarLayout->addWidget(m_subIndicatorCombo);

    toolbarLayout->addSpacing(10);

    // 刷新按钮
    m_refreshBtn = new QPushButton(QStringLiteral("刷新"), toolbar);
    m_refreshBtn->setFixedWidth(60);
    toolbarLayout->addWidget(m_refreshBtn);

    toolbarLayout->addSpacing(10);

    // 渲染引擎选择
    toolbarLayout->addWidget(new QLabel(QStringLiteral("引擎:"), toolbar));
    m_renderEngineCombo = new QComboBox(toolbar);
    m_renderEngineCombo->addItem(QStringLiteral("Widgets"), static_cast<int>(RenderEngine::Widgets));
    m_renderEngineCombo->addItem(QStringLiteral("QML"), static_cast<int>(RenderEngine::QML));
    toolbarLayout->addWidget(m_renderEngineCombo);

    toolbarLayout->addSpacing(10);

    // 性能测试按钮
    m_benchmarkBtn = new QPushButton(QStringLiteral("性能测试"), toolbar);
    m_benchmarkBtn->setFixedWidth(80);
    toolbarLayout->addWidget(m_benchmarkBtn);

    toolbarLayout->addStretch();

    // 性能显示标签
    m_performanceLabel = new QLabel(QStringLiteral(""), toolbar);
    m_performanceLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Tokens::Colors::TextSecondary));
    toolbarLayout->addWidget(m_performanceLabel);

    mainLayout->addWidget(toolbar);

    // 图表区域（水平布局：左侧K线图，右侧信息面板）
    auto* chartSplitter = new QSplitter(Qt::Horizontal, this);
    chartSplitter->setStyleSheet(QString("QSplitter::handle { background-color: %1; width: 1px; }").arg(Tokens::Colors::Border));

    // 左侧：K线图容器
    auto* chartContainer = new QWidget(chartSplitter);
    auto* chartLayout = new QVBoxLayout(chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(0);

    // K线图 (Widgets) - 添加空指针检查
    m_klineChart = new KLineChart(chartContainer);
    if (!m_klineChart) {
        LOG_ERROR("Failed to create KLineChart");
        return;
    }
    chartLayout->addWidget(m_klineChart);
    
    // 初始化缠论分析集成 - 仅在 KLineChart 有效时创建
    if (m_klineChart) {
        m_chanLun = new WealthPilot::ChanLun::ChanLunIntegration(m_klineChart, this);
    }

    // K线图 (QML) - 初始隐藏
    m_qmlKLineChart = new QmlKLineWidget(chartContainer);
    if (m_qmlKLineChart) {
        m_qmlKLineChart->hide();
        chartLayout->addWidget(m_qmlKLineChart);
    }

    // 分时图（初始隐藏）
    m_timeShareWidget = new TimeShareChart(chartContainer);
    if (m_timeShareWidget) {
        m_timeShareWidget->hide();
        chartLayout->addWidget(m_timeShareWidget);
    }

    // 右侧：股票信息面板
    m_infoPanel = new StockInfoPanel(chartSplitter);
    m_infoPanel->setMinimumWidth(280);
    m_infoPanel->setMaximumWidth(320);

    // 设置分割比例（K线图占70%，信息面板占30%）
    chartSplitter->addWidget(chartContainer);
    chartSplitter->addWidget(m_infoPanel);
    chartSplitter->setStretchFactor(0, 7);
    chartSplitter->setStretchFactor(1, 3);

    mainLayout->addWidget(chartSplitter, 1);

    // 底部信息栏
    auto* infoBar = new QWidget(this);
    infoBar->setFixedHeight(32);
    infoBar->setStyleSheet(QString("background-color: %1; border-top: 1px solid %2;")
        .arg(Tokens::Colors::BgElevated, Tokens::Colors::Border));
    
    auto* infoLayout = new QHBoxLayout(infoBar);
    infoLayout->setContentsMargins(16, 0, 16, 0);
    
    m_infoLabel = new QLabel(QStringLiteral("等待数据..."), infoBar);
    m_infoLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
        .arg(Tokens::Colors::TextSecondary));
    infoLayout->addWidget(m_infoLabel);
    infoLayout->addStretch();
    
    mainLayout->addWidget(infoBar);

    // 设置样式
    setStyleSheet(QString(R"(
        QLabel {
            color: %1;
        }
        QComboBox {
            background-color: %2;
            color: %3;
            border: 1px solid %4;
            border-radius: 4px;
            padding: 4px 8px;
            min-width: 60px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background-color: %2;
            color: %3;
            selection-background-color: %5;
        }
        QPushButton {
            background-color: %5;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: %6;
        }
    )").arg(Tokens::Colors::TextSecondary, Tokens::Colors::BgBase, 
            Tokens::Colors::TextPrimary, Tokens::Colors::Border,
            Tokens::Colors::Primary, Tokens::Colors::PrimaryHover));
}

void StockKLinePage::setupConnections()
{
    connect(m_chartTypeTab, &QTabWidget::currentChanged, this, &StockKLinePage::onChartTypeChanged);
    connect(m_periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &StockKLinePage::onPeriodChanged);
    connect(m_mainIndicatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &StockKLinePage::onMainIndicatorChanged);
    connect(m_subIndicatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &StockKLinePage::onSubIndicatorChanged);
    connect(m_renderEngineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockKLinePage::onRenderEngineChanged);
    connect(m_refreshBtn, &QPushButton::clicked, this, &StockKLinePage::onRefresh);
    connect(m_benchmarkBtn, &QPushButton::clicked, this, [this]() {
        runPerformanceBenchmark(10);
    });
    
    // K线图信号
    connect(m_klineChart, &KLineChart::crosshairMoved, this, &StockKLinePage::onCrosshairMoved);
    connect(m_klineChart, &KLineChart::klineInfoChanged, this, &StockKLinePage::onKLineInfoChanged);
}

void StockKLinePage::onChartTypeChanged(int index)
{
    m_chartType = static_cast<ChartType>(index);
    
    if (m_chartType == ChartType::KLine) {
        // 安全隐藏分时图
        if (m_timeShareWidget) {
            m_timeShareWidget->hide();
        }
        
        // 根据渲染引擎显示对应图表
        if (m_renderEngine == RenderEngine::QML) {
            if (m_qmlKLineChart) m_qmlKLineChart->show();
            if (m_klineChart) m_klineChart->hide();
        } else {
            if (m_klineChart) m_klineChart->show();
            if (m_qmlKLineChart) m_qmlKLineChart->hide();
        }
        
        if (m_periodCombo) m_periodCombo->setEnabled(true);
        if (m_mainIndicatorCombo) m_mainIndicatorCombo->setEnabled(true);
        if (m_subIndicatorCombo) m_subIndicatorCombo->setEnabled(true);
        if (m_renderEngineCombo) m_renderEngineCombo->setEnabled(true);
        loadDataWithFallback();
    } else {
        // 安全隐藏K线图
        if (m_klineChart) m_klineChart->hide();
        if (m_qmlKLineChart) m_qmlKLineChart->hide();
        if (m_timeShareWidget) m_timeShareWidget->show();
        
        if (m_periodCombo) m_periodCombo->setEnabled(false);
        if (m_mainIndicatorCombo) m_mainIndicatorCombo->setEnabled(false);
        if (m_subIndicatorCombo) m_subIndicatorCombo->setEnabled(false);
        if (m_renderEngineCombo) m_renderEngineCombo->setEnabled(false);
        loadTimeShareWithFallback();
    }
    
    LOG_DEBUG(QString("Chart type changed: %1").arg(index == 0 ? "TimeShare" : "KLine"));
}

void StockKLinePage::onPeriodChanged(int index)
{
    m_period = static_cast<StockKLinePeriod>(index);
    emit periodChanged(index);
    
    // 停止之前的实时更新
    stopRealtimeUpdate();
    
    // 加载新周期的数据
    loadDataWithFallback();
    
    // 如果是交易时间，启动实时更新
    QTime now = QTime::currentTime();
    bool isTradingTime = (now >= QTime(9, 30) && now <= QTime(11, 30)) ||
                         (now >= QTime(13, 0) && now <= QTime(15, 0));
    if (isTradingTime) {
        startRealtimeUpdate();
    }
    
    LOG_DEBUG(QString("Period changed: %1, realtime: %2").arg(index).arg(isTradingTime));
}

void StockKLinePage::onMainIndicatorChanged(int index)
{
    MainIndicator indicator = static_cast<MainIndicator>(m_mainIndicatorCombo->currentData().toInt());
    
    // 处理缠论指标
    if (indicator == MainIndicator::CHANLUN) {
        // 启用缠论分析
        if (m_chanLun) {
            m_chanLun->setEnabled(true);
        }
        // 同时设置基础指标为无，避免重叠
        m_klineChart->setMainIndicator(MainIndicator::None);
    } else {
        // 禁用缠论分析
        if (m_chanLun) {
            m_chanLun->setEnabled(false);
        }
        m_klineChart->setMainIndicator(indicator);
    }
    
    LOG_DEBUG(QString("Main indicator changed: %1").arg(index));
}

void StockKLinePage::onSubIndicatorChanged(int index)
{
    SubIndicator indicator = static_cast<SubIndicator>(m_subIndicatorCombo->currentData().toInt());
    m_klineChart->setSubIndicator(indicator);
    LOG_DEBUG(QString("Sub indicator changed: %1").arg(index));
}

void StockKLinePage::onRefresh()
{
    if (m_stockCode.isEmpty()) {
        if (m_infoLabel) {
            m_infoLabel->setText(QStringLiteral("请先选择股票"));
        }
        return;
    }
    
    if (m_infoLabel) {
        m_infoLabel->setText(QStringLiteral("刷新中..."));
    }
    
    if (m_chartType == ChartType::KLine) {
        loadFromNetwork();
    } else {
        loadTimeShareFromNetwork();
    }
    
    LOG_DEBUG(QString("Refresh: %1").arg(m_stockCode));
}

void StockKLinePage::onCrosshairMoved(const QDateTime& time, double price)
{
    if (m_infoLabel) {
        m_infoLabel->setText(QString("时间: %1 | 价格: %2")
            .arg(time.toString("yyyy-MM-dd HH:mm"), QString::number(price, 'f', 2)));
    }
}

void StockKLinePage::onKLineInfoChanged(const KLineData& kline, int index)
{
    Q_UNUSED(index);
    updateInfoLabel(kline);
}

void StockKLinePage::onKLineReceived(const QString& symbol, const QVector<KLineData>& data)
{
    if (symbol != m_stockCode) return;
    
    if (data.isEmpty()) {
        if (m_infoLabel) {
            m_infoLabel->setText(QStringLiteral("未获取到数据"));
        }
        return;
    }
    
    d->klineData = data;
    
    // 根据渲染引擎设置数据
    if (m_renderEngine == RenderEngine::QML) {
        m_qmlKLineChart->setKLineData(data);
    } else {
        m_klineChart->setData(data);
        m_klineChart->showLatest(60);
        
        // 设置默认指标
        m_klineChart->setMainIndicator(MainIndicator::MA);
        m_klineChart->setSubIndicator(SubIndicator::MACD);
        
        // 更新缠论分析数据
        if (m_chanLun) {
            m_chanLun->setKLineData(data);
        }
    }
    
    // 保存到缓存和数据库
    saveToCache();
    saveToDatabase();
    
    if (m_infoLabel) {
        m_infoLabel->setText(QStringLiteral("已加载 %1 条K线数据").arg(data.size()));
    }
    LOG_INFO(QString("KLine data received: %1 items for %2").arg(data.size()).arg(symbol));
}

// ============================================================================
// 数据加载流程：缓存 → 数据库 → 网络数据源
// ============================================================================

void StockKLinePage::loadDataWithFallback()
{
    if (m_stockCode.isEmpty()) {
        m_klineChart->clearData();
        return;
    }
    
    LOG_INFO(QString("Loading KLine data with fallback for %1").arg(m_stockCode));
    
    // 1. 尝试从缓存加载
    if (loadFromCache()) {
        LOG_INFO("KLine data loaded from cache");
        // 缓存命中，后台更新数据
        QTimer::singleShot(100, this, [this]() {
            loadFromNetwork();
        });
        return;
    }
    
    // 2. 缓存未命中，尝试从数据库加载
    if (loadFromDatabase()) {
        LOG_INFO("KLine data loaded from database");
        // 数据库命中，保存到缓存并后台更新
        saveToCache();
        QTimer::singleShot(100, this, [this]() {
            loadFromNetwork();
        });
        return;
    }
    
    // 3. 数据库也没有，从网络加载
    LOG_INFO("No local KLine data, loading from network");
    loadFromNetwork();
}

bool StockKLinePage::loadFromCache()
{
    auto* cache = CacheManager::instance();
    QString key = cacheKey();
    
    if (!cache->contains(key)) {
        return false;
    }
    
    // 检查缓存是否过期（根据周期设置不同的TTL）
    QDateTime lastUpdate = cache->get(key + "_time").toDateTime();
    if (lastUpdate.isValid()) {
        qint64 ageSecs = lastUpdate.secsTo(QDateTime::currentDateTime());
        int maxAge = 300; // 默认5分钟
        if (m_period == StockKLinePeriod::Day || 
            m_period == StockKLinePeriod::Week ||
            m_period == StockKLinePeriod::Month) {
            maxAge = 3600; // 日线以上1小时过期
        }
        if (ageSecs > maxAge) {
            LOG_DEBUG(QString("Cache expired, age: %1 seconds").arg(ageSecs));
            return false;
        }
    }
    
    QVariant dataVariant = cache->get(key);
    if (dataVariant.canConvert<QVector<KLineData>>()) {
        QVector<KLineData> data = dataVariant.value<QVector<KLineData>>();
        if (!data.isEmpty()) {
            d->klineData = data;
            m_klineChart->setData(data);
            m_klineChart->showLatest(60);
            m_klineChart->setMainIndicator(MainIndicator::MA);
            m_klineChart->setSubIndicator(SubIndicator::MACD);
            if (m_infoLabel) {
                m_infoLabel->setText(QStringLiteral("已从缓存加载 %1 条数据").arg(data.size()));
            }
            return true;
        }
    }
    
    return false;
}

bool StockKLinePage::loadFromDatabase()
{
    auto* storage = DataStorageService::instance();
    
    // 从数据库加载K线数据
    int period = static_cast<int>(toKLinePeriod(m_period));
    QVector<KLineData> data = storage->getKLineData(m_stockCode, period, 500);
    
    if (!data.isEmpty()) {
        d->klineData = data;
        m_klineChart->setData(data);
        m_klineChart->showLatest(60);
        m_klineChart->setMainIndicator(MainIndicator::MA);
        m_klineChart->setSubIndicator(SubIndicator::MACD);
        if (m_infoLabel) {
            m_infoLabel->setText(QStringLiteral("已从数据库加载 %1 条数据").arg(data.size()));
        }
        LOG_INFO(QString("KLine data loaded from database: %1 items").arg(data.size()));
        return true;
    }
    
    return false;
}

void StockKLinePage::loadFromNetwork()
{
    if (m_stockCode.isEmpty()) return;
    
    LOG_INFO(QString("Loading KLine data from network for %1").arg(m_stockCode));
    
    // 初始化数据源
    if (!m_dataSource) {
        m_dataSource = new StockDataSource(StockDataSource::Source::Sina, this);
        connect(m_dataSource, &StockDataSource::kLineReceived, 
                this, &StockKLinePage::onKLineReceived);
        connect(m_dataSource, &StockDataSource::timeShareReceived,
                this, &StockKLinePage::onTimeShareReceived);
    }
    
    // 请求K线数据
    KLinePeriod period = toKLinePeriod(m_period);
    m_dataSource->requestKLine(m_stockCode, period, 500);
    
    if (m_infoLabel) {
        m_infoLabel->setText(QStringLiteral("正在从网络获取数据..."));
    }
}

void StockKLinePage::saveToCache()
{
    if (d->klineData.isEmpty()) return;
    
    auto* cache = CacheManager::instance();
    QString key = cacheKey();
    
    // 保存K线数据
    cache->set(key, QVariant::fromValue(d->klineData), 300);
    cache->set(key + "_time", QDateTime::currentDateTime(), 300);
    
    LOG_DEBUG(QString("KLine data saved to cache: %1 items").arg(d->klineData.size()));
}

void StockKLinePage::saveToDatabase()
{
    if (d->klineData.isEmpty()) return;
    
    auto* storage = DataStorageService::instance();
    
    // 保存K线数据到数据库
    int period = static_cast<int>(toKLinePeriod(m_period));
    if (storage->saveKLineData(m_stockCode, period, d->klineData)) {
        LOG_INFO(QString("KLine data saved to database: %1 items for %2 period %3")
            .arg(d->klineData.size()).arg(m_stockCode).arg(period));
    }
}

// ============================================================================
// 分时图数据加载
// ============================================================================

// 安全获取 TimeShareChart 指针的辅助方法
namespace {
TimeShareChart* safeGetTimeShareChart(QWidget* widget) {
    if (!widget) return nullptr;
    return qobject_cast<TimeShareChart*>(widget);
}
}

void StockKLinePage::loadTimeShareWithFallback()
{
    if (m_stockCode.isEmpty()) {
        // 安全的类型转换 - 添加空指针检查
        if (m_timeShareWidget) {
            auto* timeShareChart = qobject_cast<TimeShareChart*>(m_timeShareWidget);
            if (timeShareChart) {
                timeShareChart->clearData();
            }
        }
        return;
    }
    
    LOG_INFO(QString("Loading TimeShare data with fallback for %1").arg(m_stockCode));
    
    // 1. 尝试从缓存加载
    if (loadTimeShareFromCache()) {
        LOG_INFO("TimeShare data loaded from cache");
        QTimer::singleShot(100, this, [this]() {
            loadTimeShareFromNetwork();
        });
        return;
    }
    
    // 2. 缓存未命中，尝试从数据库加载
    if (loadTimeShareFromDatabase()) {
        LOG_INFO("TimeShare data loaded from database");
        saveTimeShareToCache();
        QTimer::singleShot(100, this, [this]() {
            loadTimeShareFromNetwork();
        });
        return;
    }
    
    // 3. 从网络加载
    LOG_INFO("No local TimeShare data, loading from network");
    loadTimeShareFromNetwork();
}

bool StockKLinePage::loadTimeShareFromCache()
{
    auto* cache = CacheManager::instance();
    QString key = timeShareCacheKey();
    
    if (!cache->contains(key)) {
        return false;
    }
    
    // 检查缓存是否过期（分时图数据1分钟过期）
    QDateTime lastUpdate = cache->get(key + "_time").toDateTime();
    if (lastUpdate.isValid()) {
        qint64 ageSecs = lastUpdate.secsTo(QDateTime::currentDateTime());
        if (ageSecs > 60) { // 1分钟过期
            return false;
        }
    }
    
    // 加载分时数据
    QVariant pricesVariant = cache->get(key + "_prices");
    QVariant volumesVariant = cache->get(key + "_volumes");
    QVariant baseVariant = cache->get(key + "_base");
    
    if (pricesVariant.isValid() && volumesVariant.isValid() && baseVariant.isValid()) {
        // 反序列化分时数据
        QVariantList pricesList = pricesVariant.toList();
        QVariantList volumesList = volumesVariant.toList();
        double basePrice = baseVariant.toDouble();
        
        if (pricesList.isEmpty() || volumesList.isEmpty()) {
            return false;
        }
        
        QVector<QPair<QDateTime, double>> prices;
        QVector<qint64> volumes;
        
        for (int i = 0; i < pricesList.size() && i < volumesList.size(); ++i) {
            QVariantMap priceMap = pricesList[i].toMap();
            prices.append({
                QDateTime::fromString(priceMap["time"].toString(), Qt::ISODate),
                priceMap["price"].toDouble()
            });
            volumes.append(volumesList[i].toLongLong());
        }
        
        if (!prices.isEmpty()) {
            auto* timeShareChart = safeGetTimeShareChart(m_timeShareWidget);
            if (timeShareChart) {
                timeShareChart->setData(prices, volumes, basePrice);
            }
            if (m_infoLabel) {
                m_infoLabel->setText(QStringLiteral("已从缓存加载分时数据"));
            }
            LOG_INFO(QString("TimeShare data loaded from cache: %1 points").arg(prices.size()));
            return true;
        }
    }
    
    return false;
}

bool StockKLinePage::loadTimeShareFromDatabase()
{
    auto* storage = DataStorageService::instance();
    
    // 从数据库加载分时数据
    QVector<DataStorageService::TimeSharePoint> data = storage->getTimeShareData(m_stockCode);
    double basePrice = storage->getTimeShareBasePrice(m_stockCode);
    
    if (!data.isEmpty() && basePrice > 0) {
        // 转换为分时图数据格式
        QVector<QPair<QDateTime, double>> prices;
        QVector<qint64> volumes;
        
        for (const auto& point : data) {
            prices.append({point.time, point.price});
            volumes.append(point.volume);
        }
        
        auto* timeShareChart = safeGetTimeShareChart(m_timeShareWidget);
        if (timeShareChart) {
            timeShareChart->setData(prices, volumes, basePrice);
        }
        if (m_infoLabel) {
            m_infoLabel->setText(QStringLiteral("已从数据库加载分时数据"));
        }
        LOG_INFO(QString("TimeShare data loaded from database: %1 points").arg(data.size()));
        return true;
    }
    
    return false;
}

void StockKLinePage::loadTimeShareFromNetwork()
{
    if (m_stockCode.isEmpty()) return;
    
    LOG_INFO(QString("Loading TimeShare data from network for %1").arg(m_stockCode));
    
    // 使用真实分时数据API
    if (m_dataSource) {
        m_dataSource->requestTimeShare(m_stockCode);
    } else {
        // 如果数据源未初始化，生成演示数据
        LOG_WARNING("DataSource not initialized, using demo data");
        generateDemoTimeShareData();
    }
}

void StockKLinePage::generateDemoTimeShareData()
{
    auto* timeShareChart = safeGetTimeShareChart(m_timeShareWidget);
    if (!timeShareChart) return;
    
    QVector<QPair<QDateTime, double>> prices;
    QVector<qint64> volumes;
    
    QDateTime today = QDateTime::currentDateTime();
    today.setTime(QTime(9, 30));
    
    double basePrice = 15.0 + QRandomGenerator::global()->bounded(85.0);
    double price = basePrice;
    
    // 上午 9:30-11:30 (120分钟)
    for (int i = 0; i < 120; i++) {
        QDateTime time = today.addSecs(i * 60);
        double change = (QRandomGenerator::global()->bounded(100) - 48) / 1000.0 * basePrice;
        price = qMax(basePrice * 0.9, qMin(basePrice * 1.1, price + change));
        prices.append({time, price});
        volumes.append(10000 + QRandomGenerator::global()->bounded(90000));
    }
    
    // 下午 13:00-15:00 (120分钟)
    QDateTime afternoon = today.addSecs(3.5 * 3600);
    for (int i = 0; i < 120; i++) {
        QDateTime time = afternoon.addSecs(i * 60);
        double change = (QRandomGenerator::global()->bounded(100) - 48) / 1000.0 * basePrice;
        price = qMax(basePrice * 0.9, qMin(basePrice * 1.1, price + change));
        prices.append({time, price});
        volumes.append(10000 + QRandomGenerator::global()->bounded(90000));
    }
    
    timeShareChart->setData(prices, volumes, basePrice);
    if (m_infoLabel) {
        m_infoLabel->setText(QStringLiteral("分时图已加载（演示数据）"));
    }
    
    // 保存到缓存和数据库
    saveTimeShareToCache();
    saveTimeShareToDatabase();
}

void StockKLinePage::saveTimeShareToCache()
{
    auto* cache = CacheManager::instance();
    QString key = timeShareCacheKey();
    
    auto* timeShareChart = safeGetTimeShareChart(m_timeShareWidget);
    if (!timeShareChart) return;
    
    auto prices = timeShareChart->prices();
    auto volumes = timeShareChart->volumes();
    double basePrice = timeShareChart->basePrice();
    
    if (prices.isEmpty()) return;
    
    // 保存分时数据
    cache->set(key + "_prices", QVariant::fromValue(prices), 60);
    cache->set(key + "_volumes", QVariant::fromValue(volumes), 60);
    cache->set(key + "_base", basePrice, 60);
    cache->set(key + "_time", QDateTime::currentDateTime(), 60);
    
    LOG_DEBUG("TimeShare data saved to cache");
}

void StockKLinePage::saveTimeShareToDatabase()
{
    auto* timeShareChart = safeGetTimeShareChart(m_timeShareWidget);
    if (!timeShareChart) return;
    
    auto prices = timeShareChart->prices();
    auto volumes = timeShareChart->volumes();
    double basePrice = timeShareChart->basePrice();
    
    if (prices.isEmpty() || basePrice <= 0) return;
    
    auto* storage = DataStorageService::instance();
    
    // 转换为数据库格式
    QVector<DataStorageService::TimeSharePoint> data;
    for (int i = 0; i < prices.size() && i < volumes.size(); i++) {
        DataStorageService::TimeSharePoint point;
        point.time = prices[i].first;
        point.price = prices[i].second;
        point.volume = volumes[i];
        data.append(point);
    }
    
    if (storage->saveTimeShareData(m_stockCode, data, basePrice)) {
        LOG_INFO(QString("TimeShare data saved to database: %1 points for %2")
            .arg(data.size()).arg(m_stockCode));
    }
}

// ============================================================================
// 辅助方法
// ============================================================================

QString StockKLinePage::cacheKey() const
{
    return QString("kline_%1_%2").arg(m_stockCode, QString::number(static_cast<int>(m_period)));
}

QString StockKLinePage::timeShareCacheKey() const
{
    return QString("timeshare_%1").arg(m_stockCode);
}

void StockKLinePage::updateInfoLabel(const KLineData& kline)
{
    if (!m_infoLabel) return;
    
    double change = kline.close - kline.open;
    double changePercent = (kline.open > 0) ? (change / kline.open * 100) : 0;
    
    QString changeText = change >= 0 
        ? QString("+%1 (+%2%)").arg(change, 0, 'f', 2).arg(changePercent, 0, 'f', 2)
        : QString("%1 (%2%)").arg(change, 0, 'f', 2).arg(changePercent, 0, 'f', 2);
    
    m_infoLabel->setText(QString("日期: %1 | 开: %2 | 高: %3 | 低: %4 | 收: %5 | 涨跌: %6 | 成交量: %7")
        .arg(kline.time.toString("yyyy-MM-dd"),
             QString::number(kline.open, 'f', 2),
             QString::number(kline.high, 'f', 2),
             QString::number(kline.low, 'f', 2),
             QString::number(kline.close, 'f', 2),
             changeText,
             QString::number(kline.volume)));
}

KLinePeriod StockKLinePage::toKLinePeriod(StockKLinePeriod period) const
{
    switch (period) {
        case StockKLinePeriod::Min1:  return KLinePeriod::Minute1;
        case StockKLinePeriod::Min5:  return KLinePeriod::Minute5;
        case StockKLinePeriod::Min15: return KLinePeriod::Minute15;
        case StockKLinePeriod::Min30: return KLinePeriod::Minute30;
        case StockKLinePeriod::Min60: return KLinePeriod::Hour1;
        case StockKLinePeriod::Day:   return KLinePeriod::Day1;
        case StockKLinePeriod::Week:  return KLinePeriod::Week1;
        case StockKLinePeriod::Month: return KLinePeriod::Month1;
        default: return KLinePeriod::Day1;
    }
}

// ============================================================================
// 实时数据更新
// ============================================================================

void StockKLinePage::onTimeShareReceived(const QString& symbol, const QVector<TimeShareData>& data)
{
    if (symbol != m_stockCode) return;
    
    // 更新分时图显示
    if (m_chartType == ChartType::TimeShare && !data.isEmpty()) {
        auto* timeShareChart = safeGetTimeShareChart(m_timeShareWidget);
        if (timeShareChart) {
            timeShareChart->setData(data);
        }
        if (m_infoLabel) {
            m_infoLabel->setText(QStringLiteral("分时数据已更新"));
        }
    }
    
    LOG_DEBUG(QString("TimeShare data received: %1 points for %2").arg(data.size()).arg(symbol));
}

void StockKLinePage::onRealtimeQuoteReceived(const QString& symbol, const StockQuote& quote)
{
    if (symbol != m_stockCode) return;
    
    // 更新股票信息显示
    if (m_stockNameLabel) {
        QString changeText = quote.changePercent >= 0 ? 
            QStringLiteral("+%1%%").arg(quote.changePercent, 0, 'f', 2) :
            QStringLiteral("%1%%").arg(quote.changePercent, 0, 'f', 2);
        
        QString colorStyle = quote.changePercent >= 0 ? 
            QString("color: %1;").arg(Tokens::Colors::Success) : 
            QString("color: %1;").arg(Tokens::Colors::Danger);
        m_stockNameLabel->setText(QStringLiteral("%1 (%2) %3 <span style='%4'>%5</span>")
            .arg(quote.name, quote.symbol)
            .arg(QString::number(quote.lastPrice, 'f', 2))
            .arg(colorStyle)
            .arg(changeText));
    }
    
    // 更新右侧信息面板
    if (m_infoPanel) {
        m_infoPanel->updateQuote(quote);
        
        // 生成演示成交明细数据
        QVector<TickData> ticks;
        for (int i = 0; i < 10; ++i) {
            TickData tick;
            tick.time = QDateTime::currentDateTime().addSecs(-i * 3);
            tick.price = quote.lastPrice + (QRandomGenerator::global()->bounded(100) - 50) / 100.0;
            tick.volume = 100 + QRandomGenerator::global()->bounded(900);
            tick.direction = tick.price >= quote.lastPrice ? WealthPilot::TradeDirection::Buy : WealthPilot::TradeDirection::Sell;
            ticks.append(tick);
        }
        m_infoPanel->updateTickData(ticks);
    }
    
    LOG_DEBUG(QString("Realtime quote: %1 %2 %3%%").arg(symbol).arg(quote.lastPrice).arg(quote.changePercent));
}

void StockKLinePage::onRealtimeKLineUpdate(const QString& symbol, const RealtimeKLineUpdate& update)
{
    if (symbol != m_stockCode) return;
    
    // 更新K线图最后一根蜡烛
    if (m_chartType == ChartType::KLine && m_klineChart) {
        // 获取当前K线数据
        QVector<KLineData> currentData = m_klineChart->data();
        
        if (!currentData.isEmpty()) {
            // 更新最后一根K线
            KLineData& lastKLine = currentData.last();
            lastKLine.close = update.lastPrice;
            lastKLine.high = qMax(lastKLine.high, update.highPrice);
            lastKLine.low = qMin(lastKLine.low, update.lowPrice);
            lastKLine.volume = update.volume;
            
            // 重新设置数据以触发重绘
            m_klineChart->setData(currentData);
            
            // 更新信息标签
            if (m_infoLabel) {
                m_infoLabel->setText(QStringLiteral("实时更新: %1 (高:%2 低:%3 收:%4)")
                    .arg(QString::number(update.lastPrice, 'f', 2))
                    .arg(QString::number(lastKLine.high, 'f', 2))
                    .arg(QString::number(lastKLine.low, 'f', 2))
                    .arg(QString::number(lastKLine.close, 'f', 2)));
            }
        }
    }
    
    LOG_DEBUG(QString("Realtime KLine update: %1 %2").arg(symbol).arg(update.lastPrice));
}

void StockKLinePage::startRealtimeUpdate()
{
    if (m_stockCode.isEmpty()) return;
    
    // 初始化数据源
    if (!m_dataSource) {
        m_dataSource = new StockDataSource(StockDataSource::Source::Sina, this);
        connect(m_dataSource, &StockDataSource::kLineReceived,
                this, &StockKLinePage::onKLineReceived);
        connect(m_dataSource, &StockDataSource::timeShareReceived,
                this, &StockKLinePage::onTimeShareReceived);
        connect(m_dataSource, &StockDataSource::realtimeQuoteReceived,
                this, &StockKLinePage::onRealtimeQuoteReceived);
        connect(m_dataSource, &StockDataSource::realtimeKLineUpdate,
                this, &StockKLinePage::onRealtimeKLineUpdate);
    }
    
    // 启动实时行情推送（3秒间隔）
    m_dataSource->startRealtimeQuotes(m_stockCode, 3000);
    
    if (m_infoLabel) {
        m_infoLabel->setText(QStringLiteral("实时更新已启动"));
    }
    LOG_INFO(QString("Started realtime update for %1").arg(m_stockCode));
}

void StockKLinePage::stopRealtimeUpdate()
{
    if (m_dataSource) {
        m_dataSource->stopRealtimeQuotes();
    }
    
    if (m_infoLabel) {
        m_infoLabel->setText(QStringLiteral("实时更新已停止"));
    }
    LOG_INFO("Stopped realtime update");
}

void StockKLinePage::onRenderEngineChanged(int index)
{
    m_renderEngine = static_cast<RenderEngine>(m_renderEngineCombo->currentData().toInt());
    
    // 切换图表显示
    if (m_renderEngine == RenderEngine::Widgets) {
        m_qmlKLineChart->hide();
        m_klineChart->show();
        m_renderEngineCombo->setToolTip(QStringLiteral("传统 Widgets 渲染"));
    } else {
        m_klineChart->hide();
        m_qmlKLineChart->show();
        m_renderEngineCombo->setToolTip(QStringLiteral("QML GPU 加速渲染"));
    }
    
    // 重新加载数据
    if (!d->klineData.isEmpty()) {
        if (m_renderEngine == RenderEngine::QML) {
            m_qmlKLineChart->setKLineData(d->klineData);
        }
    }
    
    emit renderEngineChanged(index);
    LOG_INFO(QString("Render engine changed: %1").arg(index == 0 ? "Widgets" : "QML"));
}

void StockKLinePage::setRenderEngine(RenderEngine engine)
{
    m_renderEngine = engine;
    if (m_renderEngineCombo) {
        m_renderEngineCombo->setCurrentIndex(static_cast<int>(engine));
    }
}

void StockKLinePage::runPerformanceBenchmark(int iterations)
{
    m_performanceLabel->setText(QStringLiteral("性能测试中..."));
    m_benchmarkBtn->setEnabled(false);
    
    // 生成测试数据
    QVector<KLineData> testData;
    testData.reserve(1000);
    
    QDateTime time = QDateTime::currentDateTime().addDays(-1000);
    double basePrice = 100.0;
    double price = basePrice;
    
    for (int i = 0; i < 1000; ++i) {
        KLineData kline;
        kline.time = time;
        
        double change = (QRandomGenerator::global()->bounded(100) - 50) / 100.0 * 3.0;
        kline.open = price;
        kline.close = price * (1 + change / 100.0);
        kline.high = qMax(kline.open, kline.close) * 1.005;
        kline.low = qMin(kline.open, kline.close) * 0.995;
        kline.volume = 100000 + QRandomGenerator::global()->bounded(900000);
        
        testData.append(kline);
        price = kline.close;
        time = time.addDays(1);
    }
    
    // 测试 Widgets 渲染性能
    KLinePerformanceStats widgetsStats;
    widgetsStats.engineName = QStringLiteral("Widgets");
    
    QElapsedTimer timer;
    qint64 totalWidgetsTime = 0;
    
    for (int i = 0; i < iterations; ++i) {
        timer.start();
        m_klineChart->setData(testData);
        m_klineChart->repaint();
        QCoreApplication::processEvents();
        totalWidgetsTime += timer.elapsed();
    }
    widgetsStats.renderTimeMs = totalWidgetsTime / iterations;
    
    // 测试 QML 渲染性能
    KLinePerformanceStats qmlStats;
    qmlStats.engineName = QStringLiteral("QML");
    
    qint64 totalQmlTime = 0;
    
    for (int i = 0; i < iterations; ++i) {
        timer.start();
        m_qmlKLineChart->setKLineData(testData);
        QCoreApplication::processEvents();
        totalQmlTime += timer.elapsed();
    }
    qmlStats.renderTimeMs = totalQmlTime / iterations;
    
    // 计算性能提升
    double improvement = 0.0;
    if (widgetsStats.renderTimeMs > 0 && qmlStats.renderTimeMs > 0) {
        improvement = (static_cast<double>(widgetsStats.renderTimeMs) - qmlStats.renderTimeMs) 
                    / widgetsStats.renderTimeMs * 100.0;
    }
    
    // 更新显示
    QString result = QStringLiteral("Widgets: %1ms | QML: %2ms | 提升: %3%")
        .arg(widgetsStats.renderTimeMs)
        .arg(qmlStats.renderTimeMs)
        .arg(QString::number(improvement, 'f', 1));
    
    m_performanceLabel->setText(result);
    m_lastStats = m_renderEngine == RenderEngine::Widgets ? widgetsStats : qmlStats;
    
    emit performanceStatsChanged(m_lastStats);
    
    m_benchmarkBtn->setEnabled(true);
    
    LOG_INFO(QString("Performance benchmark: Widgets=%1ms, QML=%2ms, improvement=%3%")
        .arg(widgetsStats.renderTimeMs)
        .arg(qmlStats.renderTimeMs)
        .arg(improvement));
}

void StockKLinePage::measureRenderPerformance()
{
    QElapsedTimer timer;
    timer.start();
    
    if (m_renderEngine == RenderEngine::Widgets) {
        m_klineChart->repaint();
    }
    // QML 自动渲染
    
    m_lastStats.renderTimeMs = timer.elapsed();
    updatePerformanceDisplay();
}

void StockKLinePage::updatePerformanceDisplay()
{
    if (m_performanceLabel) {
        m_performanceLabel->setText(QStringLiteral("渲染: %1ms")
            .arg(m_lastStats.renderTimeMs));
    }
}
