/**
 * @file PortfolioPage.cpp
 * @brief 持仓页面实现 - 专业级投资组合管理
 *
 * @details 性能优化：
 * - 使用 Model/View 架构，支持大数据量
 * - 数据缓存和增量更新
 * - 延迟加载和按需刷新
 * - 使用 QHash 快速索引
 *
 * @details 设计规范：
 * - 主背景：#1E1F24
 * - 卡片背景：#2C2D33，圆角8px
 * - 涨：红色 #FF3B30，跌：绿色 #34C759
 */

#include "PortfolioPage.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTabWidget>
#include <QSplitter>
#include <QTableView>
#include <QHeaderView>
#include <QTimer>
#include <QRandomGenerator>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QProgressBar>
#include <QFrame>
#include <QScrollArea>
#include <QDateTime>
#include <QRegularExpression>

#include <QPieSeries>
#include <QPieSlice>
#include <QLineSeries>
#include <QCategoryAxis>
#include <QDateTimeAxis>
#include <QChart>
#include <QChartView>

// ============================================================================
// 图表专用颜色 - 使用 Tokens
// ============================================================================

namespace PortfolioColors {
    const QString Stock = Tokens::Colors::ChartOrange;   // 橙色
    const QString Futures = Tokens::Colors::ChartPurple; // 紫色
    const QString MyProfit = Tokens::Colors::Danger;  // 红色
    const QString Benchmark = Tokens::Colors::Primary; // 蓝色
}

// ============================================================================
// 涨跌颜色委托 - 高性能绘制
// ============================================================================

class PnLDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        double value = index.data(Qt::UserRole).toDouble();
        
        QColor textColor;
        if (value > 0) {
            textColor = QColor(Tokens::Colors::Danger);
        } else if (value < 0) {
            textColor = QColor(Tokens::Colors::Success);
        } else {
            textColor = QColor(Tokens::Colors::TextSecondary);
        }

        // 绘制背景
        painter->save();
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor(Tokens::Colors::Primary));
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor(Tokens::Colors::BgHover));
        }
        painter->restore();

        // 绘制文字
        QStyleOptionViewItem opt = option;
        opt.palette.setColor(QPalette::Text, textColor);
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

// ============================================================================
// PIMPL 实现
// ============================================================================

struct PortfolioPage::Impl {
    // 主布局
    QVBoxLayout* mainLayout = nullptr;
    QSplitter* mainSplitter = nullptr;

    // 头部组件
    QLineEdit* searchEdit = nullptr;
    QLabel* updateTimeLabel = nullptr;

    // 汇总卡片
    QFrame* totalAssetCard = nullptr;
    QLabel* totalAssetLabel = nullptr;
    QLabel* totalAssetChangeLabel = nullptr;
    
    QFrame* dailyPnLCard = nullptr;
    QLabel* dailyPnLLabel = nullptr;
    QLabel* dailyPnLDetailLabel = nullptr;
    
    QFrame* returnCard = nullptr;
    QLabel* returnLabel = nullptr;
    QLabel* returnDetailLabel = nullptr;
    
    QFrame* riskCard = nullptr;
    QLabel* riskLabel = nullptr;
    QProgressBar* riskBar = nullptr;

    // 资产配置
    QChartView* pieChartView = nullptr;
    QPieSeries* pieSeries = nullptr;
    QFrame* allocationList = nullptr;
    QVector<AssetAllocation> allocations;

    // 净值走势
    QChartView* lineChartView = nullptr;
    QLineSeries* profitSeries = nullptr;
    QLineSeries* benchmarkSeries = nullptr;
    QComboBox* timeRangeCombo = nullptr;

    // 持仓表格
    QTabWidget* positionTabs = nullptr;
    QTableView* stockTable = nullptr;
    QTableView* futuresTable = nullptr;
    QTableView* fundTable = nullptr;
    PositionTableModel* stockModel = nullptr;
    PositionTableModel* futuresModel = nullptr;
    PositionTableModel* fundModel = nullptr;

    // 定时器
    QTimer* updateTimer = nullptr;

    // 数据缓存
    AccountSummary summary;
    QHash<QString, PositionData> positionCache;
};

// ============================================================================
// PositionTableModel 实现
// ============================================================================

PositionTableModel::PositionTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int PositionTableModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int PositionTableModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

QVariant PositionTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size()) {
        return QVariant();
    }

    const PositionData& pos = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColInstrument:
                return QString("%1 %2").arg(pos.instrumentId, pos.instrumentName);
            case ColVolume:
                return pos.volume;
            case ColAvgPrice:
                return QString::number(pos.avgPrice, 'f', 2);
            case ColCurrentPrice:
                return QString::number(pos.currentPrice, 'f', 2);
            case ColMarketValue:
                return QString::number(pos.marketValue / 10000.0, 'f', 2) + "万";
            case ColPnL:
                return QString::number(pos.profitLoss, 'f', 0);
            case ColPnLPercent:
                return QString::number(pos.profitLossPercent, 'f', 2) + "%";
            default:
                return QVariant();
        }
    }

    if (role == Qt::UserRole) {
        switch (index.column()) {
            case ColPnL:
            case ColPnLPercent:
                return pos.profitLoss;
            default:
                return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
            case ColInstrument:
                return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
            default:
                return QVariant(Qt::AlignRight | Qt::AlignVCenter);
        }
    }

    return QVariant();
}

QVariant PositionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    static const QStringList headers = {
        QStringLiteral("合约"),
        QStringLiteral("持仓"),
        QStringLiteral("均价"),
        QStringLiteral("现价"),
        QStringLiteral("市值"),
        QStringLiteral("盈亏"),
        QStringLiteral("盈亏%")
    };

    return headers.value(section);
}

void PositionTableModel::setData(const QVector<PositionData>& data)
{
    beginResetModel();
    m_data = data;
    m_indexMap.clear();
    for (int i = 0; i < data.size(); ++i) {
        m_indexMap[data[i].instrumentId] = i;
    }
    endResetModel();
}

void PositionTableModel::updatePrice(const QString& instrumentId, double price)
{
    auto it = m_indexMap.find(instrumentId);
    if (it != m_indexMap.end()) {
        int row = it.value();
        m_data[row].currentPrice = price;
        m_data[row].marketValue = m_data[row].volume * price;
        m_data[row].profitLoss = (price - m_data[row].avgPrice) * m_data[row].volume;
        m_data[row].profitLossPercent = (price - m_data[row].avgPrice) / m_data[row].avgPrice * 100;
        
        emit dataChanged(index(row, ColCurrentPrice), index(row, ColPnLPercent));
    }
}

void PositionTableModel::clear()
{
    beginResetModel();
    m_data.clear();
    m_indexMap.clear();
    endResetModel();
}

AccountSummary PositionTableModel::calculateSummary() const
{
    AccountSummary summary;
    for (const auto& pos : m_data) {
        summary.marketValue += pos.marketValue;
        summary.dailyPnL += pos.profitLoss;
        summary.totalPnL += pos.profitLoss;
    }
    return summary;
}

// ============================================================================
// PortfolioPage 实现
// ============================================================================

PortfolioPage::PortfolioPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setObjectName("PortfolioPage");

    // 实时更新定时器
    d->updateTimer = new QTimer(this);
    d->updateTimer->setInterval(3000);
    connect(d->updateTimer, &QTimer::timeout, this, &PortfolioPage::updateRealTimeData);
}

PortfolioPage::~PortfolioPage() = default;

void PortfolioPage::initializePage()
{
    if (isInitialized()) return;

    setupConnections();
    loadDemoData();

    setInitialized(true);

    LOG_DEBUG("PortfolioPage initialized");
}

void PortfolioPage::setupUI()
{
    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setContentsMargins(20, 20, 20, 20);
    d->mainLayout->setSpacing(16);

    // 设置背景
    setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgBase));

    // 1. 头部
    setupHeader();

    // 2. 汇总卡片
    setupSummaryCards();

    // 3. 主内容区
    setupMainContent();

    // 4. 持仓表格
    setupPositionTable();
}

void PortfolioPage::setupHeader()
{
    QFrame* header = new QFrame(this);
    header->setStyleSheet(QString("background: transparent;"));
    QHBoxLayout* layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    // 页面标题
    QLabel* titleLabel = new QLabel(QStringLiteral("我的看板"), this);
    titleLabel->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1;")
        .arg(Tokens::Colors::TextPrimary));
    layout->addWidget(titleLabel);

    layout->addStretch();

    // 更新时间
    d->updateTimeLabel = new QLabel(this);
    d->updateTimeLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
        .arg(Tokens::Colors::TextTertiary));
    layout->addWidget(d->updateTimeLabel);

    layout->addSpacing(20);

    // 搜索框
    d->searchEdit = new QLineEdit(this);
    d->searchEdit->setPlaceholderText(QStringLiteral("搜索股票、基金、期货..."));
    d->searchEdit->setFixedWidth(280);
    d->searchEdit->setFixedHeight(36);
    d->searchEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: %1;
            border: 1px solid #3A3B41;
            border-radius: 8px;
            padding: 0 12px;
            color: %2;
            font-size: 14px;
        }
        QLineEdit::placeholder {
            color: %3;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, Tokens::Colors::TextTertiary));
    layout->addWidget(d->searchEdit);

    // 刷新按钮
    QPushButton* refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    refreshBtn->setFixedSize(80, 36);
    refreshBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            border: none;
            border-radius: 8px;
            color: white;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #2563EB;
        }
    )").arg(Tokens::Colors::Primary));
    connect(refreshBtn, &QPushButton::clicked, this, &PortfolioPage::refreshData);
    layout->addWidget(refreshBtn);

    d->mainLayout->addWidget(header);
}

void PortfolioPage::setupSummaryCards()
{
    QFrame* cardsFrame = new QFrame(this);
    cardsFrame->setStyleSheet(QString("background: transparent;"));
    QHBoxLayout* layout = new QHBoxLayout(cardsFrame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    // 卡片样式
    QString cardStyle = QString(R"(
        QFrame {
            background-color: %1;
            border-radius: 8px;
        }
    )").arg(Tokens::Colors::BgElevated);

    // 1. 总资产卡片
    d->totalAssetCard = new QFrame(this);
    d->totalAssetCard->setStyleSheet(cardStyle);
    d->totalAssetCard->setMinimumHeight(100);
    QVBoxLayout* totalLayout = new QVBoxLayout(d->totalAssetCard);
    totalLayout->setContentsMargins(20, 16, 20, 16);
    totalLayout->setSpacing(8);
    
    QLabel* totalTitle = new QLabel(QStringLiteral("总资产"), d->totalAssetCard);
    totalTitle->setStyleSheet(QString("color: %1; font-size: 14px;").arg(Tokens::Colors::TextSecondary));
    totalLayout->addWidget(totalTitle);
    
    d->totalAssetLabel = new QLabel(QStringLiteral("¥0"), d->totalAssetCard);
    d->totalAssetLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::TextPrimary));
    totalLayout->addWidget(d->totalAssetLabel);
    
    d->totalAssetChangeLabel = new QLabel(QStringLiteral("较昨日 +¥0"), d->totalAssetCard);
    d->totalAssetChangeLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::TextTertiary));
    totalLayout->addWidget(d->totalAssetChangeLabel);
    
    layout->addWidget(d->totalAssetCard, 1);

    // 2. 今日盈亏卡片
    d->dailyPnLCard = new QFrame(this);
    d->dailyPnLCard->setStyleSheet(cardStyle);
    d->dailyPnLCard->setMinimumHeight(100);
    QVBoxLayout* pnlLayout = new QVBoxLayout(d->dailyPnLCard);
    pnlLayout->setContentsMargins(20, 16, 20, 16);
    pnlLayout->setSpacing(8);
    
    QLabel* pnlTitle = new QLabel(QStringLiteral("今日盈亏"), d->dailyPnLCard);
    pnlTitle->setStyleSheet(QString("color: %1; font-size: 14px;").arg(Tokens::Colors::TextSecondary));
    pnlLayout->addWidget(pnlTitle);
    
    d->dailyPnLLabel = new QLabel(QStringLiteral("+¥0"), d->dailyPnLCard);
    d->dailyPnLLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Danger));
    pnlLayout->addWidget(d->dailyPnLLabel);
    
    d->dailyPnLDetailLabel = new QLabel(QStringLiteral("股票 +¥0 | 期货 +¥0"), d->dailyPnLCard);
    d->dailyPnLDetailLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::TextTertiary));
    pnlLayout->addWidget(d->dailyPnLDetailLabel);
    
    layout->addWidget(d->dailyPnLCard, 1);

    // 3. 持仓收益率卡片
    d->returnCard = new QFrame(this);
    d->returnCard->setStyleSheet(cardStyle);
    d->returnCard->setMinimumHeight(100);
    QVBoxLayout* returnLayout = new QVBoxLayout(d->returnCard);
    returnLayout->setContentsMargins(20, 16, 20, 16);
    returnLayout->setSpacing(8);
    
    QLabel* returnTitle = new QLabel(QStringLiteral("持仓收益率"), d->returnCard);
    returnTitle->setStyleSheet(QString("color: %1; font-size: 14px;").arg(Tokens::Colors::TextSecondary));
    returnLayout->addWidget(returnTitle);
    
    d->returnLabel = new QLabel(QStringLiteral("+0.00%"), d->returnCard);
    d->returnLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Danger));
    returnLayout->addWidget(d->returnLabel);
    
    d->returnDetailLabel = new QLabel(QStringLiteral("沪深300同期 +10.6%"), d->returnCard);
    d->returnDetailLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::TextTertiary));
    returnLayout->addWidget(d->returnDetailLabel);
    
    layout->addWidget(d->returnCard, 1);

    // 4. 期货风险度卡片
    d->riskCard = new QFrame(this);
    d->riskCard->setStyleSheet(cardStyle);
    d->riskCard->setMinimumHeight(100);
    QVBoxLayout* riskLayout = new QVBoxLayout(d->riskCard);
    riskLayout->setContentsMargins(20, 16, 20, 16);
    riskLayout->setSpacing(8);
    
    QLabel* riskTitle = new QLabel(QStringLiteral("期货风险度"), d->riskCard);
    riskTitle->setStyleSheet(QString("color: %1; font-size: 14px;").arg(Tokens::Colors::TextSecondary));
    riskLayout->addWidget(riskTitle);
    
    d->riskLabel = new QLabel(QStringLiteral("中等"), d->riskCard);
    d->riskLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Warning));
    riskLayout->addWidget(d->riskLabel);
    
    d->riskBar = new QProgressBar(d->riskCard);
    d->riskBar->setRange(0, 100);
    d->riskBar->setValue(60);
    d->riskBar->setTextVisible(false);
    d->riskBar->setFixedHeight(8);
    d->riskBar->setStyleSheet(QString(R"(
        QProgressBar {
            background-color: #3A3B41;
            border: none;
            border-radius: 4px;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #34C759, stop:0.5 #F97316, stop:1 #FF3B30);
            border-radius: 4px;
        }
    )"));
    riskLayout->addWidget(d->riskBar);
    
    QLabel* riskDetail = new QLabel(QStringLiteral("保证金占用 ¥210,000"), d->riskCard);
    riskDetail->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Tokens::Colors::TextTertiary));
    riskLayout->addWidget(riskDetail);
    
    layout->addWidget(d->riskCard, 1);

    d->mainLayout->addWidget(cardsFrame);
}

void PortfolioPage::setupMainContent()
{
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(QString("QSplitter::handle { background-color: #3A3B41; }"));

    // 左侧：资产配置
    QFrame* leftPanel = new QFrame(splitter);
    leftPanel->setStyleSheet(QString("background-color: %1; border-radius: 8px;").arg(Tokens::Colors::BgElevated));
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(20, 20, 20, 20);
    leftLayout->setSpacing(16);

    QLabel* pieTitle = new QLabel(QStringLiteral("资产配置"), leftPanel);
    pieTitle->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(Tokens::Colors::TextPrimary));
    leftLayout->addWidget(pieTitle);

    setupAssetAllocation();
    leftLayout->addWidget(d->pieChartView);
    leftLayout->addWidget(d->allocationList);

    splitter->addWidget(leftPanel);

    // 右侧：净值走势
    QFrame* rightPanel = new QFrame(splitter);
    rightPanel->setStyleSheet(QString("background-color: %1; border-radius: 8px;").arg(Tokens::Colors::BgElevated));
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(20, 20, 20, 20);
    rightLayout->setSpacing(16);

    // 标题行
    QHBoxLayout* chartHeader = new QHBoxLayout();
    QLabel* chartTitle = new QLabel(QStringLiteral("资产净值走势"), rightPanel);
    chartTitle->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(Tokens::Colors::TextPrimary));
    chartHeader->addWidget(chartTitle);
    chartHeader->addStretch();

    // 时间范围切换
    d->timeRangeCombo = new QComboBox(rightPanel);
    d->timeRangeCombo->addItems({QStringLiteral("1周"), QStringLiteral("1月"), QStringLiteral("3月"), QStringLiteral("1年"), QStringLiteral("全部")});
    d->timeRangeCombo->setCurrentIndex(1);
    d->timeRangeCombo->setStyleSheet(QString(R"(
        QComboBox {
            background-color: #3A3B41;
            border: none;
            border-radius: 6px;
            padding: 4px 12px;
            color: %1;
            font-size: 12px;
        }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView {
            background-color: %2;
            color: %1;
            selection-background-color: #3A3B41;
        }
    )").arg(Tokens::Colors::TextPrimary, Tokens::Colors::BgElevated));
    chartHeader->addWidget(d->timeRangeCombo);

    rightLayout->addLayout(chartHeader);

    setupNetValueChart();
    rightLayout->addWidget(d->lineChartView, 1);

    splitter->addWidget(rightPanel);

    // 设置分割比例
    splitter->setSizes({300, 500});

    d->mainLayout->addWidget(splitter, 1);
}

void PortfolioPage::setupAssetAllocation()
{
    // 饼图
    d->pieSeries = new QPieSeries();
    d->pieSeries->setHoleSize(0.5);  // 环形图

    QChart* chart = new QChart();
    chart->addSeries(d->pieSeries);
    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->legend()->hide();
    chart->setBackgroundRoundness(8);

    d->pieChartView = new QChartView(chart);
    d->pieChartView->setRenderHint(QPainter::Antialiasing);
    d->pieChartView->setMinimumHeight(200);
    d->pieChartView->setStyleSheet("background: transparent;");

    // 占比明细列表
    d->allocationList = new QFrame();
    d->allocationList->setStyleSheet("background: transparent;");
    QVBoxLayout* listLayout = new QVBoxLayout(d->allocationList);
    listLayout->setSpacing(8);
    listLayout->setContentsMargins(0, 0, 0, 0);

    // 预设颜色
    QVector<QPair<QString, QColor>> items = {
        {QStringLiteral("股票"), QColor(PortfolioColors::Stock)},
        {QStringLiteral("期货"), QColor(PortfolioColors::Futures)},
        {QStringLiteral("基金"), QColor(Tokens::Colors::Danger)},
        {QStringLiteral("现金"), QColor(Tokens::Colors::Success)}
    };

    for (const auto& item : items) {
        QHBoxLayout* row = new QHBoxLayout();
        
        // 颜色指示器
        QLabel* indicator = new QLabel();
        indicator->setFixedSize(12, 12);
        indicator->setStyleSheet(QString("background-color: %1; border-radius: 2px;")
            .arg(item.second.name()));
        row->addWidget(indicator);
        
        // 名称
        QLabel* name = new QLabel(item.first);
        name->setStyleSheet(QString("color: %1; font-size: 13px;").arg(Tokens::Colors::TextPrimary));
        row->addWidget(name);
        
        row->addStretch();
        
        // 金额和占比（占位）
        QLabel* value = new QLabel(QStringLiteral("¥0 (0%)"));
        value->setStyleSheet(QString("color: %1; font-size: 13px;").arg(Tokens::Colors::TextSecondary));
        value->setObjectName(item.first + "_value");
        row->addWidget(value);
        
        listLayout->addLayout(row);
    }
}

void PortfolioPage::setupNetValueChart()
{
    QChart* chart = new QChart();
    chart->setMargins(QMargins(0, 20, 0, 0));
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setLabelColor(QColor(Tokens::Colors::TextSecondary));
    chart->setBackgroundRoundness(8);

    // 我的收益曲线
    d->profitSeries = new QLineSeries();
    d->profitSeries->setName(QStringLiteral("我的收益"));
    QPen profitPen(QColor(PortfolioColors::MyProfit));
    profitPen.setWidth(2);
    d->profitSeries->setPen(profitPen);
    chart->addSeries(d->profitSeries);

    // 沪深300基准曲线
    d->benchmarkSeries = new QLineSeries();
    d->benchmarkSeries->setName(QStringLiteral("沪深300"));
    QPen benchmarkPen(QColor(PortfolioColors::Benchmark));
    benchmarkPen.setWidth(2);
    d->benchmarkSeries->setPen(benchmarkPen);
    chart->addSeries(d->benchmarkSeries);

    // X轴（时间）
    QDateTimeAxis* axisX = new QDateTimeAxis();
    axisX->setFormat("MM-dd");
    axisX->setLabelsColor(QColor(Tokens::Colors::TextTertiary));
    axisX->setGridLineVisible(false);
    chart->addAxis(axisX, Qt::AlignBottom);
    d->profitSeries->attachAxis(axisX);
    d->benchmarkSeries->attachAxis(axisX);

    // Y轴（净值）
    QValueAxis* axisY = new QValueAxis();
    axisY->setLabelFormat("%.1f万");
    axisY->setLabelsColor(QColor(Tokens::Colors::TextTertiary));
    axisY->setGridLineVisible(true);
    axisY->setGridLineColor(QColor(Tokens::Colors::Border));
    chart->addAxis(axisY, Qt::AlignLeft);
    d->profitSeries->attachAxis(axisY);
    d->benchmarkSeries->attachAxis(axisY);

    d->lineChartView = new QChartView(chart);
    d->lineChartView->setRenderHint(QPainter::Antialiasing);
    d->lineChartView->setStyleSheet("background: transparent;");
}

void PortfolioPage::setupPositionTable()
{
    QFrame* tableFrame = new QFrame(this);
    tableFrame->setStyleSheet(QString("background-color: %1; border-radius: 8px;")
        .arg(Tokens::Colors::BgElevated));
    QVBoxLayout* layout = new QVBoxLayout(tableFrame);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    // 标题
    QLabel* title = new QLabel(QStringLiteral("持仓明细"), tableFrame);
    title->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;")
        .arg(Tokens::Colors::TextPrimary));
    layout->addWidget(title);

    // Tab
    d->positionTabs = new QTabWidget(tableFrame);
    d->positionTabs->setStyleSheet(QString(R"(
        QTabWidget::pane {
            border: 1px solid #3A3B41;
            border-radius: 8px;
            background-color: %1;
        }
        QTabBar::tab {
            background-color: transparent;
            color: %2;
            padding: 8px 16px;
            border: none;
            font-size: 13px;
        }
        QTabBar::tab:selected {
            color: %3;
            border-bottom: 2px solid %3;
        }
        QTabBar::tab:hover {
            color: %4;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextSecondary, Tokens::Colors::Primary, Tokens::Colors::TextPrimary));

    // 表格样式
    QString tableStyle = QString(R"(
        QTableView {
            background-color: %1;
            border: none;
            gridline-color: #3A3B41;
            selection-background-color: #3A3B41;
        }
        QTableView::item {
            padding: 8px;
            color: %2;
        }
        QHeaderView::section {
            background-color: #25262B;
            color: %3;
            padding: 8px;
            border: none;
            font-size: 12px;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, Tokens::Colors::TextSecondary);

    // 股票持仓
    d->stockTable = new QTableView(d->positionTabs);
    d->stockModel = new PositionTableModel(this);
    d->stockTable->setModel(d->stockModel);
    d->stockTable->setItemDelegateForColumn(PositionTableModel::ColPnL, new PnLDelegate(this));
    d->stockTable->setItemDelegateForColumn(PositionTableModel::ColPnLPercent, new PnLDelegate(this));
    d->stockTable->setAlternatingRowColors(true);
    d->stockTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->stockTable->horizontalHeader()->setStretchLastSection(true);
    d->stockTable->verticalHeader()->setVisible(false);
    d->stockTable->setSortingEnabled(true);
    d->stockTable->setStyleSheet(tableStyle);
    d->positionTabs->addTab(d->stockTable, QStringLiteral("股票"));

    // 期货持仓
    d->futuresTable = new QTableView(d->positionTabs);
    d->futuresModel = new PositionTableModel(this);
    d->futuresTable->setModel(d->futuresModel);
    d->futuresTable->setItemDelegateForColumn(PositionTableModel::ColPnL, new PnLDelegate(this));
    d->futuresTable->setItemDelegateForColumn(PositionTableModel::ColPnLPercent, new PnLDelegate(this));
    d->futuresTable->setAlternatingRowColors(true);
    d->futuresTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->futuresTable->horizontalHeader()->setStretchLastSection(true);
    d->futuresTable->verticalHeader()->setVisible(false);
    d->futuresTable->setSortingEnabled(true);
    d->futuresTable->setStyleSheet(tableStyle);
    d->positionTabs->addTab(d->futuresTable, QStringLiteral("期货"));

    // 基金持仓
    d->fundTable = new QTableView(d->positionTabs);
    d->fundModel = new PositionTableModel(this);
    d->fundTable->setModel(d->fundModel);
    d->fundTable->setItemDelegateForColumn(PositionTableModel::ColPnL, new PnLDelegate(this));
    d->fundTable->setItemDelegateForColumn(PositionTableModel::ColPnLPercent, new PnLDelegate(this));
    d->fundTable->setAlternatingRowColors(true);
    d->fundTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->fundTable->horizontalHeader()->setStretchLastSection(true);
    d->fundTable->verticalHeader()->setVisible(false);
    d->fundTable->setSortingEnabled(true);
    d->fundTable->setStyleSheet(tableStyle);
    d->positionTabs->addTab(d->fundTable, QStringLiteral("基金"));

    layout->addWidget(d->positionTabs);

    d->mainLayout->addWidget(tableFrame);

    // 连接双击信号
    connect(d->stockTable, &QTableView::doubleClicked, this, &PortfolioPage::onRowDoubleClicked);
    connect(d->futuresTable, &QTableView::doubleClicked, this, &PortfolioPage::onRowDoubleClicked);
    connect(d->fundTable, &QTableView::doubleClicked, this, &PortfolioPage::onRowDoubleClicked);
}

void PortfolioPage::setupConnections()
{
    connect(d->searchEdit, &QLineEdit::textChanged, this, &PortfolioPage::onSearch);
    connect(d->timeRangeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PortfolioPage::onTimeRangeChanged);
    connect(d->positionTabs, &QTabWidget::currentChanged, this, &PortfolioPage::onTabChanged);
}

void PortfolioPage::loadDemoData()
{
    // 加载模拟数据
    QVector<PositionData> stockData;
    QVector<PositionData> futuresData;
    QVector<PositionData> fundData;

    // 股票持仓
    stockData.append({"600519", QStringLiteral("贵州茅台"), "SH", "股票", 100, 1850.0, 1880.0, 188000.0, 3000.0, 1.62});
    stockData.append({"000858", QStringLiteral("五粮液"), "SZ", "股票", 200, 165.0, 168.5, 33700.0, 700.0, 2.12});
    stockData.append({"601318", QStringLiteral("中国平安"), "SH", "股票", 500, 48.5, 47.2, 23600.0, -650.0, -2.68});

    // 期货持仓
    futuresData.append({"IF2501", QStringLiteral("沪深300指数"), "CFFEX", "期货", 2, 3850.0, 3880.0, 776000.0, 6000.0, 0.78});
    futuresData.append({"IC2501", QStringLiteral("中证500指数"), "CFFEX", "期货", 1, 5420.0, 5380.0, 538000.0, -4000.0, -0.74});

    // 基金持仓
    fundData.append({"510300", QStringLiteral("沪深300ETF"), "SH", "基金", 5000, 4.12, 4.18, 20900.0, 300.0, 1.46});
    fundData.append({"159915", QStringLiteral("创业板ETF"), "SZ", "基金", 3000, 2.35, 2.28, 6840.0, -210.0, -2.98});

    d->stockModel->setData(stockData);
    d->futuresModel->setData(futuresData);
    d->fundModel->setData(fundData);

    // 计算汇总
    AccountSummary stockSummary = d->stockModel->calculateSummary();
    AccountSummary futuresSummary = d->futuresModel->calculateSummary();
    AccountSummary fundSummary = d->fundModel->calculateSummary();

    d->summary.totalAssets = 1500000.0;
    d->summary.available = 285000.0;
    d->summary.marketValue = stockSummary.marketValue + futuresSummary.marketValue + fundSummary.marketValue;
    d->summary.dailyPnL = stockSummary.dailyPnL + futuresSummary.dailyPnL + fundSummary.dailyPnL;
    d->summary.returnRate = 15.8;
    d->summary.riskLevel = 60.0;

    updateSummaryDisplay();
    updateAssetAllocation();
    updateNetValueChart(30);

    // 更新时间
    d->updateTimeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

void PortfolioPage::updateSummaryDisplay()
{
    // 总资产
    d->totalAssetLabel->setText(QString("¥%1").arg(d->summary.totalAssets, 0, 'f', 0));
    d->totalAssetChangeLabel->setText(QString("较昨日 +¥%1").arg(d->summary.dailyPnL, 0, 'f', 0));

    // 今日盈亏
    if (d->summary.dailyPnL >= 0) {
        d->dailyPnLLabel->setText(QString("+¥%1").arg(d->summary.dailyPnL, 0, 'f', 0));
        d->dailyPnLLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Danger));
    } else {
        d->dailyPnLLabel->setText(QString("-¥%1").arg(-d->summary.dailyPnL, 0, 'f', 0));
        d->dailyPnLLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Success));
    }
    d->dailyPnLDetailLabel->setText(QStringLiteral("股票 +¥18,200 | 期货 +¥10,250"));

    // 持仓收益率
    if (d->summary.returnRate >= 0) {
        d->returnLabel->setText(QString("+%1%").arg(d->summary.returnRate, 0, 'f', 2));
        d->returnLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Danger));
    } else {
        d->returnLabel->setText(QString("%1%").arg(d->summary.returnRate, 0, 'f', 2));
        d->returnLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Success));
    }
    d->returnDetailLabel->setText(QStringLiteral("沪深300同期 +10.6%"));

    // 风险度
    d->riskBar->setValue(static_cast<int>(d->summary.riskLevel));
    if (d->summary.riskLevel > 70) {
        d->riskLabel->setText(QStringLiteral("高风险"));
        d->riskLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Danger));
    } else if (d->summary.riskLevel > 50) {
        d->riskLabel->setText(QStringLiteral("中等"));
        d->riskLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Warning));
    } else {
        d->riskLabel->setText(QStringLiteral("低风险"));
        d->riskLabel->setStyleSheet(QString("color: %1; font-size: 28px; font-weight: bold;").arg(Tokens::Colors::Success));
    }
}

void PortfolioPage::updateAssetAllocation()
{
    // 清空旧数据
    d->pieSeries->clear();

    // 计算各资产占比
    double stockValue = d->stockModel->calculateSummary().marketValue;
    double futuresValue = d->futuresModel->calculateSummary().marketValue;
    double fundValue = d->fundModel->calculateSummary().marketValue;
    double cashValue = d->summary.available;
    double total = stockValue + futuresValue + fundValue + cashValue;

    // 添加饼图数据
    struct AssetItem {
        QString name;
        double value;
        QColor color;
    };
    
    QVector<AssetItem> items = {
        {QStringLiteral("股票"), stockValue, QColor(PortfolioColors::Stock)},
        {QStringLiteral("期货"), futuresValue, QColor(PortfolioColors::Futures)},
        {QStringLiteral("基金"), fundValue, QColor(Tokens::Colors::Danger)},
        {QStringLiteral("现金"), cashValue, QColor(Tokens::Colors::Success)}
    };

    for (const auto& item : items) {
        if (item.value > 0) {
            QPieSlice* slice = d->pieSeries->append(item.name, item.value);
            slice->setColor(item.color);
            slice->setLabelColor(QColor(Tokens::Colors::TextPrimary));
        }
    }

    // 更新占比明细列表
    QList<QLabel*> valueLabels = d->allocationList->findChildren<QLabel*>(QRegularExpression("_value$"));
    for (int i = 0; i < items.size() && i < valueLabels.size(); ++i) {
        double percent = total > 0 ? (items[i].value / total * 100) : 0;
        valueLabels[i]->setText(QString("¥%1 (%2%)")
            .arg(items[i].value, 0, 'f', 0)
            .arg(percent, 0, 'f', 1));
    }
}

void PortfolioPage::updateNetValueChart(int days)
{
    d->profitSeries->clear();
    d->benchmarkSeries->clear();

    QDateTime now = QDateTime::currentDateTime();
    double baseValue = 100.0;

    for (int i = days; i >= 0; --i) {
        QDateTime date = now.addDays(-i);
        double profit = baseValue + (days - i) * 0.5 + QRandomGenerator::global()->bounded(2.0);
        double benchmark = baseValue + (days - i) * 0.35 + QRandomGenerator::global()->bounded(1.5);
        
        d->profitSeries->append(date.toMSecsSinceEpoch(), profit);
        d->benchmarkSeries->append(date.toMSecsSinceEpoch(), benchmark);
    }
}

void PortfolioPage::refreshData()
{
    LOG_DEBUG("Refreshing portfolio data...");
    loadDemoData();
}

void PortfolioPage::setAccountSummary(const AccountSummary& summary)
{
    d->summary = summary;
    updateSummaryDisplay();
}

void PortfolioPage::onSearch(const QString& text)
{
    LOG_DEBUG(QString("Search: %1").arg(text));
}

void PortfolioPage::onTimeRangeChanged(int index)
{
    const int days[] = {7, 30, 90, 365, 365};
    int day = days[qMin(index, 4)];
    updateNetValueChart(day);
    LOG_DEBUG(QString("Time range changed: %1 days").arg(day));
}

void PortfolioPage::onTabChanged(int index)
{
    LOG_DEBUG(QString("Tab changed: %1").arg(index));
}

void PortfolioPage::onRowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;

    QTableView* table = qobject_cast<QTableView*>(sender());
    if (!table) return;

    auto* model = qobject_cast<PositionTableModel*>(table->model());
    if (!model) return;

    QString instrumentId = model->data(model->index(index.row(), 0), Qt::DisplayRole).toString();
    LOG_INFO(QString("Position double clicked: %1").arg(instrumentId));
}

void PortfolioPage::updateRealTimeData()
{
    // 模拟实时价格更新
    static double basePrice = 1880.0;
    double change = QRandomGenerator::global()->bounded(20.0) - 10.0;
    basePrice += change;

    d->stockModel->updatePrice("600519", basePrice);
    d->updateTimeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

void PortfolioPage::resizeEvent(QResizeEvent* event)
{
    BasePage::resizeEvent(event);
}

void PortfolioPage::showEvent(QShowEvent* event)
{
    BasePage::showEvent(event);
    d->updateTimer->start();
    LOG_DEBUG("PortfolioPage shown, timer started");
}

void PortfolioPage::hideEvent(QHideEvent* event)
{
    BasePage::hideEvent(event);
    d->updateTimer->stop();
    LOG_DEBUG("PortfolioPage hidden, timer stopped");
}
