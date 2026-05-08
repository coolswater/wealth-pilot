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
 * - 主背景：Colors::BgBase
 * - 卡片背景：Colors::BgElevated
 * - 涨：Colors::Danger（红），跌：Colors::Success（绿）
 * - 紧凑布局，单屏最大信息量
 */

#include "DashboardPage.h"
#include "core/config/Tokens.h"
#include "ui/ThemeManager.h"
#include "market/StockDataSource.h"
#include "data/DataStorageService.h"
#include "core/cache/CacheManager.h"
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
#include <QMessageBox>
#include <QDialog>
#include <QTextEdit>
#include <algorithm>

// ============================================================================
// 涨跌颜色委托 - 高性能绘制
// ============================================================================

/**
 * @brief 涨跌颜色委托
 * @details 根据涨跌数据显示不同颜色，红涨绿跌
 */
class ChangeColorDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        // 先绘制背景
        painter->save();
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        // 获取数值和文本
        double value = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();
        
        // 确定颜色（红涨绿跌）- 使用 Tokens::Color
        QColor textColor;
        if (value > 0.0) {
            textColor = Tokens::Color::danger();   // 红色 - 上涨
        } else if (value < 0.0) {
            textColor = Tokens::Color::success();  // 绿色 - 下跌
        } else {
            textColor = Tokens::Color::textSecondary(); // 灰色 - 平盘
        }

        // 绘制文字
        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);
        
        // 计算文字位置（右对齐）
        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        int flags = Qt::AlignRight | Qt::AlignVCenter;
        painter->drawText(textRect, flags, text);
        painter->restore();
    }
};

/**
 * @brief 资金流向颜色委托
 */
class MoneyFlowDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        // 先绘制背景
        painter->save();
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        // 获取数值和文本
        double value = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();
        
        // 确定颜色（红涨绿跌）- 使用 Tokens::Color
        QColor textColor = value >= 0 
            ? Tokens::Color::danger()   // 红色 - 上涨
            : Tokens::Color::success();  // 绿色 - 下跌

        // 绘制文字
        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);
        
        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        int flags = Qt::AlignRight | Qt::AlignVCenter;
        painter->drawText(textRect, flags, text);
        painter->restore();
    }
};

/**
 * @brief 现价颜色委托
 * @details 根据涨跌显示现价颜色，红涨绿跌
 */
class PriceColorDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        // 先绘制背景
        painter->save();
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, Tokens::Color::primary());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor(255, 255, 255, 13));
        }
        painter->restore();

        // 获取涨跌幅数据用于颜色判断
        double changePercent = index.data(Qt::UserRole).toDouble();
        QString text = index.data(Qt::DisplayRole).toString();
        
        // 确定颜色（红涨绿跌）- 使用 Tokens::Color
        QColor textColor;
        if (changePercent > 0.0) {
            textColor = Tokens::Color::danger();    // 红色 - 上涨
        } else if (changePercent < 0.0) {
            textColor = Tokens::Color::success();   // 绿色 - 下跌
        } else {
            textColor = Tokens::Color::textPrimary(); // 白色 - 平盘
        }

        // 绘制文字
        painter->save();
        painter->setPen(textColor);
        painter->setFont(option.font);
        QRect textRect = option.rect.adjusted(4, 0, -4, 0);
        int flags = Qt::AlignRight | Qt::AlignVCenter;
        painter->drawText(textRect, flags, text);
        painter->restore();
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

    // 自选股代码列表
    QStringList watchlistSymbols = {
        "sh600519", "sh601318", "sz000858", "sz000001", "sh600036",
        "sz002594", "sz300750", "sh601012", "sz000333", "sh600900"
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
            case ColCode:
                return stock.code;
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

    // 为现价、涨跌幅、涨跌额列返回涨跌幅数据（用于颜色判断）
    if (role == Qt::UserRole) {
        return stock.changePercent;
    }

    // 为涨跌额列返回涨跌额数据（用于颜色判断）
    if (role == Qt::UserRole + 1) {
        return stock.change;
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColCode || index.column() == ColName) {
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
        QStringLiteral("代码"),
        QStringLiteral("名称"),
        QStringLiteral("最新价"),
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

    // 为现价、涨跌幅、涨跌额列返回涨跌幅数据（用于颜色判断）
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
            case ColCode: return flow.code;
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

    // 为数值列返回数据用于颜色判断（红涨绿跌）
    if (role == Qt::UserRole) {
        switch (index.column()) {
            case ColNetInflow: return flow.netInflow;
            case ColNetInflowPercent: return flow.netInflowPercent;
            case ColDay3: return flow.day3Inflow;
            case ColDay5: return flow.day5Inflow;
            default: return QVariant();
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColCode || index.column() == ColName) {
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
        QStringLiteral("代码"),
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

    // 设置背景色 - 使用 ThemeManager
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    setStyleSheet(QString("background-color: %1;").arg(theme.bgPrimary));

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

    // 初始化数据存储服务
    if (!DataStorageService::instance()->isInitialized()) {
        DataStorageService::instance()->initialize();
    }

    // 初始化缓存管理器
    CacheManager::instance()->initialize();

    setupConnections();
    
    // 数据加载流程：缓存 -> 数据库 -> 网络数据源
    loadDataWithFallback();

    setInitialized(true);

    LOG_DEBUG("DashboardPage initialized");
}

/**
 * @brief 初始化UI布局
 */
void DashboardPage::setupUI()
{
    // 注册主题监听器
    ThemeManager::instance()->registerThemeChangeListener(this, [this]() {
        updateTheme();
    });

    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setContentsMargins(0, 0, 0, 0);
    d->mainLayout->setSpacing(0);

    // 1. 顶部工具栏
    setupHeader();

    // 2. 主分割器
    d->mainSplitter = new QSplitter(Qt::Vertical, this);
    d->mainSplitter->setHandleWidth(1);
    d->mainSplitter->setChildrenCollapsible(false);  // 禁止折叠
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

    // 设置分割比例
    d->mainSplitter->setSizes({120, 350, 280});

    d->mainLayout->addWidget(d->mainSplitter, 1);
}

/**
 * @brief 初始化头部工具栏
 */
void DashboardPage::setupHeader()
{
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    QFrame* header = new QFrame(this);
    header->setFixedHeight(48);
    header->setStyleSheet(QString("background-color: %1; border-bottom: 1px solid %2;")
        .arg(theme.bgElevated, theme.border));

    QHBoxLayout* layout = new QHBoxLayout(header);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(16);

    // 页面标题
    QLabel* titleLabel = new QLabel(QStringLiteral("行情看板"), header);
    titleLabel->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;")
        .arg(theme.textPrimary));
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
    )").arg(theme.bgElevated, theme.border, theme.textPrimary, theme.primary));
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
    )").arg(theme.bgElevated, theme.border, theme.textPrimary, theme.textTertiary));
    layout->addWidget(d->searchEdit);

    layout->addStretch();

    // 时间显示
    d->timeLabel = new QLabel(header);
    d->timeLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(theme.textSecondary));
    layout->addWidget(d->timeLabel);

    layout->addSpacing(20);

    // 状态信息
    d->statusLabel = new QLabel(header);
    d->statusLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
        .arg(theme.textTertiary));
    layout->addWidget(d->statusLabel);

    d->mainLayout->addWidget(header);
}

/**
 * @brief 初始化指数面板
 */
void DashboardPage::setupIndexPanel()
{
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    d->indexPanel = new QFrame(this);
    d->indexPanel->setStyleSheet(QString("background-color: %1;").arg(theme.bgElevated));

    QHBoxLayout* layout = new QHBoxLayout(d->indexPanel);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // 创建7个指数卡片（与indexSymbols对应）
    QStringList indexNames = {
        QStringLiteral("上证指数"),
        QStringLiteral("深证成指"),
        QStringLiteral("创业板指"),
        QStringLiteral("科创50"),
        QStringLiteral("上证50"),
        QStringLiteral("沪深300"),
        QStringLiteral("北证50")
    };
    for (int i = 0; i < 7; ++i) {
        QFrame* card = new QFrame(d->indexPanel);
        card->setStyleSheet(QString(R"(
            QFrame {
                background-color: %1;
                border-radius: 6px;
                border: 1px solid %2;
            }
        )").arg(theme.bgPrimary, theme.border));

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 10, 10, 10);
        cardLayout->setSpacing(4);

        // 指数名称
        QLabel* nameLabel = new QLabel(indexNames[i], card);
        nameLabel->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold;")
            .arg(theme.textPrimary));
        cardLayout->addWidget(nameLabel);
        d->indexNameLabels.append(nameLabel);

        // 价格和涨跌
        QHBoxLayout* priceLayout = new QHBoxLayout();
        
        QLabel* priceLabel = new QLabel(QStringLiteral("0.00"), card);
        priceLabel->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold;")
            .arg(theme.textPrimary));
        priceLayout->addWidget(priceLabel, 1);
        d->indexPriceLabels.append(priceLabel);

        QLabel* changeLabel = new QLabel(QStringLiteral("+0.00%"), card);
        changeLabel->setStyleSheet(QString("color: %1; font-size: 10px;")
            .arg(theme.danger));
        priceLayout->addWidget(changeLabel, 2);
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
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    d->rankGridPanel = new QFrame(this);
    d->rankGridPanel->setStyleSheet(QString("background-color: %1;").arg(theme.bgPrimary));

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
            font-size: 12px;
        }
        QHeaderView::section {
            background-color: %5;
            color: %6;
            padding: 4px 6px;
            border: none;
            border-bottom: 1px solid %3;
            font-size: 11px;
            font-weight: bold;
        }
    )").arg(theme.bgElevated, theme.border, theme.border, theme.primary, theme.bgSurface, theme.textSecondary);

    // 设置股票排行表格列宽的辅助函数
    auto setupRankTableColumns = [](QTableView* table) {
        // 固定列宽设置（适配约600px宽度）
        table->setColumnWidth(StockRankModel::ColRank, 45);      // 排名
        table->setColumnWidth(StockRankModel::ColCode, 75);     // 代码
        table->setColumnWidth(StockRankModel::ColName, 90);      // 名称
        table->setColumnWidth(StockRankModel::ColPrice, 80);     // 现价
        table->setColumnWidth(StockRankModel::ColChange, 85);    // 涨跌幅
        // 涨跌额列自动填充剩余空间
        table->horizontalHeader()->setStretchLastSection(true);
        // 禁止用户调整列宽
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        table->horizontalHeader()->setSectionResizeMode(StockRankModel::ColChangeAmount, QHeaderView::Stretch);
    };

    // 卡片标题样式
    auto createCardWithTitle = [this, &tableStyle, &setupRankTableColumns, &theme](const QString& title, QTableView*& table, 
                                                    QAbstractTableModel* model) -> QFrame* {
        QFrame* card = new QFrame(d->rankGridPanel);
        card->setStyleSheet(QString("QFrame { background-color: %1; border-radius: 6px; }")
            .arg(theme.bgElevated));

        QVBoxLayout* layout = new QVBoxLayout(card);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // 标题栏
        QFrame* titleBar = new QFrame(card);
        titleBar->setFixedHeight(28);
        titleBar->setStyleSheet(QString("background-color: %1; border-top-left-radius: 6px; border-top-right-radius: 6px;")
            .arg(theme.bgSurface));
        QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(10, 0, 10, 0);

        QLabel* titleLabel = new QLabel(title, titleBar);
        titleLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;")
            .arg(theme.textPrimary));
        titleLayout->addWidget(titleLabel);
        titleLayout->addStretch();

        layout->addWidget(titleBar);

        // 表格
        table = new QTableView(card);
        table->setModel(model);
        // 为现价、涨跌幅、涨跌额列设置颜色委托
        table->setItemDelegateForColumn(StockRankModel::ColPrice, new PriceColorDelegate(this));
        table->setItemDelegateForColumn(StockRankModel::ColChange, new ChangeColorDelegate(this));
        table->setItemDelegateForColumn(StockRankModel::ColChangeAmount, new ChangeColorDelegate(this));
        table->setAlternatingRowColors(true);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->verticalHeader()->setVisible(false);
        table->setShowGrid(false);
        table->setStyleSheet(tableStyle);
        table->verticalHeader()->setDefaultSectionSize(22);
        layout->addWidget(table);

        // 设置列宽
        setupRankTableColumns(table);

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
        .arg(theme.bgElevated));
    QVBoxLayout* sectorLayout = new QVBoxLayout(sectorCard);
    sectorLayout->setContentsMargins(0, 0, 0, 0);
    sectorLayout->setSpacing(0);

    // Tab标题栏
    QFrame* sectorHeader = new QFrame(sectorCard);
    sectorHeader->setFixedHeight(28);
    sectorHeader->setStyleSheet(QString("background-color: %1;")
        .arg(theme.bgSurface));
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
    )").arg(theme.textSecondary, theme.primary));
    
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

    // 设置板块表格列宽（适配约600px宽度）
    d->sectorTable->setColumnWidth(0, 120);  // 板块名称
    d->sectorTable->setColumnWidth(1, 90);   // 涨跌幅
    d->sectorTable->setColumnWidth(2, 80);   // 涨/跌
    d->sectorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    d->sectorTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);  // 成交额自动填充

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
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    // 使用固定布局，不允许用户调整
    QWidget* infoContainer = new QWidget(this);
    infoContainer->setStyleSheet(QString("background-color: %1;").arg(theme.bgPrimary));
    
    QHBoxLayout* layout = new QHBoxLayout(infoContainer);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // 1. 自选股列表
    setupWatchlistPanel();
    layout->addWidget(d->watchlistContainer, 1);

    // 2. 新闻资讯
    setupNewsPanel();
    layout->addWidget(d->newsPanel, 1);

    // 3. 资金流向
    setupMoneyFlowPanel();
    layout->addWidget(d->moneyFlowContainer, 1);
    
    // 替换原来的 infoSplitter
    d->infoSplitter = nullptr;  // 不再使用 splitter
    
    // 将容器添加到主布局
    d->mainSplitter->addWidget(infoContainer);
}

/**
 * @brief 初始化自选股面板
 */
void DashboardPage::setupWatchlistPanel()
{
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    d->watchlistContainer = new QWidget(this);
    d->watchlistContainer->setStyleSheet(QString("QFrame { background-color: %1; border-radius: 6px; }")
        .arg(theme.bgElevated));

    QVBoxLayout* layout = new QVBoxLayout(d->watchlistContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 标题栏（与六宫格一致）
    QFrame* header = new QFrame(d->watchlistContainer);
    header->setFixedHeight(28);
    header->setStyleSheet(QString("background-color: %1; border-top-left-radius: 6px; border-top-right-radius: 6px;")
        .arg(theme.bgSurface));
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 0, 10, 0);

    QLabel* title = new QLabel(QStringLiteral("自选股"), header);
    title->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;")
        .arg(theme.textPrimary));
    headerLayout->addWidget(title);

    d->watchlistFilter = new QComboBox(header);
    d->watchlistFilter->addItems({QStringLiteral("全部自选"), QStringLiteral("持仓")});
    d->watchlistFilter->setStyleSheet(QString("background: transparent; color: %1; border: none; font-size: 11px;")
        .arg(theme.textSecondary));
    headerLayout->addWidget(d->watchlistFilter);
    headerLayout->addStretch();

    layout->addWidget(header);

    // 表格样式（与六宫格一致）
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
            font-size: 12px;
        }
        QHeaderView::section {
            background-color: %5;
            color: %6;
            padding: 4px 6px;
            border: none;
            border-bottom: 1px solid %3;
            font-size: 11px;
            font-weight: bold;
        }
    )").arg(theme.bgElevated, theme.border, theme.border, theme.primary, theme.bgSurface, theme.textSecondary);

    // 表格
    d->watchlistModel = new WatchlistModel(this);
    d->watchlistTable = new QTableView(d->watchlistContainer);
    d->watchlistTable->setModel(d->watchlistModel);
    // 为最新价、涨跌幅、涨跌额列设置颜色委托
    d->watchlistTable->setItemDelegateForColumn(WatchlistModel::ColPrice, new PriceColorDelegate(this));
    d->watchlistTable->setItemDelegateForColumn(WatchlistModel::ColChange, new ChangeColorDelegate(this));
    d->watchlistTable->setItemDelegateForColumn(WatchlistModel::ColChangeAmount, new ChangeColorDelegate(this));
    d->watchlistTable->setAlternatingRowColors(true);
    d->watchlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->watchlistTable->verticalHeader()->setVisible(false);
    d->watchlistTable->setShowGrid(false);
    d->watchlistTable->setStyleSheet(tableStyle);
    d->watchlistTable->verticalHeader()->setDefaultSectionSize(22);
    
    // 设置自选股表格列宽（适配约600px宽度）
    d->watchlistTable->setColumnWidth(WatchlistModel::ColRank, 40);      // 序号
    d->watchlistTable->setColumnWidth(WatchlistModel::ColCode, 70);     // 代码
    d->watchlistTable->setColumnWidth(WatchlistModel::ColName, 75);     // 名称
    d->watchlistTable->setColumnWidth(WatchlistModel::ColPrice, 70);    // 最新价
    d->watchlistTable->setColumnWidth(WatchlistModel::ColChange, 70);   // 涨跌幅
    d->watchlistTable->setColumnWidth(WatchlistModel::ColChangeAmount, 65); // 涨跌额
    d->watchlistTable->setColumnWidth(WatchlistModel::ColVolume, 60);   // 总量
    d->watchlistTable->setColumnWidth(WatchlistModel::ColAmount, 60);   // 金额
    d->watchlistTable->setColumnWidth(WatchlistModel::ColTurnover, 60); // 换手率
    // 市盈率列自动填充
    d->watchlistTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    d->watchlistTable->horizontalHeader()->setSectionResizeMode(WatchlistModel::ColPE, QHeaderView::Stretch);
    
    layout->addWidget(d->watchlistTable);
}

/**
 * @brief 初始化新闻面板
 */
void DashboardPage::setupNewsPanel()
{
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    d->newsPanel = new QFrame(this);
    d->newsPanel->setStyleSheet(QString("QFrame { background-color: %1; border-radius: 6px; }")
        .arg(theme.bgElevated));

    QVBoxLayout* layout = new QVBoxLayout(d->newsPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 标题栏（与六宫格一致）
    QFrame* header = new QFrame(d->newsPanel);
    header->setFixedHeight(28);
    header->setStyleSheet(QString("background-color: %1; border-top-left-radius: 6px; border-top-right-radius: 6px;")
        .arg(theme.bgSurface));
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 0, 10, 0);

    QLabel* title = new QLabel(QStringLiteral("24小时滚动新闻"), header);
    title->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;")
        .arg(theme.textPrimary));
    headerLayout->addWidget(title);
    headerLayout->addStretch();

    QLabel* moreLabel = new QLabel(QStringLiteral("更多 >"), header);
    moreLabel->setStyleSheet(QString("color: %1; font-size: 11px;")
        .arg(theme.textTertiary));
    headerLayout->addWidget(moreLabel);

    layout->addWidget(header);

    // 新闻列表（与六宫格表格样式一致）
    d->newsList = new QListWidget(d->newsPanel);
    d->newsList->setStyleSheet(QString(R"(
        QListWidget {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 4px;
        }
        QListWidget::item {
            color: %3;
            font-size: 12px;
            padding: 6px 8px;
            border-bottom: 1px solid %4;
        }
        QListWidget::item:hover {
            background-color: %5;
        }
    )").arg(theme.bgElevated, theme.border, theme.textPrimary, theme.border, theme.bgHover));
    layout->addWidget(d->newsList);
}

/**
 * @brief 初始化资金流向面板
 */
void DashboardPage::setupMoneyFlowPanel()
{
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    d->moneyFlowContainer = new QWidget(this);
    d->moneyFlowContainer->setStyleSheet(QString("QFrame { background-color: %1; border-radius: 6px; }")
        .arg(theme.bgElevated));

    QVBoxLayout* layout = new QVBoxLayout(d->moneyFlowContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 标题栏（与六宫格一致）
    QFrame* header = new QFrame(d->moneyFlowContainer);
    header->setFixedHeight(28);
    header->setStyleSheet(QString("background-color: %1; border-top-left-radius: 6px; border-top-right-radius: 6px;")
        .arg(theme.bgSurface));
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(10, 0, 10, 0);

    QLabel* title = new QLabel(QStringLiteral("资金流向"), header);
    title->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold;")
        .arg(theme.textPrimary));
    headerLayout->addWidget(title);

    d->moneyFlowPeriod = new QComboBox(header);
    d->moneyFlowPeriod->addItems({QStringLiteral("当日"), QStringLiteral("3日"), QStringLiteral("5日")});
    d->moneyFlowPeriod->setStyleSheet(QString("background: transparent; color: %1; border: none; font-size: 11px;")
        .arg(theme.textSecondary));
    headerLayout->addWidget(d->moneyFlowPeriod);
    headerLayout->addStretch();

    layout->addWidget(header);

    // 表格样式（与六宫格一致）
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
            font-size: 12px;
        }
        QHeaderView::section {
            background-color: %5;
            color: %6;
            padding: 4px 6px;
            border: none;
            border-bottom: 1px solid %3;
            font-size: 11px;
            font-weight: bold;
        }
    )").arg(theme.bgElevated, theme.border, theme.border, theme.primary, theme.bgSurface, theme.textSecondary);

    // 表格
    d->moneyFlowModel = new MoneyFlowModel(this);
    d->moneyFlowTable = new QTableView(d->moneyFlowContainer);
    d->moneyFlowTable->setModel(d->moneyFlowModel);
    // 为净流入、当日增仓、3日增仓、5日增仓列设置颜色委托（红涨绿跌）
    d->moneyFlowTable->setItemDelegateForColumn(MoneyFlowModel::ColNetInflow, new MoneyFlowDelegate(this));
    d->moneyFlowTable->setItemDelegateForColumn(MoneyFlowModel::ColNetInflowPercent, new MoneyFlowDelegate(this));
    d->moneyFlowTable->setItemDelegateForColumn(MoneyFlowModel::ColDay3, new MoneyFlowDelegate(this));
    d->moneyFlowTable->setItemDelegateForColumn(MoneyFlowModel::ColDay5, new MoneyFlowDelegate(this));
    d->moneyFlowTable->setAlternatingRowColors(true);
    d->moneyFlowTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->moneyFlowTable->verticalHeader()->setVisible(false);
    d->moneyFlowTable->setShowGrid(false);
    d->moneyFlowTable->setStyleSheet(tableStyle);
    d->moneyFlowTable->verticalHeader()->setDefaultSectionSize(22);
    
    // 设置资金流向表格列宽（适配约600px宽度）
    d->moneyFlowTable->setColumnWidth(MoneyFlowModel::ColRank, 45);          // 排名
    d->moneyFlowTable->setColumnWidth(MoneyFlowModel::ColCode, 75);         // 代码
    d->moneyFlowTable->setColumnWidth(MoneyFlowModel::ColName, 80);         // 名称
    d->moneyFlowTable->setColumnWidth(MoneyFlowModel::ColNetInflow, 90);     // 净流入
    d->moneyFlowTable->setColumnWidth(MoneyFlowModel::ColNetInflowPercent, 80); // 当日增仓
    d->moneyFlowTable->setColumnWidth(MoneyFlowModel::ColDay3, 80);         // 3日增仓
    // 5日增仓列自动填充
    d->moneyFlowTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    d->moneyFlowTable->horizontalHeader()->setSectionResizeMode(MoneyFlowModel::ColDay5, QHeaderView::Stretch);
    
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
    connect(d->sh5MinTable, &QTableView::doubleClicked, this, &DashboardPage::onRowDoubleClicked);
    connect(d->sz5MinTable, &QTableView::doubleClicked, this, &DashboardPage::onRowDoubleClicked);
    connect(d->watchlistTable, &QTableView::doubleClicked, this, &DashboardPage::onRowDoubleClicked);
    connect(d->moneyFlowTable, &QTableView::doubleClicked, this, &DashboardPage::onMoneyFlowRowDoubleClicked);
    connect(d->sectorTable, &QTableView::doubleClicked, this, &DashboardPage::onSectorRowDoubleClicked);
    
    // 新闻点击弹窗
    connect(d->newsList, &QListWidget::itemClicked, this, &DashboardPage::onNewsItemClicked);
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
    
    // 请求自选股数据
    d->watchlistDataSource->requestQuotes(d->watchlistSymbols);
    
    // 启动自动刷新（5秒）
    d->indexDataSource->startAutoRefresh(5000);
    d->rankDataSource->startAutoRefresh(5000);
    d->watchlistDataSource->startAutoRefresh(5000);
    
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
    
    // 更新指数名称显示
    for (int i = 0; i < d->indexData.size() && i < d->indexNameLabels.size(); ++i) {
        d->indexNameLabels[i]->setText(d->indexData[i].name);
    }
    
    // 保存数据到本地数据库
    saveIndexDataToDb(quotes);
    saveQuoteCacheToDb(quotes);
    
    d->statusLabel->setText(QString("已更新 %1")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
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
        // 换手率和市盈率需要额外数据，暂时设置为0
        stock.turnover = 0.0;
        stock.pe = 0.0;
        watchlistData.append(stock);
    }
    d->watchlistModel->setData(watchlistData);
    
    // 保存自选股数据到缓存
    saveQuoteCacheToDb(quotes);
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
        {"sz399006", QStringLiteral("创业板指"), 2156.89, 32.45, 1.53},
        {"sh000688", QStringLiteral("科创50"), 987.65, -12.34, -1.24},
        {"sh000016", QStringLiteral("上证50"), 2567.89, 15.23, 0.60},
        {"sh000300", QStringLiteral("沪深300"), 3892.45, 18.67, 0.48},
        {"bj899050", QStringLiteral("北证50"), 1234.56, -8.90, -0.72}
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
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    for (int i = 0; i < d->indexData.size() && i < d->indexPriceLabels.size(); ++i) {
        const IndexData& idx = d->indexData[i];
        d->indexPriceLabels[i]->setText(QString::number(idx.current, 'f', 2));
        
        QString changeText = idx.change >= 0 
            ? QString("+%1 (+%2%)").arg(idx.change, 0, 'f', 2).arg(idx.changePercent, 0, 'f', 2)
            : QString("%1 (%2%)").arg(idx.change, 0, 'f', 2).arg(idx.changePercent, 0, 'f', 2);
        d->indexChangeLabels[i]->setText(changeText);
        d->indexChangeLabels[i]->setStyleSheet(
            QString("color: %1; font-size: 13px;").arg(idx.change >= 0 ? theme.danger : theme.success));
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
    auto* storage = DataStorageService::instance();
    
    // 优先从本地数据库读取自选股列表
    QStringList symbols = storage->getWatchlistSymbols();
    
    if (symbols.isEmpty()) {
        // 如果本地没有数据，使用默认自选股列表
        symbols = {"sh600519", "sz000858", "sh601318", "sz000001", "sh600036", 
                   "sh601166", "sz000333", "sz002594"};
        
        // 保存默认自选股到数据库
        QStringList names = {QStringLiteral("贵州茅台"), QStringLiteral("五粮液"), 
                             QStringLiteral("中国平安"), QStringLiteral("平安银行"), 
                             QStringLiteral("招商银行"), QStringLiteral("兴业银行"),
                             QStringLiteral("美的集团"), QStringLiteral("比亚迪")};
        
        for (int i = 0; i < symbols.size(); ++i) {
            storage->addWatchlistItem(symbols[i], names[i]);
        }
    }
    
    // 从网络获取实时行情
    if (d->watchlistDataSource) {
        d->watchlistDataSource->requestQuotes(symbols);
    } else {
        // 如果网络不可用，从本地缓存加载
        QVector<CachedQuoteData> cacheData = storage->getAllQuoteCache();
        QVector<StockQuote> quotes;
        
        for (const auto& cache : cacheData) {
            if (symbols.contains(cache.symbol)) {
                StockQuote quote;
                quote.symbol = cache.symbol;
                quote.name = cache.name;
                quote.lastPrice = cache.lastPrice;
                quote.changePercent = cache.changePercent;
                quote.changeAmount = cache.changeAmount;
                quote.volume = cache.volume;
                quote.turnover = cache.amount;
                quotes.append(quote);
            }
        }
        
        if (!quotes.isEmpty()) {
            onWatchlistQuotesReceived(quotes);
        }
    }
}

/**
 * @brief 加载新闻数据
 */
void DashboardPage::loadNewsData()
{
    d->newsList->clear();
    
    auto* storage = DataStorageService::instance();
    
    // 优先从本地数据库读取新闻
    QVector<NewsItem> localNews = storage->getLatestNews(20);
    
    if (!localNews.isEmpty()) {
        for (const auto& news : localNews) {
            QString time = news.publishTime.toString("HH:mm");
            QString displayText = QString("[%1] %2").arg(time, news.title);
            d->newsList->addItem(displayText);
        }
        LOG_DEBUG(QString("Loaded %1 news from local database").arg(localNews.size()));
        return;
    }
    
    // 如果本地没有数据，使用默认新闻
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
    
    // 使用真实的股票代码和名称
    struct StockInfo {
        QString code;
        QString name;
    };
    QVector<StockInfo> stocks = {
        {"sh600519", QStringLiteral("贵州茅台")},
        {"sz300750", QStringLiteral("宁德时代")},
        {"sz002594", QStringLiteral("比亚迪")},
        {"sh601318", QStringLiteral("中国平安")},
        {"sh600036", QStringLiteral("招商银行")},
        {"sz000858", QStringLiteral("五粮液")},
        {"sz000333", QStringLiteral("美的集团")},
        {"sh601012", QStringLiteral("隆基绿能")}
    };

    for (int i = 0; i < stocks.size(); ++i) {
        MoneyFlowData flow;
        flow.rank = i + 1;
        flow.code = stocks[i].code;
        flow.name = stocks[i].name;
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
    
    // 获取发送信号的表格
    QTableView* senderTable = qobject_cast<QTableView*>(sender());
    if (!senderTable) return;
    
    // 获取模型
    QAbstractItemModel* model = senderTable->model();
    
    // 获取代码和名称列的数据
    QString code = model->data(model->index(index.row(), StockRankModel::ColCode)).toString();
    QString name = model->data(model->index(index.row(), StockRankModel::ColName)).toString();
    
    LOG_INFO(QString("Dashboard row double clicked: %1 (%2)").arg(code, name));
    
    // 发送导航信号
    emit navigateToStockKLine(code, name);
}

void DashboardPage::onMoneyFlowRowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    // 获取代码和名称
    QString code = d->moneyFlowModel->data(d->moneyFlowModel->index(index.row(), MoneyFlowModel::ColCode)).toString();
    QString name = d->moneyFlowModel->data(d->moneyFlowModel->index(index.row(), MoneyFlowModel::ColName)).toString();
    
    LOG_INFO(QString("Money flow row double clicked: %1 (%2)").arg(code, name));
    
    // 发送导航信号
    emit navigateToStockKLine(code, name);
}

void DashboardPage::onSectorRowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    // 获取板块名称
    QString sectorName = d->sectorModel->data(d->sectorModel->index(index.row(), 0)).toString();
    
    LOG_INFO(QString("Sector row double clicked: %1").arg(sectorName));
    
    // 板块点击跳转到板块详情页面
    // 由于没有专门的板块详情页面，这里显示板块成分股
    // 可以通过信号通知主窗口跳转
    QMessageBox::information(nullptr, QStringLiteral("板块详情"), 
        QStringLiteral("板块：%1\n\n该功能将在后续版本中实现，届时将显示板块成分股和相关分析。").arg(sectorName));
}

/**
 * @brief 实时数据更新
 * @note 真实数据由 StockDataSource 自动刷新，此函数仅用于备用
 */
void DashboardPage::updateRealTimeData()
{
    // 真实数据由 StockDataSource 自动刷新，此函数不再更新模拟数据
    // 仅在非交易时间检查状态
    QTime now = QTime::currentTime();
    bool isTrading = (now >= QTime(9, 30) && now <= QTime(11, 30)) ||
                     (now >= QTime(13, 0) && now <= QTime(15, 0));
    
    if (!isTrading) {
        // 非交易时间可以停止数据刷新以节省资源
        // 但保持时钟更新
    }
}

/**
 * @brief 更新时间显示
 */
void DashboardPage::updateTimeDisplay()
{
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    QDateTime now = QDateTime::currentDateTime();
    QString timeStr = now.toString("yyyy-MM-dd hh:mm:ss");

    QTime time = now.time();
    bool isTrading = (time >= QTime(9, 30) && time <= QTime(11, 30)) ||
                     (time >= QTime(13, 0) && time <= QTime(15, 0));

    if (isTrading) {
        timeStr += QStringLiteral(" 交易中");
        d->timeLabel->setStyleSheet(QString("color: %1;").arg(theme.success));
    } else {
        timeStr += QStringLiteral(" 休市");
        d->timeLabel->setStyleSheet(QString("color: %1;").arg(theme.textTertiary));
    }

    d->timeLabel->setText(timeStr);

    // 从排行榜数据计算涨跌统计
    int upCount = 0;
    int downCount = 0;
    
    // 统计沪A涨跌
    for (int i = 0; i < d->indexData.size(); ++i) {
        if (d->indexData[i].changePercent > 0) upCount++;
        else if (d->indexData[i].changePercent < 0) downCount++;
    }
    
    // 如果没有数据，显示默认值
    if (upCount == 0 && downCount == 0) {
        upCount = 1234;
        downCount = 890;
    }
    
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

// ============================================================================
// 数据存储相关函数
// ============================================================================

/**
 * @brief 检查并加载本地数据
 * @return true 如果成功加载本地数据，false 如果需要从网络获取
 */
bool DashboardPage::checkAndLoadLocalData()
{
    auto* storage = DataStorageService::instance();
    
    // 检查是否有本地数据
    if (!storage->hasLocalData()) {
        LOG_DEBUG("No local data found, will fetch from network");
        return false;
    }
    
    // 获取最后更新时间
    QDateTime lastUpdate = storage->getLastUpdateTime("quotes");
    QDateTime now = QDateTime::currentDateTime();
    
    // 如果数据是今天的，直接使用
    if (lastUpdate.isValid() && lastUpdate.date() == now.date()) {
        LOG_DEBUG("Loading today's cached data");
        loadLocalData();
        return true;
    }
    
    // 检查是否在交易时间外
    QTime time = now.time();
    bool isTrading = (time >= QTime(9, 30) && time <= QTime(11, 30)) ||
                     (time >= QTime(13, 0) && time <= QTime(15, 0));
    
    if (!isTrading) {
        // 非交易时间，使用本地数据
        LOG_DEBUG("Outside trading hours, loading local data");
        loadLocalData();
        return true;
    }
    
    return false;
}

/**
 * @brief 加载本地缓存数据
 */
void DashboardPage::loadLocalData()
{
    auto* storage = DataStorageService::instance();
    
    // 加载行情缓存
    QVector<CachedQuoteData> cacheData = storage->getAllQuoteCache();
    if (!cacheData.isEmpty()) {
        // 转换为 StockQuote 格式
        QVector<StockQuote> quotes;
        for (const auto& cache : cacheData) {
            StockQuote quote;
            quote.symbol = cache.symbol;
            quote.name = cache.name;
            quote.lastPrice = cache.lastPrice;
            quote.changePercent = cache.changePercent;
            quote.changeAmount = cache.changeAmount;
            quote.volume = cache.volume;
            quote.turnover = cache.amount;
            quotes.append(quote);
        }
        
        // 分发数据
        onIndexQuotesReceived(quotes);
        onRankQuotesReceived(quotes);
        onWatchlistQuotesReceived(quotes);
        
        LOG_INFO(QString("Loaded %1 cached quotes").arg(cacheData.size()));
    }
    
    // 加载本地新闻
    QVector<NewsItem> newsItems = storage->getLatestNews(20);
    if (!newsItems.isEmpty()) {
        d->newsList->clear();
        for (const auto& news : newsItems) {
            QString time = news.publishTime.toString("HH:mm");
            QString displayText = QString("[%1] %2").arg(time, news.title);
            d->newsList->addItem(displayText);
        }
        LOG_DEBUG(QString("Loaded %1 cached news").arg(newsItems.size()));
    }
    
    // 加载其他数据
    loadSectorData();
    loadMoneyFlowData();
    
    updateTimeDisplay();
    d->statusLabel->setText(QStringLiteral("本地数据"));
}

/**
 * @brief 保存指数数据到数据库
 */
void DashboardPage::saveIndexDataToDb(const QVector<StockQuote>& quotes)
{
    // 检查是否收盘（15:00后）
    QTime now = QTime::currentTime();
    if (now < QTime(15, 0)) {
        return;  // 未收盘，不保存
    }
    
    auto* storage = DataStorageService::instance();
    QVector<IndexHistoryData> indexData;
    QDate today = QDate::currentDate();
    
    for (const auto& quote : quotes) {
        // 只保存指数数据
        if (quote.symbol.startsWith("sh000") || quote.symbol.startsWith("sz399") ||
            quote.symbol.startsWith("bj899")) {
            IndexHistoryData data;
            data.code = quote.symbol;
            data.name = quote.name;
            data.closePrice = quote.lastPrice;
            data.changePercent = quote.changePercent;
            data.volume = quote.volume;
            data.amount = quote.turnover;
            data.date = today;
            indexData.append(data);
        }
    }
    
    if (!indexData.isEmpty()) {
        storage->saveIndexDataBatch(indexData);
        LOG_INFO(QString("Saved %1 index records to database").arg(indexData.size()));
    }
}

/**
 * @brief 保存行情缓存到数据库
 */
void DashboardPage::saveQuoteCacheToDb(const QVector<StockQuote>& quotes)
{
    auto* storage = DataStorageService::instance();
    QVector<CachedQuoteData> cacheData;
    QDateTime now = QDateTime::currentDateTime();
    
    for (const auto& quote : quotes) {
        CachedQuoteData data;
        data.symbol = quote.symbol;
        data.name = quote.name;
        data.lastPrice = quote.lastPrice;
        data.changePercent = quote.changePercent;
        data.changeAmount = quote.changeAmount;
        data.volume = quote.volume;
        data.amount = quote.turnover;
        data.update_time = now;
        cacheData.append(data);
    }
    
    if (!cacheData.isEmpty()) {
        storage->saveQuoteCacheBatch(cacheData);
        storage->setLastUpdateTime("quotes", now);
        LOG_DEBUG(QString("Saved %1 quotes to cache").arg(cacheData.size()));
    }
}

/**
 * @brief 保存新闻到数据库
 */
void DashboardPage::saveNewsToDb(const QVector<NewsItem>& news)
{
    auto* storage = DataStorageService::instance();
    storage->saveNewsBatch(news);
    storage->setLastUpdateTime("news", QDateTime::currentDateTime());
    LOG_DEBUG(QString("Saved %1 news items").arg(news.size()));
}

// ============================================================================
// 数据加载流程：缓存 -> 数据库 -> 网络数据源
// ============================================================================

/**
 * @brief 数据加载主流程：缓存 -> 数据库 -> 网络数据源
 */
void DashboardPage::loadDataWithFallback()
{
    LOG_INFO("Starting data load with fallback strategy...");
    
    // 1. 尝试从缓存加载
    if (loadFromCache()) {
        LOG_INFO("Data loaded from cache successfully");
        // 缓存命中，后台更新数据
        QTimer::singleShot(100, this, [this]() {
            loadFromNetwork();
        });
        return;
    }
    
    // 2. 缓存未命中，尝试从数据库加载
    if (loadFromDatabase()) {
        LOG_INFO("Data loaded from database successfully");
        // 数据库命中，保存到缓存并后台更新
        saveToCache();
        QTimer::singleShot(100, this, [this]() {
            loadFromNetwork();
        });
        return;
    }
    
    // 3. 数据库也没有，从网络加载
    LOG_INFO("No local data, loading from network...");
    loadFromNetwork();
}

/**
 * @brief 从缓存加载数据
 * @return 是否成功加载
 */
bool DashboardPage::loadFromCache()
{
    auto* cache = CacheManager::instance();
    
    // 检查缓存是否有数据
    if (!cache->contains("dashboard_index_data") &&
        !cache->contains("dashboard_rank_data") &&
        !cache->contains("dashboard_watchlist_data")) {
        LOG_DEBUG("Cache miss for dashboard data");
        return false;
    }
    
    // 检查缓存是否过期（5分钟）
    QDateTime lastUpdate = cache->get("dashboard_last_update").toDateTime();
    if (lastUpdate.isValid()) {
        qint64 ageSecs = lastUpdate.secsTo(QDateTime::currentDateTime());
        if (ageSecs > 300) { // 5分钟过期
            LOG_DEBUG(QString("Cache expired, age: %1 seconds").arg(ageSecs));
            return false;
        }
    }
    
    bool loaded = false;
    
    // 加载指数数据
    if (cache->contains("dashboard_index_data")) {
        QVariant indexVariant = cache->get("dashboard_index_data");
        if (indexVariant.canConvert<QVector<IndexData>>()) {
            d->indexData = indexVariant.value<QVector<IndexData>>();
            updateIndexDisplay();
            loaded = true;
            LOG_DEBUG(QString("Loaded %1 index items from cache").arg(d->indexData.size()));
        }
    }
    
    // 加载排行榜数据
    if (cache->contains("dashboard_rank_data")) {
        QVariant rankVariant = cache->get("dashboard_rank_data");
        if (rankVariant.canConvert<QVector<StockRankData>>()) {
            QVector<StockRankData> rankData = rankVariant.value<QVector<StockRankData>>();
            if (d->shGainModel) {
                d->shGainModel->setData(rankData);
            }
            loaded = true;
            LOG_DEBUG(QString("Loaded %1 rank items from cache").arg(rankData.size()));
        }
    }
    
    // 加载自选股数据
    if (cache->contains("dashboard_watchlist_data")) {
        QVariant watchlistVariant = cache->get("dashboard_watchlist_data");
        if (watchlistVariant.canConvert<QVector<StockRankData>>()) {
            QVector<StockRankData> watchlistData = watchlistVariant.value<QVector<StockRankData>>();
            if (d->watchlistModel) {
                d->watchlistModel->setData(watchlistData);
            }
            loaded = true;
            LOG_DEBUG(QString("Loaded %1 watchlist items from cache").arg(watchlistData.size()));
        }
    }
    
    // 加载新闻数据
    if (cache->contains("dashboard_news_data")) {
        QVariant newsVariant = cache->get("dashboard_news_data");
        if (newsVariant.canConvert<QVector<NewsData>>()) {
            QVector<NewsData> newsData = newsVariant.value<QVector<NewsData>>();
            if (d->newsList) {
                d->newsList->clear();
                for (const auto& news : newsData) {
                    QListWidgetItem* item = new QListWidgetItem(
                        QString("[%1] %2").arg(news.category, news.title));
                    d->newsList->addItem(item);
                }
            }
            loaded = true;
            LOG_DEBUG(QString("Loaded %1 news items from cache").arg(newsData.size()));
        }
    }
    
    // 加载资金流向数据
    if (cache->contains("dashboard_moneyflow_data")) {
        QVariant mfVariant = cache->get("dashboard_moneyflow_data");
        if (mfVariant.canConvert<QVector<MoneyFlowData>>()) {
            QVector<MoneyFlowData> mfData = mfVariant.value<QVector<MoneyFlowData>>();
            if (d->moneyFlowModel) {
                d->moneyFlowModel->setData(mfData);
            }
            loaded = true;
            LOG_DEBUG(QString("Loaded %1 money flow items from cache").arg(mfData.size()));
        }
    }
    
    // 加载板块数据
    if (cache->contains("dashboard_sector_data")) {
        QVariant sectorVariant = cache->get("dashboard_sector_data");
        if (sectorVariant.canConvert<QVector<SectorData>>()) {
            QVector<SectorData> sectorData = sectorVariant.value<QVector<SectorData>>();
            if (d->sectorModel) {
                d->sectorModel->setData(sectorData);
            }
            loaded = true;
            LOG_DEBUG(QString("Loaded %1 sector items from cache").arg(sectorData.size()));
        }
    }
    
    if (loaded) {
        updateTimeDisplay();
        d->statusLabel->setText(QStringLiteral("已从缓存加载"));
    }
    
    return loaded;
}

/**
 * @brief 从数据库加载数据
 * @return 是否成功加载
 */
bool DashboardPage::loadFromDatabase()
{
    auto* storage = DataStorageService::instance();
    
    // 检查数据库是否有数据
    if (!storage->hasLocalData()) {
        LOG_DEBUG("No local data in database");
        return false;
    }
    
    bool loaded = false;
    
    // 加载指数数据
    QVector<IndexHistoryData> indexHistory = storage->getLatestIndexData();
    if (!indexHistory.isEmpty()) {
        d->indexData.clear();
        for (const auto& hist : indexHistory) {
            IndexData data;
            data.code = hist.code;
            data.name = hist.name;
            data.current = hist.closePrice;
            data.change = hist.closePrice * hist.changePercent / 100.0;
            data.changePercent = hist.changePercent;
            data.volume = hist.volume;
            data.amount = hist.amount;
            d->indexData.append(data);
        }
        updateIndexDisplay();
        loaded = true;
        LOG_DEBUG(QString("Loaded %1 index items from database").arg(indexHistory.size()));
    }
    
    // 加载行情缓存数据
    QVector<CachedQuoteData> quoteCache = storage->getAllQuoteCache();
    if (!quoteCache.isEmpty()) {
        // 分类处理：排行榜和自选股
        QVector<StockRankData> rankData;
        QVector<StockRankData> watchlistData;
        
        // 获取自选股列表
        QStringList watchlistSymbols = storage->getWatchlistSymbols();
        
        int rank = 1;
        for (const auto& quote : quoteCache) {
            StockRankData stock;
            stock.rank = rank++;
            stock.code = quote.symbol;
            stock.name = quote.name;
            stock.price = quote.lastPrice;
            stock.changePercent = quote.changePercent;
            stock.change = quote.changeAmount;
            stock.volume = quote.volume;
            stock.amount = quote.amount;
            
            // 判断是自选股还是排行榜
            if (watchlistSymbols.contains(quote.symbol)) {
                watchlistData.append(stock);
            } else {
                rankData.append(stock);
            }
        }
        
        // 更新模型
        if (d->shGainModel && !rankData.isEmpty()) {
            d->shGainModel->setData(rankData);
        }
        if (d->watchlistModel && !watchlistData.isEmpty()) {
            d->watchlistModel->setData(watchlistData);
        }
        loaded = true;
        LOG_DEBUG(QString("Loaded %1 quotes from database").arg(quoteCache.size()));
    }
    
    // 加载新闻数据
    QVector<NewsItem> newsItems = storage->getLatestNews(50);
    if (!newsItems.isEmpty() && d->newsList) {
        d->newsList->clear();
        for (const auto& news : newsItems) {
            QString timeStr = news.publishTime.toString("HH:mm");
            QString category = news.category.isEmpty() ? QStringLiteral("新闻") : news.category;
            QString displayText = QString("[%1] %2").arg(category, news.title);
            QListWidgetItem* item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, news.id);
            d->newsList->addItem(item);
        }
        loaded = true;
        LOG_DEBUG(QString("Loaded %1 news items from database").arg(newsItems.size()));
    }
    
    if (loaded) {
        updateTimeDisplay();
        QDateTime lastUpdate = storage->getLastUpdateTime("quotes");
        if (lastUpdate.isValid()) {
            d->statusLabel->setText(QString("已从数据库加载 (%1)")
                .arg(lastUpdate.toString("HH:mm:ss")));
        } else {
            d->statusLabel->setText(QStringLiteral("已从数据库加载"));
        }
    }
    
    return loaded;
}

/**
 * @brief 从网络加载数据
 */
void DashboardPage::loadFromNetwork()
{
    LOG_INFO("Loading data from network...");
    
    // 初始化数据源
    if (!d->indexDataSource) {
        d->indexDataSource = new StockDataSource(StockDataSource::Source::Sina, this);
        d->rankDataSource = new StockDataSource(StockDataSource::Source::Sina, this);
        d->watchlistDataSource = new StockDataSource(StockDataSource::Source::Sina, this);
        
        // 连接信号
        connect(d->indexDataSource, &StockDataSource::quotesReceived,
                this, [this](const QVector<StockQuote>& quotes) {
                    onIndexQuotesReceived(quotes);
                    // 保存到缓存和数据库
                    saveToCache();
                    saveToDatabase();
                });
        connect(d->rankDataSource, &StockDataSource::quotesReceived,
                this, [this](const QVector<StockQuote>& quotes) {
                    onRankQuotesReceived(quotes);
                    saveToCache();
                    saveToDatabase();
                });
        connect(d->watchlistDataSource, &StockDataSource::quotesReceived,
                this, [this](const QVector<StockQuote>& quotes) {
                    onWatchlistQuotesReceived(quotes);
                    saveToCache();
                    saveToDatabase();
                });
    }
    
    // 请求指数数据
    d->indexDataSource->requestQuotes(d->indexSymbols);
    
    // 请求排行榜数据
    d->rankDataSource->requestQuotes(d->hotStockSymbols);
    
    // 请求自选股数据
    d->watchlistDataSource->requestQuotes(d->watchlistSymbols);
    
    // 启动自动刷新（5秒）
    d->indexDataSource->startAutoRefresh(5000);
    d->rankDataSource->startAutoRefresh(5000);
    d->watchlistDataSource->startAutoRefresh(5000);
    
    // 加载其他数据（新闻、资金流向等暂时用模拟数据）
    loadNewsData();
    loadMoneyFlowData();
    loadSectorData();
    
    updateTimeDisplay();
    d->statusLabel->setText(QStringLiteral("正在从网络获取数据..."));
}

/**
 * @brief 保存数据到缓存
 */
void DashboardPage::saveToCache()
{
    auto* cache = CacheManager::instance();
    
    // 保存指数数据
    if (!d->indexData.isEmpty()) {
        QVariant indexVariant = QVariant::fromValue(d->indexData);
        cache->set("dashboard_index_data", indexVariant, 300); // 5分钟TTL
    }
    
    // 保存排行榜数据
    if (d->shGainModel) {
        QVector<StockRankData> rankData;
        // 从模型获取数据... 这里简化处理
        cache->set("dashboard_rank_data", QVariant::fromValue(rankData), 300);
    }
    
    // 保存自选股数据
    if (d->watchlistModel) {
        QVector<StockRankData> watchlistData;
        cache->set("dashboard_watchlist_data", QVariant::fromValue(watchlistData), 300);
    }
    
    // 保存更新时间
    cache->set("dashboard_last_update", QDateTime::currentDateTime(), 300);
    
    LOG_DEBUG("Dashboard data saved to cache");
}

/**
 * @brief 保存数据到数据库
 */
void DashboardPage::saveToDatabase()
{
    auto* storage = DataStorageService::instance();
    
    // 保存指数历史数据（收盘后）
    QTime now = QTime::currentTime();
    if (now >= QTime(15, 0)) {
        QVector<IndexHistoryData> indexHistory;
        QDate today = QDate::currentDate();
        
        for (const auto& idx : d->indexData) {
            IndexHistoryData data;
            data.code = idx.code;
            data.name = idx.name;
            data.closePrice = idx.current;
            data.changePercent = idx.changePercent;
            data.volume = idx.volume;
            data.amount = idx.amount;
            data.date = today;
            indexHistory.append(data);
        }
        
        if (!indexHistory.isEmpty()) {
            storage->saveIndexDataBatch(indexHistory);
            LOG_DEBUG(QString("Saved %1 index records to database").arg(indexHistory.size()));
        }
    }
    
    // 更新最后更新时间
    storage->setLastUpdateTime("quotes", QDateTime::currentDateTime());
    
    LOG_DEBUG("Dashboard data saved to database");
}

/**
 * @brief 新闻点击弹窗
 */
void DashboardPage::onNewsItemClicked(QListWidgetItem* item)
{
    if (!item) return;
    
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    // 获取新闻ID
    QString newsId = item->data(Qt::UserRole).toString();
    QString newsTitle = item->text();
    
    LOG_INFO(QString("News clicked: %1").arg(newsTitle));
    
    // 从数据库获取新闻详情
    auto* storage = DataStorageService::instance();
    
    // 创建弹窗
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("新闻详情"));
    dialog->setMinimumSize(500, 400);
    dialog->setStyleSheet(QString(R"(
        QDialog {
            background-color: %1;
        }
        QLabel {
            color: %2;
        }
        QPushButton {
            background-color: %3;
            color: %2;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: %4;
        }
        QTextEdit {
            background-color: %1;
            color: %2;
            border: 1px solid %5;
            border-radius: 4px;
        }
    )").arg(theme.bgPrimary, theme.textPrimary, theme.primary, theme.primaryHover, theme.border));
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);
    
    // 标题
    QLabel* titleLabel = new QLabel(newsTitle, dialog);
    titleLabel->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1;")
        .arg(theme.textPrimary));
    titleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);
    
    // 内容（模拟内容）
    QTextEdit* contentEdit = new QTextEdit(dialog);
    contentEdit->setReadOnly(true);
    contentEdit->setText(QStringLiteral(
        "这是新闻的详细内容。\n\n"
        "在实际应用中，这里会显示从数据库或网络获取的完整新闻内容。\n\n"
        "新闻ID: %1\n\n"
        "发布时间: %2"
    ).arg(newsId, QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));
    layout->addWidget(contentEdit);
    
    // 关闭按钮
    QPushButton* closeButton = new QPushButton(QStringLiteral("关闭"), dialog);
    closeButton->setFixedWidth(100);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    layout->addLayout(buttonLayout);
    
    dialog->exec();
    dialog->deleteLater();
}

/**
 * @brief 主题切换更新
 * @details 重新应用所有内联样式，响应主题切换
 */
void DashboardPage::updateTheme()
{
    ThemeColors theme = ThemeManager::instance()->currentTheme();
    
    // 更新页面背景色
    setStyleSheet(QString("background-color: %1;").arg(theme.bgPrimary));
    
    // 更新主分割器
    if (d->mainSplitter) {
        d->mainSplitter->setStyleSheet(
            QString("QSplitter::handle { background-color: %1; }").arg(theme.border));
    }
    
    // 更新头部工具栏
    if (d->searchEdit) {
        d->searchEdit->setStyleSheet(QString(R"(
            QLineEdit {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 4px;
                padding: 6px 12px;
                color: %3;
            }
            QLineEdit::placeholder { color: %4; }
        )").arg(theme.bgElevated, theme.border, theme.textPrimary, theme.textTertiary));
    }
    
    if (d->marketCombo) {
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
        )").arg(theme.bgElevated, theme.border, theme.textPrimary, theme.primary));
    }
    
    if (d->timeLabel) {
        d->timeLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
            .arg(theme.textSecondary));
    }
    
    if (d->statusLabel) {
        d->statusLabel->setStyleSheet(QString("color: %1; font-size: 13px;")
            .arg(theme.textTertiary));
    }
    
    // 更新指数面板
    if (d->indexPanel) {
        d->indexPanel->setStyleSheet(QString("background-color: %1;").arg(theme.bgElevated));
    }
    
    // 更新指数标签颜色
    for (int i = 0; i < d->indexNameLabels.size() && i < d->indexData.size(); ++i) {
        if (d->indexNameLabels[i]) {
            d->indexNameLabels[i]->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold;")
                .arg(theme.textPrimary));
        }
        if (d->indexPriceLabels[i]) {
            d->indexPriceLabels[i]->setStyleSheet(QString("color: %1; font-size: 15px; font-weight: bold;")
                .arg(theme.textPrimary));
        }
        if (d->indexChangeLabels[i]) {
            const IndexData& idx = d->indexData[i];
            QString color = idx.change >= 0 ? theme.danger : theme.success;
            d->indexChangeLabels[i]->setStyleSheet(QString("color: %1; font-size: 13px;").arg(color));
        }
    }
    
    // 更新六宫格排行榜面板
    if (d->rankGridPanel) {
        d->rankGridPanel->setStyleSheet(QString("background-color: %1;").arg(theme.bgPrimary));
    }
    
    // 更新表格样式
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
            font-size: 12px;
        }
        QHeaderView::section {
            background-color: %5;
            color: %6;
            padding: 4px 6px;
            border: none;
            border-bottom: 1px solid %3;
            font-size: 11px;
            font-weight: bold;
        }
    )").arg(theme.bgElevated, theme.border, theme.border, theme.primary, theme.bgSurface, theme.textSecondary);
    
    // 应用表格样式
    if (d->shGainTable) d->shGainTable->setStyleSheet(tableStyle);
    if (d->szGainTable) d->szGainTable->setStyleSheet(tableStyle);
    if (d->sh5MinTable) d->sh5MinTable->setStyleSheet(tableStyle);
    if (d->sz5MinTable) d->sz5MinTable->setStyleSheet(tableStyle);
    if (d->sectorTable) d->sectorTable->setStyleSheet(tableStyle);
    if (d->watchlistTable) d->watchlistTable->setStyleSheet(tableStyle);
    if (d->moneyFlowTable) d->moneyFlowTable->setStyleSheet(tableStyle);
    
    // 更新新闻列表样式
    if (d->newsList) {
        d->newsList->setStyleSheet(QString(R"(
            QListWidget {
                background-color: %1;
                border: 1px solid %2;
                border-radius: 6px;
                padding: 4px;
            }
            QListWidget::item {
                color: %3;
                font-size: 12px;
                padding: 6px 8px;
                border-bottom: 1px solid %4;
            }
            QListWidget::item:hover {
                background-color: %5;
            }
        )").arg(theme.bgElevated, theme.border, theme.textPrimary, theme.border, theme.bgHover));
    }
    
    // 更新板块Tab样式
    if (d->sectorTabs) {
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
        )").arg(theme.textSecondary, theme.primary));
    }
    
    // 刷新显示
    update();
    
    LOG_DEBUG("DashboardPage theme updated");
}
