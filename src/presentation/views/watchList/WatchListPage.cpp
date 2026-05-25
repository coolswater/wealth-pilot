/**
 * @file WatchListPage.cpp
 * @brief 自选股页面实现 - 使用 DataHub 数据中心
 *
 * @details 实现功能：
 * - 个人自选股管理
 * - 实时行情展示
 * - DataHub 数据订阅（自动生命周期管理）
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "WatchListPage.h"
#include "core/config/Tokens.h"
#include "core/datahub/DataHub.h"
#include "presentation/components/StyleHelper.h"
#include "presentation/styles/ButtonStyles.h"
#include "shared/utils/Logger.h"

using WealthPilot::WatchListModel;
using WealthPilot::WatchListPage;

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QInputDialog>

using namespace Tokens::Colors;

// ============================================================================
// 涨跌颜色委托
// ============================================================================

class WatchListDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        bool ok = false;
        double changePercent = index.data(Qt::UserRole).toDouble(&ok);
        
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        
        if (index.column() == WatchListModel::ColChange || 
            index.column() == WatchListModel::ColChangeAmount) {
            if (ok) {
                if (changePercent > 0) {
                    opt.palette.setColor(QPalette::Text, QColor(Danger));
                } else if (changePercent < 0) {
                    opt.palette.setColor(QPalette::Text, QColor(Success));
                } else {
                    opt.palette.setColor(QPalette::Text, QColor(TextSecondary));
                }
            }
        }
        
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

// ============================================================================
// WatchListModel 实现
// ============================================================================

WatchListModel::WatchListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int WatchListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_data.size();
}

int WatchListModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant WatchListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size())
        return QVariant();

    const StockQuote& quote = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColCode:
            return quote.symbol.mid(2);
        case ColName:
            return quote.name;
        case ColPrice:
            return QString::number(quote.lastPrice, 'f', 2);
        case ColChange: {
            QString sign = quote.changePercent >= 0 ? "+" : "";
            return QString("%1%2%").arg(sign).arg(quote.changePercent, 0, 'f', 2);
        }
        case ColChangeAmount:
            return QString::number(quote.changeAmount, 'f', 2);
        case ColVolume:
            return formatVolume(quote.volume);
        case ColAmount:
            return formatMoney(quote.turnover);
        case ColHigh:
            return QString::number(quote.highPrice, 'f', 2);
        case ColLow:
            return QString::number(quote.lowPrice, 'f', 2);
        }
    }
    
    if (role == Qt::UserRole && index.column() == ColChange) {
        return quote.changePercent;
    }
    
    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColCode || index.column() == ColName)
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }

    return QVariant();
}

QVariant WatchListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case ColCode: return QStringLiteral("代码");
    case ColName: return QStringLiteral("名称");
    case ColPrice: return QStringLiteral("最新价");
    case ColChange: return QStringLiteral("涨跌幅");
    case ColChangeAmount: return QStringLiteral("涨跌额");
    case ColVolume: return QStringLiteral("成交量");
    case ColAmount: return QStringLiteral("成交额");
    case ColHigh: return QStringLiteral("最高");
    case ColLow: return QStringLiteral("最低");
    }
    return QVariant();
}

void WatchListModel::setData(const QVector<StockQuote>& quotes)
{
    beginResetModel();
    m_data = quotes;
    m_symbolIndex.clear();
    for (int i = 0; i < quotes.size(); ++i) {
        m_symbolIndex[quotes[i].symbol] = i;
    }
    endResetModel();
}

void WatchListModel::updateQuote(const QString& symbol, const StockQuote& quote)
{
    int row = findRowBySymbol(symbol);
    if (row >= 0 && row < m_data.size()) {
        m_data[row] = quote;
        emit dataChanged(index(row, 0), index(row, ColCount - 1));
    }
}

int WatchListModel::findRowBySymbol(const QString& symbol) const
{
    auto it = m_symbolIndex.find(symbol);
    if (it != m_symbolIndex.end()) {
        return it.value();
    }
    return -1;
}

void WatchListModel::addSymbol(const QString& symbol)
{
    if (!m_symbolSet.contains(symbol)) {
        m_symbolSet.insert(symbol);
    }
}

void WatchListModel::removeSymbol(int row)
{
    if (row >= 0 && row < m_data.size()) {
        QString symbol = m_data[row].symbol;
        beginRemoveRows(QModelIndex(), row, row);
        m_data.removeAt(row);
        m_symbolSet.remove(symbol);
        endRemoveRows();
    }
}

void WatchListModel::clear()
{
    beginResetModel();
    m_data.clear();
    m_symbolSet.clear();
    endResetModel();
}

QStringList WatchListModel::symbols() const
{
    QStringList result;
    for (const auto& quote : m_data) {
        result.append(quote.symbol);
    }
    return result;
}

QString WatchListModel::formatVolume(qint64 volume)
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

QString WatchListModel::formatMoney(double value)
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

// ============================================================================
// WatchListPage::Impl
// ============================================================================

class WatchListPage::Impl {
public:
    // UI 组件
    QTableView* tableView = nullptr;
    WatchListModel* model = nullptr;
    QSortFilterProxyModel* proxyModel = nullptr;
    QLabel* statusLabel = nullptr;
    QLineEdit* searchInput = nullptr;
    QLineEdit* addInput = nullptr;
    QPushButton* addBtn = nullptr;
    QPushButton* removeBtn = nullptr;
    
    // 数据源
    StockDataSource* dataSource = nullptr;
    
    // 状态
    std::atomic<bool> isVisible{true};
    
    // 默认自选股
    QStringList defaultSymbols = {
        "sh600519",  // 贵州茅台
        "sh601318",  // 中国平安
        "sz000858",  // 五粮液
        "sz000001",  // 平安银行
        "sh600036",  // 招商银行
    };
};

// ============================================================================
// WatchListPage 实现
// ============================================================================

WatchListPage::WatchListPage(QWidget* parent)
    : DataHubPageBase(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
}

WatchListPage::~WatchListPage()
{
    saveWatchList();
    // DataHub 自动取消订阅，无需手动清理
}

QString WatchListPage::pageId() const
{
    return QStringLiteral("WatchListPage");
}

void WatchListPage::initializePage()
{
    if (isInitialized()) return;
    
    // 设置 DataHub 数据订阅
    setupDataHubSubscriptions();
    
    // 加载自选股列表
    loadWatchList();
    
    // 请求初始数据
    requestStockData();
    
    setInitialized(true);
    LOG_INFO("WatchListPage initialized with DataHub");
}

void WatchListPage::setupDataHubSubscriptions()
{
    // 订阅自选股行情数据
    for (const QString& symbol : d->defaultSymbols) {
        subscribeQuote(symbol, [this](const StockQuote& quote) {
            // 更新模型数据
            int row = d->model->findRowBySymbol(quote.symbol);
            if (row >= 0) {
                d->model->updateQuote(quote.symbol, quote);
            }
        });
        m_subscribedSymbols.append(symbol);
    }
    
    // 使用模式订阅监听所有行情更新
    dataHub().subscribePattern(this, "market:quote:*",
        [this](const QString& topic, const QVariant& value) {
            Q_UNUSED(topic)
            Q_UNUSED(value)
            // 可选：处理模式匹配的更新
        });
}

void WatchListPage::onPageActivated(const QVariantMap& params)
{
    Q_UNUSED(params);
    d->isVisible = true;
    // DataHub 自动管理订阅，无需手动启动
    // 刷新数据
    requestStockData();
}

void WatchListPage::onPageDeactivated()
{
    d->isVisible = false;
    // DataHub 自动管理订阅，无需手动停止
}

void WatchListPage::addStock(const QString& symbol, const QString& name)
{
    Q_UNUSED(name);
    QString fullSymbol = symbol;
    
    // 添加前缀
    if (!symbol.startsWith("sh") && !symbol.startsWith("sz")) {
        if (symbol.startsWith("6")) {
            fullSymbol = "sh" + symbol;
        } else {
            fullSymbol = "sz" + symbol;
        }
    }
    
    if (!d->defaultSymbols.contains(fullSymbol)) {
        d->defaultSymbols.append(fullSymbol);
        d->model->addSymbol(fullSymbol);
        saveWatchList();
        requestStockData();
    }
}

// ============================================================================
// UI 设置
// ============================================================================

void WatchListPage::setupUI()
{
    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
    }

    // 页面头部
    auto* header = StyleHelper::createPageHeader(this, QStringLiteral("自选股"));
    mainLayout->addWidget(header);

    // 内容区域
    auto* contentWidget = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(10, 10, 10, 10);
    contentLayout->setSpacing(0);

    // 工具栏
    auto* toolbarLayout = new QHBoxLayout;

    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    ButtonStyles::setRefresh(refreshBtn);

    d->addInput = new QLineEdit();
    d->addInput->setPlaceholderText(QStringLiteral("添加股票代码"));
    d->addInput->setMaximumWidth(150);

    d->addBtn = new QPushButton(QStringLiteral("添加"));
    d->addBtn->setObjectName("addBtn");
    ButtonStyles::setAdd(d->addBtn);

    d->removeBtn = new QPushButton(QStringLiteral("删除"));
    ButtonStyles::setDelete(d->removeBtn);

    auto* filterLabel = new QLabel(QStringLiteral("搜索:"));
    filterLabel->setProperty("secondary", true);
    d->searchInput = new QLineEdit();
    d->searchInput->setPlaceholderText(QStringLiteral("搜索..."));
    d->searchInput->setMaximumWidth(120);

    toolbarLayout->addWidget(refreshBtn);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(d->addInput);
    toolbarLayout->addWidget(d->addBtn);
    toolbarLayout->addWidget(d->removeBtn);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(filterLabel);
    toolbarLayout->addWidget(d->searchInput);

    contentLayout->addLayout(toolbarLayout);

    // 表格模型
    d->model = new WatchListModel(this);
    d->proxyModel = new QSortFilterProxyModel(this);
    d->proxyModel->setSourceModel(d->model);
    d->proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    d->proxyModel->setFilterKeyColumn(-1);

    // 表格视图
    d->tableView = new QTableView(this);
    d->tableView->setModel(d->proxyModel);

    d->tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    d->tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    d->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    d->tableView->setSortingEnabled(true);
    d->tableView->setAlternatingRowColors(true);
    d->tableView->setShowGrid(false);
    d->tableView->verticalHeader()->setVisible(false);
    d->tableView->horizontalHeader()->setStretchLastSection(true);
    d->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    d->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    d->tableView->setItemDelegate(new WatchListDelegate(this));

    // 列宽
    d->tableView->setColumnWidth(WatchListModel::ColCode, 80);
    d->tableView->setColumnWidth(WatchListModel::ColName, 100);
    d->tableView->setColumnWidth(WatchListModel::ColPrice, 80);
    d->tableView->setColumnWidth(WatchListModel::ColChange, 80);
    d->tableView->setColumnWidth(WatchListModel::ColChangeAmount, 80);
    d->tableView->setColumnWidth(WatchListModel::ColVolume, 90);
    d->tableView->setColumnWidth(WatchListModel::ColAmount, 90);
    d->tableView->setColumnWidth(WatchListModel::ColHigh, 80);
    d->tableView->setColumnWidth(WatchListModel::ColLow, 80);

    contentLayout->addWidget(d->tableView);

    // 状态栏
    d->statusLabel = new QLabel(QStringLiteral("正在加载自选股..."), this);
    d->statusLabel->setObjectName("statusLabel");
    contentLayout->addWidget(d->statusLabel);

    mainLayout->addWidget(contentWidget, 1);

    // 连接按钮
    connect(refreshBtn, &QPushButton::clicked, this, &WatchListPage::onRefreshData);
    connect(d->addBtn, &QPushButton::clicked, this, &WatchListPage::onAddStock);
    connect(d->removeBtn, &QPushButton::clicked, this, &WatchListPage::onRemoveStock);
}

void WatchListPage::setupConnections()
{
    connect(d->searchInput, &QLineEdit::textChanged, 
            this, &WatchListPage::onSearchChanged);
    connect(d->tableView, &QTableView::doubleClicked,
            this, &WatchListPage::onRowDoubleClicked);
}

// ============================================================================
// 数据处理
// ============================================================================

void WatchListPage::loadWatchList()
{
    QSettings settings("WealthPilot", "WatchList");
    QStringList symbols = settings.value("symbols").toStringList();
    
    if (symbols.isEmpty()) {
        symbols = d->defaultSymbols;
    }
    
    d->defaultSymbols = symbols;
    for (const QString& symbol : symbols) {
        d->model->addSymbol(symbol);
    }
    
    LOG_INFO(QString("Loaded %1 watchlist symbols").arg(symbols.size()));
}

void WatchListPage::saveWatchList()
{
    QSettings settings("WealthPilot", "WatchList");
    settings.setValue("symbols", d->model->symbols());
    settings.sync();
    LOG_INFO("WatchList saved");
}

void WatchListPage::requestStockData()
{
    // 通过 DataHub 请求数据
    QStringList topics;
    for (const QString& symbol : d->defaultSymbols) {
        topics.append(QString("market:quote:%1").arg(symbol));
    }
    
    if (!topics.isEmpty()) {
        requestData(topics, true);
        d->statusLabel->setText(QStringLiteral("正在请求行情数据..."));
    }
}

void WatchListPage::onQuotesReceived(const QVector<StockQuote>& quotes)
{
    d->model->setData(quotes);
    
    d->statusLabel->setText(QString(QStringLiteral("自选股 %1 只 · %2"))
        .arg(quotes.size())
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

void WatchListPage::onSearchChanged(const QString& text)
{
    d->proxyModel->setFilterFixedString(text);
}

void WatchListPage::onRefreshData()
{
    requestStockData();
}

void WatchListPage::onAddStock()
{
    QString symbol = d->addInput->text().trimmed();
    if (!symbol.isEmpty()) {
        addStock(symbol);
        d->addInput->clear();
    }
}

void WatchListPage::onRemoveStock()
{
    QModelIndexList selected = d->tableView->selectionModel()->selectedRows();
    if (!selected.isEmpty()) {
        QModelIndex sourceIndex = d->proxyModel->mapToSource(selected.first());
        d->model->removeSymbol(sourceIndex.row());
        
        // 同步 defaultSymbols
        d->defaultSymbols = d->model->symbols();
        saveWatchList();
        
        LOG_INFO("Stock removed from watchlist");
    }
}

void WatchListPage::onRowDoubleClicked(const QModelIndex& index)
{
    QModelIndex sourceIndex = d->proxyModel->mapToSource(index);
    QString code = d->model->data(d->model->index(sourceIndex.row(), WatchListModel::ColCode)).toString();
    QString name = d->model->data(d->model->index(sourceIndex.row(), WatchListModel::ColName)).toString();
    
    LOG_INFO(QString("Double clicked: %1 (%2)").arg(code, name));
    
    QVariantMap params;
    params["symbol"] = code;
    params["name"] = name;
    emit navigateToKLinePage(code, params);
}
