/**
 * @file StockQuotesPage.cpp
 * @brief 股票行情页面实现
 */

#include "StockQuotesPage.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"
#include "market/StockDataSource.h"

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

namespace WealthPilot {

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

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColCode:
            return quote.symbol.mid(2);  // 跳过 sh/sz 前缀
        case ColName:
            return quote.name;
        case ColPrice:
            return QString::number(quote.price, 'f', 2);
        case ColChange:
            return QString::number(quote.change, 'f', 2);
        case ColChangePercent:
            return QString::number(quote.changePercent, 'f', 2) + "%";
        case ColVolume:
            return Tokens::formatVolume(quote.volume);
        default:
            return QVariant();
        }
    }

    // 涨跌颜色
    if (role == Qt::ForegroundRole) {
        if (index.column() == ColChange || index.column() == ColChangePercent) {
            if (quote.change > 0) {
                return QColor(Tokens::Colors::Danger);  // 红涨
            } else if (quote.change < 0) {
                return QColor(Tokens::Colors::Success);  // 绿跌
            }
        }
    }

    // 用于排序的用户数据
    if (role == Qt::UserRole) {
        switch (index.column()) {
        case ColPrice:
            return quote.price;
        case ColChange:
            return quote.change;
        case ColChangePercent:
            return quote.changePercent;
        case ColVolume:
            return quote.volume;
        default:
            return data(index, Qt::DisplayRole);
        }
    }

    return QVariant();
}

QVariant StockQuoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case ColCode:
        return QStringLiteral("代码");
    case ColName:
        return QStringLiteral("名称");
    case ColPrice:
        return QStringLiteral("最新价");
    case ColChange:
        return QStringLiteral("涨跌额");
    case ColChangePercent:
        return QStringLiteral("涨跌幅");
    case ColVolume:
        return QStringLiteral("成交量");
    default:
        return QVariant();
    }
}

void StockQuoteModel::setData(const QVector<StockQuoteData>& quotes)
{
    beginResetModel();
    m_data = quotes;
    endResetModel();
}

void StockQuoteModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}

// ============================================================================
// StockQuotesPage 实现
// ============================================================================

StockQuotesPage::StockQuotesPage(QWidget* parent)
    : BasePage(parent)
    , m_searchEdit(new QLineEdit(this))
    , m_filterCombo(new QComboBox(this))
    , m_tableView(new QTableView(this))
    , m_model(new StockQuoteModel(this))
    , m_statusLabel(new QLabel(this))
{
    setupUI();
    setupConnections();
}

StockQuotesPage::~StockQuotesPage()
{
}

void StockQuotesPage::initializePage()
{
    if (isInitialized()) return;
    
    // 加载示例数据
    QVector<StockQuoteData> sampleData;
    sampleData.append({"sh600000", "浦发银行", 10.50, 0.15, 1.45, 125000000});
    sampleData.append({"sh600036", "招商银行", 35.80, 0.42, 1.18, 98000000});
    sampleData.append({"sz000001", "平安银行", 12.30, -0.08, -0.65, 87000000});
    sampleData.append({"sz000002", "万科A", 8.90, 0.05, 0.56, 156000000});
    sampleData.append({"sh601318", "中国平安", 45.60, 0.88, 1.97, 76000000});
    
    m_model->setData(sampleData);
    m_statusLabel->setText(QStringLiteral("已加载 %1 只股票").arg(sampleData.size()));
    
    setInitialized(true);
    LOG_DEBUG("StockQuotesPage initialized");
}

void StockQuotesPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 顶部工具栏
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    // 搜索框
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索股票代码或名称"));
    m_searchEdit->setFixedWidth(200);
    toolbarLayout->addWidget(m_searchEdit);

    // 筛选下拉框
    m_filterCombo->addItem(QStringLiteral("全部"), QStringLiteral("all"));
    m_filterCombo->addItem(QStringLiteral("沪A"), QStringLiteral("sh"));
    m_filterCombo->addItem(QStringLiteral("深A"), QStringLiteral("sz"));
    m_filterCombo->setFixedWidth(100);
    toolbarLayout->addWidget(m_filterCombo);

    // 刷新按钮
    auto* refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    refreshBtn->setFixedWidth(80);
    toolbarLayout->addWidget(refreshBtn);

    toolbarLayout->addStretch();

    // 状态标签
    m_statusLabel->setText(QStringLiteral("未加载数据"));
    toolbarLayout->addWidget(m_statusLabel);

    mainLayout->addLayout(toolbarLayout);

    // 表格视图
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    
    // 设置列宽
    m_tableView->setColumnWidth(StockQuoteModel::ColCode, 80);
    m_tableView->setColumnWidth(StockQuoteModel::ColName, 120);
    m_tableView->setColumnWidth(StockQuoteModel::ColPrice, 90);
    m_tableView->setColumnWidth(StockQuoteModel::ColChange, 90);
    m_tableView->setColumnWidth(StockQuoteModel::ColChangePercent, 90);
    m_tableView->setColumnWidth(StockQuoteModel::ColVolume, 100);

    mainLayout->addWidget(m_tableView);

    // 设置样式
    setStyleSheet(R"(
        QTableView {
            background-color: #0F1419;
            alternate-background-color: #1A1F2E;
            gridline-color: #2D3748;
            border: 1px solid #2D3748;
            border-radius: 4px;
        }
        QTableView::item {
            padding: 4px;
        }
        QTableView::item:selected {
            background-color: #3B82F6;
            color: white;
        }
        QHeaderView::section {
            background-color: #1A1F2E;
            color: #A0AEC0;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #2D3748;
            font-weight: bold;
        }
        QLineEdit, QComboBox {
            background-color: #1A1F2E;
            color: #E2E8F0;
            border: 1px solid #2D3748;
            border-radius: 4px;
            padding: 6px;
        }
        QPushButton {
            background-color: #3B82F6;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: #2563EB;
        }
        QLabel {
            color: #A0AEC0;
        }
    )");
}

void StockQuotesPage::setupConnections()
{
    // 搜索
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StockQuotesPage::onSearchChanged);
    
    // 刷新按钮
    auto* refreshBtn = findChild<QPushButton*>();
    if (refreshBtn) {
        connect(refreshBtn, &QPushButton::clicked, this, &StockQuotesPage::onRefreshData);
    }
    
    // 双击行
    connect(m_tableView, &QTableView::doubleClicked, this, &StockQuotesPage::onRowDoubleClicked);
}

void StockQuotesPage::onSearchChanged(const QString& text)
{
    Q_UNUSED(text);
    // TODO: 实现搜索过滤
    LOG_DEBUG(QString("Search: %1").arg(text));
}

void StockQuotesPage::onRefreshData()
{
    // 重新加载示例数据
    initializePage();
    LOG_DEBUG("StockQuotesPage data refreshed");
}

void StockQuotesPage::onRowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;
    
    int row = index.row();
    QString symbol = m_model->data(m_model->index(row, StockQuoteModel::ColCode), Qt::DisplayRole).toString();
    QString name = m_model->data(m_model->index(row, StockQuoteModel::ColName), Qt::DisplayRole).toString();
    
    // 补全代码前缀
    QString fullSymbol;
    if (symbol.startsWith("6")) {
        fullSymbol = "sh" + symbol;
    } else {
        fullSymbol = "sz" + symbol;
    }
    
    emit navigateToKLinePage(fullSymbol, name);
    LOG_DEBUG(QString("Navigate to KLine: %1 %2").arg(fullSymbol, name));
}

} // namespace WealthPilot
