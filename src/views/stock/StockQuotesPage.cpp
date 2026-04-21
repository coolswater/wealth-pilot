/**
 * @file StockQuotesPage.cpp
 * @brief 股票行情页面实现 - 对接真实行情数据
 */

#include "StockQuotesPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QLineEdit>
#include <QTimer>
#include <QRandomGenerator>
#include <QMessageBox>

#include "ui/components/CardWidget.h"
#include "core/config/Tokens.h"
#include "market/StockDataSource.h"
#include "utils/Logger.h"

using namespace Tokens::Colors;

// ============================================================================
// Impl 结构体
// ============================================================================

struct StockQuotesPage::Impl {
    // UI 组件
    QTableWidget* table = nullptr;
    QComboBox* filterCombo = nullptr;
    QLineEdit* searchEdit = nullptr;
    QLabel* statusLabel = nullptr;
    
    // 数据源
    StockDataSource* dataSource = nullptr;
    QTimer* refreshTimer = nullptr;
    
    // 数据缓存
    QVector<StockQuote> allQuotes;
    QVector<StockQuote> filteredQuotes;
    QString currentFilter;
    QString searchText;
    
    // 默认股票列表（热门A股）
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
    };
};

// ============================================================================
// 构造/析构
// ============================================================================

StockQuotesPage::StockQuotesPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    setupConnections();
}

StockQuotesPage::~StockQuotesPage()
{
    if (d->refreshTimer) {
        d->refreshTimer->stop();
    }
    if (d->dataSource) {
        d->dataSource->stopAutoRefresh();
    }
}

// ============================================================================
// 页面接口
// ============================================================================

QString StockQuotesPage::pageId() const
{
    return QStringLiteral("StockQuotesPage");
}

void StockQuotesPage::initializePage()
{
    // 初始化数据源
    d->dataSource = new StockDataSource(StockDataSource::Source::Sina, this);
    connect(d->dataSource, &StockDataSource::quotesReceived,
            this, &StockQuotesPage::onQuotesReceived);
    
    // 请求初始数据
    d->dataSource->requestQuotes(d->defaultSymbols);
    
    // 启动自动刷新（5秒间隔）
    d->dataSource->startAutoRefresh(5000);
    
    LOG_INFO("StockQuotesPage initialized with " + QString::number(d->defaultSymbols.size()) + " symbols");
}

// ============================================================================
// UI 设置
// ============================================================================

void StockQuotesPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // ========== 头部 ==========
    QHBoxLayout* headerLayout = new QHBoxLayout();

    QLabel* titleLabel = new QLabel("股票行情", this);
    titleLabel->setStyleSheet(QString(R"(
        font-size: 24px;
        font-weight: 700;
        color: %1;
    )").arg(TextPrimary));
    headerLayout->addWidget(titleLabel);

    headerLayout->addStretch();

    // 状态标签
    d->statusLabel = new QLabel("正在加载...", this);
    d->statusLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TextSecondary));
    headerLayout->addWidget(d->statusLabel);

    mainLayout->addLayout(headerLayout);

    // ========== 工具栏 ==========
    QHBoxLayout* toolbarLayout = new QHBoxLayout();

    // 搜索框
    d->searchEdit = new QLineEdit(this);
    d->searchEdit->setPlaceholderText("搜索股票代码或名称...");
    d->searchEdit->setFixedWidth(280);
    d->searchEdit->setFixedHeight(36);
    d->searchEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 18px;
            padding: 0 16px;
            color: %3;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: %4;
        }
    )").arg(BgElevated, Border, TextPrimary, Primary));
    toolbarLayout->addWidget(d->searchEdit);

    toolbarLayout->addStretch();

    // 刷新按钮
    QPushButton* refreshBtn = new QPushButton("刷新", this);
    refreshBtn->setFixedSize(80, 36);
    refreshBtn->setCursor(Qt::PointingHandCursor);
    refreshBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 6px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: %3;
        }
    )").arg(Primary, QString("#FFFFFF"), QString("#2563EB")));
    connect(refreshBtn, &QPushButton::clicked, this, &StockQuotesPage::onRefreshData);
    toolbarLayout->addWidget(refreshBtn);

    // 筛选下拉框
    d->filterCombo = new QComboBox(this);
    d->filterCombo->addItems({"全部", "涨幅榜", "跌幅榜", "成交额", "自选股"});
    d->filterCombo->setFixedWidth(120);
    d->filterCombo->setFixedHeight(36);
    d->filterCombo->setStyleSheet(QString(R"(
        QComboBox {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 0 12px;
            color: %3;
            font-size: 14px;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox QAbstractItemView {
            background-color: %1;
            border: 1px solid %2;
            selection-background-color: %4;
        }
    )").arg(BgElevated, Border, TextPrimary, Primary));
    toolbarLayout->addWidget(d->filterCombo);

    mainLayout->addLayout(toolbarLayout);

    // ========== 数据表格 ==========
    CardWidget* tableCard = new CardWidget("", this);

    d->table = new QTableWidget(this);
    d->table->setColumnCount(8);
    d->table->setHorizontalHeaderLabels({
        "股票名称", "股票代码", "最新价", "涨跌幅", "涨跌额", 
        "成交量", "成交额", "操作"
    });
    
    // 设置列宽
    QHeaderView* header = d->table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Fixed);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->setSectionResizeMode(7, QHeaderView::Fixed);
    d->table->setColumnWidth(0, 120);
    d->table->setColumnWidth(1, 100);
    d->table->setColumnWidth(2, 100);
    d->table->setColumnWidth(7, 120);
    
    // 表格样式
    d->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->table->setAlternatingRowColors(true);
    d->table->verticalHeader()->setVisible(false);
    d->table->setShowGrid(false);
    d->table->setStyleSheet(QString(R"(
        QTableWidget {
            background-color: transparent;
            border: none;
            gridline-color: transparent;
        }
        QTableWidget::item {
            padding: 12px 8px;
            border-bottom: 1px solid %1;
            color: %2;
            font-size: 14px;
        }
        QTableWidget::item:selected {
            background-color: %3;
        }
        QHeaderView::section {
            background-color: %4;
            color: %5;
            padding: 12px 8px;
            border: none;
            border-bottom: 1px solid %1;
            font-size: 13px;
            font-weight: 600;
        }
    )").arg(Border, TextPrimary, 
            QString("rgba(59, 130, 246, 0.2)"),
            BgElevated, TextSecondary));

    tableCard->setContent(d->table);
    mainLayout->addWidget(tableCard);
}

void StockQuotesPage::setupConnections()
{
    // 搜索
    connect(d->searchEdit, &QLineEdit::textChanged,
            this, &StockQuotesPage::onSearchTextChanged);
    
    // 筛选
    connect(d->filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StockQuotesPage::onFilterChanged);
}

// ============================================================================
// 数据处理
// ============================================================================

void StockQuotesPage::onQuotesReceived(const QVector<StockQuote>& quotes)
{
    d->allQuotes = quotes;
    applyFilter();
    
    // 更新状态
    d->statusLabel->setText(QString("已更新 %1 只股票 · %2")
        .arg(quotes.size())
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
    
    LOG_DEBUG(QString("Received %1 stock quotes").arg(quotes.size()));
}

void StockQuotesPage::updateTable(const QVector<StockQuote>& quotes)
{
    d->table->setRowCount(quotes.size());
    
    for (int i = 0; i < quotes.size(); ++i) {
        updateTableRow(i, quotes[i]);
    }
}

void StockQuotesPage::updateTableRow(int row, const StockQuote& quote)
{
    // 股票名称
    QTableWidgetItem* nameItem = new QTableWidgetItem(quote.name);
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
    d->table->setItem(row, 0, nameItem);
    
    // 股票代码（简化显示）
    QString displayCode = quote.symbol;
    displayCode.remove("sh").remove("sz");
    QTableWidgetItem* codeItem = new QTableWidgetItem(displayCode);
    codeItem->setFlags(codeItem->flags() & ~Qt::ItemIsEditable);
    codeItem->setForeground(QColor(TextSecondary));
    d->table->setItem(row, 1, codeItem);
    
    // 最新价
    QTableWidgetItem* priceItem = new QTableWidgetItem(
        QString::number(quote.lastPrice, 'f', 2));
    priceItem->setFlags(priceItem->flags() & ~Qt::ItemIsEditable);
    priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    d->table->setItem(row, 2, priceItem);
    
    // 涨跌幅
    QString changeText = QString("%1%")
        .arg(quote.changePercent >= 0 ? "+" : "")
        .arg(quote.changePercent, 0, 'f', 2);
    QTableWidgetItem* changeItem = new QTableWidgetItem(changeText);
    changeItem->setFlags(changeItem->flags() & ~Qt::ItemIsEditable);
    changeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QColor changeColor = quote.changePercent >= 0 ? QColor(Danger) : QColor(Success);
    changeItem->setForeground(changeColor);
    d->table->setItem(row, 3, changeItem);
    
    // 涨跌额
    QTableWidgetItem* amountItem = new QTableWidgetItem(
        QString::number(quote.changeAmount, 'f', 2));
    amountItem->setFlags(amountItem->flags() & ~Qt::ItemIsEditable);
    amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    amountItem->setForeground(changeColor);
    d->table->setItem(row, 4, amountItem);
    
    // 成交量
    QTableWidgetItem* volumeItem = new QTableWidgetItem(formatVolume(quote.volume));
    volumeItem->setFlags(volumeItem->flags() & ~Qt::ItemIsEditable);
    volumeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    d->table->setItem(row, 5, volumeItem);
    
    // 成交额
    QTableWidgetItem* turnoverItem = new QTableWidgetItem(formatMoney(quote.turnover));
    turnoverItem->setFlags(turnoverItem->flags() & ~Qt::ItemIsEditable);
    turnoverItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    d->table->setItem(row, 6, turnoverItem);
    
    // 操作按钮
    QWidget* btnWidget = new QWidget();
    QHBoxLayout* btnLayout = new QHBoxLayout(btnWidget);
    btnLayout->setContentsMargins(4, 4, 4, 4);
    btnLayout->setSpacing(4);
    
    QPushButton* viewBtn = new QPushButton("查看");
    viewBtn->setFixedSize(50, 26);
    viewBtn->setCursor(Qt::PointingHandCursor);
    viewBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 4px;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: %3;
        }
    )").arg(QString("rgba(59, 130, 246, 0.2)"), Primary, QString("rgba(59, 130, 246, 0.3)")));
    viewBtn->setProperty("symbol", quote.symbol);
    connect(viewBtn, &QPushButton::clicked, this, &StockQuotesPage::onViewButtonClicked);
    btnLayout->addWidget(viewBtn);
    
    QPushButton* addBtn = new QPushButton("+");
    addBtn->setFixedSize(26, 26);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: none;
            border-radius: 4px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: %3;
        }
    )").arg(QString("rgba(16, 185, 129, 0.2)"), Success, QString("rgba(16, 185, 129, 0.3)")));
    addBtn->setProperty("symbol", quote.symbol);
    addBtn->setProperty("name", quote.name);
    connect(addBtn, &QPushButton::clicked, this, &StockQuotesPage::onAddToFavoriteClicked);
    btnLayout->addWidget(addBtn);
    
    d->table->setCellWidget(row, 7, btnWidget);
}

void StockQuotesPage::applyFilter()
{
    d->filteredQuotes.clear();
    
    int filterIndex = d->filterCombo->currentIndex();
    
    for (const auto& quote : d->allQuotes) {
        // 搜索过滤
        if (!d->searchText.isEmpty()) {
            if (!quote.name.contains(d->searchText, Qt::CaseInsensitive) &&
                !quote.symbol.contains(d->searchText, Qt::CaseInsensitive)) {
                continue;
            }
        }
        d->filteredQuotes.append(quote);
    }
    
    // 排序
    switch (filterIndex) {
        case 1: // 涨幅榜
            std::sort(d->filteredQuotes.begin(), d->filteredQuotes.end(),
                [](const StockQuote& a, const StockQuote& b) {
                    return a.changePercent > b.changePercent;
                });
            break;
        case 2: // 跌幅榜
            std::sort(d->filteredQuotes.begin(), d->filteredQuotes.end(),
                [](const StockQuote& a, const StockQuote& b) {
                    return a.changePercent < b.changePercent;
                });
            break;
        case 3: // 成交额
            std::sort(d->filteredQuotes.begin(), d->filteredQuotes.end(),
                [](const StockQuote& a, const StockQuote& b) {
                    return a.turnover > b.turnover;
                });
            break;
        default:
            break;
    }
    
    updateTable(d->filteredQuotes);
}

// ============================================================================
// 事件处理
// ============================================================================

void StockQuotesPage::onSearchTextChanged(const QString& text)
{
    d->searchText = text.trimmed();
    applyFilter();
}

void StockQuotesPage::onFilterChanged(int index)
{
    Q_UNUSED(index);
    applyFilter();
}

void StockQuotesPage::onRefreshData()
{
    if (d->dataSource) {
        d->statusLabel->setText("正在刷新...");
        d->dataSource->requestQuotes(d->defaultSymbols);
    }
}

void StockQuotesPage::onViewButtonClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        QString symbol = btn->property("symbol").toString();
        LOG_INFO(QString("View stock: %1").arg(symbol));
        // TODO: 跳转到股票详情页或K线页
    }
}

void StockQuotesPage::onAddToFavoriteClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        QString symbol = btn->property("symbol").toString();
        QString name = btn->property("name").toString();
        LOG_INFO(QString("Add to favorite: %1 (%2)").arg(name, symbol));
        // TODO: 添加到自选股
        QMessageBox::information(this, "提示", 
            QString("已添加 %1 到自选股").arg(name));
    }
}

// ============================================================================
// 工具函数
// ============================================================================

QString StockQuotesPage::formatVolume(qint64 volume) const
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

QString StockQuotesPage::formatMoney(double value) const
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
