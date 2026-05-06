/**
 * @file StockKLinePage.cpp
 * @brief 股票K线图页面实现
 */

#include "StockKLinePage.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

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

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // 背景
        painter.fillRect(rect(), QColor("#0F1419"));
        
        if (m_prices.isEmpty()) {
            painter.setPen(QColor("#4A5568"));
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
        painter.setPen(QPen(QColor("#1E293B"), 1));
        for (int i = 0; i <= 4; i++) {
            int y = 40 + i * chartHeight / 4;
            painter.drawLine(margin, y, width() - margin, y);
        }
        
        // 绘制基准线（昨日收盘价）
        int baseY = 40 + chartHeight / 2;
        painter.setPen(QPen(QColor("#6B7280"), 1, Qt::DashLine));
        painter.drawLine(margin, baseY, width() - margin, baseY);
        
        // 绘制价格线
        painter.setPen(QPen(QColor("#3B82F6"), 2));
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
        
        QColor fillColor = m_prices.last().second >= m_basePrice 
            ? QColor(239, 68, 68, 30)  // 红色
            : QColor(16, 185, 129, 30); // 绿色
        painter.fillPath(fillPath, fillColor);
        
        // 绘制成交量
        if (!m_volumes.isEmpty()) {
            qint64 maxVol = *std::max_element(m_volumes.begin(), m_volumes.end());
            if (maxVol > 0) {
                for (int i = 0; i < m_volumes.size() && i < m_prices.size(); i++) {
                    int x = margin + i * chartWidth / (m_prices.size() - 1);
                    int volHeight = static_cast<int>(m_volumes[i] * volumeHeight / maxVol);
                    
                    // 根据价格涨跌设置颜色
                    QColor barColor = m_prices[i].second >= m_basePrice 
                        ? QColor(239, 68, 68)  // 红色
                        : QColor(16, 185, 129); // 绿色
                    
                    painter.fillRect(x - 2, height() - 40 - volHeight, 4, volHeight, barColor);
                }
            }
        }
        
        // 绘制价格轴标签
        painter.setPen(QColor("#9CA3AF"));
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
    
    // 加载数据
    if (m_chartType == ChartType::KLine) {
        loadKLineData();
    } else {
        loadTimeShareData();
    }
    
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
    loadKLineData();
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

    // 图表区域
    auto* chartContainer = new QWidget(this);
    auto* chartLayout = new QVBoxLayout(chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(0);

    // K线图
    m_klineChart = new KLineChart(chartContainer);
    chartLayout->addWidget(m_klineChart);

    // 分时图（初始隐藏）
    m_timeShareWidget = new TimeShareChart(chartContainer);
    m_timeShareWidget->hide();
    chartLayout->addWidget(m_timeShareWidget);

    mainLayout->addWidget(chartContainer, 1);

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
        loadKLineData();
    } else {
        m_klineChart->hide();
        m_timeShareWidget->show();
        m_periodCombo->setEnabled(false);
        m_mainIndicatorCombo->setEnabled(false);
        m_subIndicatorCombo->setEnabled(false);
        loadTimeShareData();
    }
    
    LOG_DEBUG(QString("Chart type changed: %1").arg(index == 0 ? "TimeShare" : "KLine"));
}

void StockKLinePage::onPeriodChanged(int index)
{
    m_period = static_cast<StockKLinePeriod>(index);
    emit periodChanged(index);
    loadKLineData();
    LOG_DEBUG(QString("Period changed: %1").arg(index));
}

void StockKLinePage::onMainIndicatorChanged(int index)
{
    MainIndicator indicator = static_cast<MainIndicator>(m_mainIndicatorCombo->currentData().toInt());
    m_klineChart->setMainIndicator(indicator);
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
        loadKLineData();
    } else {
        loadTimeShareData();
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

void StockKLinePage::loadKLineData()
{
    if (m_stockCode.isEmpty()) {
        m_klineChart->clearData();
        return;
    }
    
    // 生成演示数据
    generateDemoKLineData();
}

void StockKLinePage::loadTimeShareData()
{
    if (m_stockCode.isEmpty()) {
        if (m_timeShareWidget) {
            static_cast<TimeShareChart*>(m_timeShareWidget)->clearData();
        }
        return;
    }
    
    generateDemoTimeShareData();
}

void StockKLinePage::generateDemoKLineData()
{
    QVector<KLineData> data;
    
    // 生成演示K线数据
    QDateTime baseTime = QDateTime::currentDateTime().addDays(-100);
    double basePrice = 10.0 + QRandomGenerator::global()->bounded(90.0);
    double price = basePrice;
    
    for (int i = 0; i < 100; i++) {
        KLineData kline;
        kline.time = baseTime.addDays(i);
        
        // 随机生成开高低收
        double change = (QRandomGenerator::global()->bounded(100) - 50) / 100.0 * 0.05 * price;
        kline.open = price;
        kline.close = price + change;
        kline.high = qMax(kline.open, kline.close) * (1.0 + QRandomGenerator::global()->bounded(100) / 1000.0);
        kline.low = qMin(kline.open, kline.close) * (1.0 - QRandomGenerator::global()->bounded(100) / 1000.0);
        kline.volume = 1000000 + QRandomGenerator::global()->bounded(9000000);
        kline.turnover = kline.volume * (kline.open + kline.close) / 2;
        
        data.append(kline);
        price = kline.close;
    }
    
    d->klineData = data;
    m_klineChart->setData(data);
    m_klineChart->showLatest(60);
    
    // 设置默认指标
    m_klineChart->setMainIndicator(MainIndicator::MA);
    m_klineChart->setSubIndicator(SubIndicator::MACD);
    
    m_infoLabel->setText(QStringLiteral("已加载 %1 条K线数据").arg(data.size()));
}

void StockKLinePage::generateDemoTimeShareData()
{
    if (!m_timeShareWidget) return;
    auto* timeShareChart = static_cast<TimeShareChart*>(m_timeShareWidget);
    
    QVector<QPair<QDateTime, double>> prices;
    QVector<qint64> volumes;
    
    // 生成分时数据（9:30-11:30, 13:00-15:00）
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
    QDateTime afternoon = today.addSecs(3.5 * 3600); // 13:00
    for (int i = 0; i < 120; i++) {
        QDateTime time = afternoon.addSecs(i * 60);
        double change = (QRandomGenerator::global()->bounded(100) - 48) / 1000.0 * basePrice;
        price = qMax(basePrice * 0.9, qMin(basePrice * 1.1, price + change));
        prices.append({time, price});
        volumes.append(10000 + QRandomGenerator::global()->bounded(90000));
    }
    
    timeShareChart->setData(prices, volumes, basePrice);
    m_infoLabel->setText(QStringLiteral("分时图已加载"));
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
