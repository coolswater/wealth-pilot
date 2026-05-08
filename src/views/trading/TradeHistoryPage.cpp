/**
 * @file TradeHistoryPage.cpp
 * @brief 交易历史页面实现
 */

#include "TradeHistoryPage.h"
#include "core/config/Tokens.h"
#include "ui/components/PageStyles.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QStringConverter>

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
    mainLayout->setContentsMargins(Tokens::Spacing::MD, Tokens::Spacing::MD, Tokens::Spacing::MD, Tokens::Spacing::MD);
    mainLayout->setSpacing(Tokens::Spacing::SM);

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
    m_table->setStyleSheet(QString(R"(
        QTableWidget {
            background-color: %1;
            alternate-background-color: %2;
            gridline-color: %3;
            border: 1px solid %3;
            border-radius: 4px;
        }
        QTableWidget::item {
            padding: 4px;
        }
        QHeaderView::section {
            background-color: %2;
            color: %4;
            padding: 6px;
            border: none;
            border-bottom: 1px solid %3;
            font-weight: bold;
        }
    )").arg(Tokens::Colors::BgSurface, Tokens::Colors::BgBase, Tokens::Colors::Border, Tokens::Colors::TextSecondary));

    m_refreshBtn->setStyleSheet(PageStyles::primaryButton());
    m_exportBtn->setStyleSheet(PageStyles::secondaryButton());

    m_filterCombo->setStyleSheet(PageStyles::comboBox());
    m_startDate->setStyleSheet(PageStyles::dateEdit());
    m_endDate->setStyleSheet(PageStyles::dateEdit());
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
    // 获取筛选条件
    QString filterText = m_filterCombo->currentText();
    QDate startDate = m_startDate->date();
    QDate endDate = m_endDate->date();

    // 实现筛选逻辑
    // 根据筛选条件过滤交易记录
    QVector<TradeRecord> filteredRecords;
    
    for (const auto& record : m_records) {
        bool match = true;
        
        // 按合约代码筛选
        if (!filterText.isEmpty() && filterText != QStringLiteral("全部")) {
            if (!record.instrumentId.contains(filterText, Qt::CaseInsensitive)) {
                match = false;
            }
        }
        
        // 按日期范围筛选
        QDate recordDate = record.tradeTime.date();
        if (recordDate < startDate || recordDate > endDate) {
            match = false;
        }
        
        if (match) {
            filteredRecords.append(record);
        }
    }
    
    // 更新表格显示筛选后的数据
    m_table->setRowCount(filteredRecords.size());
    for (int i = 0; i < filteredRecords.size(); ++i) {
        const auto& record = filteredRecords[i];
        m_table->setItem(i, 0, new QTableWidgetItem(record.tradeId));
        m_table->setItem(i, 1, new QTableWidgetItem(record.instrumentId));
        m_table->setItem(i, 2, new QTableWidgetItem(record.instrumentName));
        m_table->setItem(i, 3, new QTableWidgetItem(record.tradeTime.toString("yyyy-MM-dd HH:mm:ss")));
        m_table->setItem(i, 4, new QTableWidgetItem(QString::number(record.price, 'f', 2)));
        m_table->setItem(i, 5, new QTableWidgetItem(QString::number(record.quantity)));
    }
    
    LOG_DEBUG(QString("Filter applied: %1 records (from %2 total)")
        .arg(filteredRecords.size())
        .arg(m_records.size()));
}

void TradeHistoryPage::onExportClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, 
        QStringLiteral("导出交易记录"),
        QString(),
        QStringLiteral("CSV文件 (*.csv)"));
    
    if (!fileName.isEmpty()) {
        QFile file(fileName);

        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            // Qt6 使用 setEncoding 替代 setCodec
            out.setEncoding(QStringConverter::Utf8);

            // 写入CSV头部
            out << QStringLiteral("交易ID,订单ID,合约代码,合约名称,交易时间,价格,数量\n");

            // 写入数据
            for (const auto& record : m_records)
            {
                out << QString("%1,%2,%3,%4,%5,%6,%7\n")
                       .arg(record.tradeId)
                       .arg(record.orderId)
                       .arg(record.instrumentId)
                       .arg(record.instrumentName)
                       .arg(record.tradeTime.toString("yyyy-MM-dd HH:mm:ss"))
                       .arg(record.price, 0, 'f', 2)
                       .arg(record.quantity);
            }

            file.close();

            QMessageBox::information(this, QStringLiteral("提示"),
                                     QStringLiteral("成功导出 %1 条记录").arg(m_records.size()));

            LOG_INFO(QString("Exported %1 records to: %2").arg(m_records.size()).arg(fileName));
        }
        else
        {
            QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("无法创建文件"));
        }
    }
}

} // namespace WealthPilot
