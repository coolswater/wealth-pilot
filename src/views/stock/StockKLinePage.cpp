/**
 * @file StockKLinePage.cpp
 * @brief 股票K线图页面实现 - 专业级股票行情分析
 *
 * @details 实现：
 * - K线图展示（多周期支持）
 * - 技术指标计算与叠加
 * - 成交量分析
 * - 实时行情刷新
 * - 高性能绘制优化
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "StockKLinePage.h"
#include "ui/components/KLineChart.h"
#include "core/types/MarketTypes.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QSplitter>
#include <QTableWidget>
#include <QHeaderView>
#include <QTimer>
#include <QPainter>
#include <QKeyEvent>
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>

// ============================================================================
// PIMPL 实现
// ============================================================================

struct StockKLinePage::Impl {
    // 股票信息
    QString stockCode;
    QString exchange;
    QString stockName;
    
    // K线数据
    QVector<KLineData> klineData;
    StockKLinePeriod period = StockKLinePeriod::Day;
    int adjustType = 1;  // 默认前复权
    
    // 技术指标
    QMap<TechnicalIndicator, QVector<int>> indicators;
    QMap<TechnicalIndicator, QVector<double>> indicatorValues;
    
    // UI 组件
    QLineEdit* searchEdit = nullptr;
    QComboBox* periodCombo = nullptr;
    QComboBox* adjustCombo = nullptr;
    QComboBox* indicatorCombo = nullptr;
    QPushButton* refreshBtn = nullptr;
    
    QLabel* stockNameLabel = nullptr;
    QLabel* priceLabel = nullptr;
    QLabel* changeLabel = nullptr;
    QLabel* volumeLabel = nullptr;
    QLabel* turnoverLabel = nullptr;
    
    KLineChart* klineChart = nullptr;
    QTableWidget* infoTable = nullptr;
    
    // 定时器
    QTimer* refreshTimer = nullptr;
    
    // 行情数据
    double lastPrice = 0.0;
    double prevClose = 0.0;
    double high = 0.0;
    double low = 0.0;
    qint64 volume = 0;
    double turnover = 0.0;
};

// ============================================================================
// 构造与析构
// ============================================================================

StockKLinePage::StockKLinePage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    initUI();
    initConnections();
    
    LOG_DEBUG("StockKLinePage created");
}

StockKLinePage::~StockKLinePage()
{
    if (d->refreshTimer) {
        d->refreshTimer->stop();
    }
    LOG_DEBUG("StockKLinePage destroyed");
}

// ============================================================================
// BasePage 接口实现
// ============================================================================

void StockKLinePage::onPageActivated(const QVariantMap& params)
{
    Q_UNUSED(params);
    LOG_DEBUG(QString("StockKLinePage activated: %1").arg(d->stockCode));
    
    // 启动定时刷新
    if (d->refreshTimer) {
        d->refreshTimer->start(3000);  // 3秒刷新一次
    }
    
    // 加载数据
    if (!d->stockCode.isEmpty()) {
        loadKLineData();
    }
}

void StockKLinePage::onPageDeactivated()
{
    LOG_DEBUG("StockKLinePage deactivated");
    
    // 停止定时刷新
    if (d->refreshTimer) {
        d->refreshTimer->stop();
    }
}

// ============================================================================
// 数据接口
// ============================================================================

void StockKLinePage::setStock(const QString& stockCode, const QString& exchange)
{
    d->stockCode = stockCode;
    d->exchange = exchange;
    
    // 加载股票信息
    loadStockInfo();
    
    // 加载K线数据
    loadKLineData();
    
    emit stockChanged(stockCode);
}

QString StockKLinePage::stockCode() const
{
    return d->stockCode;
}

void StockKLinePage::setPeriod(StockKLinePeriod period)
{
    if (d->period != period) {
        d->period = period;
        loadKLineData();
        emit periodChanged(period);
    }
}

void StockKLinePage::setAdjustType(int adjust)
{
    d->adjustType = adjust;
    loadKLineData();
}

void StockKLinePage::addIndicator(TechnicalIndicator indicator, const QVector<int>& params)
{
    d->indicators[indicator] = params;
    calculateIndicators();
}

void StockKLinePage::removeIndicator(TechnicalIndicator indicator)
{
    d->indicators.remove(indicator);
    d->indicatorValues.remove(indicator);
    
    // 从图表移除
    if (d->klineChart) {
        d->klineChart->removeIndicator(QString::number(static_cast<int>(indicator)));
    }
}

void StockKLinePage::clearIndicators()
{
    d->indicators.clear();
    d->indicatorValues.clear();
    
    if (d->klineChart) {
        d->klineChart->clearIndicators();
    }
}

// ============================================================================
// UI 初始化
// ============================================================================

void StockKLinePage::initUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // 工具栏
    initToolBar();
    
    // 主区域
    initMainArea();
    
    // 信息面板
    initInfoPanel();
    
    // 定时器
    d->refreshTimer = new QTimer(this);
    d->refreshTimer->setInterval(3000);
}

void StockKLinePage::initToolBar()
{
    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(40);
    toolbar->setStyleSheet("QWidget { background: #1a1a2e; }"
                          "QLabel { color: #ffffff; }"
                          "QComboBox { background: #2d2d44; color: #ffffff; padding: 4px 8px; border: 1px solid #3d3d5c; border-radius: 4px; }"
                          "QComboBox::drop-down { border: none; }"
                          "QComboBox QAbstractItemView { background: #2d2d44; color: #ffffff; selection-background-color: #4a4a6a; }"
                          "QPushButton { background: #2d2d44; color: #ffffff; padding: 4px 12px; border: 1px solid #3d3d5c; border-radius: 4px; }"
                          "QPushButton:hover { background: #3d3d5c; }"
                          "QLineEdit { background: #2d2d44; color: #ffffff; padding: 4px 8px; border: 1px solid #3d3d5c; border-radius: 4px; }");
    
    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(10, 5, 10, 5);
    layout->setSpacing(10);
    
    // 股票搜索
    d->searchEdit = new QLineEdit(toolbar);
    d->searchEdit->setPlaceholderText(QStringLiteral("输入股票代码/名称"));
    d->searchEdit->setFixedWidth(150);
    layout->addWidget(d->searchEdit);
    
    // 股票名称
    d->stockNameLabel = new QLabel(QStringLiteral("请输入股票代码"), toolbar);
    d->stockNameLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    d->stockNameLabel->setMinimumWidth(120);
    layout->addWidget(d->stockNameLabel);
    
    // 价格信息
    d->priceLabel = new QLabel("--", toolbar);
    d->priceLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ff4d4f;");
    d->priceLabel->setMinimumWidth(80);
    layout->addWidget(d->priceLabel);
    
    d->changeLabel = new QLabel("--", toolbar);
    d->changeLabel->setMinimumWidth(80);
    layout->addWidget(d->changeLabel);
    
    layout->addStretch();
    
    // 周期选择
    layout->addWidget(new QLabel(QStringLiteral("周期:"), toolbar));
    d->periodCombo = new QComboBox(toolbar);
    d->periodCombo->addItems({QStringLiteral("1分"), QStringLiteral("5分"), 
                              QStringLiteral("15分"), QStringLiteral("30分"), 
                              QStringLiteral("60分"), QStringLiteral("日K"), 
                              QStringLiteral("周K"), QStringLiteral("月K")});
    d->periodCombo->setCurrentIndex(5);  // 默认日K
    d->periodCombo->setFixedWidth(60);
    layout->addWidget(d->periodCombo);
    
    // 复权选择
    layout->addWidget(new QLabel(QStringLiteral("复权:"), toolbar));
    d->adjustCombo = new QComboBox(toolbar);
    d->adjustCombo->addItems({QStringLiteral("不复权"), QStringLiteral("前复权"), QStringLiteral("后复权")});
    d->adjustCombo->setCurrentIndex(1);  // 默认前复权
    d->adjustCombo->setFixedWidth(70);
    layout->addWidget(d->adjustCombo);
    
    // 指标选择
    layout->addWidget(new QLabel(QStringLiteral("指标:"), toolbar));
    d->indicatorCombo = new QComboBox(toolbar);
    d->indicatorCombo->addItems({QStringLiteral("无"), QStringLiteral("MA"), 
                                 QStringLiteral("EMA"), QStringLiteral("MACD"), 
                                 QStringLiteral("KDJ"), QStringLiteral("BOLL"), 
                                 QStringLiteral("RSI")});
    d->indicatorCombo->setFixedWidth(70);
    layout->addWidget(d->indicatorCombo);
    
    // 刷新按钮
    d->refreshBtn = new QPushButton(QStringLiteral("刷新"), toolbar);
    d->refreshBtn->setFixedWidth(60);
    layout->addWidget(d->refreshBtn);
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(toolbar);
}

void StockKLinePage::initMainArea()
{
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // K线图
    d->klineChart = new KLineChart(splitter);
    d->klineChart->setMinimumWidth(600);
    
    // 设置K线样式（中国习惯：红涨绿跌）
    KLineStyle style;
    style.upColor = QColor("#ff4d4f");      // 红色上涨
    style.downColor = QColor("#00b578");    // 绿色下跌
    style.flatColor = QColor("#888888");    // 灰色平盘
    style.candleWidth = 8;
    style.candleSpacing = 2;
    style.showVolume = true;
    style.volumeHeightRatio = 0.25;
    d->klineChart->setStyle(style);
    
    splitter->addWidget(d->klineChart);
    
    // 信息面板
    d->infoTable = new QTableWidget(splitter);
    d->infoTable->setColumnCount(2);
    d->infoTable->setHorizontalHeaderLabels({QStringLiteral("项目"), QStringLiteral("数值")});
    d->infoTable->horizontalHeader()->setStretchLastSection(true);
    d->infoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->infoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->infoTable->setFixedWidth(200);
    
    // 添加信息项
    QStringList items = {QStringLiteral("开盘"), QStringLiteral("最高"), QStringLiteral("最低"),
                        QStringLiteral("收盘"), QStringLiteral("成交量"), QStringLiteral("成交额"),
                        QStringLiteral("换手率"), QStringLiteral("振幅"), QStringLiteral("市盈率"),
                        QStringLiteral("市净率")};
    d->infoTable->setRowCount(items.size());
    for (int i = 0; i < items.size(); ++i) {
        d->infoTable->setItem(i, 0, new QTableWidgetItem(items[i]));
        d->infoTable->setItem(i, 1, new QTableWidgetItem("--"));
    }
    
    splitter->addWidget(d->infoTable);
    splitter->setSizes({800, 200});
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(splitter, 1);
}

void StockKLinePage::initInfoPanel()
{
    // 信息面板已在 initMainArea 中创建
}

void StockKLinePage::initConnections()
{
    // 周期切换
    connect(d->periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockKLinePage::onPeriodChanged);
    
    // 复权切换
    connect(d->adjustCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockKLinePage::onAdjustChanged);
    
    // 指标切换
    connect(d->indicatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockKLinePage::onIndicatorChanged);
    
    // 刷新按钮
    connect(d->refreshBtn, &QPushButton::clicked,
            this, &StockKLinePage::onRefresh);
    
    // 搜索框
    connect(d->searchEdit, &QLineEdit::returnPressed,
            this, [this]() {
                onSearchStock(d->searchEdit->text());
            });
    
    // K线图十字光标
    connect(d->klineChart, &KLineChart::crosshairMoved,
            this, &StockKLinePage::onCrosshairMoved);
    
    // 定时刷新
    connect(d->refreshTimer, &QTimer::timeout,
            this, &StockKLinePage::onTimerRefresh);
}

// ============================================================================
// 槽函数
// ============================================================================

void StockKLinePage::onPeriodChanged(int index)
{
    setPeriod(static_cast<StockKLinePeriod>(index));
}

void StockKLinePage::onAdjustChanged(int index)
{
    setAdjustType(index);
}

void StockKLinePage::onIndicatorChanged(int index)
{
    // 先清除之前的指标
    clearIndicators();
    
    TechnicalIndicator indicator = static_cast<TechnicalIndicator>(index);
    if (indicator != TechnicalIndicator::None) {
        addIndicator(indicator);
    }
}

void StockKLinePage::onRefresh()
{
    loadKLineData();
}

void StockKLinePage::onSearchStock(const QString& keyword)
{
    if (keyword.isEmpty()) {
        return;
    }
    
    // TODO: 实现股票搜索
    // 这里应该调用股票搜索服务
    LOG_INFO(QString("Search stock: %1").arg(keyword));
    
    // 临时设置股票代码
    setStock(keyword, keyword.startsWith("6") ? "SH" : "SZ");
}

void StockKLinePage::onKLineDataUpdated()
{
    if (d->klineChart && !d->klineData.isEmpty()) {
        d->klineChart->setData(d->klineData);
    }
}

void StockKLinePage::onCrosshairMoved(const QDateTime& time, double price)
{
    // 查找对应的成交量
    qint64 volume = 0;
    for (const auto& kline : d->klineData) {
        if (kline.time == time) {
            volume = kline.volume;
            break;
        }
    }
    
    emit crosshairMoved(time, price, volume);
    
    // 更新信息面板
    updateInfoPanel(time, price, volume);
}

void StockKLinePage::onTimerRefresh()
{
    // 实时行情刷新
    if (!d->stockCode.isEmpty()) {
        // TODO: 调用行情服务获取最新数据
        // 这里应该更新最后一条K线
    }
}

// ============================================================================
// 事件处理
// ============================================================================

void StockKLinePage::showEvent(QShowEvent* event)
{
    BasePage::showEvent(event);
    
    if (d->refreshTimer) {
        d->refreshTimer->start(3000);
    }
}

void StockKLinePage::hideEvent(QHideEvent* event)
{
    BasePage::hideEvent(event);
    
    if (d->refreshTimer) {
        d->refreshTimer->stop();
    }
}

void StockKLinePage::keyPressEvent(QKeyEvent* event)
{
    // 快捷键处理
    switch (event->key()) {
    case Qt::Key_F5:
        // 刷新
        onRefresh();
        break;
    case Qt::Key_Left:
        // 向左移动
        if (d->klineChart) {
            d->klineChart->pan(-50);
        }
        break;
    case Qt::Key_Right:
        // 向右移动
        if (d->klineChart) {
            d->klineChart->pan(50);
        }
        break;
    case Qt::Key_Up:
        // 放大
        if (d->klineChart) {
            d->klineChart->zoom(1.2);
        }
        break;
    case Qt::Key_Down:
        // 缩小
        if (d->klineChart) {
            d->klineChart->zoom(0.8);
        }
        break;
    default:
        BasePage::keyPressEvent(event);
    }
}

// ============================================================================
// 数据加载
// ============================================================================

void StockKLinePage::loadKLineData()
{
    if (d->stockCode.isEmpty()) {
        return;
    }
    
    // TODO: 调用数据服务加载K线数据
    // 这里使用模拟数据进行测试
    
    d->klineData.clear();
    
    // 生成模拟数据
    QDateTime now = QDateTime::currentDateTime();
    double basePrice = 10.0 + QRandomGenerator::global()->bounded(100);
    
    for (int i = 0; i < 200; ++i) {
        KLineData kline;
        kline.time = now.addDays(-200 + i);
        
        // 模拟价格波动
        double change = (QRandomGenerator::global()->bounded(100) - 50) / 1000.0;  // -5% ~ +5%
        double open = basePrice * (1 + change);
        double close = open * (1 + (QRandomGenerator::global()->bounded(100) - 50) / 1000.0);
        double high = qMax(open, close) * (1 + QRandomGenerator::global()->bounded(50) / 1000.0);
        double low = qMin(open, close) * (1 - QRandomGenerator::global()->bounded(50) / 1000.0);
        
        kline.open = open;
        kline.close = close;
        kline.high = high;
        kline.low = low;
        kline.volume = 1000000 + QRandomGenerator::global()->bounded(5000000);
        kline.turnover = kline.volume * (open + close) / 2;
        
        d->klineData.append(kline);
        basePrice = close;
    }
    
    // 更新图表
    if (d->klineChart) {
        d->klineChart->setData(d->klineData);
        d->klineChart->showLatest(100);
    }
    
    // 计算技术指标
    calculateIndicators();
    
    LOG_DEBUG(QString("KLine data loaded: %1, count: %2")
        .arg(d->stockCode).arg(d->klineData.size()));
}

void StockKLinePage::loadStockInfo()
{
    if (d->stockCode.isEmpty()) {
        return;
    }
    
    // TODO: 调用数据服务加载股票信息
    // 这里使用模拟数据
    
    d->stockName = d->stockCode + QStringLiteral(" 股票");
    d->lastPrice = 10.0 + QRandomGenerator::global()->bounded(100);
    d->prevClose = d->lastPrice * (1 - (QRandomGenerator::global()->bounded(100) - 50) / 1000.0);
    
    updateStockInfo();
}

void StockKLinePage::calculateIndicators()
{
    if (d->klineData.isEmpty()) {
        return;
    }
    
    // 提取收盘价
    QVector<double> closes;
    for (const auto& kline : d->klineData) {
        closes.append(kline.close);
    }
    
    // 计算各指标
    for (auto it = d->indicators.begin(); it != d->indicators.end(); ++it) {
        TechnicalIndicator indicator = it.key();
        const QVector<int>& params = it.value();
        
        switch (indicator) {
        case TechnicalIndicator::MA: {
            int period = params.isEmpty() ? 5 : params[0];
            QVector<double> ma = calculateMA(closes, period);
            if (d->klineChart) {
                d->klineChart->addIndicator(QStringLiteral("MA%1").arg(period), ma, QColor("#ff9800"));
            }
            break;
        }
        case TechnicalIndicator::EMA: {
            int period = params.isEmpty() ? 12 : params[0];
            QVector<double> ema = calculateEMA(closes, period);
            if (d->klineChart) {
                d->klineChart->addIndicator(QStringLiteral("EMA%1").arg(period), ema, QColor("#2196f3"));
            }
            break;
        }
        case TechnicalIndicator::MACD: {
            QVector<double> dif, dea, macd;
            calculateMACD(closes, 12, 26, 9, dif, dea, macd);
            if (d->klineChart) {
                d->klineChart->addIndicator(QStringLiteral("DIF"), dif, QColor("#ff9800"));
                d->klineChart->addIndicator(QStringLiteral("DEA"), dea, QColor("#2196f3"));
            }
            break;
        }
        case TechnicalIndicator::BOLL: {
            QVector<double> mid, upper, lower;
            calculateBOLL(closes, 20, 2.0, mid, upper, lower);
            if (d->klineChart) {
                d->klineChart->addIndicator(QStringLiteral("BOLL-MID"), mid, QColor("#ff9800"));
                d->klineChart->addIndicator(QStringLiteral("BOLL-UPPER"), upper, QColor("#4caf50"));
                d->klineChart->addIndicator(QStringLiteral("BOLL-LOWER"), lower, QColor("#f44336"));
            }
            break;
        }
        case TechnicalIndicator::RSI: {
            int period = params.isEmpty() ? 14 : params[0];
            QVector<double> rsi = calculateRSI(closes, period);
            if (d->klineChart) {
                d->klineChart->addIndicator(QStringLiteral("RSI%1").arg(period), rsi, QColor("#9c27b0"));
            }
            break;
        }
        default:
            break;
        }
    }
}

// ============================================================================
// 技术指标计算
// ============================================================================

QVector<double> StockKLinePage::calculateMA(const QVector<double>& prices, int period)
{
    QVector<double> result;
    if (prices.size() < period) {
        return result;
    }
    
    result.resize(prices.size());
    
    // 前 period-1 个数据无法计算
    for (int i = 0; i < period - 1; ++i) {
        result[i] = 0;
    }
    
    // 计算第一个 MA
    double sum = 0;
    for (int i = 0; i < period; ++i) {
        sum += prices[i];
    }
    result[period - 1] = sum / period;
    
    // 后续使用滑动窗口
    for (int i = period; i < prices.size(); ++i) {
        sum = sum - prices[i - period] + prices[i];
        result[i] = sum / period;
    }
    
    return result;
}

QVector<double> StockKLinePage::calculateEMA(const QVector<double>& prices, int period)
{
    QVector<double> result;
    if (prices.isEmpty()) {
        return result;
    }
    
    result.resize(prices.size());
    
    // EMA 计算系数
    double k = 2.0 / (period + 1);
    
    // 第一个 EMA 使用第一个价格
    result[0] = prices[0];
    
    // 计算 EMA
    for (int i = 1; i < prices.size(); ++i) {
        result[i] = prices[i] * k + result[i - 1] * (1 - k);
    }
    
    return result;
}

void StockKLinePage::calculateMACD(const QVector<double>& prices, int fast, int slow, int signal,
                                   QVector<double>& dif, QVector<double>& dea, QVector<double>& macd)
{
    if (prices.size() < slow) {
        return;
    }
    
    // 计算 EMA
    QVector<double> emaFast = calculateEMA(prices, fast);
    QVector<double> emaSlow = calculateEMA(prices, slow);
    
    // 计算 DIF
    dif.resize(prices.size());
    for (int i = 0; i < prices.size(); ++i) {
        dif[i] = emaFast[i] - emaSlow[i];
    }
    
    // 计算 DEA（DIF 的 EMA）
    dea = calculateEMA(dif, signal);
    
    // 计算 MACD
    macd.resize(prices.size());
    for (int i = 0; i < prices.size(); ++i) {
        macd[i] = (dif[i] - dea[i]) * 2;
    }
}

void StockKLinePage::calculateKDJ(const QVector<double>& highs, const QVector<double>& lows,
                                  const QVector<double>& closes, int n, int m1, int m2,
                                  QVector<double>& k, QVector<double>& d, QVector<double>& j)
{
    int size = closes.size();
    if (size < n) {
        return;
    }
    
    k.resize(size);
    d.resize(size);
    j.resize(size);
    
    for (int i = 0; i < size; ++i) {
        if (i < n - 1) {
            k[i] = 50;
            d[i] = 50;
            j[i] = 50;
            continue;
        }
        
        // 计算 n 日内的最高价和最低价
        double highest = highs[i];
        double lowest = lows[i];
        for (int j = i - n + 1; j <= i; ++j) {
            highest = qMax(highest, highs[j]);
            lowest = qMin(lowest, lows[j]);
        }
        
        // 计算 RSV
        double rsv = (highest == lowest) ? 50 : (closes[i] - lowest) / (highest - lowest) * 100;
        
        // 计算 K、D、J
        k[i] = (2.0 / 3) * k[i - 1] + (1.0 / 3) * rsv;
        d[i] = (2.0 / 3) * d[i - 1] + (1.0 / 3) * k[i];
        j[i] = 3 * k[i] - 2 * d[i];
    }
}

void StockKLinePage::calculateBOLL(const QVector<double>& prices, int n, double k,
                                   QVector<double>& mid, QVector<double>& upper, QVector<double>& lower)
{
    int size = prices.size();
    if (size < n) {
        return;
    }
    
    mid.resize(size);
    upper.resize(size);
    lower.resize(size);
    
    // 计算 MA
    mid = calculateMA(prices, n);
    
    // 计算标准差
    for (int i = n - 1; i < size; ++i) {
        double sum = 0;
        for (int j = i - n + 1; j <= i; ++j) {
            sum += qPow(prices[j] - mid[i], 2);
        }
        double stdDev = qSqrt(sum / n);
        
        upper[i] = mid[i] + k * stdDev;
        lower[i] = mid[i] - k * stdDev;
    }
}

QVector<double> StockKLinePage::calculateRSI(const QVector<double>& prices, int period)
{
    QVector<double> result;
    if (prices.size() < period + 1) {
        return result;
    }
    
    result.resize(prices.size());
    
    // 计算价格变化
    QVector<double> changes;
    for (int i = 1; i < prices.size(); ++i) {
        changes.append(prices[i] - prices[i - 1]);
    }
    
    // 前 period 个数据无法计算
    for (int i = 0; i <= period; ++i) {
        result[i] = 50;  // 默认值
    }
    
    // 计算 RSI
    for (int i = period; i < changes.size(); ++i) {
        double upSum = 0;
        double downSum = 0;
        
        for (int j = i - period + 1; j <= i; ++j) {
            if (changes[j] > 0) {
                upSum += changes[j];
            } else {
                downSum += qAbs(changes[j]);
            }
        }
        
        if (upSum + downSum == 0) {
            result[i + 1] = 50;
        } else {
            result[i + 1] = upSum / (upSum + downSum) * 100;
        }
    }
    
    return result;
}

// ============================================================================
// UI 更新
// ============================================================================

void StockKLinePage::updateStockInfo()
{
    if (d->stockNameLabel) {
        d->stockNameLabel->setText(d->stockName);
    }
    
    if (d->priceLabel) {
        d->priceLabel->setText(QString::number(d->lastPrice, 'f', 2));
        
        // 根据涨跌设置颜色
        double change = d->lastPrice - d->prevClose;
        if (change > 0) {
            d->priceLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #ff4d4f;");
        } else if (change < 0) {
            d->priceLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #00b578;");
        } else {
            d->priceLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #888888;");
        }
    }
    
    if (d->changeLabel && d->prevClose > 0) {
        double change = d->lastPrice - d->prevClose;
        double changePercent = change / d->prevClose * 100;
        QString text = QString("%1%2%")
            .arg(change >= 0 ? "+" : "")
            .arg(changePercent, 0, 'f', 2);
        d->changeLabel->setText(text);
        
        if (change > 0) {
            d->changeLabel->setStyleSheet("color: #ff4d4f;");
        } else if (change < 0) {
            d->changeLabel->setStyleSheet("color: #00b578;");
        } else {
            d->changeLabel->setStyleSheet("color: #888888;");
        }
    }
}

void StockKLinePage::updateIndicatorPanel()
{
    // 更新指标面板（如果需要）
}

void StockKLinePage::updateInfoPanel(const QDateTime& time, double price, double volume)
{
    if (!d->infoTable || d->klineData.isEmpty()) {
        return;
    }
    
    // 查找对应的K线数据
    for (const auto& kline : d->klineData) {
        if (kline.time == time) {
            d->infoTable->item(0, 1)->setText(QString::number(kline.open, 'f', 2));
            d->infoTable->item(1, 1)->setText(QString::number(kline.high, 'f', 2));
            d->infoTable->item(2, 1)->setText(QString::number(kline.low, 'f', 2));
            d->infoTable->item(3, 1)->setText(QString::number(kline.close, 'f', 2));
            d->infoTable->item(4, 1)->setText(QString::number(kline.volume));
            d->infoTable->item(5, 1)->setText(QString::number(kline.turnover, 'f', 0));
            break;
        }
    }
}
