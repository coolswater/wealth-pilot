/**
 * @file StockQuotesPage.cpp
 * @brief 股票行情页面实现
 * @details 实现股票行情列表展示、搜索筛选、数据刷新等功能
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "StockQuotesPage.h"
#include "ui/components/StyleHelper.h"
#include "ui/delegates/ColorDelegates.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QComboBox>
#include <QStyledItemDelegate>
#include <QTimer>

using namespace Tokens;

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
    // 根节点才有子节点
    if (parent.isValid())
    {
        return 0;
    }
    return m_data.size();
}

int StockQuoteModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return ColCount;
}

QVariant StockQuoteModel::data(const QModelIndex& index, int role) const
{
    // 检查索引有效性
    if (!index.isValid() || index.row() >= m_data.size())
    {
        return QVariant();
    }

    const StockQuoteData& quote = m_data[index.row()];

    // 显示角色
    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case ColCode:
            // 跳过 sh/sz 前缀，只显示数字代码
            return quote.symbol.mid(2);
        case ColName:
            return quote.name;
        case ColPrice:
            return QString::number(quote.price, 'f', 2);
        case ColChange:
            return QString::number(quote.change, 'f', 2);
        case ColChangePercent:
            return QString::number(quote.changePercent, 'f', 2) + "%";
        case ColVolume:
            return formatVolume(quote.volume);
        case ColTurnover:
            return formatVolume(quote.turnover);
        case ColHigh:
            return QString::number(quote.high, 'f', 2);
        case ColLow:
            return QString::number(quote.low, 'f', 2);
        default:
            return QVariant();
        }
    }

    // 前景色角色 - 涨跌颜色
    if (role == Qt::ForegroundRole)
    {
        // 涨跌额和涨跌幅列显示红涨绿跌
        if (index.column() == ColChange || index.column() == ColChangePercent)
        {
            if (quote.change > 0)
            {
                return QColor(Colors::Danger); // 红涨
            }
            else if (quote.change < 0)
            {
                return QColor(Colors::Success); // 绿跌
            }
        }
        // 最新价列也显示涨跌颜色
        if (index.column() == ColPrice)
        {
            if (quote.change > 0)
            {
                return QColor(Colors::Danger);
            }
            else if (quote.change < 0)
            {
                return QColor(Colors::Success);
            }
        }
    }

    // 用户角色 - 用于排序
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
        default:
            return data(index, Qt::DisplayRole);
        }
    }

    return QVariant();
}

QVariant StockQuoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // 只处理水平表头
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    switch (section)
    {
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
    case ColTurnover:
        return QStringLiteral("成交额");
    case ColHigh:
        return QStringLiteral("最高价");
    case ColLow:
        return QStringLiteral("最低价");
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

StockQuoteData StockQuoteModel::getQuote(int row) const
{
    if (row >= 0 && row < m_data.size())
    {
        return m_data[row];
    }
    return StockQuoteData();
}

// ============================================================================
// StockQuotesPage 实现
// ============================================================================

StockQuotesPage::StockQuotesPage(QWidget* parent)
    : BasePage(parent)
      , m_searchEdit(new QLineEdit(this))
      , m_filterCombo(new QComboBox(this))
      , m_refreshBtn(new QPushButton(this))
      , m_tableView(new QTableView(this))
      , m_model(new StockQuoteModel(this))
      , m_proxyModel(new QSortFilterProxyModel(this))
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
    if (isInitialized()) {
        return;
    }

    loadDemoData();
    setInitialized(true);
    LOG_INFO("StockQuotesPage initialized");
}

void StockQuotesPage::setupUI()
{
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);

    // ========== 标题栏 ==========
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);
    
    QLabel* titleLabel = new QLabel(QStringLiteral("股票行情"), this);
    StyleHelper::setTitleLabel(titleLabel);
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();

    // 状态标签
    m_statusLabel->setText(QStringLiteral("未加载数据"));
    StyleHelper::setLabelText(m_statusLabel);
    headerLayout->addWidget(m_statusLabel);

    mainLayout->addLayout(headerLayout);

    // ========== 工具栏 ==========
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    // 搜索框
    m_searchEdit->setPlaceholderText(QStringLiteral("搜索股票代码或名称"));
    m_searchEdit->setObjectName(QStringLiteral("searchEdit"));
    m_searchEdit->setFixedWidth(220);
    toolbarLayout->addWidget(m_searchEdit);

    // 筛选下拉框
    m_filterCombo->addItem(QStringLiteral("全部"), QStringLiteral("all"));
    m_filterCombo->addItem(QStringLiteral("沪A"), QStringLiteral("sh"));
    m_filterCombo->addItem(QStringLiteral("深A"), QStringLiteral("sz"));
    m_filterCombo->addItem(QStringLiteral("创业板"), QStringLiteral("sz300"));
    m_filterCombo->addItem(QStringLiteral("科创板"), QStringLiteral("sh688"));
    m_filterCombo->setObjectName(QStringLiteral("filterCombo"));
    m_filterCombo->setFixedWidth(100);
    toolbarLayout->addWidget(m_filterCombo);

    toolbarLayout->addStretch();

    // 刷新按钮
    m_refreshBtn->setText(QStringLiteral("刷新"));
    StyleHelper::setPrimaryButton(m_refreshBtn);
    m_refreshBtn->setFixedWidth(80);
    toolbarLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(toolbarLayout);

    // ========== 表格视图 ==========
    // 设置代理模型
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(-1); // 搜索所有列
    m_proxyModel->setSortRole(Qt::UserRole);

    m_tableView->setModel(m_proxyModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setObjectName(QStringLiteral("stockTableView"));

    // 设置列宽
    m_tableView->setColumnWidth(StockQuoteModel::ColCode, 80);
    m_tableView->setColumnWidth(StockQuoteModel::ColName, 120);
    m_tableView->setColumnWidth(StockQuoteModel::ColPrice, 90);
    m_tableView->setColumnWidth(StockQuoteModel::ColChange, 90);
    m_tableView->setColumnWidth(StockQuoteModel::ColChangePercent, 90);
    m_tableView->setColumnWidth(StockQuoteModel::ColVolume, 100);
    m_tableView->setColumnWidth(StockQuoteModel::ColTurnover, 100);
    m_tableView->setColumnWidth(StockQuoteModel::ColHigh, 80);
    m_tableView->setColumnWidth(StockQuoteModel::ColLow, 80);

    // 设置颜色委托（红涨绿跌）
    auto* changeDelegate = new WealthPilot::ChangeColorDelegate(this);
    auto* priceDelegate = new WealthPilot::PriceColorDelegate(this);
    m_tableView->setItemDelegateForColumn(StockQuoteModel::ColPrice, priceDelegate);
    m_tableView->setItemDelegateForColumn(StockQuoteModel::ColChange, changeDelegate);
    m_tableView->setItemDelegateForColumn(StockQuoteModel::ColChangePercent, changeDelegate);

    mainLayout->addWidget(m_tableView);
}

void StockQuotesPage::setupConnections()
{
    // 搜索框文本改变
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &StockQuotesPage::onSearchChanged);
    
    // 筛选条件改变
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockQuotesPage::onFilterChanged);
    
    // 刷新按钮点击
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &StockQuotesPage::onRefreshData);

    // 表格行双击
    connect(m_tableView, &QTableView::doubleClicked,
            this, &StockQuotesPage::onRowDoubleClicked);
}

void StockQuotesPage::loadDemoData()
{
    // 演示数据
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
    m_statusLabel->setText(QStringLiteral("已加载 %1 只股票").arg(m_allData.size()));

    LOG_INFO(QString("Loaded %1 stock quotes").arg(m_allData.size()));
}

void StockQuotesPage::applyFilter()
{
    QString searchText = m_searchEdit->text().trimmed();
    QString filterType = m_filterCombo->currentData().toString();

    // 构建过滤正则表达式
    QString pattern;

    // 市场筛选
    if (filterType != QStringLiteral("all"))
    {
        if (filterType == QStringLiteral("sh"))
        {
            pattern = QStringLiteral("^6[0-9]{5}$");
        }
        else if (filterType == QStringLiteral("sz"))
        {
            pattern = QStringLiteral("^(000|002|300)[0-9]{3}$");
        }
        else if (filterType == QStringLiteral("sz300"))
        {
            pattern = QStringLiteral("^300[0-9]{3}$");
        }
        else if (filterType == QStringLiteral("sh688"))
        {
            pattern = QStringLiteral("^688[0-9]{3}$");
        }
    }

    // 搜索文本过滤
    if (!searchText.isEmpty())
    {
        m_proxyModel->setFilterFixedString(searchText);
        m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    }
    else
    {
        m_proxyModel->setFilterFixedString(QString());
    }

    // 更新状态
    int visibleCount = m_proxyModel->rowCount();
    m_statusLabel->setText(QStringLiteral("显示 %1 / %2 只股票")
                           .arg(visibleCount).arg(m_allData.size()));
}

void StockQuotesPage::onSearchChanged(const QString& text)
{
    Q_UNUSED(text);
    applyFilter();
    LOG_DEBUG(QString("Search changed: %1").arg(text));
}

void StockQuotesPage::onFilterChanged(int index)
{
    Q_UNUSED(index);
    applyFilter();
    LOG_DEBUG(QString("Filter changed: %1").arg(m_filterCombo->currentText()));
}

void StockQuotesPage::onRefreshData()
{
    // 模拟刷新数据
    m_statusLabel->setText(QStringLiteral("刷新中..."));

    // 使用定时器模拟网络延迟
    QTimer::singleShot(500, this, [this]()
    {
        loadDemoData();
        LOG_INFO("Stock quotes data refreshed");
    });
}

void StockQuotesPage::onRowDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }
    
    // 获取源模型索引
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    int row = sourceIndex.row();

    // 获取股票数据
    StockQuoteData quote = m_model->getQuote(row);

    if (!quote.symbol.isEmpty())
    {
        // 发送导航信号
        emit navigateToKLinePage(quote.symbol, quote.name);
        LOG_INFO(QString("Navigate to KLine: %1 (%2)").arg(quote.symbol, quote.name));
    }
}

} // namespace WealthPilot
