#include "AccountPage.h"
#include "ui/components/PageStyles.h"
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
    , m_balance(0.0), m_available(0.0), m_margin(0.0)
    , m_frozenMargin(0.0), m_commission(0.0)
    , m_closeProfit(0.0), m_positionProfit(0.0)
    , m_totalProfit(0.0), m_totalLoss(0.0)
    , m_totalCommission(0.0), m_maxDrawdown(0.0), m_winRate(0.0)
{
}

AccountPage::~AccountPage()
{
}

void AccountPage::initializePage()
{
    initUI();
    initConnections();
}

void AccountPage::onPageActivated(const QVariantMap &params)
{
    Q_UNUSED(params);
    refreshData();
}

void AccountPage::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("账户资金", this);
    titleLabel->setStyleSheet(PageStyles::titleText());
    
    m_refreshBtn = new QPushButton("刷新", this);
    m_exportBtn = new QPushButton("导出", this);
    m_refreshBtn->setStyleSheet(PageStyles::secondaryButton());
    m_exportBtn->setStyleSheet(PageStyles::secondaryButton());
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_refreshBtn);
    headerLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(headerLayout);
    
    // Summary Cards Row 1
    QHBoxLayout *summaryLayout = new QHBoxLayout();
    summaryLayout->setSpacing(12);
    
    auto createCard = [](const QString &title) -> QFrame* {
        QFrame *card = new QFrame();
        card->setStyleSheet(PageStyles::statCard());
        QVBoxLayout *layout = new QVBoxLayout(card);
        layout->setSpacing(4);
        QLabel *tLabel = new QLabel(title, card);
        tLabel->setStyleSheet(PageStyles::labelText());
        QLabel *vLabel = new QLabel("0.00", card);
        vLabel->setStyleSheet(PageStyles::valueText());
        layout->addWidget(tLabel);
        layout->addWidget(vLabel);
        return card;
    };
    
    QFrame *balanceCard = createCard("账户余额");
    m_balanceLabel = balanceCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(balanceCard);
    
    QFrame *availableCard = createCard("可用资金");
    m_availableLabel = availableCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(availableCard);
    
    QFrame *marginCard = createCard("占用保证金");
    m_marginLabel = marginCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(marginCard);
    
    QFrame *frozenCard = createCard("冻结保证金");
    m_frozenMarginLabel = frozenCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(frozenCard);
    
    QFrame *commissionCard = createCard("手续费");
    m_commissionLabel = commissionCard->findChildren<QLabel*>().last();
    summaryLayout->addWidget(commissionCard);
    
    mainLayout->addLayout(summaryLayout);
    
    // Summary Cards Row 2 (P&L)
    QHBoxLayout *pnlLayout = new QHBoxLayout();
    pnlLayout->setSpacing(12);
    
    QFrame *closeProfitCard = createCard("已实现盈亏");
    m_closeProfitLabel = closeProfitCard->findChildren<QLabel*>().last();
    pnlLayout->addWidget(closeProfitCard);
    
    QFrame *posProfitCard = createCard("浮动盈亏");
    m_positionProfitLabel = posProfitCard->findChildren<QLabel*>().last();
    pnlLayout->addWidget(posProfitCard);
    
    QFrame *totalProfitCard = createCard("总盈利");
    m_totalProfitLabel = totalProfitCard->findChildren<QLabel*>().last();
    m_totalProfitLabel->setStyleSheet(PageStyles::valueText(PageStyles::upColor()));
    pnlLayout->addWidget(totalProfitCard);
    
    QFrame *totalLossCard = createCard("总亏损");
    m_totalLossLabel = totalLossCard->findChildren<QLabel*>().last();
    m_totalLossLabel->setStyleSheet(PageStyles::valueText(PageStyles::downColor()));
    pnlLayout->addWidget(totalLossCard);
    
    QFrame *winRateCard = createCard("胜率");
    m_winRateLabel = winRateCard->findChildren<QLabel*>().last();
    pnlLayout->addWidget(winRateCard);
    
    mainLayout->addLayout(pnlLayout);
    
    // Filter Bar
    QHBoxLayout *filterLayout = new QHBoxLayout();
    
    filterLayout->addWidget(new QLabel("类型:", this));
    m_typeFilterCombo = new QComboBox(this);
    m_typeFilterCombo->addItems({"全部", "入金", "出金", "盈利", "亏损", "手续费", "转账"});
    m_typeFilterCombo->setStyleSheet(PageStyles::comboBox());
    filterLayout->addWidget(m_typeFilterCombo);
    
    filterLayout->addWidget(new QLabel("从:", this));
    m_startDateEdit = new QDateEdit(QDate::currentDate().addDays(-30), this);
    m_startDateEdit->setCalendarPopup(true);
    m_startDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_startDateEdit->setStyleSheet(PageStyles::dateEdit());
    filterLayout->addWidget(m_startDateEdit);
    
    filterLayout->addWidget(new QLabel("到:", this));
    m_endDateEdit = new QDateEdit(QDate::currentDate(), this);
    m_endDateEdit->setCalendarPopup(true);
    m_endDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_endDateEdit->setStyleSheet(PageStyles::dateEdit());
    filterLayout->addWidget(m_endDateEdit);
    
    filterLayout->addStretch();
    mainLayout->addLayout(filterLayout);
    
    // Fund Flow Table
    m_fundFlowTable = new QTableWidget(this);
    m_fundFlowTable->setColumnCount(5);
    m_fundFlowTable->setHorizontalHeaderLabels({"时间", "类型", "金额", "余额", "备注"});
    m_fundFlowTable->horizontalHeader()->setStretchLastSection(true);
    m_fundFlowTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_fundFlowTable->setAlternatingRowColors(true);
    m_fundFlowTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_fundFlowTable->verticalHeader()->setVisible(false);
    m_fundFlowTable->setStyleSheet(PageStyles::table());
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
    updateSummary();
}

void AccountPage::refreshData() { emit requestRefresh(); }
void AccountPage::onDateRangeChanged() { updateFundFlowTable(); }

void AccountPage::onExportClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "导出资金流水",
        QString("fund_flow_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
        "CSV Files (*.csv)");
    
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "时间,类型,金额,余额,备注\n";
            for (const auto &r : m_fundFlowRecords) {
                out << QString("%1,%2,%3,%4,%5\n")
                    .arg(r.time.toString("yyyy-MM-dd HH:mm:ss"))
                    .arg(r.type).arg(r.amount, 0, 'f', 2)
                    .arg(r.balance, 0, 'f', 2).arg(r.remark);
            }
            file.close();
            QMessageBox::information(this, "导出完成", "导出成功！");
        }
    }
}

void AccountPage::updateSummary()
{
    m_balanceLabel->setText(QString::number(m_balance, 'f', 2));
    m_availableLabel->setText(QString::number(m_available, 'f', 2));
    m_marginLabel->setText(QString::number(m_margin, 'f', 2));
    m_frozenMarginLabel->setText(QString::number(m_frozenMargin, 'f', 2));
    m_commissionLabel->setText(QString::number(m_commission, 'f', 2));
    
    // P&L with color
    m_closeProfitLabel->setText(QString::number(m_closeProfit, 'f', 2));
    m_closeProfitLabel->setStyleSheet(PageStyles::valueText(
        m_closeProfit >= 0 ? PageStyles::upColor() : PageStyles::downColor()));
    
    m_positionProfitLabel->setText(QString::number(m_positionProfit, 'f', 2));
    m_positionProfitLabel->setStyleSheet(PageStyles::valueText(
        m_positionProfit >= 0 ? PageStyles::upColor() : PageStyles::downColor()));
    
    m_totalProfitLabel->setText(QString::number(m_totalProfit, 'f', 2));
    m_totalLossLabel->setText(QString::number(m_totalLoss, 'f', 2));
    m_winRateLabel->setText(QString::number(m_winRate, 'f', 1) + "%");
}

void AccountPage::updateFundFlowTable()
{
    QString filterType = m_typeFilterCombo->currentText();
    QDate startDate = m_startDateEdit->date();
    QDate endDate = m_endDateEdit->date();
    
    QVector<FundFlowRecord> filtered;
    for (const auto &r : m_fundFlowRecords) {
        QDate d = r.time.date();
        if (d < startDate || d > endDate) continue;
        if (filterType != "全部" && r.type != filterType) continue;
        filtered.append(r);
    }
    
    m_fundFlowTable->setRowCount(filtered.size());
    for (int i = 0; i < filtered.size(); ++i) {
        const auto &r = filtered[i];
        m_fundFlowTable->setItem(i, 0, new QTableWidgetItem(r.time.toString("yyyy-MM-dd HH:mm:ss")));
        m_fundFlowTable->setItem(i, 1, new QTableWidgetItem(r.type));
        
        auto *amountItem = new QTableWidgetItem(QString::number(r.amount, 'f', 2));
        amountItem->setForeground(QColor(r.amount >= 0 ? PageStyles::upColor() : PageStyles::downColor()));
        m_fundFlowTable->setItem(i, 2, amountItem);
        
        m_fundFlowTable->setItem(i, 3, new QTableWidgetItem(QString::number(r.balance, 'f', 2)));
        m_fundFlowTable->setItem(i, 4, new QTableWidgetItem(r.remark));
    }
}
