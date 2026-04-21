/**
 * @file DashboardPage.cpp
 * @brief 金融行情综合看板页面实现 - 六宫格布局
 *
 * @details 布局结构：
 * - 顶部：指数分时图
 * - 中部：六宫格排行榜（沪A涨跌/沪5分钟/深A涨跌/深5分钟/板块热力图）
 * - 底部：自选股/新闻/资金流向
 *
 * @details 设计规范：
 * - 主背景：#0d1117
 * - 卡片背景：#161b22
 * - 涨：#ff4d4f（红），跌：#00b578（绿）
 * - 紧凑布局，单屏最大信息量
 */

#include "DashboardPage.h"
#include "core/config/Tokens.h"
#include "market/StockDataSource.h"
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
#include <QDateTime>
#include <QRandomGenerator>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QScrollArea>
#include <QFrame>
#include <QListWidget>
#include <QProgressBar>
#include <algorithm>

// ============================================================================
// 涨跌颜色委托 - 高性能绘制
// ============================================================================

/**
 * @brief 涨跌颜色委托
 * @details 根据涨跌数据显示不同颜色，涨停/跌停特殊高亮
 */
class ChangeColorDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        double value = index.data(Qt::UserRole).toDouble();
        
        // 确定颜色
        QColor textColor;
        QColor bgColor;
        
        if (value > 9.9) {
            // 涨停
            textColor = QColor(Tokens::Colors::Danger);
            bgColor = QColor(Tokens::Colors::DangerBg);
        } else if (value > 0.01) {
            textColor = QColor(Tokens::Colors::Danger);
            bgColor = QColor(Tokens::Colors::DangerBg);
        } else if (value < -9.9) {
            // 跌停
            textColor = QColor(Tokens::Colors::Success);
            bgColor = QColor(Tokens::Colors::SuccessBg);
        } else if (value < -0.01) {
            textColor = QColor(Tokens::Colors::Success);
            bgColor = QColor(Tokens::Colors::SuccessBg);
        } else {
            textColor = QColor(Tokens::Colors::TextSecondary);
            bgColor = Qt::transparent;
        }

        // 绘制背景
        painter->save();
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor(Tokens::Colors::Primary));
        } else if (bgColor != Qt::transparent) {
            painter->fillRect(option.rect, bgColor);
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

/**
 * @brief 资金流向进度条委托
 */
class MoneyFlowDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        double value = index.data(Qt::UserRole).toDouble();
        
        // 确定颜色
        QColor textColor = value >= 0 
            ? QColor(Tokens::Colors::Danger) 
            : QColor(Tokens::Colors::Success);

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

struct DashboardPage::Impl {
    // 数据源
    StockDataSource* indexDataSource = nullptr;      ///< 指数数据源
    StockDataSource* rankDataSource = nullptr;       ///< 排行榜数据源
    StockDataSource* watchlistDataSource = nullptr;  ///< 自选股数据源
    
    // 主布局
    QVBoxLayout* mainLayout = nullptr;
    QSplitter* mainSplitter = nullptr;

    // 头部组件
    QLineEdit* searchEdit = nullptr;
    QComboBox* marketCombo = nullptr;
    QLabel* timeLabel = nullptr;
    QLabel* statusLabel = nullptr;

    // 指数面板
    QFrame* indexPanel = nullptr;
    QVector<QLabel*> indexNameLabels;
    QVector<QLabel*> indexPriceLabels;
    QVector<QLabel*> indexChangeLabels;
    QVector<IndexData> indexData;

    // 指数代码列表
    QStringList indexSymbols = {
        "sh000001",  // 上证指数
        "sz399001",  // 深证成指
        "sz399006",  // 创业板指
        "sh000688",  // 科创50
        "sh000016",  // 上证50
        "sh000300",  // 沪深300
        "bj899050",  // 北证50
    };

    // 热门股票代码（用于排行榜）
    QStringList hotStockSymbols = {
        "sh600519", "sh601318", "sz000858", "sz000001", "sh600036",
        "sz002594", "sz300750", "sh601012", "sz000333", "sh600900",
        "sz002415", "sh601888", "sz000002", "sh600276", "sz002304",
        "sh601166", "sz000651", "sh601398", "sz002352", "sh600030",
        "sz000725", "sh601288", "sz002475", "sh600000", "sz000063"
    };

    // 六宫格排行榜
    QFrame* rankGridPanel = nullptr;
    QGridLayout* rankGridLayout = nullptr;
    
    // 沪A涨跌榜
    QTableView* shGainTable = nullptr;
    StockRankModel* shGainModel = nullptr;
    
    // 沪5分钟涨跌榜
    QTableView* sh5MinTable = nullptr;
    StockRankModel* sh5MinModel = nullptr;
    
    // 深A涨跌榜
    QTableView* szGainTable = nullptr;
    StockRankModel* szGainModel = nullptr;
    
    // 深5分钟涨跌榜
    QTableView* sz5MinTable = nullptr;
    StockRankModel* sz5MinModel = nullptr;
    
    // 板块热力图
    QTabWidget* sectorTabs = nullptr;
    QTableView* sectorTable = nullptr;
    SectorHeatmapModel* sectorModel = nullptr;

    // 底部信息区
    QSplitter* infoSplitter = nullptr;
    
    // 自选股
    QWidget* watchlistContainer = nullptr;
    QTableView* watchlistTable = nullptr;
    WatchlistModel* watchlistModel = nullptr;
    QComboBox* watchlistFilter = nullptr;
    
    // 新闻
    QFrame* newsPanel = nullptr;
    QListWidget* newsList = nullptr;
    
    // 资金流向
    QWidget* moneyFlowContainer = nullptr;
    QTableView* moneyFlowTable = nullptr;
    MoneyFlowModel* moneyFlowModel = nullptr;
    QComboBox* moneyFlowPeriod = nullptr;

    // 定时器
    QTimer* updateTimer = nullptr;
    QTimer* clockTimer = nullptr;
};

// ============================================================================
// StockRankModel 实现
// ============================================================================

StockRankModel::StockRankModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int StockRankModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int StockRankModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

QVariant StockRankModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size()) {
        return QVariant();
    }

    const StockRankData& stock = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColRank:
                return stock.rank;
            case ColName:
                return stock.name;
            case ColPrice:
                return QString::number(stock.price, 'f', 2);
            case ColChange:
                return QString::number(stock.changePercent, 'f', 2) + "%";
            case ColChangeAmount:
                return formatValue(stock.change);
            default:
                return QVariant();
        }
    }

    if (role == Qt::UserRole) {
        return stock.changePercent;
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColName) {
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }

    return QVariant();
}

QVariant StockRankModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    static const QStringList headers = {
        QStringLiteral("排名"),
        QStringLiteral("名称"),
        QStringLiteral("现价"),
        QStringLiteral("涨跌幅"),
        QStringLiteral("涨跌额")
    };

    return headers.value(section);
}

void StockRankModel::setData(const QVector<StockRankData>& data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

void StockRankModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}

QString StockRankModel::formatValue(double value)
{
    if (qAbs(value) >= 10000) {
        return QString::number(value / 10000.0, 'f', 2) + "万";
    }
    return QString::number(value, 'f', 2);
}

// ============================================================================
// WatchlistModel 实现
// ============================================================================

WatchlistModel::WatchlistModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int WatchlistModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int WatchlistModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

QVariant WatchlistModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size()) {
        return QVariant();
    }

    const StockRankData& stock = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColRank: return stock.rank;
            case ColCode: return stock.code;
            case ColName: return stock.name;
            case ColPrice: return QString::number(stock.price, 'f', 2);
            case ColChange: return QString::number(stock.changePercent, 'f', 2) + "%";
            case ColChangeAmount: return QString::number(stock.change, 'f', 2);
            case ColVolume: return QString::number(stock.volume / 10000.0, 'f', 0) + "万";
            case ColAmount: return QString::number(stock.amount / 100000000.0, 'f', 2) + "亿";
            case ColTurnover: return QString::number(stock.turnover, 'f', 2) + "%";
            case ColPE: return stock.pe > 0 ? QString::number(stock.pe, 'f', 2) : "--";
            default: return QVariant();
        }
    }

    if (role == Qt::UserRole) {
        return stock.changePercent;
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColName || index.column() == ColCode) {
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }

    return QVariant();
}

QVariant WatchlistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    static const QStringList headers = {
        QStringLiteral("序号"),
        QStringLiteral("代码"),
        QStringLiteral("名称"),
        QStringLiteral("最新价"),
        QStringLiteral("涨跌幅"),
        QStringLiteral("涨跌额"),
        QStringLiteral("总量"),
        QStringLiteral("金额"),
        QStringLiteral("换手率"),
        QStringLiteral("市盈率")
    };

    return headers.value(section);
}

void WatchlistModel::setData(const QVector<StockRankData>& data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

void WatchlistModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}

// ============================================================================
// SectorHeatmapModel 实现
// ============================================================================

SectorHeatmapModel::SectorHeatmapModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int SectorHeatmapModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int SectorHeatmapModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 4;
}

QVariant SectorHeatmapModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size()) {
        return QVariant();
    }

    const SectorData& sector = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return sector.name;
            case 1: return QString::number(sector.changePercent, 'f', 2) + "%";
            case 2: return QString("%1/%2").arg(sector.upCount).arg(sector.downCount);
            case 3: return QString::number(sector.amount / 100000000.0, 'f', 0) + "亿";
            default: return QVariant();
        }
    }

    if (role == Qt::UserRole) {
        return sector.changePercent;
    }

    if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignCenter);
    }

    return QVariant();
}

QVariant SectorHeatmapModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    static const QStringList headers = {
        QStringLiteral("板块"),
        QStringLiteral("涨跌幅"),
        QStringLiteral("涨/跌"),
        QStringLiteral("成交额")
    };

    return headers.value(section);
}

void SectorHeatmapModel::setData(const QVector<SectorData>& data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

void SectorHeatmapModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}

// ============================================================================
// MoneyFlowModel 实现
// ============================================================================

MoneyFlowModel::MoneyFlowModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int MoneyFlowModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int MoneyFlowModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : ColCount;
}

QVariant MoneyFlowModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size()) {
        return QVariant();
    }

    const MoneyFlowData& flow = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColRank: return flow.rank;
            case ColName: return flow.name;
            case ColNetInflow:
                return QString::number(flow.netInflow / 100000000.0, 'f', 2) + "亿";
            case ColNetInflowPercent:
                return QString::number(flow.netInflowPercent, 'f', 2) + "%";
            case ColDay3:
                return QString::number(flow.day3Inflow / 100000000.0, 'f', 2) + "亿";
            case ColDay5:
                return QString::number(flow.day5Inflow / 100000000.0, 'f', 2) + "亿";
            default: return QVariant();
        }
    }

    if (role == Qt::UserRole) {
        return flow.netInflow;
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColName) {
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }

    return QVariant();
}

QVariant MoneyFlowModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }

    static const QStringList headers = {
        QStringLiteral("排名"),
        QStringLiteral("名称"),
        QStringLiteral("净流入"),
        QStringLiteral("当日增仓"),
        QStringLiteral("3日增仓"),
        QStringLiteral("5日增仓")
    };

    return headers.value(section);
}

void MoneyFlowModel::setData(const QVector<MoneyFlowData>& data)
{
    beginResetModel();
    m_data = data;
    endResetModel();
}

void MoneyFlowModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}

// ============================================================================
// DashboardPage 实现
// ============================================================================

DashboardPage::DashboardPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setObjectName("DashboardPage");

    // 设置背景色
    setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgBase));

    // 实时更新定时器
    d->updateTimer = new QTimer(this);
    d->updateTimer->setInterval(3000);
    connect(d->updateTimer, &QTimer::timeout, this, &DashboardPage::updateRealTimeData);

    // 时钟定时器
    d->clockTimer = new QTimer(this);
    d->clockTimer->setInterval(1000);
    connect(d->clockTimer, &QTimer::timeout, this, &DashboardPage::updateTimeDisplay);
}

DashboardPage::~DashboardPage()
{
    if (d->updateTimer) d->updateTimer->stop();
    if (d->clockTimer) d->clockTimer->stop();
}

void DashboardPage::initializePage()
{
    if (isInitialized()) return;

    setupConnections();
    loadRealData();  // 使用真实数据

    setInitialized(true);
    emit pageStatusChanged(QStringLiteral("initialized"));

    LOG_DEBUG("DashboardPage initialized with real data");
}

/**
 * @brief 初始化UI布局
 */
void DashboardPage::setupUI()
{
    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setContentsMargins(0, 0, 0, 0);
    d->mainLayout->setSpacing(0);

    // 1. 顶部工具栏
    setupHeader();

    // 2. 主分割器
    d->mainSplitter = new QSplitter(Qt::Vertical, this);
    d->mainSplitter->setHandleWidth(1);
    d->mainSplitter->setStyleSheet(
        QString("QSplitter::handle { background-color: %1; }").arg(Tokens::Colors::Border));

    // 3. 指数面板（顶部）
    setupIndexPanel();
    d->mainSplitter->addWidget(d->indexPanel);

    // 4. 六宫格排行榜（中部60%）
    setupRankGrid();
    d->mainSplitter->addWidget(d->rankGridPanel);

    // 5. 底部信息区（底部40%）
    setupInfoPanel();
    d->mainSplitter->addWidget(d->infoSplitter);

    // 设置分割比例
    d->mainSplitter->setSizes({120, 350, 280});

    d->mainLayout->addWidget(d->mainSplitter, 1);
}

/**
 * @brief 初始化头部工具栏
 */
void DashboardPage::setupHeader()
{
    QFrame* header = new QFrame(this);
    header->setFixedHeight(48);
    header->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(Tokens::Colors::BgElevated, Tokens::Colors::Border));

    QHBoxLayout* layout = new QHBoxLayout(header);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(16);

    // 页面标题
    QLabel* titleLabel = new QLabel(QStringLiteral("行情看板"), header);
    titleLabel->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;")
        .arg(Tokens::Colors::TextPrimary));
    layout->addWidget(titleLabel);

    layout->addSpacing(20);

    // 市场选择
    d->marketCombo = new QComboBox(header);
    d->marketCombo->addItems({
        QStringLiteral("全部A股"),
        QStringLiteral("沪市主板"),
        QStringLiteral("深市主板"),
        QStringLiteral("创业板"),
        QStringLiteral("科创板")
    });
    d->marketCombo->setStyleSheet(QString(R"(
        QComboBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 4px 12px;
            color: %3;
            min-width: 100px;
        }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView {
            background-color: %1;
            color: %3;
            selection-background-color: %4;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::Border, 
            Tokens::Colors::TextPrimary, Tokens::Colors::Primary));
    layout->addWidget(d->marketCombo);

    layout->addSpacing(20);

    // 搜索框
    d->searchEdit = new QLineEdit(header);
    d->searchEdit->setPlaceholderText(QStringLiteral("搜索股票代码/名称..."));
    d->searchEdit->setFixedWidth(220);
    d->searchEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            padding: 6px 12px;
            color: %3;
        }
        QLineEdit::placeholder { color: %4; }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::Border, 
            Tokens::Colors::TextPrimary, Tokens::Colors::TextTertiary));
    layout->addWidget(d->searchEdit);

    layout->addStretch();

    // 时间显示
    d->timeLabel = new QLabel(header);
    d->timeLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(Tokens::Colors::TextSecondary));
    layout->addWidget(d->timeLabel);

    layout->addSpacing(20);

    // 状态信息
    d->statusLabel = new QLabel(header);
    d->statusLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(Tokens::Colors::TextTertiary));
    layout->addWidget(d->statusLabel);

    d->mainLayout->addWidget(header);
}

/**
 * @brief 初始化指数面板
 */
void DashboardPage::setupIndexPanel()
{
    d->indexPanel = new QFrame(this);
    d->indexPanel->setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgElevated));

    QHBoxLayout* layout = new QHBoxLayout(d->indexPanel);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // 创建5个指数卡片
    QStringList indexNames = {
        QStringLiteral("上证指数"),
        QStringLiteral("深证成指"),
        QStringLiteral("沪深300"),
        QStringLiteral("创业板指"),
        QStringLiteral("科创50")
    };

    for (int i = 0; i < 5; ++i) {
        QFrame* card = new QFrame(d->indexPanel);
        card->setStyleSheet(QString(R"(
            QFrame {
                background-color: %1;
                border-radius: 6px;
                border: 1px solid %2;
            }
        )").arg(Tokens::Colors::BgBase, Tokens::Colors::Border));

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 10, 12, 10);
        cardLayout->setSpacing(4);

        // 指数名称
        QLabel* nameLabel = new QLabel(indexNames[i], card);
        nameLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
            .arg(Tokens::Colors::TextSecondary));
        cardLayout->addWidget(nameLabel);
        d->indexNameLabels.append(nameLabel);

        // 价格和涨跌
        QHBoxLayout* priceLayout = new QHBoxLayout();
        
        QLabel* priceLabel = new QLabel(QStringLiteral("0.00"), card);
        priceLabel->setStyleSheet(QString("color: %1; font-size: 22px; font-weight: bold;")
            .arg(Tokens::Colors::TextPrimary));
        priceLayout->addWidget(priceLabel);
        d->indexPriceLabels.append(priceLabel);

        QLabel* changeLabel = new QLabel(QStringLiteral("+0.00%"), card);
        changeLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
            .arg(Tokens::Colors::Danger));
        priceLayout->addWidget(changeLabel);
        priceLayout->addStretch();
        d->indexChangeLabels.append(changeLabel);

        cardLayout->addLayout(priceLayout);

        layout->addWidget(card, 1);
    }
}

/**
 * @brief 初始化六宫格排行榜
 */
void DashboardPage::setupRankGrid()
{
    d->rankGridPanel = new QFrame(this);
    d->rankGridPanel->setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgBase));

    d->rankGridLayout = new QGridLayout(d->rankGridPanel);
    d->rankGridLayout->setContentsMargins(8, 8, 8, 8);
    d->rankGridLayout->setSpacing(8);

    // 表格样式
    QString tableStyle = QString(R"(
        QTableView {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            gridline-color: %3;
            selection-background-color: %4;
            font-family: 'Consolas', 'JetBrains Mono', monospace;
        }
        QTableView::item {
            padding: 2px 4px;
            color: %5;
            font-size: 12px;
        }
        QHeaderView::section {
            background-color: %6;
            color: %7;
            padding: 4px 6px;
            border: none;
            border-bottom: 1px solid %3;
            font-size: 11px;
            font-weight: bold;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::Border, Tokens::Colors::Border,
            Tokens::Colors::Primary, Tokens::Colors::TextPrimary, 
            Tokens::Colors::BgSurface, Tokens::Colors::TextSecondary);

    // 卡片标题样式
    auto createCardWithTitle = [this, &tableStyle](const QString& title, QTableView*& table, 
                                                    QAbstractTableModel* model) -> QFrame* {
        QFrame* card = new QFrame(d->rankGridPanel);
        card->setStyleSheet(QString("QFrame { background-color: %1; border-radius: 6px; }")
            .arg(Tokens::Colors::BgElevated));

        QVBoxLayout* layout = new QVBoxLayout(card);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // 标题栏
        QFrame* titleBar = new QFrame(card);
        titleBar->setFixedHeight(28);
        titleBar->setStyleSheet(QString("background-color: %1; border-top-left-radius: 6px; border-top-right-radius: 6px;")
            .arg(Tokens::Colors::BgSurface));
        QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(10, 0, 10, 0);

        QLabel* titleLabel = new QLabel(title, titleBar);
        titleLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;")
            .arg(Tokens::Colors::TextPrimary));
        titleLayout->addWidget(titleLabel);
        titleLayout->addStretch();

        layout->addWidget(titleBar);

        // 表格
        table = new QTableView(card);
        table->setModel(model);
        table->setItemDelegateForColumn(StockRankModel::ColChange, new ChangeColorDelegate(this));
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->verticalHeader()->setVisible(false);
        table->setShowGrid(false);
        table->setStyleSheet(tableStyle);
        table->horizontalHeader()->setStretchLastSection(true);
        table->verticalHeader()->setDefaultSectionSize(22);
        layout->addWidget(table);

        return card;
    };

    // 第1行：沪A涨跌榜、深A涨跌榜、板块热力图
    // 左上：沪A涨跌榜
    d->shGainModel = new StockRankModel(this);
    QFrame* shGainCard = createCardWithTitle(QStringLiteral("沪A涨幅榜"), d->shGainTable, d->shGainModel);
    d->rankGridLayout->addWidget(shGainCard, 0, 0);

    // 中上：深A涨跌榜
    d->szGainModel = new StockRankModel(this);
    QFrame* szGainCard = createCardWithTitle(QStringLiteral("深A涨幅榜"), d->szGainTable, d->szGainModel);
    d->rankGridLayout->addWidget(szGainCard, 0, 1);

    // 右上：板块热力图（带Tab）
    QFrame* sectorCard = new QFrame(d->rankGridPanel);
    sectorCard->setStyleSheet(QString("QFrame { background-color: %1; border-radius: 6px; }")
        .arg(Tokens::Colors::BgElevated));
    QVBoxLayout* sectorLayout = new QVBoxLayout(sectorCard);
    sectorLayout->setContentsMargins(0, 0, 0, 0);
    sectorLayout->setSpacing(0);

    // Tab标题栏
    QFrame* sectorHeader = new QFrame(sectorCard);
    sectorHeader->setFixedHeight(28);
    sectorHeader->setStyleSheet(QString("background-color: %1;")
        .arg(Tokens::Colors::BgSurface));
    QHBoxLayout* sectorHeaderLayout = new QHBoxLayout(sectorHeader);
    sectorHeaderLayout->setContentsMargins(8, 0, 8, 0);

    d->sectorTabs = new QTabWidget(sectorHeader);
    d->sectorTabs->setStyleSheet(QString(R"(
        QTabWidget::pane { border: none; background: transparent; }
        QTabBar::tab {
            background: transparent;
            color: %1;
            padding: 4px 12px;
            border: none;
            font-size: 11px;
        }
        QTabBar::tab:selected {
            color: %2;
            border-bottom: 2px solid %2;
        }
    )").arg(Tokens::Colors::TextSecondary, Tokens::Colors::Primary));
    
    // 添加Tab
    QStringList tabNames = {QStringLiteral("行业"), QStringLiteral("概念"), QStringLiteral("地区")};
    for (const QString& tabName : tabNames) {
        QLabel* placeholder = new QLabel(tabName);
        d->sectorTabs->addTab(placeholder, tabName);
    }
    sectorHeaderLayout->addWidget(d->sectorTabs);

    sectorLayout->addWidget(sectorHeader);

    // 板块表格
    d->sectorModel = new SectorHeatmapModel(this);
    d->sectorTable = new QTableView(sectorCard);
    d->sectorTable->setModel(d->sectorModel);
    d->sectorTable->setItemDelegateForColumn(1, new ChangeColorDelegate(this));
    d->sectorTable->setAlternatingRowColors(true);
    d->sectorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->sectorTable->verticalHeader()->setVisible(false);
    d->sectorTable->setShowGrid(false);
    d->sectorTable->setStyleSheet(tableStyle);
    d->sectorTable->verticalHeader()->setDefaultSectionSize(22);
    sectorLayout->addWidget(d->sectorTable);

    d->rankGridLayout->addWidget(sectorCard, 0, 2, 2, 1); // 跨两行

    // 第2行：沪5分钟涨跌榜、深5分钟涨跌榜
    // 左中：沪5分钟涨跌榜
    d->sh5MinModel = new StockRankModel(this);
    QFrame* sh5MinCard = createCardWithTitle(QStringLiteral("沪5分钟涨幅"), d->sh5MinTable, d->sh5MinModel);
    d->rankGridLayout->addWidget(sh5MinCard, 1, 0);

    // 中中：深5分钟涨跌榜
    d->sz5MinModel = new StockRankModel(this);
    QFrame* sz5MinCard = createCardWithTitle(QStringLiteral("深5分钟涨幅"), d->sz5MinTable, d->sz5MinModel);
    d->rankGridLayout->addWidget(sz5MinCard, 1, 1);

    // 设置列宽比例
    d->rankGridLayout->setColumnStretch(0, 1);
    d->rankGridLayout->setColumnStretch(1, 1);
    d->rankGridLayout->setColumnStretch(2, 1);
    d->rankGridLayout->setRowStretch(0, 1);
    d->rankGridLayout->setRowStretch(1, 1);
}

/**
 * @brief 初始化底部信息区
 */
void DashboardPage::setupInfoPanel()
{
    d->infoSplitter = new QSplitter(Qt::Horizontal, this);
    d->infoSplitter->setHandleWidth(1);
    d->infoSplitter->setStyleSheet(
        QString("QSplitter::handle { background-color: %1; }").arg(Tokens::Colors::Border));

    // 1. 自选股列表
    setupWatchlistPanel();
    d->infoSplitter->addWidget(d->watchlistContainer);

    // 2. 新闻资讯
    setupNewsPanel();
    d->infoSplitter->addWidget(d->newsPanel);

    // 3. 资金流向
    setupMoneyFlowPanel();
    d->infoSplitter->addWidget(d->moneyFlowContainer);

    // 设置分割比例
    d->infoSplitter->setSizes({350, 300, 300});
}

/**
 * @brief 初始化自选股面板
 */
void DashboardPage::setupWatchlistPanel()
{
    d->watchlistContainer = new QWidget(this);
    d->watchlistContainer->setStyleSheet(QString("background-color: %1;")
        .arg(Tokens::Colors::BgElevated));

    QVBoxLayout* layout = new QVBoxLayout(d->watchlistContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 标题栏
    QFrame* header = new QFrame(d->watchlistContainer);
    header->setFixedHeight(36);
    header->setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgSurface));
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 0, 12, 0);

    QLabel* title = new QLabel(QStringLiteral("自选股"), header);
    title->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;")
        .arg(Tokens::Colors::TextPrimary));
    headerLayout->addWidget(title);

    d->watchlistFilter = new QComboBox(header);
    d->watchlistFilter->addItems({QStringLiteral("全部自选"), QStringLiteral("持仓")});
    d->watchlistFilter->setStyleSheet(QString("background: transparent; color: %1; border: none; font-size: 12px;")
        .arg(Tokens::Colors::TextSecondary));
    headerLayout->addWidget(d->watchlistFilter);
    headerLayout->addStretch();

    layout->addWidget(header);

    // 表格
    d->watchlistModel = new WatchlistModel(this);
    d->watchlistTable = new QTableView(d->watchlistContainer);
    d->watchlistTable->setModel(d->watchlistModel);
    d->watchlistTable->setItemDelegateForColumn(WatchlistModel::ColChange, new ChangeColorDelegate(this));
    d->watchlistTable->setAlternatingRowColors(true);
    d->watchlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->watchlistTable->verticalHeader()->setVisible(false);
    d->watchlistTable->setShowGrid(false);
    d->watchlistTable->verticalHeader()->setDefaultSectionSize(24);
    d->watchlistTable->setStyleSheet(QString(R"(
        QTableView {
            background-color: %1;
            border: none;
            gridline-color: %2;
            font-family: 'Consolas', 'JetBrains Mono', monospace;
        }
        QTableView::item { padding: 2px 4px; color: %3; font-size: 12px; }
        QHeaderView::section {
            background-color: %4;
            color: %5;
            padding: 4px 6px;
            border: none;
            font-size: 11px;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::Border, 
            Tokens::Colors::TextPrimary, Tokens::Colors::BgSurface, 
            Tokens::Colors::TextSecondary));
    layout->addWidget(d->watchlistTable);
}

/**
 * @brief 初始化新闻面板
 */
void DashboardPage::setupNewsPanel()
{
    d->newsPanel = new QFrame(this);
    d->newsPanel->setStyleSheet(QString("background-color: %1;")
        .arg(Tokens::Colors::BgElevated));

    QVBoxLayout* layout = new QVBoxLayout(d->newsPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 标题栏
    QFrame* header = new QFrame(d->newsPanel);
    header->setFixedHeight(36);
    header->setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgSurface));
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 0, 12, 0);

    QLabel* title = new QLabel(QStringLiteral("24小时滚动新闻"), header);
    title->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;")
        .arg(Tokens::Colors::TextPrimary));
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    QLabel* moreLabel = new QLabel(QStringLiteral("更多 >"), header);
    moreLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
        .arg(Tokens::Colors::TextTertiary));
    headerLayout->addWidget(moreLabel);

    layout->addWidget(header);

    // 新闻列表
    d->newsList = new QListWidget(d->newsPanel);
    d->newsList->setStyleSheet(QString(R"(
        QListWidget {
            background-color: %1;
            border: none;
            padding: 4px;
        }
        QListWidget::item {
            color: %2;
            font-size: 12px;
            padding: 6px 8px;
            border-bottom: 1px solid %3;
        }
        QListWidget::item:hover {
            background-color: %4;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, 
            Tokens::Colors::Border, Tokens::Colors::BgHover));
    layout->addWidget(d->newsList);
}

/**
 * @brief 初始化资金流向面板
 */
void DashboardPage::setupMoneyFlowPanel()
{
    d->moneyFlowContainer = new QWidget(this);
    d->moneyFlowContainer->setStyleSheet(QString("background-color: %1;")
        .arg(Tokens::Colors::BgElevated));

    QVBoxLayout* layout = new QVBoxLayout(d->moneyFlowContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 标题栏
    QFrame* header = new QFrame(d->moneyFlowContainer);
    header->setFixedHeight(36);
    header->setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgSurface));
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(12, 0, 12, 0);

    QLabel* title = new QLabel(QStringLiteral("资金流向"), header);
    title->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold;")
        .arg(Tokens::Colors::TextPrimary));
    headerLayout->addWidget(title);

    d->moneyFlowPeriod = new QComboBox(header);
    d->moneyFlowPeriod->addItems({QStringLiteral("当日"), QStringLiteral("3日"), QStringLiteral("5日")});
    d->moneyFlowPeriod->setStyleSheet(QString("background: transparent; color: %1; border: none; font-size: 12px;")
        .arg(Tokens::Colors::TextSecondary));
    headerLayout->addWidget(d->moneyFlowPeriod);
    headerLayout->addStretch();

    layout->addWidget(header);

    // 表格
    d->moneyFlowModel = new MoneyFlowModel(this);
    d->moneyFlowTable = new QTableView(d->moneyFlowContainer);
    d->moneyFlowTable->setModel(d->moneyFlowModel);
    d->moneyFlowTable->setItemDelegateForColumn(MoneyFlowModel::ColNetInflow, new MoneyFlowDelegate(this));
    d->moneyFlowTable->setAlternatingRowColors(true);
    d->moneyFlowTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->moneyFlowTable->verticalHeader()->setVisible(false);
    d->moneyFlowTable->setShowGrid(false);
    d->moneyFlowTable->verticalHeader()->setDefaultSectionSize(24);
    d->moneyFlowTable->setStyleSheet(QString(R"(
        QTableView {
            background-color: %1;
            border: none;
            gridline-color: %2;
            font-family: 'Consolas', 'JetBrains Mono', monospace;
        }
        QTableView::item { padding: 2px 4px; color: %3; font-size: 12px; }
        QHeaderView::section {
            background-color: %4;
            color: %5;
            padding: 4px 6px;
            border: none;
            font-size: 11px;
        }
    )").arg(Tokens::Colors::BgElevated, Tokens::Colors::Border, 
            Tokens::Colors::TextPrimary, Tokens::Colors::BgSurface, 
            Tokens::Colors::TextSecondary));
    layout->addWidget(d->moneyFlowTable);
}

/**
 * @brief 连接信号槽
 */
void DashboardPage::setupConnections()
{
    connect(d->searchEdit, &QLineEdit::textChanged, this, &DashboardPage::onSearchChanged);
    connect(d->marketCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DashboardPage::onMarketChanged);
    connect(d->watchlistFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DashboardPage::onWatchlistFilterChanged);
    connect(d->moneyFlowPeriod, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DashboardPage::onMoneyFlowPeriodChanged);
    connect(d->sectorTabs, &QTabWidget::currentChanged, this, &DashboardPage::onSectorTabChanged);

    // 双击信号
    connect(d->shGainTable, &QTableView::doubleClicked, this, &DashboardPage::onRowDoubleClicked);
    connect(d->szGainTable, &QTableView::doubleClicked, this, &DashboardPage::onRowDoubleClicked);
    connect(d->watchlistTable, &QTableView::doubleClicked, this, &DashboardPage::onRowDoubleClicked);
}

/**
 * @brief 加载演示数据
 */
void DashboardPage::loadDemoData()
{
    loadIndexData();
    loadRankData();
    loadWatchlistData();
    loadNewsData();
    loadMoneyFlowData();
    loadSectorData();

    updateTimeDisplay();
}

/**
 * @brief 加载真实行情数据
 */
void DashboardPage::loadRealData()
{
    // 初始化数据源
    d->indexDataSource = new StockDataSource(StockDataSource::Source::Sina, this);
    d->rankDataSource = new StockDataSource(StockDataSource::Source::Sina, this);
    d->watchlistDataSource = new StockDataSource(StockDataSource::Source::Sina, this);
    
    // 连接信号
    connect(d->indexDataSource, &StockDataSource::quotesReceived,
            this, &DashboardPage::onIndexQuotesReceived);
    connect(d->rankDataSource, &StockDataSource::quotesReceived,
            this, &DashboardPage::onRankQuotesReceived);
    connect(d->watchlistDataSource, &StockDataSource::quotesReceived,
            this, &DashboardPage::onWatchlistQuotesReceived);
    
    // 请求指数数据
    d->indexDataSource->requestQuotes(d->indexSymbols);
    
    // 请求排行榜数据
    d->rankDataSource->requestQuotes(d->hotStockSymbols);
    
    // 启动自动刷新（5秒）
    d->indexDataSource->startAutoRefresh(5000);
    d->rankDataSource->startAutoRefresh(5000);
    
    // 加载其他数据（新闻、资金流向等暂时用模拟数据）
    loadNewsData();
    loadMoneyFlowData();
    loadSectorData();
    
    updateTimeDisplay();
    
    LOG_INFO("DashboardPage: Real data loading started");
}

/**
 * @brief 处理指数数据
 */
void DashboardPage::onIndexQuotesReceived(const QVector<StockQuote>& quotes)
{
    d->indexData.clear();
    
    for (const auto& quote : quotes) {
        IndexData data;
        data.code = quote.symbol;
        data.name = quote.name;
        data.current = quote.lastPrice;
        data.change = quote.changeAmount;
        data.changePercent = quote.changePercent;
        d->indexData.append(data);
    }
    
    updateIndexDisplay();
    
    d->statusLabel->setText(QString("已更新 %1 %2")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
        .arg(QStringLiteral("指数数据已更新")));
}

/**
 * @brief 处理排行榜数据
 */
void DashboardPage::onRankQuotesReceived(const QVector<StockQuote>& quotes)
{
    // 按涨跌幅排序
    QVector<StockQuote> sortedQuotes = quotes;
    std::sort(sortedQuotes.begin(), sortedQuotes.end(),
        [](const StockQuote& a, const StockQuote& b) {
            return a.changePercent > b.changePercent;
        });
    
    // 沪A涨幅榜（前15名）
    QVector<StockRankData> shGainData;
    int rank = 1;
    for (const auto& quote : sortedQuotes) {
        if (quote.symbol.startsWith("sh") && quote.changePercent > 0) {
            StockRankData stock;
            stock.rank = rank++;
            stock.code = quote.symbol;
            stock.name = quote.name;
            stock.price = quote.lastPrice;
            stock.changePercent = quote.changePercent;
            stock.change = quote.changeAmount;
            stock.volume = quote.volume;
            stock.amount = quote.turnover;
            shGainData.append(stock);
            if (shGainData.size() >= 15) break;
        }
    }
    d->shGainModel->setData(shGainData);
    
    // 深A涨幅榜
    QVector<StockRankData> szGainData;
    rank = 1;
    for (const auto& quote : sortedQuotes) {
        if (quote.symbol.startsWith("sz") && quote.changePercent > 0) {
            StockRankData stock;
            stock.rank = rank++;
            stock.code = quote.symbol;
            stock.name = quote.name;
            stock.price = quote.lastPrice;
            stock.changePercent = quote.changePercent;
            stock.change = quote.changeAmount;
            stock.volume = quote.volume;
            stock.amount = quote.turnover;
            szGainData.append(stock);
            if (szGainData.size() >= 15) break;
        }
    }
    d->szGainModel->setData(szGainData);
    
    // 沪A跌幅榜
    QVector<StockRankData> shLossData;
    rank = 1;
    for (int i = sortedQuotes.size() - 1; i >= 0; --i) {
        const auto& quote = sortedQuotes[i];
        if (quote.symbol.startsWith("sh") && quote.changePercent < 0) {
            StockRankData stock;
            stock.rank = rank++;
            stock.code = quote.symbol;
            stock.name = quote.name;
            stock.price = quote.lastPrice;
            stock.changePercent = quote.changePercent;
            stock.change = quote.changeAmount;
            stock.volume = quote.volume;
            stock.amount = quote.turnover;
            shLossData.append(stock);
            if (shLossData.size() >= 15) break;
        }
    }
    d->sh5MinModel->setData(shLossData);
    
    // 深A跌幅榜
    QVector<StockRankData> szLossData;
    rank = 1;
    for (int i = sortedQuotes.size() - 1; i >= 0; --i) {
        const auto& quote = sortedQuotes[i];
        if (quote.symbol.startsWith("sz") && quote.changePercent < 0) {
            StockRankData stock;
            stock.rank = rank++;
            stock.code = quote.symbol;
            stock.name = quote.name;
            stock.price = quote.lastPrice;
            stock.changePercent = quote.changePercent;
            stock.change = quote.changeAmount;
            stock.volume = quote.volume;
            stock.amount = quote.turnover;
            szLossData.append(stock);
            if (szLossData.size() >= 15) break;
        }
    }
    d->sz5MinModel->setData(szLossData);
}

/**
 * @brief 处理自选股数据
 */
void DashboardPage::onWatchlistQuotesReceived(const QVector<StockQuote>& quotes)
{
    QVector<StockRankData> watchlistData;
    int rank = 1;
    for (const auto& quote : quotes) {
        StockRankData stock;
        stock.rank = rank++;
        stock.code = quote.symbol;
        stock.name = quote.name;
        stock.price = quote.lastPrice;
        stock.changePercent = quote.changePercent;
        stock.change = quote.changeAmount;
        stock.volume = quote.volume;
        stock.amount = quote.turnover;
        watchlistData.append(stock);
    }
    d->watchlistModel->setData(watchlistData);
}

/**
 * @brief 加载指数数据
 */
void DashboardPage::loadIndexData()
{
    d->indexData.clear();

    struct IndexInfo {
        QString code;
        QString name;
        double current;
        double change;
        double changePercent;
    };

    QVector<IndexInfo> indices = {
        {"sh000001", QStringLiteral("上证指数"), 3256.78, 23.45, 0.72},
        {"sz399001", QStringLiteral("深证成指"), 10856.34, -45.67, -0.42},
        {"sh000300", QStringLiteral("沪深300"), 3892.45, 18.67, 0.48},
        {"sz399006", QStringLiteral("创业板指"), 2156.89, 32.45, 1.53},
        {"sh000688", QStringLiteral("科创50"), 987.65, -12.34, -1.24}
    };

    for (const auto& idx : indices) {
        IndexData data;
        data.code = idx.code;
        data.name = idx.name;
        data.current = idx.current;
        data.change = idx.change;
        data.changePercent = idx.changePercent;
        d->indexData.append(data);
    }

    updateIndexDisplay();
}

/**
 * @brief 更新指数显示
 */
void DashboardPage::updateIndexDisplay()
{
    for (int i = 0; i < d->indexData.size() && i < d->indexPriceLabels.size(); ++i) {
        const IndexData& idx = d->indexData[i];
        d->indexPriceLabels[i]->setText(QString::number(idx.current, 'f', 2));
        
        QString changeText = idx.change >= 0 
            ? QString("+%1 (+%2%)").arg(idx.change, 0, 'f', 2).arg(idx.changePercent, 0, 'f', 2)
            : QString("%1 (%2%)").arg(idx.change, 0, 'f', 2).arg(idx.changePercent, 0, 'f', 2);
        d->indexChangeLabels[i]->setText(changeText);
        d->indexChangeLabels[i]->setStyleSheet(
            QString("color: %1; font-size: 13px;").arg(idx.change >= 0 ? Tokens::Colors::Danger : Tokens::Colors::Success));
    }
}

/**
 * @brief 加载排行数据
 */
void DashboardPage::loadRankData()
{
    // 沪A涨幅榜
    QVector<StockRankData> shGainData;
    for (int i = 1; i <= 15; ++i) {
        StockRankData stock;
        stock.rank = i;
        stock.code = QString("60%1").arg(1000 + i, 4, 10, QChar('0'));
        stock.name = QStringLiteral("沪股") + QString::number(i);
        stock.price = 10.0 + QRandomGenerator::global()->bounded(100);
        stock.changePercent = 10.0 - i * 0.5 + QRandomGenerator::global()->bounded(0.3);
        stock.change = stock.price * stock.changePercent / 100.0;
        shGainData.append(stock);
    }
    d->shGainModel->setData(shGainData);

    // 深A涨幅榜
    QVector<StockRankData> szGainData;
    for (int i = 1; i <= 15; ++i) {
        StockRankData stock;
        stock.rank = i;
        stock.code = QString("00%1").arg(1000 + i, 4, 10, QChar('0'));
        stock.name = QStringLiteral("深股") + QString::number(i);
        stock.price = 15.0 + QRandomGenerator::global()->bounded(80);
        stock.changePercent = 9.5 - i * 0.5 + QRandomGenerator::global()->bounded(0.3);
        stock.change = stock.price * stock.changePercent / 100.0;
        szGainData.append(stock);
    }
    d->szGainModel->setData(szGainData);

    // 沪5分钟涨幅
    QVector<StockRankData> sh5MinData;
    for (int i = 1; i <= 15; ++i) {
        StockRankData stock;
        stock.rank = i;
        stock.code = QString("60%1").arg(2000 + i, 4, 10, QChar('0'));
        stock.name = QStringLiteral("沪股") + QString::number(100 + i);
        stock.price = 20.0 + QRandomGenerator::global()->bounded(50);
        stock.changePercent = 5.0 - i * 0.3 + QRandomGenerator::global()->bounded(0.2);
        stock.change = stock.price * stock.changePercent / 100.0;
        sh5MinData.append(stock);
    }
    d->sh5MinModel->setData(sh5MinData);

    // 深5分钟涨幅
    QVector<StockRankData> sz5MinData;
    for (int i = 1; i <= 15; ++i) {
        StockRankData stock;
        stock.rank = i;
        stock.code = QString("00%1").arg(2000 + i, 4, 10, QChar('0'));
        stock.name = QStringLiteral("深股") + QString::number(200 + i);
        stock.price = 25.0 + QRandomGenerator::global()->bounded(40);
        stock.changePercent = 4.5 - i * 0.3 + QRandomGenerator::global()->bounded(0.2);
        stock.change = stock.price * stock.changePercent / 100.0;
        sz5MinData.append(stock);
    }
    d->sz5MinModel->setData(sz5MinData);
}

/**
 * @brief 加载板块数据
 */
void DashboardPage::loadSectorData()
{
    QVector<SectorData> sectorData;
    QStringList sectors = {
        QStringLiteral("半导体"), QStringLiteral("新能源"), QStringLiteral("医药生物"),
        QStringLiteral("白酒"), QStringLiteral("银行"), QStringLiteral("证券"),
        QStringLiteral("房地产"), QStringLiteral("汽车"), QStringLiteral("电子"),
        QStringLiteral("计算机"), QStringLiteral("通信"), QStringLiteral("传媒"),
        QStringLiteral("电力"), QStringLiteral("钢铁"), QStringLiteral("煤炭")
    };

    for (int i = 0; i < sectors.size(); ++i) {
        SectorData sector;
        sector.rank = i + 1;
        sector.name = sectors[i];
        sector.changePercent = 5.0 - i * 0.6 + QRandomGenerator::global()->bounded(0.5);
        sector.upCount = 20 + QRandomGenerator::global()->bounded(30);
        sector.downCount = 10 + QRandomGenerator::global()->bounded(20);
        sector.amount = 1000000000 + QRandomGenerator::global()->bounded(5000000000);
        sectorData.append(sector);
    }
    d->sectorModel->setData(sectorData);
}

/**
 * @brief 加载自选股数据
 */
void DashboardPage::loadWatchlistData()
{
    QVector<StockRankData> watchlistData;
    QStringList codes = {"600519", "000858", "601318", "000001", "600036", "601166", "000333", "002594"};
    QStringList names = {QStringLiteral("贵州茅台"), QStringLiteral("五粮液"), 
                         QStringLiteral("中国平安"), QStringLiteral("平安银行"), 
                         QStringLiteral("招商银行"), QStringLiteral("兴业银行"),
                         QStringLiteral("美的集团"), QStringLiteral("比亚迪")};

    for (int i = 0; i < codes.size(); ++i) {
        StockRankData stock;
        stock.rank = i + 1;
        stock.code = codes[i];
        stock.name = names[i];
        stock.price = 50.0 + QRandomGenerator::global()->bounded(1800);
        stock.changePercent = -5.0 + QRandomGenerator::global()->bounded(10);
        stock.change = stock.price * stock.changePercent / 100.0;
        stock.volume = 100000 + QRandomGenerator::global()->bounded(1000000);
        stock.amount = stock.price * stock.volume;
        stock.turnover = 0.5 + QRandomGenerator::global()->bounded(5);
        stock.pe = 10 + QRandomGenerator::global()->bounded(40);
        watchlistData.append(stock);
    }
    d->watchlistModel->setData(watchlistData);
}

/**
 * @brief 加载新闻数据
 */
void DashboardPage::loadNewsData()
{
    d->newsList->clear();

    QStringList newsItems = {
        QStringLiteral("【要闻】央行：稳健的货币政策要灵活精准、合理适度"),
        QStringLiteral("【要闻】沪深两市成交额突破1.2万亿元，北向资金净买入超50亿"),
        QStringLiteral("【研报】中金公司：看好下半年A股市场表现"),
        QStringLiteral("【公告】贵州茅台：2024年一季度净利润同比增长18%"),
        QStringLiteral("【要闻】科技板块午后拉升，半导体领涨"),
        QStringLiteral("【研报】中信证券：新能源汽车销量创新高，产业链受益"),
        QStringLiteral("【公告】比亚迪：4月新能源汽车销量同比增长40%"),
        QStringLiteral("【要闻】国务院发布稳经济一揽子政策措施"),
        QStringLiteral("【研报】国泰君安：看好消费复苏主线"),
        QStringLiteral("【公告】中国平安：一季度保费收入同比增长5%")
    };

    for (const QString& news : newsItems) {
        QString time = QString::number(15 - d->newsList->count()).rightJustified(2, '0') + ":" + 
                       QString::number(QRandomGenerator::global()->bounded(60)).rightJustified(2, '0');
        QString displayText = QString("[%1] %2").arg(time, news);
        d->newsList->addItem(displayText);
    }
}

/**
 * @brief 加载资金流向数据
 */
void DashboardPage::loadMoneyFlowData()
{
    QVector<MoneyFlowData> flowData;
    QStringList names = {QStringLiteral("贵州茅台"), QStringLiteral("宁德时代"), 
                         QStringLiteral("比亚迪"), QStringLiteral("中国平安"),
                         QStringLiteral("招商银行"), QStringLiteral("五粮液"),
                         QStringLiteral("美的集团"), QStringLiteral("隆基绿能")};

    for (int i = 0; i < names.size(); ++i) {
        MoneyFlowData flow;
        flow.rank = i + 1;
        flow.code = QString("600%1").arg(500 + i, 3, 10, QChar('0'));
        flow.name = names[i];
        flow.netInflow = (100000000 + QRandomGenerator::global()->bounded(500000000)) * (i % 2 == 0 ? 1 : -1);
        flow.netInflowPercent = 3.0 + QRandomGenerator::global()->bounded(10) * (i % 2 == 0 ? 1 : -1);
        flow.day3Inflow = flow.netInflow * 2.5;
        flow.day5Inflow = flow.netInflow * 4.0;
        flowData.append(flow);
    }
    d->moneyFlowModel->setData(flowData);
}

/**
 * @brief 刷新数据
 */
void DashboardPage::refreshData()
{
    loadDemoData();
    LOG_DEBUG("Dashboard data refreshed");
}

/**
 * @brief 搜索变化处理
 */
void DashboardPage::onSearchChanged(const QString& text)
{
    LOG_DEBUG(QString("Search: %1").arg(text));
}

/**
 * @brief 市场切换处理
 */
void DashboardPage::onMarketChanged(int index)
{
    LOG_DEBUG(QString("Market changed: %1").arg(index));
}

/**
 * @brief 板块Tab切换处理
 */
void DashboardPage::onSectorTabChanged(int index)
{
    LOG_DEBUG(QString("Sector tab changed: %1").arg(index));
    loadSectorData();
}

/**
 * @brief 自选股筛选变化处理
 */
void DashboardPage::onWatchlistFilterChanged(int index)
{
    LOG_DEBUG(QString("Watchlist filter changed: %1").arg(index));
}

/**
 * @brief 资金流向周期变化处理
 */
void DashboardPage::onMoneyFlowPeriodChanged(int index)
{
    LOG_DEBUG(QString("Money flow period changed: %1").arg(index));
}

/**
 * @brief 行双击处理
 */
void DashboardPage::onRowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    LOG_INFO(QString("Row double clicked: %1").arg(index.row()));
}

/**
 * @brief 实时数据更新
 */
void DashboardPage::updateRealTimeData()
{
    // 模拟实时价格更新
    for (int i = 0; i < d->indexData.size(); ++i) {
        double change = QRandomGenerator::global()->bounded(10.0) - 5.0;
        d->indexData[i].current += change;
        d->indexData[i].change += change;
    }
    updateIndexDisplay();
}

/**
 * @brief 更新时间显示
 */
void DashboardPage::updateTimeDisplay()
{
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("yyyy-MM-dd hh:mm:ss");

    QTime time = now.time();
    bool isTrading = (time >= QTime(9, 30) && time <= QTime(11, 30)) ||
                     (time >= QTime(13, 0) && time <= QTime(15, 0));

    if (isTrading) {
        timeStr += QStringLiteral(" 交易中");
        d->timeLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::Success));
    } else {
        timeStr += QStringLiteral(" 休市");
        d->timeLabel->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::TextTertiary));
    }

    d->timeLabel->setText(timeStr);

    // 更新状态
    int upCount = 1234;
    int downCount = 890;
    d->statusLabel->setText(QString("上涨:%1 下跌:%2").arg(upCount).arg(downCount));
}

void DashboardPage::resizeEvent(QResizeEvent* event)
{
    BasePage::resizeEvent(event);
}

void DashboardPage::showEvent(QShowEvent* event)
{
    BasePage::showEvent(event);
    d->updateTimer->start();
    d->clockTimer->start();
    LOG_DEBUG("DashboardPage shown, timers started");
}

void DashboardPage::hideEvent(QHideEvent* event)
{
    BasePage::hideEvent(event);
    d->updateTimer->stop();
    d->clockTimer->stop();
    LOG_DEBUG("DashboardPage hidden, timers stopped");
}
