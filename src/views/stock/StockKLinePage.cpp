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

// ============================================================================
// 分时图组件
// ============================================================================

/**
 * @brief 分时图组件
 */
class TimeShareChart : public QWidget
{
public:
    explicit TimeShareChart(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(300);
        setMouseTracking(true);
    }
    
    void setData(const QVector<QPair<QDateTime, double>>& prices, 
                 const QVector<qint64>& volumes,
                 double basePrice = 0.0)
    {
        m_prices = prices;
        m_volumes = volumes;
        m_basePrice = basePrice;
        update();
    }
    
    void clearData()
    {
        m_prices.clear();
        m_volumes.clear();
        m_basePrice = 0.0;
        update();
    }
    
    QVector<QPair<QDateTime, double>> prices() const { return m_prices; }
    QVector<qint64> volumes() const { return m_volumes; }
    double basePrice() const { return m_basePrice; }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 背景
        painter.fillRect(rect(), QColor(Tokens::Colors::BgSurface));
        
        if (m_prices.isEmpty()) {
            painter.setPen(QColor(Tokens::Colors::TextSecondary));
            painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("暂无数据"));
            return;
        }
        
        // 计算价格范围
        double minPrice = m_prices.first().second;
        double maxPrice = m_prices.first().second;
        for (const auto& p : m_prices) {
            minPrice = qMin(minPrice, p.second);
            maxPrice = qMax(maxPrice, p.second);
        }
        
        // 扩展范围以显示基准线
        double range = qMax(maxPrice - m_basePrice, m_basePrice - minPrice) * 1.1;
        minPrice = m_basePrice - range;
        maxPrice = m_basePrice + range;
        
        int margin = 60;
        int chartWidth = width() - margin * 2;
        int chartHeight = height() - 80;
        int volumeHeight = 80;
        
        // 绘制网格
        painter.setPen(QPen(QColor(Tokens::Colors::Border), 1));
        for (int i = 0; i <= 4; i++) {
            int y = 40 + i * chartHeight / 4;
            painter.drawLine(margin, y, width() - margin, y);
        }
        
        // 绘制基准线（昨日收盘价）
        int baseY = 40 + chartHeight / 2;
        painter.setPen(QPen(QColor(Tokens::Colors::TextTertiary), 1, Qt::DashLine));
        painter.drawLine(margin, baseY, width() - margin, baseY);
        
        // 绘制价格线
        painter.setPen(QPen(QColor(Tokens::Colors::Primary), 2));
        QPainterPath path;
        bool first = true;
        
        for (int i = 0; i < m_prices.size(); i++) {
            double price = m_prices[i].second;
            int x = margin + i * chartWidth / (m_prices.size() - 1);
            int y = 40 + static_cast<int>((maxPrice - price) * chartHeight / (maxPrice - minPrice));
            
            if (first) {
                path.moveTo(x, y);
                first = false;
            } else {
                path.lineTo(x, y);
            }
        }
        painter.drawPath(path);
        
        // 绘制填充区域
        QPainterPath fillPath = path;
        fillPath.lineTo(width() - margin, baseY);
        fillPath.lineTo(margin, baseY);
        fillPath.closeSubpath();
        
        // 使用 Tokens 颜色（红涨绿跌）
        QColor fillColor = m_prices.last().second >= m_basePrice 
            ? QColor(Tokens::Colors::Danger)  // 红色 - 上涨
            : QColor(Tokens::Colors::Success); // 绿色 - 下跌
        fillColor.setAlpha(30);  // 设置透明度
        painter.fillPath(fillPath, fillColor);
        
        // 绘制成交量
        if (!m_volumes.isEmpty()) {
            qint64 maxVol = *std::max_element(m_volumes.begin(), m_volumes.end());
            if (maxVol > 0) {
                for (int i = 0; i < m_volumes.size() && i < m_prices.size(); i++) {
                    int x = margin + i * chartWidth / (m_prices.size() - 1);
                    int volHeight = static_cast<int>(m_volumes[i] * volumeHeight / maxVol);
                    
                    // 根据价格涨跌设置颜色（使用 Tokens）
                    QColor barColor = m_prices[i].second >= m_basePrice 
                        ? QColor(Tokens::Colors::Danger)  // 红色 - 上涨
                        : QColor(Tokens::Colors::Success); // 绿色 - 下跌
                    
                    painter.fillRect(x - 2, height() - 40 - volHeight, 4, volHeight, barColor);
                }
            }
        }
        
        // 绘制价格轴标签
        painter.setPen(QColor(Tokens::Colors::TextSecondary));
        painter.setFont(QFont("Microsoft YaHei", 9));
        painter.drawText(5, 45, QString::number(maxPrice, 'f', 2));
        painter.drawText(5, baseY + 5, QString::number(m_basePrice, 'f', 2));
        painter.drawText(5, height() - 45, QString::number(minPrice, 'f', 2));
        
        // 绘制时间轴标签
        if (m_prices.size() >= 4) {
            painter.drawText(margin, height() - 15, "09:30");
            painter.drawText(margin + chartWidth / 4, height() - 15, "11:00");
            painter.drawText(margin + chartWidth / 2, height() - 15, "13:00");
            painter.drawText(margin + chartWidth * 3 / 4, height() - 15, "14:00");
            painter.drawText(width() - margin - 20, height() - 15, "15:00");
        }
    }

private:
    QVector<QPair<QDateTime, double>> m_prices;
    QVector<qint64> m_volumes;
    double m_basePrice = 0.0;
};

// ============================================================================
// StockKLinePage 实现
// ============================================================================

struct StockKLinePage::Impl {
    QTimer* refreshTimer = nullptr;
    QVector<KLineData> klineData;
};

StockKLinePage::StockKLinePage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
}

StockKLinePage::~StockKLinePage() = default;

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

    toolbarLayout->addStretch();

    mainLayout->addWidget(toolbar);

    // 图表区域（水平布局：左侧K线图，右侧信息面板）
    auto* chartSplitter = new QSplitter(Qt::Horizontal, this);
    chartSplitter->setStyleSheet(QString("QSplitter::handle { background-color: %1; width: 1px; }").arg(Tokens::Colors::Border));

    // 左侧：K线图容器
    auto* chartContainer = new QWidget(chartSplitter);
    auto* chartLayout = new QVBoxLayout(chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(0);

    // K线图
    m_klineChart = new KLineChart(chartContainer);
    chartLayout->addWidget(m_klineChart);
    
    // 初始化缠论分析集成
    m_chanLun = new WealthPilot::ChanLun::ChanLunIntegration(m_klineChart, this);

    // 分时图（初始隐藏）
    m_timeShareWidget = new TimeShareChart(chartContainer);
    m_timeShareWidget->hide();
    chartLayout->addWidget(m_timeShareWidget);

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
    connect(m_refreshBtn, &QPushButton::clicked, this, &StockKLinePage::onRefresh);
    
    // K线图信号
    connect(m_klineChart, &KLineChart::crosshairMoved, this, &StockKLinePage::onCrosshairMoved);
    connect(m_klineChart, &KLineChart::klineInfoChanged, this, &StockKLinePage::onKLineInfoChanged);
}

void StockKLinePage::onChartTypeChanged(int index)
{
    m_chartType = static_cast<ChartType>(index);
    
    if (m_chartType == ChartType::KLine) {
        m_timeShareWidget->hide();
        m_klineChart->show();
        m_periodCombo->setEnabled(true);
        m_mainIndicatorCombo->setEnabled(true);
        m_subIndicatorCombo->setEnabled(true);
        loadDataWithFallback();
    } else {
        m_klineChart->hide();
        m_timeShareWidget->show();
        m_periodCombo->setEnabled(false);
        m_mainIndicatorCombo->setEnabled(false);
        m_subIndicatorCombo->setEnabled(false);
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
        m_infoLabel->setText(QStringLiteral("请先选择股票"));
        return;
    }
    
    m_infoLabel->setText(QStringLiteral("刷新中..."));
    
    if (m_chartType == ChartType::KLine) {
        loadFromNetwork();
    } else {
        loadTimeShareFromNetwork();
    }
    
    LOG_DEBUG(QString("Refresh: %1").arg(m_stockCode));
}

void StockKLinePage::onCrosshairMoved(const QDateTime& time, double price)
{
    m_infoLabel->setText(QString("时间: %1 | 价格: %2")
        .arg(time.toString("yyyy-MM-dd HH:mm"), QString::number(price, 'f', 2)));
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
        m_infoLabel->setText(QStringLiteral("未获取到数据"));
        return;
    }
    
    d->klineData = data;
    m_klineChart->setData(data);
    m_klineChart->showLatest(60);
    
    // 设置默认指标
    m_klineChart->setMainIndicator(MainIndicator::MA);
    m_klineChart->setSubIndicator(SubIndicator::MACD);
    
    // 更新缠论分析数据
    if (m_chanLun) {
        m_chanLun->setKLineData(data);
    }
    
    // 保存到缓存和数据库
    saveToCache();
    saveToDatabase();
    
    m_infoLabel->setText(QStringLiteral("已加载 %1 条K线数据").arg(data.size()));
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
            m_infoLabel->setText(QStringLiteral("已从缓存加载 %1 条数据").arg(data.size()));
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
        m_infoLabel->setText(QStringLiteral("已从数据库加载 %1 条数据").arg(data.size()));
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
    }
    
    // 请求K线数据
    KLinePeriod period = toKLinePeriod(m_period);
    m_dataSource->requestKLine(m_stockCode, period, 500);
    
    m_infoLabel->setText(QStringLiteral("正在从网络获取数据..."));
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

void StockKLinePage::loadTimeShareWithFallback()
{
    if (m_stockCode.isEmpty()) {
        static_cast<TimeShareChart*>(m_timeShareWidget)->clearData();
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
        // TODO: 反序列化分时数据
        // 暂时返回false
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
        
        auto* timeShareChart = static_cast<TimeShareChart*>(m_timeShareWidget);
        timeShareChart->setData(prices, volumes, basePrice);
        m_infoLabel->setText(QStringLiteral("已从数据库加载分时数据"));
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
    auto* timeShareChart = static_cast<TimeShareChart*>(m_timeShareWidget);
    
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
    m_infoLabel->setText(QStringLiteral("分时图已加载（演示数据）"));
    
    // 保存到缓存和数据库
    saveTimeShareToCache();
    saveTimeShareToDatabase();
}

void StockKLinePage::saveTimeShareToCache()
{
    auto* cache = CacheManager::instance();
    QString key = timeShareCacheKey();
    
    auto* timeShareChart = static_cast<TimeShareChart*>(m_timeShareWidget);
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
    auto* timeShareChart = static_cast<TimeShareChart*>(m_timeShareWidget);
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
        // 绘制分时图
        QPainter painter(m_timeShareWidget);
        painter.setRenderHint(QPainter::Antialiasing);

        QRect chartRect = m_timeShareWidget->rect().adjusted(60, 20, -20, -40);

        // 绘制背景网格
        painter.setPen(QPen(QColor(Tokens::Colors::Border), 1));
        for (int i = 0; i <= 4; ++i)
        {
            int y = chartRect.top() + i * chartRect.height() / 4;
            painter.drawLine(chartRect.left(), y, chartRect.right(), y);
        }

        // 计算价格范围
        double minPrice = std::numeric_limits<double>::max();
        double maxPrice = std::numeric_limits<double>::min();
        for (const auto& point : data)
        {
            minPrice = qMin(minPrice, point.price);
            maxPrice = qMax(maxPrice, point.price);
        }

        // 绘制分时线
        if (maxPrice > minPrice)
        {
            painter.setPen(QPen(QColor(Tokens::Colors::Primary), 2));
            QPainterPath path;

            for (int i = 0; i < data.size(); ++i)
            {
                double x = chartRect.left() + i * chartRect.width() / (data.size() - 1);
                double y = chartRect.bottom() - (data[i].price - minPrice) / (maxPrice - minPrice) * chartRect.height();

                if (i == 0)
                {
                    path.moveTo(x, y);
                }
                else
                {
                    path.lineTo(x, y);
                }
            }

            painter.drawPath(path);
        }

        // 绘制价格标签
        painter.setPen(QColor(Tokens::Colors::TextPrimary));
        painter.drawText(chartRect.left() - 50, chartRect.top(), QString::number(maxPrice, 'f', 2));
        painter.drawText(chartRect.left() - 50, chartRect.bottom(), QString::number(minPrice, 'f', 2));

        m_infoLabel->setText(QStringLiteral("分时数据已更新"));
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
            m_infoLabel->setText(QStringLiteral("实时更新: %1 (高:%2 低:%3 收:%4)")
                .arg(QString::number(update.lastPrice, 'f', 2))
                .arg(QString::number(lastKLine.high, 'f', 2))
                .arg(QString::number(lastKLine.low, 'f', 2))
                .arg(QString::number(lastKLine.close, 'f', 2)));
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
    
    m_infoLabel->setText(QStringLiteral("实时更新已启动"));
    LOG_INFO(QString("Started realtime update for %1").arg(m_stockCode));
}

void StockKLinePage::stopRealtimeUpdate()
{
    if (m_dataSource) {
        m_dataSource->stopRealtimeQuotes();
    }
    
    m_infoLabel->setText(QStringLiteral("实时更新已停止"));
    LOG_INFO("Stopped realtime update");
}
