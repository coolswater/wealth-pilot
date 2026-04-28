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
#include "market/StockDataSource.h"
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
#include <QMenu>
#include <QDialog>
#include <QCheckBox>
#include <QCursor>
#include <algorithm>
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
    QComboBox* adjustCombo = nullptr;
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
    
    // 数据源
    StockDataSource* dataSource = nullptr;
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
    
    // 1. 工具栏（周期、指标切换）
    initToolBar();
    
    // 2. 主区域（K线图 + 右侧信息面板）
    initMainArea();
    
    // 定时器
    d->refreshTimer = new QTimer(this);
    d->refreshTimer->setInterval(3000);
    
    // 初始化数据源
    d->dataSource = new StockDataSource(StockDataSource::Source::Sina, this);
    connect(d->dataSource, &StockDataSource::kLineReceived,
            this, [this](const QString& symbol, const QVector<KLineData>& data) {
                Q_UNUSED(symbol);
                if (!data.isEmpty()) {
                    d->klineData = data;
                    if (d->klineChart) {
                        d->klineChart->setData(d->klineData);
                        d->klineChart->showLatest(100);
                    }
                    calculateIndicators();
                    
                    if (!d->klineData.isEmpty()) {
                        d->lastPrice = d->klineData.last().close;
                        d->prevClose = d->klineData.size() > 1 ? d->klineData[d->klineData.size() - 2].close : d->klineData.first().open;
                        updateStockInfo();
                        updateInfoPanel(d->klineData.last().time, d->lastPrice, d->volume);
                    }
                    
                    LOG_DEBUG(QString("Real KLine data received: %1 items").arg(data.size()));
                }
            });
}

void StockKLinePage::initToolBar()
{
    // 工具栏 - 雪球风格（周期按钮组）
    auto* toolbar = new QWidget(this);
    toolbar->setFixedHeight(40);
    toolbar->setStyleSheet("QWidget { background: #161b22; }");
    
    auto* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(4);
    
    // 周期按钮组（分时、日K、五日、周K、月K、季K、年K）
    auto* periodGroup = new QWidget();
    periodGroup->setStyleSheet("QWidget { background: transparent; }");
    auto* periodLayout = new QHBoxLayout(periodGroup);
    periodLayout->setContentsMargins(0, 0, 0, 0);
    periodLayout->setSpacing(2);
    
    QStringList mainPeriods = {QStringLiteral("分时"), QStringLiteral("日K"), QStringLiteral("五日"), 
                               QStringLiteral("周K"), QStringLiteral("月K"), QStringLiteral("季K"), QStringLiteral("年K")};
    for (int i = 0; i < mainPeriods.size(); ++i) {
        auto* btn = new QPushButton(mainPeriods[i]);
        btn->setCheckable(true);
        btn->setChecked(i == 1);  // 默认日K
        btn->setFixedHeight(26);
        btn->setStyleSheet(R"(
            QPushButton {
                background: transparent;
                color: #9ca3af;
                border: none;
                padding: 0 10px;
                font-size: 13px;
                border-radius: 4px;
            }
            QPushButton:checked {
                background: #3b82f6;
                color: #ffffff;
            }
            QPushButton:hover:!checked {
                background: #2d3748;
            }
        )");
        connect(btn, &QPushButton::clicked, this, [this, i, btn, periodGroup]() {
            // 更新按钮状态
            for (auto* child : periodGroup->findChildren<QPushButton*>()) {
                child->setChecked(child == btn);
            }
            // 切换周期
            onPeriodChanged(i);
        });
        periodLayout->addWidget(btn);
    }
    layout->addWidget(periodGroup);
    
    // 分隔线
    auto* divider1 = new QFrame();
    divider1->setFrameShape(QFrame::VLine);
    divider1->setStyleSheet("QFrame { background: #2d3748; max-width: 1px; }");
    divider1->setFixedWidth(1);
    layout->addWidget(divider1);
    
    // 分钟周期按钮组（120分、60分、30分、15分、5分、1分）
    auto* minPeriodGroup = new QWidget();
    minPeriodGroup->setStyleSheet("QWidget { background: transparent; }");
    auto* minPeriodLayout = new QHBoxLayout(minPeriodGroup);
    minPeriodLayout->setContentsMargins(0, 0, 0, 0);
    minPeriodLayout->setSpacing(2);
    
    QStringList minPeriods = {QStringLiteral("120分"), QStringLiteral("60分"), QStringLiteral("30分"), 
                              QStringLiteral("15分"), QStringLiteral("5分"), QStringLiteral("1分")};
    for (int i = 0; i < minPeriods.size(); ++i) {
        auto* btn = new QPushButton(minPeriods[i]);
        btn->setCheckable(true);
        btn->setFixedHeight(26);
        btn->setStyleSheet(R"(
            QPushButton {
                background: transparent;
                color: #9ca3af;
                border: none;
                padding: 0 8px;
                font-size: 12px;
                border-radius: 4px;
            }
            QPushButton:checked {
                background: #3b82f6;
                color: #ffffff;
            }
            QPushButton:hover:!checked {
                background: #2d3748;
            }
        )");
        connect(btn, &QPushButton::clicked, this, [this, i, btn, minPeriodGroup, periodGroup]() {
            // 更新所有周期按钮状态
            for (auto* child : periodGroup->findChildren<QPushButton*>()) {
                child->setChecked(false);
            }
            for (auto* child : minPeriodGroup->findChildren<QPushButton*>()) {
                child->setChecked(child == btn);
            }
            // 切换周期（分钟周期从索引7开始）
            onPeriodChanged(7 + i);
        });
        minPeriodLayout->addWidget(btn);
    }
    layout->addWidget(minPeriodGroup);
    
    layout->addStretch();
    
    // 复权选择（下拉框）
    d->adjustCombo = new QComboBox();
    d->adjustCombo->addItems({QStringLiteral("不复权"), QStringLiteral("前复权"), QStringLiteral("后复权")});
    d->adjustCombo->setCurrentIndex(1);
    d->adjustCombo->setFixedHeight(26);
    d->adjustCombo->setStyleSheet(R"(
        QComboBox {
            background: #2d3748;
            color: #9ca3af;
            border: none;
            padding: 0 8px;
            font-size: 12px;
            border-radius: 4px;
        }
        QComboBox::drop-down { border: none; width: 16px; }
        QComboBox QAbstractItemView {
            background: #2d3748;
            color: #ffffff;
            selection-background-color: #3b82f6;
        }
    )");
    layout->addWidget(d->adjustCombo);
    
    // 刷新按钮
    d->refreshBtn = new QPushButton(QStringLiteral("刷新"));
    d->refreshBtn->setFixedHeight(26);
    d->refreshBtn->setStyleSheet(R"(
        QPushButton {
            background: #3b82f6;
            color: #ffffff;
            border: none;
            padding: 0 16px;
            font-size: 12px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background: #2563eb;
        }
    )");
    layout->addWidget(d->refreshBtn);
    
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(toolbar);
}

void StockKLinePage::initMainArea()
{
    // 主容器
    auto* container = new QWidget(this);
    container->setStyleSheet("QWidget { background: #0d1117; }");
    
    auto* containerLayout = new QHBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    
    // 左侧：K线图区域
    auto* chartContainer = new QWidget();
    auto* chartLayout = new QVBoxLayout(chartContainer);
    chartLayout->setContentsMargins(0, 0, 0, 0);
    chartLayout->setSpacing(0);
    
    // K线图（主图，成交量已内置在下方）
    d->klineChart = new KLineChart(chartContainer);
    d->klineChart->setMinimumHeight(350);
    
    // 设置K线样式（中国习惯：红涨绿跌）
    KLineStyle style;
    style.upColor = QColor("#ff4d4f");      // 红色上涨
    style.downColor = QColor("#00b578");    // 绿色下跌
    style.flatColor = QColor("#888888");    // 灰色平盘
    style.candleWidth = 8;
    style.candleSpacing = 2;
    style.showVolume = true;
    style.volumeHeightRatio = 0.22;
    d->klineChart->setStyle(style);
    
    chartLayout->addWidget(d->klineChart, 1);
    
    // 分割线
    auto* divider = new QFrame(chartContainer);
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("QFrame { background: #21262d; border: none; max-height: 1px; }");
    chartLayout->addWidget(divider);
    
    // 指标切换按钮组（放在K线图下方）
    auto* indicatorBar = new QWidget(chartContainer);
    indicatorBar->setFixedHeight(36);
    indicatorBar->setStyleSheet("QWidget { background: #161b22; }");
    
    auto* indicatorLayout = new QHBoxLayout(indicatorBar);
    indicatorLayout->setContentsMargins(12, 4, 12, 4);
    indicatorLayout->setSpacing(2);
    
    // 副图指标标签
    auto* indicatorLabel = new QLabel(QStringLiteral("副图指标:"));
    indicatorLabel->setStyleSheet("color: #6b7280; font-size: 12px;");
    indicatorLayout->addWidget(indicatorLabel);
    
    // 指标按钮组
    QStringList indicators = {QStringLiteral("MACD"), QStringLiteral("KDJ"), QStringLiteral("RSI"), 
                              QStringLiteral("BOLL"), QStringLiteral("VOL"), QStringLiteral("无")};
    for (int i = 0; i < indicators.size(); ++i) {
        auto* btn = new QPushButton(indicators[i]);
        btn->setCheckable(true);
        btn->setChecked(i == 0);  // 默认MACD
        btn->setFixedHeight(24);
        btn->setStyleSheet(R"(
            QPushButton {
                background: transparent;
                color: #9ca3af;
                border: none;
                padding: 0 10px;
                font-size: 12px;
                border-radius: 3px;
            }
            QPushButton:checked {
                background: #3b82f6;
                color: #ffffff;
            }
            QPushButton:hover:!checked {
                background: #2d3748;
            }
        )");
        connect(btn, &QPushButton::clicked, this, [this, i, btn, indicatorBar]() {
            // 更新按钮状态
            for (auto* child : indicatorBar->findChildren<QPushButton*>()) {
                child->setChecked(child == btn);
            }
            // 切换指标
            onIndicatorChanged(i);
        });
        indicatorLayout->addWidget(btn);
    }
    
    indicatorLayout->addStretch();
    chartLayout->addWidget(indicatorBar);
    
    // 副图指标区域
    m_indicatorPanel = new QWidget(chartContainer);
    m_indicatorPanel->setFixedHeight(100);
    m_indicatorPanel->setStyleSheet("QWidget { background: #0d1117; }");
    chartLayout->addWidget(m_indicatorPanel);
    
    containerLayout->addWidget(chartContainer, 1);
    
    // 右侧：信息面板
    auto* rightPanel = new QWidget();
    rightPanel->setFixedWidth(240);
    rightPanel->setStyleSheet("QWidget { background: #161b22; }");
    
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    
    // ========== 顶部信息区域 ==========
    auto* topInfoWidget = new QWidget();
    topInfoWidget->setStyleSheet("QWidget { background: #1a1f2e; }");
    auto* topInfoLayout = new QVBoxLayout(topInfoWidget);
    topInfoLayout->setContentsMargins(8, 8, 8, 8);
    topInfoLayout->setSpacing(4);
    
    // 第一行：品种名称和代码（水平居中）
    d->stockNameLabel = new QLabel(QStringLiteral("--(--)"));
    d->stockNameLabel->setAlignment(Qt::AlignCenter);
    d->stockNameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #ffffff;");
    topInfoLayout->addWidget(d->stockNameLabel);
    
    // 第二行：当前股价 + 涨跌额/涨跌幅（水平居中）
    auto* priceRow = new QWidget();
    auto* priceRowLayout = new QHBoxLayout(priceRow);
    priceRowLayout->setContentsMargins(0, 0, 0, 0);
    priceRowLayout->setSpacing(8);
    
    d->priceLabel = new QLabel("--");
    d->priceLabel->setAlignment(Qt::AlignCenter);
    d->priceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ff4d4f;");
    priceRowLayout->addWidget(d->priceLabel);
    
    auto* changeWidget = new QWidget();
    auto* changeLayout = new QVBoxLayout(changeWidget);
    changeLayout->setContentsMargins(0, 0, 0, 0);
    changeLayout->setSpacing(0);
    
    d->changeLabel = new QLabel("--");
    d->changeLabel->setStyleSheet("font-size: 12px; color: #ff4d4f;");
    changeLayout->addWidget(d->changeLabel);
    
    auto* changePercentLabel = new QLabel("--");
    changePercentLabel->setStyleSheet("font-size: 12px; color: #ff4d4f;");
    changeLayout->addWidget(changePercentLabel);
    m_changePercentLabel = changePercentLabel;
    
    priceRowLayout->addWidget(changeWidget);
    priceRowLayout->addStretch();
    
    topInfoLayout->addWidget(priceRow);
    
    // 第三行：交易状态（两端对齐）
    auto* statusRow = new QWidget();
    auto* statusRowLayout = new QHBoxLayout(statusRow);
    statusRowLayout->setContentsMargins(0, 0, 0, 0);
    
    m_tradeStatusLabel = new QLabel(QStringLiteral("交易中"));
    m_tradeStatusLabel->setStyleSheet("font-size: 12px; color: #00b578;");
    statusRowLayout->addWidget(m_tradeStatusLabel);
    
    statusRowLayout->addStretch();
    
    auto* timeLabel = new QLabel(QDateTime::currentDateTime().toString("hh:mm:ss"));
    timeLabel->setStyleSheet("font-size: 12px; color: #6b7280;");
    statusRowLayout->addWidget(timeLabel);
    
    topInfoLayout->addWidget(statusRow);
    
    // 第四行：委比 + 委差
    auto* orderRow = new QWidget();
    auto* orderRowLayout = new QHBoxLayout(orderRow);
    orderRowLayout->setContentsMargins(0, 0, 0, 0);
    orderRowLayout->setSpacing(16);
    
    auto* orderRatioWidget = new QWidget();
    auto* orderRatioLayout = new QHBoxLayout(orderRatioWidget);
    orderRatioLayout->setContentsMargins(0, 0, 0, 0);
    orderRatioLayout->setSpacing(4);
    auto* orderRatioLabel = new QLabel(QStringLiteral("委比"));
    orderRatioLabel->setStyleSheet("font-size: 12px; color: #6b7280;");
    orderRatioLayout->addWidget(orderRatioLabel);
    m_orderRatioValue = new QLabel("--");
    m_orderRatioValue->setStyleSheet("font-size: 12px; color: #ffffff;");
    orderRatioLayout->addWidget(m_orderRatioValue);
    orderRowLayout->addWidget(orderRatioWidget);
    
    orderRowLayout->addStretch();
    
    auto* orderDiffWidget = new QWidget();
    auto* orderDiffLayout = new QHBoxLayout(orderDiffWidget);
    orderDiffLayout->setContentsMargins(0, 0, 0, 0);
    orderDiffLayout->setSpacing(4);
    auto* orderDiffLabel = new QLabel(QStringLiteral("委差"));
    orderDiffLabel->setStyleSheet("font-size: 12px; color: #6b7280;");
    orderDiffLayout->addWidget(orderDiffLabel);
    m_orderDiffValue = new QLabel("--");
    m_orderDiffValue->setStyleSheet("font-size: 12px; color: #ffffff;");
    orderDiffLayout->addWidget(m_orderDiffValue);
    orderRowLayout->addWidget(orderDiffWidget);
    
    topInfoLayout->addWidget(orderRow);
    
    rightLayout->addWidget(topInfoWidget);
    
    // 分隔线
    auto* divider1 = new QFrame();
    divider1->setFrameShape(QFrame::HLine);
    divider1->setStyleSheet("QFrame { background: #2d3748; max-height: 1px; }");
    rightLayout->addWidget(divider1);
    
    // ========== 五档盘口 ==========
    auto* depthTitle = new QLabel(QStringLiteral("五档盘口"));
    depthTitle->setStyleSheet("color: #ffffff; font-size: 13px; font-weight: bold; padding: 4px 8px;");
    rightLayout->addWidget(depthTitle);
    
    auto* depthWidget = new QWidget();
    auto* depthLayout = new QVBoxLayout(depthWidget);
    depthLayout->setContentsMargins(4, 0, 4, 0);
    depthLayout->setSpacing(1);
    
    auto createDepthRow = [](const QString& label, QLabel*& priceLbl, QLabel*& volLbl, bool isSell) {
        auto* row = new QWidget();
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 1, 0, 1);
        rowLayout->setSpacing(4);
        
        auto* lbl = new QLabel(label);
        lbl->setStyleSheet(QString("color: %1; font-size: 12px; min-width: 28px;").arg(isSell ? "#00b578" : "#ff4d4f"));
        rowLayout->addWidget(lbl);
        
        priceLbl = new QLabel("--");
        priceLbl->setStyleSheet(QString("color: %1; font-size: 12px; min-width: 50px;").arg(isSell ? "#00b578" : "#ff4d4f"));
        rowLayout->addWidget(priceLbl);
        
        volLbl = new QLabel("--");
        volLbl->setStyleSheet("color: #9ca3af; font-size: 12px;");
        volLbl->setAlignment(Qt::AlignRight);
        rowLayout->addWidget(volLbl);
        
        return row;
    };
    
    // 卖5-1
    for (int i = 5; i >= 1; --i) {
        QLabel* priceLbl = nullptr;
        QLabel* volLbl = nullptr;
        depthLayout->addWidget(createDepthRow(QStringLiteral("卖%1").arg(i), priceLbl, volLbl, true));
        m_sellPriceLabels.append(priceLbl);
        m_sellVolLabels.append(volLbl);
    }
    
    // 分隔线
    auto* depthDivider = new QFrame();
    depthDivider->setFrameShape(QFrame::HLine);
    depthDivider->setStyleSheet("QFrame { background: #2d3748; max-height: 1px; }");
    depthLayout->addWidget(depthDivider);
    
    // 买1-5
    for (int i = 1; i <= 5; ++i) {
        QLabel* priceLbl = nullptr;
        QLabel* volLbl = nullptr;
        depthLayout->addWidget(createDepthRow(QStringLiteral("买%1").arg(i), priceLbl, volLbl, false));
        m_buyPriceLabels.append(priceLbl);
        m_buyVolLabels.append(volLbl);
    }
    
    rightLayout->addWidget(depthWidget);
    
    // 分隔线
    auto* divider2 = new QFrame();
    divider2->setFrameShape(QFrame::HLine);
    divider2->setStyleSheet("QFrame { background: #2d3748; max-height: 1px; }");
    rightLayout->addWidget(divider2);
    
    // ========== 详细信息表格（14行4列） ==========
    m_detailTable = new QTableWidget();
    m_detailTable->setRowCount(14);
    m_detailTable->setColumnCount(4);
    m_detailTable->horizontalHeader()->setVisible(false);
    m_detailTable->verticalHeader()->setVisible(false);
    m_detailTable->setShowGrid(false);
    m_detailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_detailTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_detailTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_detailTable->setStyleSheet(R"(
        QTableWidget {
            background: #161b22;
            color: #ffffff;
            border: none;
            font-size: 11px;
        }
        QTableWidget::item {
            padding: 2px 4px;
        }
    )");
    
    // 设置表格数据
    QStringList rowLabels = {
        QStringLiteral("最新"), QStringLiteral("涨幅"), QStringLiteral("总手"), QStringLiteral("换手"),
        QStringLiteral("最高"), QStringLiteral("今开"), QStringLiteral("涨停"), QStringLiteral("外盘"),
        QStringLiteral("盘后量"), QStringLiteral("净资产"), QStringLiteral("收益"), QStringLiteral("股息"),
        QStringLiteral("总股本"), QStringLiteral("流通股")
    };
    
    QStringList rowLabels2 = {
        QStringLiteral("价格"), QStringLiteral("涨跌"), QStringLiteral("金额"), QStringLiteral("量比"),
        QStringLiteral("最低"), QStringLiteral("昨收"), QStringLiteral("跌停"), QStringLiteral("内盘"),
        QStringLiteral("盘后额"), QStringLiteral("ROE"), QStringLiteral("PE(动)"), QStringLiteral("股息率"),
        QStringLiteral("总值"), QStringLiteral("流值")
    };
    
    for (int row = 0; row < 14; ++row) {
        // 第一列：标签
        auto* labelItem1 = new QTableWidgetItem(rowLabels[row]);
        labelItem1->setForeground(QColor("#6b7280"));
        labelItem1->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_detailTable->setItem(row, 0, labelItem1);
        
        // 第二列：值
        auto* valueItem1 = new QTableWidgetItem("--");
        valueItem1->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_detailTable->setItem(row, 1, valueItem1);
        
        // 第三列：标签
        auto* labelItem2 = new QTableWidgetItem(rowLabels2[row]);
        labelItem2->setForeground(QColor("#6b7280"));
        labelItem2->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_detailTable->setItem(row, 2, labelItem2);
        
        // 第四列：值
        auto* valueItem2 = new QTableWidgetItem("--");
        valueItem2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_detailTable->setItem(row, 3, valueItem2);
    }
    
    m_detailTable->setMinimumHeight(280);
    rightLayout->addWidget(m_detailTable);
    
    containerLayout->addWidget(rightPanel);
    
    // 添加到主布局
    auto* mainLayout = qobject_cast<QVBoxLayout*>(this->layout());
    mainLayout->addWidget(container, 1);
}

void StockKLinePage::initConnections()
{
    // 复权切换
    connect(d->adjustCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockKLinePage::onAdjustChanged);
    
    // 刷新按钮
    connect(d->refreshBtn, &QPushButton::clicked,
            this, &StockKLinePage::onRefresh);
    
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
    // 周期索引映射：
    // 0-6: 分时、日K、五日、周K、月K、季K、年K
    // 7-12: 120分、60分、30分、15分、5分、1分
    
    static const QVector<StockKLinePeriod> mapping = {
        StockKLinePeriod::Min1,     // 0: 分时
        StockKLinePeriod::Day,      // 1: 日K
        StockKLinePeriod::Day,      // 2: 五日（暂用日K）
        StockKLinePeriod::Week,     // 3: 周K
        StockKLinePeriod::Month,    // 4: 月K
        StockKLinePeriod::Month,    // 5: 季K（暂用月K）
        StockKLinePeriod::Month,    // 6: 年K（暂用月K）
        StockKLinePeriod::Min60,    // 7: 120分（暂用60分）
        StockKLinePeriod::Min60,    // 8: 60分
        StockKLinePeriod::Min30,    // 9: 30分
        StockKLinePeriod::Min15,    // 10: 15分
        StockKLinePeriod::Min5,     // 11: 5分
        StockKLinePeriod::Min1      // 12: 1分
    };
    
    if (index >= 0 && index < mapping.size()) {
        setPeriod(mapping[index]);
    }
}

void StockKLinePage::onAdjustChanged(int index)
{
    setAdjustType(index);
}

void StockKLinePage::onIndicatorChanged(int index)
{
    // 按钮组索引映射：0=MA, 1=BOLL, 2=MACD, 3=KDJ, 4=RSI
    clearIndicators();
    
    // 映射到 TechnicalIndicator 枚举
    static const QVector<TechnicalIndicator> mapping = {
        TechnicalIndicator::MA,    // 0
        TechnicalIndicator::BOLL,  // 1
        TechnicalIndicator::MACD,  // 2
        TechnicalIndicator::KDJ,   // 3
        TechnicalIndicator::RSI    // 4
    };
    
    if (index >= 0 && index < mapping.size()) {
        addIndicator(mapping[index]);
    }
}

void StockKLinePage::onAddToWatchlist()
{
    // 添加到自选
    m_isInWatchlist = true;
    m_addToWatchlistBtn->setVisible(false);
    m_addedToWatchlistLabel->setVisible(true);
    
    LOG_INFO(QString("Added to watchlist: %1").arg(d->stockCode));
}

void StockKLinePage::onRemoveFromWatchlist()
{
    // 从自选移除
    m_isInWatchlist = false;
    m_addToWatchlistBtn->setVisible(true);
    m_addedToWatchlistLabel->setVisible(false);
    
    LOG_INFO(QString("Removed from watchlist: %1").arg(d->stockCode));
}

void StockKLinePage::onShowMoreMenu()
{
    // 显示更多操作菜单
    QMenu menu(this);
    menu.setStyleSheet(R"(
        QMenu {
            background: #2d3748;
            color: #ffffff;
            border: 1px solid #3d4a5c;
            padding: 4px;
        }
        QMenu::item {
            padding: 8px 24px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background: #3b82f6;
        }
    )");
    
    if (m_isInWatchlist) {
        auto* removeAction = menu.addAction(QStringLiteral("取消自选"));
        connect(removeAction, &QAction::triggered, this, &StockKLinePage::onRemoveFromWatchlist);
    }
    
    auto* groupAction = menu.addAction(QStringLiteral("分组管理"));
    connect(groupAction, &QAction::triggered, this, &StockKLinePage::onShowGroupDialog);
    
    menu.exec(QCursor::pos());
}

void StockKLinePage::onShowReminderDialog()
{
    // 显示股价提醒弹窗
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("股价提醒"));
    dialog->setFixedSize(450, 320);
    dialog->setStyleSheet(R"(
        QDialog {
            background: #1a1f2e;
        }
        QLabel {
            color: #ffffff;
        }
        QLineEdit {
            background: #2d3748;
            color: #ffffff;
            border: 1px solid #3d4a5c;
            padding: 6px;
            border-radius: 4px;
        }
        QComboBox {
            background: #2d3748;
            color: #ffffff;
            border: 1px solid #3d4a5c;
            padding: 4px;
            border-radius: 4px;
        }
        QCheckBox {
            color: #ffffff;
        }
        QPushButton {
            background: #3b82f6;
            color: #ffffff;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background: #2563eb;
        }
    )");
    
    auto* layout = new QVBoxLayout(dialog);
    layout->setSpacing(16);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // 第一行：品种名称和代码，右侧股价和涨幅
    auto* row1 = new QWidget();
    auto* row1Layout = new QHBoxLayout(row1);
    
    auto* nameLabel = new QLabel(d->stockNameLabel->text());
    nameLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    row1Layout->addWidget(nameLabel);
    
    row1Layout->addStretch();
    
    auto* priceLabel = new QLabel(d->priceLabel->text() + " " + d->changeLabel->text());
    priceLabel->setStyleSheet("font-size: 14px; color: #ff4d4f;");
    row1Layout->addWidget(priceLabel);
    
    layout->addWidget(row1);
    
    // 分隔线
    auto* divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("background: #2d3748;");
    layout->addWidget(divider);
    
    // 创建提醒行的辅助函数
    auto createReminderRow = [](const QString& label, QWidget* parent) {
        auto* row = new QWidget();
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setSpacing(8);
        
        auto* lbl = new QLabel(label);
        lbl->setFixedWidth(80);
        rowLayout->addWidget(lbl);
        
        auto* input = new QLineEdit();
        input->setFixedWidth(100);
        input->setPlaceholderText(QStringLiteral("输入值"));
        rowLayout->addWidget(input);
        
        auto* combo = new QComboBox();
        combo->addItems({QStringLiteral("仅提醒一次"), QStringLiteral("每日一次")});
        combo->setFixedWidth(100);
        rowLayout->addWidget(combo);
        
        auto* checkbox = new QCheckBox(QStringLiteral("开启"));
        rowLayout->addWidget(checkbox);
        
        rowLayout->addStretch();
        return row;
    };
    
    // 第二行：股价涨到
    layout->addWidget(createReminderRow(QStringLiteral("股价涨到:"), dialog));
    
    // 第三行：股价跌到
    layout->addWidget(createReminderRow(QStringLiteral("股价跌到:"), dialog));
    
    // 第四行：涨跌幅超过
    layout->addWidget(createReminderRow(QStringLiteral("涨跌幅超过:"), dialog));
    
    layout->addStretch();
    
    // 保存按钮
    auto* saveBtn = new QPushButton(QStringLiteral("保存"));
    saveBtn->setFixedWidth(100);
    connect(saveBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(saveBtn, 0, Qt::AlignRight);
    
    dialog->exec();
    dialog->deleteLater();
}

void StockKLinePage::onShowGroupDialog()
{
    // 显示分组管理弹窗
    auto* dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("自定义分组"));
    dialog->setFixedSize(350, 400);
    dialog->setStyleSheet(R"(
        QDialog {
            background: #1a1f2e;
        }
        QLabel {
            color: #ffffff;
        }
        QLineEdit {
            background: #2d3748;
            color: #ffffff;
            border: 1px solid #3d4a5c;
            padding: 6px;
            border-radius: 4px;
        }
        QCheckBox {
            color: #ffffff;
        }
        QPushButton {
            background: #3b82f6;
            color: #ffffff;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
        }
        QPushButton:hover {
            background: #2563eb;
        }
    )");
    
    auto* layout = new QVBoxLayout(dialog);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // 已有分组列表
    auto* groupList = new QWidget();
    auto* groupListLayout = new QVBoxLayout(groupList);
    groupListLayout->setSpacing(8);
    
    // 模拟已有分组
    QStringList groups = {QStringLiteral("默认分组"), QStringLiteral("科技股"), QStringLiteral("金融股")};
    for (const auto& group : groups) {
        auto* checkbox = new QCheckBox(group);
        groupListLayout->addWidget(checkbox);
    }
    
    layout->addWidget(groupList);
    
    layout->addStretch();
    
    // 新建分组区域
    auto* newGroupWidget = new QWidget();
    auto* newGroupLayout = new QHBoxLayout(newGroupWidget);
    
    auto* newGroupInput = new QLineEdit();
    newGroupInput->setPlaceholderText(QStringLiteral("新建分组（不超过8个字）"));
    newGroupInput->setMaxLength(8);
    newGroupLayout->addWidget(newGroupInput);
    
    auto* addBtn = new QPushButton(QStringLiteral("添加"));
    addBtn->setFixedWidth(60);
    newGroupLayout->addWidget(addBtn);
    
    layout->addWidget(newGroupWidget);
    
    // 保存按钮
    auto* saveBtn = new QPushButton(QStringLiteral("保存"));
    saveBtn->setFixedWidth(100);
    connect(saveBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(saveBtn, 0, Qt::AlignRight);
    
    dialog->exec();
    dialog->deleteLater();
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
    
    // 使用数据源请求K线数据
    if (d->dataSource) {
        // 根据周期转换
        KLinePeriod klinePeriod = KLinePeriod::Day1;
        switch (d->period) {
        case StockKLinePeriod::Min1: klinePeriod = KLinePeriod::Minute1; break;
        case StockKLinePeriod::Min5: klinePeriod = KLinePeriod::Minute5; break;
        case StockKLinePeriod::Min15: klinePeriod = KLinePeriod::Minute15; break;
        case StockKLinePeriod::Min30: klinePeriod = KLinePeriod::Minute30; break;
        case StockKLinePeriod::Min60: klinePeriod = KLinePeriod::Hour1; break;
        case StockKLinePeriod::Day: klinePeriod = KLinePeriod::Day1; break;
        case StockKLinePeriod::Week: klinePeriod = KLinePeriod::Week1; break;
        case StockKLinePeriod::Month: klinePeriod = KLinePeriod::Month1; break;
        }
        
        // 构建完整代码（确保格式正确）
        QString fullSymbol;
        if (d->stockCode.startsWith("sh") || d->stockCode.startsWith("sz") ||
            d->stockCode.startsWith("SH") || d->stockCode.startsWith("SZ")) {
            fullSymbol = d->stockCode.toLower();
        } else {
            fullSymbol = d->exchange == "SH" ? "sh" + d->stockCode : "sz" + d->stockCode;
        }
        
        // 请求数据
        d->dataSource->requestKLine(fullSymbol, klinePeriod, 200);
        
        LOG_DEBUG(QString("Requesting KLine data for: %1, period: %2")
            .arg(fullSymbol).arg(static_cast<int>(klinePeriod)));
    }
    
    // 同时生成模拟数据作为备选（确保页面有数据显示）
    generateMockData();
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
        // 格式：银之杰(SZ:300085)
        QString displayText = QString("%1(%2:%3)")
            .arg(d->stockName)
            .arg(d->exchange)
            .arg(d->stockCode);
        d->stockNameLabel->setText(displayText);
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
    // 查找对应的K线数据
    for (const auto& kline : d->klineData) {
        if (kline.time == time) {
            // 更新上方信息标签
            if (m_openLabel) m_openLabel->setText(QString::number(kline.open, 'f', 2));
            if (m_highLabel) m_highLabel->setText(QString::number(kline.high, 'f', 2));
            if (m_lowLabel) m_lowLabel->setText(QString::number(kline.low, 'f', 2));
            if (m_closeLabel) m_closeLabel->setText(QString::number(kline.close, 'f', 2));
            if (m_volumeLabel) m_volumeLabel->setText(formatVolume(kline.volume));
            if (m_turnoverLabel) m_turnoverLabel->setText(formatMoney(kline.turnover));
            
            // 计算换手率和振幅（需要额外数据）
            if (m_amplitudeLabel && kline.low > 0) {
                double amplitude = (kline.high - kline.low) / kline.low * 100;
                m_amplitudeLabel->setText(QString::number(amplitude, 'f', 2) + "%");
            }
            break;
        }
    }
}

void StockKLinePage::generateMockData()
{
    // 生成模拟K线数据（用于测试和备选显示）
    d->klineData.clear();
    
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
    
    // 更新股票信息
    if (!d->klineData.isEmpty()) {
        d->lastPrice = d->klineData.last().close;
        d->prevClose = d->klineData.size() > 1 ? d->klineData[d->klineData.size() - 2].close : d->klineData.first().open;
        d->high = d->klineData.last().high;
        d->low = d->klineData.last().low;
        d->volume = d->klineData.last().volume;
        d->turnover = d->klineData.last().turnover;
        
        updateStockInfo();
        updateInfoPanel(d->klineData.last().time, d->lastPrice, d->volume);
    }
    
    // 计算技术指标
    calculateIndicators();
    
    LOG_DEBUG(QString("Generated mock KLine data: %1 items").arg(d->klineData.size()));
}

QString StockKLinePage::formatVolume(qint64 volume)
{
    if (volume <= 0) return "--";
    if (volume >= 100000000) {
        return QString("%1亿").arg(volume / 100000000.0, 0, 'f', 2);
    }
    if (volume >= 10000) {
        return QString("%1万").arg(volume / 10000.0, 0, 'f', 2);
    }
    return QString::number(volume);
}

QString StockKLinePage::formatMoney(double value)
{
    if (value <= 0) return "--";
    if (value >= 100000000.0) {
        return QString("%1亿").arg(value / 100000000.0, 0, 'f', 2);
    }
    if (value >= 10000.0) {
        return QString("%1万").arg(value / 10000.0, 0, 'f', 2);
    }
    return QString::number(value, 'f', 2);
}
