#include "AccountPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <cmath>

AccountPage::AccountPage(QWidget *parent)
    : BasePage(parent)
    , m_balance(0.0)
    , m_available(0.0)
    , m_margin(0.0)
    , m_frozenMargin(0.0)
    , m_commission(0.0)
    , m_closeProfit(0.0)
    , m_positionProfit(0.0)
    , m_totalProfit(0.0)
    , m_totalLoss(0.0)
    , m_totalCommission(0.0)
    , m_maxDrawdown(0.0)
    , m_winRate(0.0)
{
}

AccountPage::~AccountPage()
{
}

void AccountPage::initializePage()
{
    initUI();
    initConnections();
    updateStyles();
}

void AccountPage::onPageActivated(const QVariantMap &params)
{
    Q_UNUSED(params);
    refreshData();
}

QFrame* AccountPage::createSummaryCard(const QString &title, const QString &value,
                                        const QString &change, bool isUp)
{
    QFrame *card = new QFrame(this);
    card->setObjectName("summaryCard");
    card->setStyleSheet(R"(
        QFrame#summaryCard {
            background-color: #2C2D33;
            border-radius: 8px;
            padding: 12px;
        }
    )");
    
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setSpacing(4);
    
    QLabel *titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("color: #8E8E93; font-size: 12px;");
    
    QLabel *valueLabel = new QLabel(value, card);
    valueLabel->setStyleSheet("color: #FFFFFF; font-size: 18px; font-weight: bold;");
    
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    
    if (!change.isEmpty()) {
        QLabel *changeLabel = new QLabel(change, card);
        changeLabel->setStyleSheet(QString("color: %1; font-size: 12px;")
                                   .arg(isUp ? "#FF3B30" : "#34C759"));
        layout->addWidget(changeLabel);
    }
    
    return card;
}

void AccountPage::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // ===== Header =====
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QLabel *titleLabel = new QLabel("Account & Funds", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");
    
    m_refreshBtn = new QPushButton("Refresh", this);
    m_exportBtn = new QPushButton("Export", this);
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_refreshBtn);
    headerLayout->addWidget(m_exportBtn);
    
    mainLayout->addLayout(headerLayout);
    
    // ===== Summary Cards =====
    QHBoxLayout *summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(12);
    
    // Balance card
    QFrame *balanceCard = createSummaryCard("Account Balance", "0.00");
    m_balanceLabel = balanceCard->findChildren<QLabel*>().value(1);
    summaryLayout->addWidget(balanceCard);
    
    // Available card
    QFrame *availableCard = createSummaryCard("Available Funds", "0.00");
    m_availableLabel = availableCard->findChildren<QLabel*>().value(1);
    summaryLayout->addWidget(availableCard);
    
    // Margin card
    QFrame *marginCard = createSummaryCard("Occupied Margin", "0.00");
    m_marginLabel = marginCard->findChildren<QLabel*>().value(1);
    summaryLayout->addWidget(marginCard);
    
    // Frozen margin card
    QFrame *frozenCard = createSummaryCard("Frozen Margin", "0.00");
    m_frozenMarginLabel = frozenCard->findChildren<QLabel*>().value(1);
    summaryLayout->addWidget(frozenCard);
    
    // Commission card
    QFrame *commissionCard = createSummaryCard("Total Commission", "0.00");
    m_commissionLabel = commissionCard->findChildren<QLabel*>().value(1);
    summaryLayout->addWidget(commissionCard);
    
    mainLayout->addLayout(summaryLayout);
    
    // ===== Profit/Loss Row =====
    QHBoxLayout *pnlLayout = new QHBoxLayout();
    pnlLayout->setSpacing(12);
    
    QFrame *closeProfitCard = createSummaryCard("Realized P&L", "0.00");
    m_closeProfitLabel = closeProfitCard->findChildren<QLabel*>().value(1);
    pnlLayout->addWidget(closeProfitCard);
    
    QFrame *posProfitCard = createSummaryCard("Unrealized P&L", "0.00");
    m_positionProfitLabel = posProfitCard->findChildren<QLabel*>().value(1);
    pnlLayout->addWidget(posProfitCard);
    
    QFrame *totalProfitCard = createSummaryCard("Total Profit", "0.00");
    m_totalProfitLabel = totalProfitCard->findChildren<QLabel*>().value(1);
    pnlLayout->addWidget(totalProfitCard);
    
    QFrame *totalLossCard = createSummaryCard("Total Loss", "0.00");
    m_totalLossLabel = totalLossCard->findChildren<QLabel*>().value(1);
    pnlLayout->addWidget(totalLossCard);
    
    QFrame *winRateCard = createSummaryCard("Win Rate", "0.00%");
    m_winRateLabel = winRateCard->findChildren<QLabel*>().value(1);
    pnlLayout->addWidget(winRateCard);
    
    mainLayout->addLayout(pnlLayout);
    
    // ===== Filter Bar =====
    QHBoxLayout *filterLayout = new QHBoxLayout();
    
    filterLayout->addWidget(new QLabel("Type:", this));
    m_typeFilterCombo = new QComboBox(this);
    m_typeFilterCombo->addItems({"All", "Deposit", "Withdraw", "Profit", "Loss", "Commission", "Transfer"});
    filterLayout->addWidget(m_typeFilterCombo);
    
    filterLayout->addWidget(new QLabel("From:", this));
    m_startDateEdit = new QDateEdit(QDate::currentDate().addDays(-30), this);
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd");
    filterLayout->addWidget(m_startDateEdit);
    
    filterLayout->addWidget(new QLabel("To:", this));
    m_endDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDisplayFormat("yyyy-MM-dd");
    filterLayout->addWidget(m_endDateEdit);
    
    filterLayout->addStretch();
    
    mainLayout->addLayout(filterLayout);
    
    // ===== Fund Flow Table =====
    m_fundFlowTable = new QTableWidget(this);
    m_fundFlowTable->setColumnCount(5);
    m_fundFlowTable->setHorizontalHeaderLabels({"Time", "Type", "Amount", "Balance", "Remark"});
    m_fundFlowTable->horizontalHeader()->setStretchLastSection(true);
    m_fundFlowTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fundFlowTable->setAlternatingRowColors(true);
    m_fundFlowTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fundFlowTable->verticalHeader()->setVisible(false);
    
    mainLayout->addWidget(m_fundFlowTable, 1);
}

void AccountPage::initConnections()
{
    connect(m_refreshBtn, &QPushButton::clicked, this, &AccountPage::refreshData);
    connect(m_exportBtn, &QPushButton::clicked, this, &AccountPage::onExportClicked);
    connect(m_typeFilterCombo, &QComboBox::currentTextChanged, this, &AccountPage::onDateRangeChanged);
    connect(m_startDateEdit, &QDateEdit::dateChanged, this, &AccountPage::onDateRangeChanged);
    connect(m_endDateEdit, &QDateEdit::dateChanged, this, &AccountPage::onDateRangeChanged);
}

void AccountPage::updateStyles()
{
    setStyleSheet(R"(
        QWidget {
            background-color: #1E1F24;
        }
        QTableWidget {
            background-color: #2C2D33;
            color: #FFFFFF;
            gridline-color: #3A3B41;
            border: 1px solid #3A3B41;
            border-radius: 6px;
        }
        QTableWidget::item {
            padding: 6px;
        }
        QTableWidget::item:selected {
            background-color: #3A3B41;
        }
        QHeaderView::section {
            background-color: #2C2D33;
            color: #8E8E93;
            padding: 8px;
            border: none;
            border-bottom: 1px solid #3A3B41;
        }
        QPushButton {
            background-color: #2C2D33;
            color: #FFFFFF;
            border: 1px solid #3A3B41;
            border-radius: 4px;
            padding: 6px 16px;
        }
        QPushButton:hover {
            background-color: #3A3B41;
            border-color: #FF9500;
        }
        QComboBox, QDateEdit {
            background-color: #2C2D33;
            color: #FFFFFF;
            border: 1px solid #3A3B41;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QLabel {
            color: #FFFFFF;
        }
    )");
}

void AccountPage::setAccountData(double balance, double available, double margin,
                                  double frozenMargin, double commission, double closeProfit,
                                  double positionProfit)
{
    m_balance = balance;
    m_available = available;
    m_margin = margin;
    m_frozenMargin = frozenMargin;
    m_commission = commission;
    m_closeProfit = closeProfit;
    m_positionProfit = positionProfit;
    
    updateSummary();
}

void AccountPage::addFundFlowRecord(const FundFlowRecord &record)
{
    m_fundFlowRecords.append(record);
    updateFundFlowTable();
}

void AccountPage::setFundFlowRecords(const QVector<FundFlowRecord> &records)
{
    m_fundFlowRecords = records;
    updateFundFlowTable();
}

void AccountPage::setStatistics(double totalProfit, double totalLoss, double totalCommission,
                                 double maxDrawdown, double winRate)
{
    m_totalProfit = totalProfit;
    m_totalLoss = totalLoss;
    m_totalCommission = totalCommission;
    m_maxDrawdown = maxDrawdown;
    m_winRate = winRate;
    
    updateStatistics();
}

void AccountPage::refreshData()
{
    emit requestRefresh();
}

void AccountPage::onDateRangeChanged()
{
    updateFundFlowTable();
}

void AccountPage::onExportClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Export Fund Flow",
        QString("fund_flow_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
        "CSV Files (*.csv)");
    
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "Time,Type,Amount,Balance,Remark\n";
            for (const auto &record : m_fundFlowRecords) {
                out << QString("%1,%2,%3,%4,%5\n")
                    .arg(record.time.toString("yyyy-MM-dd HH:mm:ss"))
                    .arg(record.type)
                    .arg(record.amount, 0, 'f', 2)
                    .arg(record.balance, 0, 'f', 2)
                    .arg(record.remark);
            }
            file.close();
            QMessageBox::information(this, "Export Complete", "Exported successfully!");
        }
    }
}

void AccountPage::updateSummary()
{
    if (m_balanceLabel) m_balanceLabel->setText(QString::number(m_balance, 'f', 2));
    if (m_availableLabel) m_availableLabel->setText(QString::number(m_available, 'f', 2));
    if (m_marginLabel) m_marginLabel->setText(QString::number(m_margin, 'f', 2));
    if (m_frozenMarginLabel) m_frozenMarginLabel->setText(QString::number(m_frozenMargin, 'f', 2));
    if (m_commissionLabel) m_commissionLabel->setText(QString::number(m_commission, 'f', 2));
    
    if (m_closeProfitLabel) {
        m_closeProfitLabel->setText(QString::number(m_closeProfit, 'f', 2));
        m_closeProfitLabel->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold;")
                                           .arg(m_closeProfit >= 0 ? "#FF3B30" : "#34C759"));
    }
    
    if (m_positionProfitLabel) {
        m_positionProfitLabel->setText(QString::number(m_positionProfit, 'f', 2));
        m_positionProfitLabel->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold;")
                                              .arg(m_positionProfit >= 0 ? "#FF3B30" : "#34C759"));
    }
}

void AccountPage::updateFundFlowTable()
{
    QString filterType = m_typeFilterCombo->currentText();
    QDate startDate = m_startDateEdit->date();
    QDate endDate = m_endDateEdit->date();
    
    // Filter records
    QVector<FundFlowRecord> filtered;
    for (const auto &record : m_fundFlowRecords) {
        QDate recordDate = record.time.date();
        if (recordDate < startDate || recordDate > endDate) continue;
        if (filterType != "All" && record.type != filterType) continue;
        filtered.append(record);
    }
    
    m_fundFlowTable->setRowCount(filtered.size());
    
    for (int i = 0; i < filtered.size(); ++i) {
        const auto &record = filtered[i];
        
        auto *timeItem = new QTableWidgetItem(record.time.toString("yyyy-MM-dd HH:mm:ss"));
        auto *typeItem = new QTableWidgetItem(record.type);
        auto *amountItem = new QTableWidgetItem(QString::number(record.amount, 'f', 2));
        auto *balanceItem = new QTableWidgetItem(QString::number(record.balance, 'f', 2));
        auto *remarkItem = new QTableWidgetItem(record.remark);
        
        // Color amount based on positive/negative
        if (record.amount >= 0) {
            amountItem->setForeground(QColor("#FF3B30"));
        } else {
            amountItem->setForeground(QColor("#34C759"));
        }
        
        m_fundFlowTable->setItem(i, 0, timeItem);
        m_fundFlowTable->setItem(i, 1, typeItem);
        m_fundFlowTable->setItem(i, 2, amountItem);
        m_fundFlowTable->setItem(i, 3, balanceItem);
        m_fundFlowTable->setItem(i, 4, remarkItem);
    }
}

void AccountPage::updateStatistics()
{
    if (m_totalProfitLabel) m_totalProfitLabel->setText(QString::number(m_totalProfit, 'f', 2));
    if (m_totalLossLabel) m_totalLossLabel->setText(QString::number(m_totalLoss, 'f', 2));
    if (m_totalCommissionLabel) m_totalCommissionLabel->setText(QString::number(m_totalCommission, 'f', 2));
    if (m_maxDrawdownLabel) m_maxDrawdownLabel->setText(QString::number(m_maxDrawdown, 'f', 2) + "%");
    if (m_winRateLabel) m_winRateLabel->setText(QString::number(m_winRate, 'f', 1) + "%");
}
