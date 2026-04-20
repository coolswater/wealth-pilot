#include "TradeHistoryPage.h"
#include "ui/components/PageStyles.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
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
}

void TradeHistoryPage::onPageActivated(const QVariantMap &params)
{
    Q_UNUSED(params);
}

void TradeHistoryPage::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("成交记录", this);
    titleLabel->setStyleSheet(PageStyles::titleText());
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);
    
    // Statistics Cards
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(12);
    
    auto createStatCard = [](const QString &title, const QString &color) -> QFrame* {
        QFrame *frame = new QFrame();
        frame->setStyleSheet(PageStyles::statCard(color));
        QVBoxLayout *layout = new QVBoxLayout(frame);
        layout->setSpacing(2);
        QLabel *tLabel = new QLabel(title, frame);
        tLabel->setStyleSheet(PageStyles::labelText());
        QLabel *vLabel = new QLabel("0", frame);
        vLabel->setStyleSheet(PageStyles::valueText(color));
        layout->addWidget(tLabel);
        layout->addWidget(vLabel);
        return frame;
    };
    
    QFrame *totalCard = createStatCard("总交易", PageStyles::primaryColor());
    m_totalTradesLabel = totalCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(totalCard);
    
    QFrame *winCard = createStatCard("盈利", PageStyles::upColor());
    m_winCountLabel = winCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(winCard);
    
    QFrame *lossCard = createStatCard("亏损", PageStyles::downColor());
    m_lossCountLabel = lossCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(lossCard);
    
    QFrame *winRateCard = createStatCard("胜率", "#5856D6");
    m_winRateLabel = winRateCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(winRateCard);
    
    QFrame *profitCard = createStatCard("总盈利", PageStyles::upColor());
    m_totalProfitLabel = profitCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(profitCard);
    
    QFrame *lossCard2 = createStatCard("总亏损", PageStyles::downColor());
    m_totalLossLabel = lossCard2->findChildren<QLabel*>().last();
    statsLayout->addWidget(lossCard2);
    
    QFrame *pfCard = createStatCard("盈亏比", PageStyles::warningColor());
    m_profitFactorLabel = pfCard->findChildren<QLabel*>().last();
    statsLayout->addWidget(pfCard);
    
    mainLayout->addLayout(statsLayout);
    
    // Filter Bar
    QHBoxLayout *filterLayout = new QHBoxLayout();
    
    filterLayout->addWidget(new QLabel("方向:", this));
    m_directionFilterCombo = new QComboBox(this);
    m_directionFilterCombo->addItems({"全部", "买入", "卖出"});
    m_directionFilterCombo->setStyleSheet(PageStyles::comboBox());
    filterLayout->addWidget(m_directionFilterCombo);
    
    filterLayout->addSpacing(12);
    filterLayout->addWidget(new QLabel("类型:", this));
    m_typeFilterCombo = new QComboBox(this);
    m_typeFilterCombo->addItems({"全部", "开仓", "平仓"});
    m_typeFilterCombo->setStyleSheet(PageStyles::comboBox());
    filterLayout->addWidget(m_typeFilterCombo);
    
    filterLayout->addSpacing(12);
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
    
    m_refreshBtn = new QPushButton("刷新", this);
    m_refreshBtn->setStyleSheet(PageStyles::secondaryButton());
    m_exportBtn = new QPushButton("导出", this);
    m_exportBtn->setStyleSheet(PageStyles::secondaryButton());
    filterLayout->addWidget(m_refreshBtn);
    filterLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(filterLayout);
    
    // Trade Table
    m_tradeTable = new QTableWidget(this);
    m_tradeTable->setColumnCount(9);
    m_tradeTable->setHorizontalHeaderLabels({"成交ID", "合约", "时间", "价格", "数量", "方向", "类型", "盈亏", "备注"});
    m_tradeTable->horizontalHeader()->setStretchLastSection(true);
    m_tradeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tradeTable->setAlternatingRowColors(true);
    m_tradeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tradeTable->verticalHeader()->setVisible(false);
    m_tradeTable->setStyleSheet(PageStyles::table());
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
    QString dirFilter = m_directionFilterCombo->currentText();
    QString typeFilter = m_typeFilterCombo->currentText();
    QDate startDate = m_startDateEdit->date();
    QDate endDate = m_endDateEdit->date();
    
    QVector<TradeRecord> filtered;
    for (const auto &r : m_records) {
        QDate d = r.tradeTime.date();
        if (d < startDate || d > endDate) continue;
        if (dirFilter != "全部") {
            bool isBuy = (dirFilter == "买入");
            if (r.isBuy != isBuy) continue;
        }
        if (typeFilter != "全部") {
            bool isOpen = (typeFilter == "开仓");
            if (r.isOpen != isOpen) continue;
        }
        filtered.append(r);
    }
    return filtered;
}

void TradeHistoryPage::onRefreshClicked() { emit requestRefresh(); }

void TradeHistoryPage::onExportClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "导出", "trade_history.csv", "CSV Files (*.csv)");
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "成交ID,合约,时间,价格,数量,方向,类型,盈亏,备注\n";
            for (const auto &r : getFilteredRecords()) {
                out << QString("%1,%2,%3,%4,%5,%6,%7,%8,%9\n")
                    .arg(r.tradeId).arg(r.instrumentId)
                    .arg(r.tradeTime.toString("yyyy-MM-dd HH:mm:ss"))
                    .arg(r.price, 0, 'f', 2).arg(r.quantity)
                    .arg(r.isBuy ? "买入" : "卖出")
                    .arg(r.isOpen ? "开仓" : "平仓")
                    .arg(r.profit, 0, 'f', 2).arg(r.remark);
            }
            file.close();
            QMessageBox::information(this, "导出", "导出成功！");
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
        m_tradeTable->setItem(i, 5, new QTableWidgetItem(r.isBuy ? "买入" : "卖出"));
        m_tradeTable->setItem(i, 6, new QTableWidgetItem(r.isOpen ? "开仓" : "平仓"));
        
        auto *profitItem = new QTableWidgetItem(QString::number(r.profit, 'f', 2));
        profitItem->setForeground(QColor(r.profit >= 0 ? PageStyles::upColor() : PageStyles::downColor()));
        m_tradeTable->setItem(i, 7, profitItem);
        m_tradeTable->setItem(i, 8, new QTableWidgetItem(r.remark));
    }
}

TradeHistoryPage::Statistics TradeHistoryPage::calculateStatistics() const
{
    Statistics stats = {};
    for (const auto &r : getFilteredRecords()) {
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
    stats.totalTrades = getFilteredRecords().size();
    int closed = stats.winCount + stats.lossCount;
    if (closed > 0) stats.winRate = (double)stats.winCount / closed * 100.0;
    if (stats.totalLoss > 0) stats.profitFactor = stats.totalProfit / stats.totalLoss;
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
