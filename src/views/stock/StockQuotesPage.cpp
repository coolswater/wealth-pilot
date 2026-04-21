/**
 * @file StockQuotesPage.cpp
 * @brief 股票行情页面实现 - 与期货页面样式统一
 */

#include "StockQuotesPage.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

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

using namespace Tokens::Colors;

// ============================================================================
// 涨跌颜色委托
// ============================================================================

class StockQuoteDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        // 获取涨跌幅（用于着色）
        bool ok = false;
        double changePercent = index.data(Qt::UserRole).toDouble(&ok);
        
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        
        // 根据涨跌设置颜色
        if (index.column() == StockQuoteModel::ColChange || 
            index.column() == StockQuoteModel::ColChangeAmount) {
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

    const StockQuote& quote = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColCode:
            return quote.symbol.mid(2);  // 移除 sh/sz 前缀
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
        }
    }
    
    // UserRole 用于涨跌颜色
    if (role == Qt::UserRole && index.column() == ColChange) {
        return quote.changePercent;
    }
    
    // TextAlignmentRole
    if (role == Qt::TextAlignmentRole) {
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

    switch (section) {
    case ColCode: return QStringLiteral("代码");
    case ColName: return QStringLiteral("名称");
    case ColPrice: return QStringLiteral("最新价");
    case ColChange: return QStringLiteral("涨跌幅");
    case ColChangeAmount: return QStringLiteral("涨跌额");
    case ColVolume: return QStringLiteral("成交量");
    case ColAmount: return QStringLiteral("成交额");
    }
    return QVariant();
}

void StockQuoteModel::setData(const QVector<StockQuote>& quotes)
{
    beginResetModel();
    m_data = quotes;
    m_symbolIndex.clear();
    for (int i = 0; i < m_data.size(); ++i) {
        m_symbolIndex[m_data[i].symbol] = i;
    }
    endResetModel();
}

void StockQuoteModel::updateQuote(const StockQuote& quote)
{
    auto it = m_symbolIndex.find(quote.symbol);
    if (it != m_symbolIndex.end()) {
        m_data[it.value()] = quote;
        QModelIndex idx = index(it.value(), 0);
        QModelIndex lastIdx = index(it.value(), ColCount - 1);
        emit dataChanged(idx, lastIdx);
    }
}

void StockQuoteModel::clear()
{
    beginResetModel();
    m_data.clear();
    m_symbolIndex.clear();
    endResetModel();
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

QString StockQuoteModel::formatMoney(double value)
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
// StockQuotesPage::Impl
// ============================================================================

class StockQuotesPage::Impl {
public:
    // UI 组件
    QTableView* tableView = nullptr;
    StockQuoteModel* model = nullptr;
    QSortFilterProxyModel* proxyModel = nullptr;
    QLabel* statusLabel = nullptr;
    QLineEdit* searchInput = nullptr;
    QLineEdit* contractInput = nullptr;
    QPushButton* subscribeBtn = nullptr;
    QComboBox* activityFilter = nullptr;
    
    // 数据源
    StockDataSource* dataSource = nullptr;
    
    // 状态
    std::atomic<bool> isVisible{true};
    
    // 默认股票列表
    QStringList defaultSymbols = {
        "sh600519",  // 贵州茅台
        "sh601318",  // 中国平安
        "sz000858",  // 五粮液
        "sz000001",  // 平安银行
        "sh600036",  // 招商银行
        "sz002594",  // 比亚迪
        "sz300750",  // 宁德时代
        "sh601012",  // 隆基绿能
        "sz000333",  // 美的集团
        "sh600900",  // 长江电力
        "sz002415",  // 海康威视
        "sh601888",  // 中国中免
        "sz000002",  // 万科A
        "sh600276",  // 恒瑞医药
        "sz002304",  // 洋河股份
        "sh601166",  // 兴业银行
        "sz000651",  // 格力电器
        "sh601398",  // 工商银行
        "sz002352",  // 顺丰控股
        "sh600030",  // 中信证券
    };
};

// ============================================================================
// StockQuotesPage 实现
// ============================================================================

StockQuotesPage::StockQuotesPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
}

StockQuotesPage::~StockQuotesPage()
{
    if (d->dataSource) {
        d->dataSource->stopAutoRefresh();
    }
}

QString StockQuotesPage::pageId() const
{
    return QStringLiteral("StockQuotesPage");
}

void StockQuotesPage::initializePage()
{
    if (isInitialized()) return;
    
    // 初始化数据源
    d->dataSource = new StockDataSource(StockDataSource::Source::Sina, this);
    connect(d->dataSource, &StockDataSource::quotesReceived,
            this, &StockQuotesPage::onQuotesReceived);
    
    // 请求初始数据
    requestStockData();
    
    // 启动自动刷新（5秒）
    d->dataSource->startAutoRefresh(5000);
    
    setInitialized(true);
    LOG_INFO("StockQuotesPage initialized");
}

void StockQuotesPage::onPageActivated(const QVariantMap& params)
{
    Q_UNUSED(params);
    d->isVisible = true;
    if (d->dataSource) {
        d->dataSource->startAutoRefresh(5000);
    }
}

void StockQuotesPage::onPageDeactivated()
{
    d->isVisible = false;
    if (d->dataSource) {
        d->dataSource->stopAutoRefresh();
    }
}

// ============================================================================
// UI 设置
// ============================================================================

void StockQuotesPage::setupUI()
{
    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(10, 10, 10, 10);
    }

    // 工具栏
    auto* toolbarLayout = new QHBoxLayout;

    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"));
    refreshBtn->setProperty("ghost", true);

    d->contractInput = new QLineEdit();
    d->contractInput->setPlaceholderText(QStringLiteral("输入股票代码"));
    d->contractInput->setMaximumWidth(150);

    d->subscribeBtn = new QPushButton(QStringLiteral("订阅"));
    d->subscribeBtn->setObjectName("subscribeBtn");
    d->subscribeBtn->setProperty("primary", true);

    d->activityFilter = new QComboBox(this);
    d->activityFilter->addItem(QStringLiteral("全部"), 0);
    d->activityFilter->addItem(QStringLiteral("涨幅榜"), 1);
    d->activityFilter->addItem(QStringLiteral("跌幅榜"), 2);
    d->activityFilter->addItem(QStringLiteral("成交额"), 3);
    d->activityFilter->setCurrentIndex(0);
    d->activityFilter->setMaximumWidth(120);

    auto* filterLabel = new QLabel(QStringLiteral("筛选:"));
    filterLabel->setProperty("secondary", true);
    d->searchInput = new QLineEdit();
    d->searchInput->setPlaceholderText(QStringLiteral("搜索..."));
    d->searchInput->setMaximumWidth(120);

    toolbarLayout->addWidget(refreshBtn);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(d->contractInput);
    toolbarLayout->addWidget(d->subscribeBtn);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(new QLabel(QStringLiteral("显示:")));
    toolbarLayout->addWidget(d->activityFilter);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(filterLabel);
    toolbarLayout->addWidget(d->searchInput);

    mainLayout->addLayout(toolbarLayout);

    // 表格模型
    d->model = new StockQuoteModel(this);
    d->proxyModel = new QSortFilterProxyModel(this);
    d->proxyModel->setSourceModel(d->model);
    d->proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    d->proxyModel->setFilterKeyColumn(-1);  // 搜索所有列

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
    d->tableView->setItemDelegate(new StockQuoteDelegate(this));

    // 列宽
    d->tableView->setColumnWidth(StockQuoteModel::ColCode, 80);
    d->tableView->setColumnWidth(StockQuoteModel::ColName, 100);
    d->tableView->setColumnWidth(StockQuoteModel::ColPrice, 80);
    d->tableView->setColumnWidth(StockQuoteModel::ColChange, 80);
    d->tableView->setColumnWidth(StockQuoteModel::ColChangeAmount, 80);
    d->tableView->setColumnWidth(StockQuoteModel::ColVolume, 90);
    d->tableView->setColumnWidth(StockQuoteModel::ColAmount, 90);

    mainLayout->addWidget(d->tableView);

    // 状态栏
    d->statusLabel = new QLabel(QStringLiteral("正在加载股票数据..."), this);
    d->statusLabel->setObjectName("statusLabel");
    mainLayout->addWidget(d->statusLabel);

    // 连接刷新按钮
    connect(refreshBtn, &QPushButton::clicked, this, [this]() {
        requestStockData();
        LOG_INFO("Stock quotes refresh requested");
    });

    // 连接订阅按钮
    connect(d->subscribeBtn, &QPushButton::clicked, this, [this]() {
        QString symbol = d->contractInput->text().trimmed();
        if (!symbol.isEmpty()) {
            // 添加前缀
            if (!symbol.startsWith("sh") && !symbol.startsWith("sz")) {
                if (symbol.startsWith("6")) {
                    symbol = "sh" + symbol;
                } else {
                    symbol = "sz" + symbol;
                }
            }
            d->defaultSymbols.append(symbol);
            requestStockData();
            LOG_INFO(QString("Subscribed: %1").arg(symbol));
        }
    });
}

void StockQuotesPage::setupConnections()
{
    connect(d->searchInput, &QLineEdit::textChanged, 
            this, &StockQuotesPage::onSearchChanged);
    connect(d->activityFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockQuotesPage::onFilterChanged);
    connect(d->tableView, &QTableView::doubleClicked,
            this, &StockQuotesPage::onRowDoubleClicked);
}

// ============================================================================
// 数据处理
// ============================================================================

void StockQuotesPage::requestStockData()
{
    if (d->dataSource && !d->defaultSymbols.isEmpty()) {
        d->dataSource->requestQuotes(d->defaultSymbols);
        updateStatus(QStringLiteral("正在请求行情数据..."));
    }
}

void StockQuotesPage::onQuotesReceived(const QVector<StockQuote>& quotes)
{
    // 根据筛选排序
    QVector<StockQuote> sortedQuotes = quotes;
    int filterMode = d->activityFilter->currentData().toInt();
    
    switch (filterMode) {
    case 1: // 涨幅榜
        std::sort(sortedQuotes.begin(), sortedQuotes.end(),
            [](const StockQuote& a, const StockQuote& b) {
                return a.changePercent > b.changePercent;
            });
        break;
    case 2: // 跌幅榜
        std::sort(sortedQuotes.begin(), sortedQuotes.end(),
            [](const StockQuote& a, const StockQuote& b) {
                return a.changePercent < b.changePercent;
            });
        break;
    case 3: // 成交额
        std::sort(sortedQuotes.begin(), sortedQuotes.end(),
            [](const StockQuote& a, const StockQuote& b) {
                return a.turnover > b.turnover;
            });
        break;
    }
    
    d->model->setData(sortedQuotes);
    
    updateStatus(QString(QStringLiteral("已更新 %1 只股票 · %2"))
        .arg(quotes.size())
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

void StockQuotesPage::onSearchChanged(const QString& text)
{
    d->proxyModel->setFilterFixedString(text);
}

void StockQuotesPage::onFilterChanged(int index)
{
    Q_UNUSED(index);
    // 重新请求数据以应用排序
    if (d->dataSource) {
        d->dataSource->requestQuotes(d->defaultSymbols);
    }
}

void StockQuotesPage::onRefreshData()
{
    requestStockData();
}

void StockQuotesPage::onRowDoubleClicked(const QModelIndex& index)
{
    QModelIndex sourceIndex = d->proxyModel->mapToSource(index);
    QString code = d->model->data(d->model->index(sourceIndex.row(), StockQuoteModel::ColCode)).toString();
    QString name = d->model->data(d->model->index(sourceIndex.row(), StockQuoteModel::ColName)).toString();
    
    LOG_INFO(QString("Double clicked: %1 (%2)").arg(code, name));
    
    // 发送导航信号
    QVariantMap params;
    params["symbol"] = code;
    params["name"] = name;
    emit navigateToKLinePage(code, params);
}

void StockQuotesPage::updateStatus(const QString& text)
{
    if (d->statusLabel) {
        d->statusLabel->setText(text);
    }
}
