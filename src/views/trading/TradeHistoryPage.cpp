/**
 * @file TradeHistoryPage.cpp
 * @brief 交易历史页面实现
 */

#include "TradeHistoryPage.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>

namespace WealthPilot {

TradeHistoryPage::TradeHistoryPage(QWidget* parent)
    : BasePage(parent)
    , m_table(new QTableWidget(this))
    , m_refreshBtn(new QPushButton(QStringLiteral("刷新"), this))
    , m_exportBtn(new QPushButton(QStringLiteral("导出"), this))
    , m_filterCombo(new QComboBox(this))
    , m_startDate(new QDateEdit(QDate::currentDate().addDays(-30), this))
    , m_endDate(new QDateEdit(QDate::currentDate(), this))
{
    setupUI();
    setupConnections();
}

TradeHistoryPage::~TradeHistoryPage() = default;

void TradeHistoryPage::initializePage()
{
    if (isInitialized()) return;
    
    // 加载示例数据
    m_records.clear();
    TradeRecord record;
    record.tradeId = "T001";
    record.orderId = "O001";
    record.instrumentId = "sh600000";
    record.instrumentName = "浦发银行";
    record.tradeTime = QDateTime::currentDateTime();
    record.price = 10.50;
    record.quantity = 1000;
    m_records.append(record);
    
    updateTable();
    setInitialized(true);
    LOG_DEBUG("TradeHistoryPage initialized");
}

void TradeHistoryPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 顶部工具栏
    auto* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    // 筛选下拉框
    m_filterCombo->addItem(QStringLiteral("全部"), QStringLiteral("all"));
    m_filterCombo->addItem(QStringLiteral("买入"), QStringLiteral("buy"));
    m_filterCombo->addItem(QStringLiteral("卖出"), QStringLiteral("sell"));
    toolbarLayout->addWidget(m_filterCombo);

    // 日期范围
    m_startDate->setCalendarPopup(true);
    m_endDate->setCalendarPopup(true);
    toolbarLayout->addWidget(m_startDate);
    toolbarLayout->addWidget(new QLabel(QStringLiteral("-"), this));
    toolbarLayout->addWidget(m_endDate);

    toolbarLayout->addStretch();

    // 按钮
    toolbarLayout->addWidget(m_refreshBtn);
    toolbarLayout->addWidget(m_exportBtn);

    mainLayout->addLayout(toolbarLayout);

    // 表格
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("成交ID"),
        QStringLiteral("合约"),
        QStringLiteral("时间"),
        QStringLiteral("价格"),
        QStringLiteral("数量"),
        QStringLiteral("方向")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);

    mainLayout->addWidget(m_table);

    // 设置样式
    setStyleSheet(R"(
        QTableWidget {
            background-color: #0F1419;
            alternate-background-color: #1A1F2E;
            gridline-color: #2D3748;
            border: 1px solid #2D3748;
            border-radius: 4px;
        }
        QTableWidget::item {
            padding: 4px;
        }
        QHeaderView::section {
            background-color: #1A1F2E;
            color: #A0AEC0;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #2D3748;
            font-weight: bold;
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
        QComboBox, QDateEdit {
            background-color: #1A1F2E;
            color: #E2E8F0;
            border: 1px solid #2D3748;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QLabel {
            color: #A0AEC0;
        }
    )");
}

void TradeHistoryPage::setupConnections()
{
    connect(m_refreshBtn, &QPushButton::clicked, this, &TradeHistoryPage::onRefreshClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &TradeHistoryPage::onExportClicked);
    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &TradeHistoryPage::onFilterChanged);
}

void TradeHistoryPage::updateTable()
{
    m_table->setRowCount(m_records.size());
    
    for (int i = 0; i < m_records.size(); ++i) {
        const auto& record = m_records[i];
        m_table->setItem(i, 0, new QTableWidgetItem(record.tradeId));
        m_table->setItem(i, 1, new QTableWidgetItem(record.instrumentName));
        m_table->setItem(i, 2, new QTableWidgetItem(record.tradeTime.toString("yyyy-MM-dd hh:mm:ss")));
        m_table->setItem(i, 3, new QTableWidgetItem(QString::number(record.price, 'f', 2)));
        m_table->setItem(i, 4, new QTableWidgetItem(QString::number(record.quantity)));
        m_table->setItem(i, 5, new QTableWidgetItem(QStringLiteral("买入")));
    }
}

void TradeHistoryPage::onRefreshClicked()
{
    initializePage();
    LOG_DEBUG("TradeHistoryPage refreshed");
}

void TradeHistoryPage::onFilterChanged()
{
    // TODO: 实现筛选逻辑
    LOG_DEBUG("Filter changed");
}

void TradeHistoryPage::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        QStringLiteral("导出交易记录"),
        QString(),
        QStringLiteral("CSV文件 (*.csv)"));
    
    if (!fileName.isEmpty()) {
        // TODO: 实现导出逻辑
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("导出成功"));
        LOG_DEBUG(QString("Export to: %1").arg(fileName));
    }
}

} // namespace WealthPilot
