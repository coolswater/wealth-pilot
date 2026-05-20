/**
 * @file StockQuotesPage.cpp
 * @brief 股票行情页面实现 - 使用 DataHub 数据中心
 *
 * @details 实现功能：
 * - 股票行情列表展示
 * - 搜索筛选排序
 * - DataHub 数据订阅（自动生命周期管理）
 * - 实时数据更新
 *
 * @details 优化集成：
 * - PageTemplate 页面模板系统
 * - PerformanceMonitor 性能监控
 * - ErrorHandler 统一错误处理
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "StockQuotesPage.h"
#include "core/config/Tokens.h"
#include "ui/styles/ButtonStyles.h"
#include "ui/components/PageTemplate.h"
#include "core/monitoring/PerformanceMonitor.h"
#include "core/base/ErrorHandler.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QPainter>

using namespace Tokens::Colors;

namespace WealthPilot {

// ============================================================================
// 涨跌颜色委托
// ============================================================================

/**
 * @brief 股票行情委托 - 实现涨跌颜色显示
 * 
 * @details 颜色规则：
 * - 涨：红色（Danger）
 * - 跌：绿色（Success）
 * - 平：灰色（TextSecondary）
 */
class StockQuoteDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        // 获取涨跌幅数据
        bool ok = false;
        double changePercent = index.data(Qt::UserRole).toDouble(&ok);

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        // 涨跌额、涨跌幅、价格列显示颜色
        if (index.column() == StockQuoteModel::ColChange ||
            index.column() == StockQuoteModel::ColChangePercent ||
            index.column() == StockQuoteModel::ColPrice)
        {
            if (ok)
            {
                if (changePercent > 0) {
                    opt.palette.setColor(QPalette::Text, QColor(Danger));  // 红涨
                } else if (changePercent < 0) {
                    opt.palette.setColor(QPalette::Text, QColor(Success));  // 绿跌
                } else {
                    opt.palette.setColor(QPalette::Text, QColor(TextSecondary));  // 平盘
                }
            }
        }

        QStyledItemDelegate::paint(painter, opt, index);
    }
};

// ============================================================================
// StockQuoteModel 实现
// ============================================================================

StockQuoteModel::StockQuoteModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int StockQuoteModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_data.size();
}

int StockQuoteModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant StockQuoteModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size())
        return QVariant();

    const StockQuoteData& quote = m_data[index.row()];

    // ========== 显示角色 ==========
    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case ColCode:
            return quote.symbol.mid(2);  // 去掉 sh/sz 前缀
        case ColName:
            return quote.name;
        case ColPrice:
            return QString::number(quote.price, 'f', 2);
        case ColChange:
            return QString::number(quote.change, 'f', 2);
        case ColChangePercent:
            {
                QString sign = quote.changePercent >= 0 ? "+" : "";
                return QString("%1%2%").arg(sign).arg(quote.changePercent, 0, 'f', 2);
            }
        case ColVolume:
            return formatVolume(quote.volume);
        case ColTurnover:
            return formatVolume(quote.turnover);
        case ColHigh:
            return QString::number(quote.high, 'f', 2);
        case ColLow:
            return QString::number(quote.low, 'f', 2);
        }
    }

    // ========== 用户角色（用于排序） ==========
    if (role == Qt::UserRole)
    {
        switch (index.column())
        {
        case ColPrice:
            return quote.price;
        case ColChange:
            return quote.change;
        case ColChangePercent:
            return quote.changePercent;
        case ColVolume:
            return quote.volume;
        case ColTurnover:
            return quote.turnover;
        case ColHigh:
            return quote.high;
        case ColLow:
            return quote.low;
        }
    }

    // ========== 对齐方式 ==========
    if (role == Qt::TextAlignmentRole)
    {
        if (index.column() == ColCode || index.column() == ColName)
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }

    return QVariant();
}

QVariant StockQuoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
    case ColCode: return QStringLiteral("代码");
    case ColName: return QStringLiteral("名称");
    case ColPrice: return QStringLiteral("最新价");
    case ColChange: return QStringLiteral("涨跌额");
    case ColChangePercent: return QStringLiteral("涨跌幅");
    case ColVolume: return QStringLiteral("成交量");
    case ColTurnover: return QStringLiteral("成交额");
    case ColHigh: return QStringLiteral("最高");
    case ColLow: return QStringLiteral("最低");
    }
    return QVariant();
}

void StockQuoteModel::setData(const QVector<StockQuoteData>& quotes)
{
    beginResetModel();
    m_data = quotes;
    
    // 重建索引
    m_symbolIndex.clear();
    for (int i = 0; i < m_data.size(); ++i) {
        m_symbolIndex[m_data[i].symbol] = i;
    }
    
    endResetModel();
}

void StockQuoteModel::updateQuote(const QString& symbol, const StockQuoteData& quote)
{
    int row = findRowBySymbol(symbol);
    if (row >= 0) {
        m_data[row] = quote;
        emit dataChanged(index(row, 0), index(row, ColCount - 1));
    }
}

void StockQuoteModel::clear()
{
    beginResetModel();
    m_data.clear();
    m_symbolIndex.clear();
    endResetModel();
}

StockQuoteData StockQuoteModel::getQuote(int row) const
{
    if (row >= 0 && row < m_data.size())
        return m_data[row];
    return StockQuoteData();
}

int StockQuoteModel::findRowBySymbol(const QString& symbol) const
{
    auto it = m_symbolIndex.find(symbol);
    if (it != m_symbolIndex.end()) {
        return it.value();
    }
    return -1;
}

QString StockQuoteModel::formatVolume(qint64 volume)
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

// ============================================================================
// StockQuotesPage 实现
// ============================================================================

StockQuotesPage::StockQuotesPage(QWidget* parent)
    : DataHubPageBase(parent)
    , m_searchEdit(new QLineEdit(this))
    , m_filterCombo(new QComboBox(this))
    , m_refreshBtn(new QPushButton(QStringLiteral("刷新"), this))
    , m_tableView(new QTableView(this))
    , m_model(new StockQuoteModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
    , m_statusLabel(new QLabel(this))
{
    setupUI();
    setupConnections();
}

StockQuotesPage::~StockQuotesPage() = default;

void StockQuotesPage::initializePage()
{
    if (isInitialized()) return;

    // ============================================================
    // 1. 设置 DataHub 订阅
    // ============================================================
    setupDataHubSubscriptions();

    // ============================================================
    // 2. 加载初始数据
    // ============================================================
    loadDemoData();

    setInitialized(true);
    LOG_INFO("[StockQuotesPage] Initialized with DataHub subscriptions");
}

void StockQuotesPage::setupUI()
{
    PERF_TIMER("StockQuotesPage::setupUI");
    
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ========== 工具栏 - 使用 PageTemplate 按钮样式 ==========
    auto* toolbarLayout = new QHBoxLayout;
    toolbarLayout->setSpacing(8);

    // 刷新按钮
    m_refreshBtn->setStyleSheet(PageTemplate::standardButtonStyleSheet());
    toolbarLayout->addWidget(m_refreshBtn);

    // 筛选下拉框
    m_filterCombo->addItem(QStringLiteral("全部"), QStringLiteral("all"));
    m_filterCombo->addItem(QStringLiteral("沪A"), QStringLiteral("sh"));
    m_filterCombo->addItem(QStringLiteral("深A"), QStringLiteral("sz"));
    m_filterCombo->addItem(QStringLiteral("创业板"), QStringLiteral("sz300"));
    m_filterCombo->addItem(QStringLiteral("科创板"), QStringLiteral("sh688"));
    toolbarLayout->addWidget(m_filterCombo);

    toolbarLayout->addStretch();

    // 搜索标签和框
    auto* searchLabel = new QLabel(QStringLiteral("搜索:"), this);
    searchLabel->setProperty("secondary", true);
    toolbarLayout->addWidget(searchLabel);

    m_searchEdit->setPlaceholderText(QStringLiteral("搜索..."));
    m_searchEdit->setMaximumWidth(120);
    toolbarLayout->addWidget(m_searchEdit);

    mainLayout->addLayout(toolbarLayout);

    // ========== 表格模型 ==========
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(-1);  // 搜索所有列
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortRole(Qt::UserRole);  // 使用数值排序

    // ========== 表格视图 - 使用 PageTemplate 样式 ==========
    m_tableView->setModel(m_proxyModel);
    m_tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSortingEnabled(true);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setShowGrid(false);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_tableView->setItemDelegate(new StockQuoteDelegate(this));

    // 应用 PageTemplate 表格样式
    PageTemplate::applyTableStyle(m_tableView);

    // 设置列宽
    m_tableView->setColumnWidth(StockQuoteModel::ColCode, 80);
    m_tableView->setColumnWidth(StockQuoteModel::ColName, 100);
    m_tableView->setColumnWidth(StockQuoteModel::ColPrice, 80);
    m_tableView->setColumnWidth(StockQuoteModel::ColChange, 80);
    m_tableView->setColumnWidth(StockQuoteModel::ColChangePercent, 80);
    m_tableView->setColumnWidth(StockQuoteModel::ColVolume, 90);
    m_tableView->setColumnWidth(StockQuoteModel::ColTurnover, 90);
    m_tableView->setColumnWidth(StockQuoteModel::ColHigh, 80);
    m_tableView->setColumnWidth(StockQuoteModel::ColLow, 80);

    mainLayout->addWidget(m_tableView);

    // ========== 状态栏 ==========
    m_statusLabel->setText(QStringLiteral("正在加载股票行情..."));
    mainLayout->addWidget(m_statusLabel);
}

void StockQuotesPage::setupConnections()
{
    // 搜索和筛选
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StockQuotesPage::onSearchChanged);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockQuotesPage::onFilterChanged);
    
    // 刷新按钮
    connect(m_refreshBtn, &QPushButton::clicked, this, &StockQuotesPage::onRefreshData);
    
    // 双击导航
    connect(m_tableView, &QTableView::doubleClicked, this, &StockQuotesPage::onRowDoubleClicked);
}

void StockQuotesPage::setupDataHubSubscriptions()
{
    // ============================================================
    // 订阅股票行情数据
    // 
    // 使用 DataHub 的优势：
    // 1. 自动生命周期管理：页面销毁时自动取消订阅
    // 2. 统一刷新策略：由 DataHub 调度，无需独立 QTimer
    // 3. 数据去重：同一股票只请求一次
    // ============================================================

    // 默认订阅的股票列表
    m_subscribedSymbols = {
        "sh600000", "sh600036", "sh600519",
        "sz000001", "sz000002", "sz000333",
        "sh601318", "sh601398", "sz000858",
        "sz300750", "sh688981", "sz300059"
    };

    // 方法1: 使用模式订阅（监听所有 market:quote:* 更新）
    // 适合需要处理多只股票的场景
    dataHub().subscribePattern(this, "market:quote:*",
        [this](const QString& topic, const QVariant& value) {
            // 解析 topic 获取股票代码
            auto parts = topic.split(':');
            if (parts.size() >= 3) {
                QString symbol = parts[2];
                
                // 更新模型数据
                if (value.canConvert<StockQuote>()) {
                    StockQuote quote = value.value<StockQuote>();
                    StockQuoteData data;
                    data.symbol = quote.symbol;
                    data.name = quote.name;
                    data.price = quote.lastPrice;
                    data.change = quote.changeAmount;
                    data.changePercent = quote.changePercent;
                    data.volume = quote.volume;
                    data.turnover = quote.turnover;
                    data.high = quote.highPrice;
                    data.low = quote.lowPrice;
                    data.open = quote.openPrice;
                    data.prevClose = quote.preClose;
                    
                    m_model->updateQuote(symbol, data);
                }
            }
        });

    // 方法2: 使用便捷方法订阅特定股票
    // 适合只需要关注特定股票的场景
    for (const auto& symbol : m_subscribedSymbols) {
        subscribeQuote(symbol, [this, symbol](const StockQuote& quote) {
            // 更新单只股票数据
            StockQuoteData data;
            data.symbol = quote.symbol;
            data.name = quote.name;
            data.price = quote.lastPrice;
            data.change = quote.changeAmount;
            data.changePercent = quote.changePercent;
            data.volume = quote.volume;
            data.turnover = quote.turnover;
            data.high = quote.highPrice;
            data.low = quote.lowPrice;
            data.open = quote.openPrice;
            data.prevClose = quote.preClose;
            
            m_model->updateQuote(symbol, data);
        });
    }

    // 请求初始数据
    QStringList topics;
    for (const auto& symbol : m_subscribedSymbols) {
        topics << QString("market:quote:%1").arg(symbol);
    }
    requestData(topics, true);

    LOG_INFO(QString("[StockQuotesPage] Subscribed to %1 stock symbols via DataHub")
             .arg(m_subscribedSymbols.size()));
}

void StockQuotesPage::loadDemoData()
{
    // 演示数据（实际项目中会从 DataHub 获取）
    m_allData = {
        {"sh600000", QStringLiteral("浦发银行"), 10.50, 0.15, 1.45, 125000000, 1300000000, 10.65, 10.35, 10.35, 10.35},
        {"sh600036", QStringLiteral("招商银行"), 35.80, 0.42, 1.18, 98000000, 3500000000, 36.20, 35.40, 35.38, 35.38},
        {"sh600519", QStringLiteral("贵州茅台"), 1850.00, 25.00, 1.37, 3500000, 6500000000, 1875.00, 1825.00, 1825.00, 1825.00},
        {"sz000001", QStringLiteral("平安银行"), 12.30, -0.08, -0.65, 87000000, 1070000000, 12.45, 12.20, 12.38, 12.38},
        {"sz000002", QStringLiteral("万科A"), 8.90, 0.05, 0.56, 156000000, 1380000000, 9.00, 8.85, 8.85, 8.85},
        {"sz000333", QStringLiteral("美的集团"), 58.60, 0.80, 1.38, 28000000, 1640000000, 59.50, 57.80, 57.80, 57.80},
        {"sh601318", QStringLiteral("中国平安"), 45.60, 0.88, 1.97, 76000000, 3470000000, 46.50, 44.80, 44.72, 44.72},
        {"sh601398", QStringLiteral("工商银行"), 5.15, 0.03, 0.59, 210000000, 1080000000, 5.20, 5.12, 5.12, 5.12},
        {"sz000858", QStringLiteral("五粮液"), 165.80, 2.50, 1.53, 12000000, 1990000000, 168.50, 163.30, 163.30, 163.30},
        {"sz300750", QStringLiteral("宁德时代"), 215.60, -3.20, -1.46, 8500000, 1830000000, 220.00, 214.00, 218.80, 218.80},
        {"sh688981", QStringLiteral("中芯国际"), 52.30, 0.80, 1.55, 15000000, 785000000, 53.50, 51.50, 51.50, 51.50},
        {"sz300059", QStringLiteral("东方财富"), 18.65, 0.35, 1.91, 95000000, 1770000000, 19.00, 18.30, 18.30, 18.30}
    };

    m_model->setData(m_allData);
    m_statusLabel->setText(QString(QStringLiteral("股票 %1 只 · %2"))
                           .arg(m_allData.size())
                           .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));

    LOG_INFO(QString("[StockQuotesPage] Loaded %1 demo quotes").arg(m_allData.size()));
}

void StockQuotesPage::applyFilter()
{
    QString searchText = m_searchEdit->text().trimmed();

    // 搜索文本过滤
    if (!searchText.isEmpty()) {
        m_proxyModel->setFilterFixedString(searchText);
    } else {
        m_proxyModel->setFilterFixedString(QString());
    }

    // 更新状态
    int visibleCount = m_proxyModel->rowCount();
    m_statusLabel->setText(QString(QStringLiteral("股票 %1/%2 只 · %3"))
                           .arg(visibleCount).arg(m_allData.size())
                           .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

void StockQuotesPage::onSearchChanged(const QString& text)
{
    Q_UNUSED(text);
    applyFilter();
}

void StockQuotesPage::onFilterChanged(int index)
{
    Q_UNUSED(index);
    applyFilter();
}

void StockQuotesPage::onRefreshData()
{
    m_statusLabel->setText(QStringLiteral("正在刷新..."));

    // 通过 DataHub 请求刷新数据
    QStringList topics;
    for (const auto& symbol : m_subscribedSymbols) {
        topics << QString("market:quote:%1").arg(symbol);
    }
    requestData(topics, true);  // force = true

    LOG_INFO("[StockQuotesPage] Refresh requested via DataHub");
}

void StockQuotesPage::onRowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;

    // 获取源模型索引
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    StockQuoteData quote = m_model->getQuote(sourceIndex.row());

    if (!quote.symbol.isEmpty()) {
        emit navigateToKLinePage(quote.symbol, quote.name);
        LOG_INFO(QString("[StockQuotesPage] Navigate to KLine: %1 (%2)")
                 .arg(quote.symbol, quote.name));
    }
}

} // namespace WealthPilot