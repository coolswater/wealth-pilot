#include "TradeHistoryPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

TradeHistoryPage::TradeHistoryPage(QWidget *parent)
    : BasePage(parent)
{
}

TradeHistoryPage::~TradeHistoryPage()
{
}

void TradeHistoryPage::initializePage()
{
    initUI();
    initConnections();
    updateStyles();
}

void TradeHistoryPage::onPageActivated(const QVariantMap &params)
{
    Q_UNUSED(params);
    onRefreshClicked();
}

void TradeHistoryPage::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("Trade History", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);
    
    // Statistics Cards
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(12);
    
    auto createStatCard = [](const QString &title, const QString &color) -> QFrame* {
        QFrame *frame = new QFrame();
        frame->setStyleSheet("background-color: #2C2D33; border-radius: 6px; padding: 8px 12px;");
        QVBoxLayout *layout = new QVBoxLayout(frame);
        layout->setSpacing(2);
        QLabel *tLabel = new QLabel(title, frame);
        tLabel->setStyleSheet("color: #8E8E93; font-size: 11px;");
        QLabel *vLabel = new QLabel("0", frame);
        vLabel->setStyleSheet(QString("color: %1; font-size: 16px; font-weight: bold;").arg(color));
        layout->addWidget(tLabel);
        layout->addWidget(vLabel);
        return frame;
    };
    
    QFrame *totalCard = createStatCard("Total Trades", "#FFFFFF");
    m_totalTradesLabel = totalCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(totalCard);
    
    QFrame *winCard = createStatCard("Wins", "#FF3B30");
    m_winCountLabel = winCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(winCard);
    
    QFrame *lossCard = createStatCard("Losses", "#34C759");
    m_lossCountLabel = lossCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(lossCard);
    
    QFrame *winRateCard = createStatCard("Win Rate", "#5856D6");
    m_winRateLabel = winRateCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(winRateCard);
    
    QFrame *profitCard = createStatCard("Total Profit", "#FF3B30");
    m_totalProfitLabel = profitCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(profitCard);
    
    QFrame *lossCard2 = createStatCard("Total Loss", "#34C759");
    m_totalLossLabel = lossCard2->findChildren<QLabel*>().last();
    statsLayout->addWidget(lossCard2);
    
    QFrame *pfCard = createStatCard("Profit Factor", "#FF9500");
    m_profitFactorLabel = pfCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(pfCard);
    
    mainLayout->addLayout(statsLayout);
    
    // Filter Bar
    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel("Direction:", this));
    m_directionFilterCombo = new QComboBox(this);
    m_directionFilterCombo->addItems({"All", "Buy", "Sell"});
    filterLayout->addWidget(m_directionFilterCombo);
    filterLayout->addSpacing(12);
    filterLayout->addWidget(new QLabel("Type:", this));
    m_typeFilterCombo = new QComboBox(this);
    m_typeFilterCombo->addItems({"All", "Open", "Close"});
    filterLayout->addWidget(m_typeFilterCombo);
    filterLayout->addSpacing(12);
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
    m_refreshBtn = new QPushButton("Refresh", this);
    m_exportBtn = new QPushButton("Export", this);
    filterLayout->addWidget(m_refreshBtn);
    filterLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(filterLayout);
    
    // Trade Table
    m_tradeTable = new QTableWidget(this);
    m_tradeTable->setColumnCount(9);
    m_tradeTable->setHorizontalHeaderLabels({"Trade ID", "Contract", "Time", "Price", "Qty", "Direction", "Type", "Profit", "Remark"});
    m_tradeTable->horizontalHeader()->setStretchLastSection(true);
    m_tradeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tradeTable->setAlternatingRowColors(true);
    m_tradeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tradeTable->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_tradeTable, 1);
}

void TradeHistoryPage::initConnections()
{
    connect(m_refreshBtn, &QPushButton::clicked, this, &TradeHistoryPage::onRefreshClicked);
    connect(m_exportBtn, &QPushButton::clicked, this, &TradeHistoryPage::onExportClicked);
    connect(m_directionFilterCombo, &QComboBox::currentTextChanged, this, &TradeHistoryPage::onFilterChanged);
    connect(m_typeFilterCombo, &QComboBox::currentTextChanged, this, &TradeHistoryPage::onFilterChanged);
    connect(m_startDateEdit, &QDateEdit::dateChanged, this, &TradeHistoryPage::onFilterChanged);
    connect(m_endDateEdit, &QDateEdit::dateChanged, this, &TradeHistoryPage::onFilterChanged);
}

void TradeHistoryPage::updateStyles()
{
    setStyleSheet(R"(
        QWidget { background-color: #1E1F24; }
        QTableWidget { background-color: #2C2D33; color: #FFFFFF; gridline-color: #3A3B41; border: 1px solid #3A3B41; border-radius: 6px; }
        QTableWidget::item { padding: 6px; }
        QTableWidget::item:selected { background-color: #3A3B41; }
        QHeaderView::section { background-color: #2C2D33; color: #8E8E93; padding: 8px; border: none; border-bottom: 1px solid #3A3B41; }
        QPushButton { background-color: #2C2D33; color: #FFFFFF; border: 1px solid #3A3B41; border-radius: 4px; padding: 6px 16px; }
        QPushButton:hover { background-color: #3A3B41; border-color: #FF9500; }
        QComboBox, QDateEdit { background-color: #2C2D33; color: #FFFFFF; border: 1px solid #3A3B41; border-radius: 4px; padding: 4px 8px; }
        QLabel { color: #FFFFFF; }
    )");
}

void TradeHistoryPage::addTradeRecord(const TradeRecord &record)
{
    m_records.append(record);
    updateTable();
    updateStatistics();
}

void TradeHistoryPage::setTradeRecords(const QVector<TradeRecord> &records)
{
    m_records = records;
    updateTable();
    updateStatistics();
}

QVector<TradeHistoryPage::TradeRecord> TradeHistoryPage::getFilteredRecords() const
{
    QString directionFilter = m_directionFilterCombo->currentText();
    QString typeFilter = m_typeFilterCombo->currentText();
    QDate startDate = m_startDateEdit->date();
    QDate endDate = m_endDateEdit->date();
    
    QVector<TradeRecord> filtered;
    for (const auto &record : m_records) {
        QDate recordDate = record.tradeTime.date();
        if (recordDate < startDate || recordDate > endDate) continue;
        if (directionFilter != "All") {
            bool isBuy = (directionFilter == "Buy");
            if (record.isBuy != isBuy) continue;
        }
        if (typeFilter != "All") {
            bool isOpen = (typeFilter == "Open");
            if (record.isOpen != isOpen) continue;
        }
        filtered.append(record);
    }
    return filtered;
}

void TradeHistoryPage::onRefreshClicked() { emit requestRefresh(); }

void TradeHistoryPage::onExportClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Export", "trade_history.csv", "CSV Files (*.csv)");
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "Trade ID,Contract,Time,Price,Quantity,Direction,Type,Profit,Remark\n";
            for (const auto &r : getFilteredRecords()) {
                out << QString("%1,%2,%3,%4,%5,%6,%7,%8,%9\n")
                    .arg(r.tradeId).arg(r.instrumentId)
                    .arg(r.tradeTime.toString("yyyy-MM-dd HH:mm:ss"))
                    .arg(r.price, 0, 'f', 2).arg(r.quantity)
                    .arg(r.isBuy ? "Buy" : "Sell").arg(r.isOpen ? "Open" : "Close")
                    .arg(r.profit, 0, 'f', 2).arg(r.remark);
            }
            file.close();
            QMessageBox::information(this, "Export", "Exported successfully!");
        }
    }
}

void TradeHistoryPage::onFilterChanged()
{
    updateTable();
    updateStatistics();
}

void TradeHistoryPage::updateTable()
{
    QVector<TradeRecord> filtered = getFilteredRecords();
    m_tradeTable->setRowCount(filtered.size());
    
    for (int i = 0; i < filtered.size(); ++i) {
        const auto &r = filtered[i];
        m_tradeTable->setItem(i, 0, new QTableWidgetItem(r.tradeId));
        m_tradeTable->setItem(i, 1, new QTableWidgetItem(r.instrumentId));
        m_tradeTable->setItem(i, 2, new QTableWidgetItem(r.tradeTime.toString("yyyy-MM-dd HH:mm:ss")));
        m_tradeTable->setItem(i, 3, new QTableWidgetItem(QString::number(r.price, 'f', 2)));
        m_tradeTable->setItem(i, 4, new QTableWidgetItem(QString::number(r.quantity)));
        m_tradeTable->setItem(i, 5, new QTableWidgetItem(r.isBuy ? "Buy" : "Sell"));
        m_tradeTable->setItem(i, 6, new QTableWidgetItem(r.isOpen ? "Open" : "Close"));
        auto *profitItem = new QTableWidgetItem(QString::number(r.profit, 'f', 2));
        profitItem->setForeground(QBrush(QColor(r.profit >= 0 ? "#FF3B30" : "#34C759")));
        m_tradeTable->setItem(i, 7, profitItem);
        m_tradeTable->setItem(i, 8, new QTableWidgetItem(r.remark));
    }
}

TradeHistoryPage::Statistics TradeHistoryPage::calculateStatistics() const
{
    Statistics stats = {};
    QVector<TradeRecord> filtered = getFilteredRecords();
    stats.totalTrades = filtered.size();
    
    for (const auto &r : filtered) {
        if (!r.isOpen) {
            if (r.profit >= 0) {
                stats.winCount++;
                stats.totalProfit += r.profit;
            } else {
                stats.lossCount++;
                stats.totalLoss += std::abs(r.profit);
            }
        }
        stats.totalCommission += r.commission;
    }
    
    int closedTrades = stats.winCount + stats.lossCount;
    if (closedTrades > 0) {
        stats.winRate = (double)stats.winCount / closedTrades * 100.0;
    }
    if (stats.totalLoss > 0) {
        stats.profitFactor = stats.totalProfit / stats.totalLoss;
    }
    return stats;
}

void TradeHistoryPage::updateStatistics()
{
    Statistics stats = calculateStatistics();
    m_totalTradesLabel->setText(QString::number(stats.totalTrades));
    m_winCountLabel->setText(QString::number(stats.winCount));
    m_lossCountLabel->setText(QString::number(stats.lossCount));
    m_winRateLabel->setText(QString::number(stats.winRate, 'f', 1) + "%");
    m_totalProfitLabel->setText(QString::number(stats.totalProfit, 'f', 2));
    m_totalLossLabel->setText(QString::number(stats.totalLoss, 'f', 2));
    m_profitFactorLabel->setText(QString::number(stats.profitFactor, 'f', 2));
}
