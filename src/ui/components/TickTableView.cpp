/**
 * @file TickTableView.cpp
 * @brief 分笔成交表格实现
 */

#include "TickTableView.h"
#include "core/config/Tokens.h"
#include <QTableWidgetItem>
#include <QScrollBar>
#include <QHeaderView>

// ========== PIMPL 实现 ==========

struct TickTableView::Impl {
    int maxRows = 500;              ///< 最大行数
    int currentRow = 0;             ///< 当前行索引

    // 颜色定义（使用主题令牌）
    QColor buyColor{Tokens::Colors::Danger};     ///< 买入红色
    QColor sellColor{Tokens::Colors::Success};    ///< 卖出绿色
    QColor textColor{Tokens::Colors::TextPrimary};    ///< 文字颜色
    QColor altRowColor{Tokens::Colors::BgElevated};  ///< 交替行颜色
};

// ========== 构造与析构 ==========

TickTableView::TickTableView(QWidget *parent)
    : QTableWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

TickTableView::~TickTableView() = default;

// ========== 公共接口 ==========

void TickTableView::addTick(const QString& time, double price, int volume, const QString& flag)
{
    // 检查是否需要删除旧行
    trimExcessRows();

    // 添加新行
    int row = rowCount();
    insertRow(row);

    // 时间列
    QTableWidgetItem* timeItem = new QTableWidgetItem(time);
    timeItem->setTextAlignment(Qt::AlignCenter);
    timeItem->setForeground(QBrush(d->textColor));
    setItem(row, 0, timeItem);

    // 价格列
    QTableWidgetItem* priceItem = new QTableWidgetItem(QString::number(price, 'f', 2));
    priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    setItem(row, 1, priceItem);

    // 成交量列
    QTableWidgetItem* volumeItem = new QTableWidgetItem(QString::number(volume));
    volumeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    volumeItem->setForeground(QBrush(d->textColor));
    setItem(row, 2, volumeItem);

    // 方向列
    QTableWidgetItem* flagItem = new QTableWidgetItem(flag);
    flagItem->setTextAlignment(Qt::AlignCenter);
    setItem(row, 3, flagItem);

    // 设置行样式
    updateRowStyle(row, flag);

    // 自动滚动到底部
    autoScrollToBottom();

    d->currentRow = row;
}

void TickTableView::clearTicks()
{
    setRowCount(0);
    d->currentRow = 0;
}

void TickTableView::setMaxRows(int max)
{
    d->maxRows = qMax(10, max);  // 最小10行
}

int TickTableView::maxRows() const
{
    return d->maxRows;
}

int TickTableView::currentRowCount() const
{
    return rowCount();
}

// ========== 私有方法 ==========

void TickTableView::setupUI()
{
    // 设置列数
    setColumnCount(4);

    // 设置表头
    setupHeader();

    // 基本样式
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAlternatingRowColors(false);
    setShowGrid(false);
    setSortingEnabled(false);

    // 隐藏垂直表头
    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(24);

    // 设置列宽
    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);

    setColumnWidth(0, 80);   // 时间
    setColumnWidth(2, 80);   // 成交量
    setColumnWidth(3, 50);   // 方向

    // 样式（使用主题令牌）
    QString style = QString(R"(
        QTableWidget {
            background-color: %1;
            border: none;
            gridline-color: %2;
        }
        QTableWidget::item {
            padding: 2px;
        }
        QHeaderView::section {
            background-color: %3;
            color: %4;
            padding: 4px;
            border: none;
            border-bottom: 1px solid %2;
        }
    )")
        .arg(Tokens::Colors::BgSurface)       // 表格背景
        .arg(Tokens::Colors::Border)           // 网格线颜色
        .arg(Tokens::Colors::BgElevated)       // 表头背景
        .arg(Tokens::Colors::TextSecondary);   // 表头文字
    
    setStyleSheet(style);
}

void TickTableView::setupHeader()
{
    QStringList headers;
    headers << QStringLiteral("时间")
            << QStringLiteral("价格")
            << QStringLiteral("成交量")
            << QStringLiteral("方向");
    setHorizontalHeaderLabels(headers);
}

void TickTableView::autoScrollToBottom()
{
    // 滚动到最后一行
    scrollToBottom();

    // 确保滚动条在底部
    QScrollBar* vScrollBar = verticalScrollBar();
    if (vScrollBar) {
        vScrollBar->setValue(vScrollBar->maximum());
    }
}

void TickTableView::trimExcessRows()
{
    int currentCount = rowCount();
    if (currentCount >= d->maxRows) {
        // 删除多余的行（从顶部删除）
        int removeCount = currentCount - d->maxRows + 1;
        for (int i = 0; i < removeCount; ++i) {
            removeRow(0);
        }
    }
}

void TickTableView::updateRowStyle(int row, const QString& flag)
{
    // 根据买卖方向设置颜色
    QColor priceColor = (flag == QStringLiteral("买")) ? d->buyColor : d->sellColor;

    // 设置价格颜色
    QTableWidgetItem* priceItem = item(row, 1);
    if (priceItem) {
        priceItem->setForeground(QBrush(priceColor));
    }

    // 设置方向颜色
    QTableWidgetItem* flagItem = item(row, 3);
    if (flagItem) {
        flagItem->setForeground(QBrush(priceColor));
    }

    // 设置背景色（交替行）
    QColor bgColor = (row % 2 == 0) ? QColor(Tokens::Colors::BgSurface) : QColor(Tokens::Colors::BgElevated);
    for (int col = 0; col < columnCount(); ++col) {
        QTableWidgetItem* cellItem = item(row, col);
        if (cellItem) {
            cellItem->setBackground(QBrush(bgColor));
        }
    }
}
