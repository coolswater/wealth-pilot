/**
 * @file StockQuotesPage.cpp
 * @brief 股票行情页面实现
 * @details 实现股票行情列表展示、搜索筛选、数据刷新等功能
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "StockQuotesPage.h"
#include "core/config/Tokens.h"
#include "ui/styles/ButtonStyles.h"
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
#include <QTimer>

using namespace Tokens::Colors;

namespace WealthPilot {

// ============================================================================
// 涨跌颜色委托
// ============================================================================

class StockQuoteDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        bool ok = false;
        double changePercent = index.data(Qt::UserRole).toDouble(&ok);

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);

        // 涨跌额和涨跌幅列显示红涨绿跌
        if (index.column() == StockQuoteModel::ColChange ||
            index.column() == StockQuoteModel::ColChangePercent ||
            index.column() == StockQuoteModel::ColPrice)
        {
            if (ok)
            {
                if (changePercent > 0)
                {
                    opt.palette.setColor(QPalette::Text, QColor(Danger));
                }
                else if (changePercent < 0)
                {
                    opt.palette.setColor(QPalette::Text, QColor(Success));
                }
                else
                {
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

    const StockQuoteData& quote = m_data[index.row()];

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case ColCode:
            return quote.symbol.mid(2);
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
    endResetModel();
}

void StockQuoteModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}

StockQuoteData StockQuoteModel::getQuote(int row) const
{
    if (row >= 0 && row < m_data.size())
        return m_data[row];
    return StockQuoteData();
}

QString StockQuoteModel::formatVolume(qint64 volume)
{
    if (volume <= 0) return "--";
    if (volume >= 100000000)
    {
        return QString("%1亿").arg(volume / 100000000.0, 0, 'f', 2);
    }
    if (volume >= 10000)
    {
        return QString("%1万").arg(volume / 10000.0, 0, 'f', 2);
    }
    return QString::number(volume);
}

// ============================================================================
// StockQuotesPage 实现
// ============================================================================

StockQuotesPage::StockQuotesPage(QWidget* parent)
    : BasePage(parent)
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

    loadDemoData();
    setInitialized(true);
    LOG_INFO("StockQuotesPage initialized");
}

void StockQuotesPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // ========== 工具栏 ==========
    auto* toolbarLayout = new QHBoxLayout;
    toolbarLayout->setSpacing(8);

    // 刷新按钮
    ButtonStyles::setRefresh(m_refreshBtn);
    toolbarLayout->addWidget(m_refreshBtn);

    // 筛选下拉框
    m_filterCombo->addItem(QStringLiteral("全部"), QStringLiteral("all"));
    m_filterCombo->addItem(QStringLiteral("沪A"), QStringLiteral("sh"));
    m_filterCombo->addItem(QStringLiteral("深A"), QStringLiteral("sz"));
    m_filterCombo->addItem(QStringLiteral("创业板"), QStringLiteral("sz300"));
    m_filterCombo->addItem(QStringLiteral("科创板"), QStringLiteral("sh688"));
    toolbarLayout->addWidget(m_filterCombo);

    toolbarLayout->addStretch();

    // 搜索标签
    auto* searchLabel = new QLabel(QStringLiteral("搜索:"), this);
    searchLabel->setProperty("secondary", true);
    toolbarLayout->addWidget(searchLabel);

    // 搜索框
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索..."));
    m_searchEdit->setMaximumWidth(120);
    toolbarLayout->addWidget(m_searchEdit);

    mainLayout->addLayout(toolbarLayout);

    // ========== 表格模型 ==========
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(-1);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortRole(Qt::UserRole);

    // ========== 表格视图 ==========
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

    // 列宽
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
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StockQuotesPage::onSearchChanged);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockQuotesPage::onFilterChanged);
    connect(m_refreshBtn, &QPushButton::clicked, this, &StockQuotesPage::onRefreshData);
    connect(m_tableView, &QTableView::doubleClicked, this, &StockQuotesPage::onRowDoubleClicked);
}

void StockQuotesPage::loadDemoData()
{
    m_allData = {
        {"sh600000", QStringLiteral("浦发银行"), 10.50, 0.15, 1.45, 125000000, 1300000000, 10.65, 10.35, 10.35, 10.35},
        {"sh600036", QStringLiteral("招商银行"), 35.80, 0.42, 1.18, 98000000, 3500000000, 36.20, 35.40, 35.38, 35.38},
        {
            "sh600519", QStringLiteral("贵州茅台"), 1850.00, 25.00, 1.37, 3500000, 6500000000, 1875.00, 1825.00, 1825.00,
            1825.00
        },
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

    LOG_INFO(QString("Loaded %1 stock quotes").arg(m_allData.size()));
}

void StockQuotesPage::applyFilter()
{
    QString searchText = m_searchEdit->text().trimmed();
    QString filterType = m_filterCombo->currentData().toString();

    // 搜索文本过滤
    if (!searchText.isEmpty())
    {
        m_proxyModel->setFilterFixedString(searchText);
    }
    else
    {
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

    QTimer::singleShot(500, this, [this]()
    {
        loadDemoData();
        LOG_INFO("Stock quotes refreshed");
    });
}

void StockQuotesPage::onRowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid()) return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    StockQuoteData quote = m_model->getQuote(sourceIndex.row());

    if (!quote.symbol.isEmpty())
    {
        emit navigateToKLinePage(quote.symbol, quote.name);
        LOG_INFO(QString("Navigate to KLine: %1 (%2)").arg(quote.symbol, quote.name));
    }
}

} // namespace WealthPilot
