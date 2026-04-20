#ifndef TRADEHISTORYPAGE_H
#define TRADEHISTORYPAGE_H

#include "core/base/BasePage.h"
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QVector>
#include <QDateTime>

/**
 * @brief TradeHistoryPage - Trade history and execution records page
 */
class TradeHistoryPage : public BasePage
{
    Q_OBJECT

public:
    // Trade record structure
    struct TradeRecord {
        QString tradeId;
        QString orderId;
        QString instrumentId;
        QString instrumentName;
        
        QDateTime tradeTime;
        double price;
        int quantity;
        
        bool isBuy;
        bool isOpen;
        
        double commission;
        double profit;
        
        QString remark;
    };

    explicit TradeHistoryPage(QWidget *parent = nullptr);
    ~TradeHistoryPage();
    
    // BasePage interface
    QString pageId() const override { return "tradeHistory"; }
    QString pageName() const override { return QStringLiteral("成交记录"); }
    void initializePage() override;
    void onPageActivated(const QVariantMap &params) override;

    void addTradeRecord(const TradeRecord &record);
    void setTradeRecords(const QVector<TradeRecord> &records);
    QVector<TradeRecord> getFilteredRecords() const;

public slots:
    void onRefreshClicked();
    void onExportClicked();
    void onFilterChanged();

signals:
    void requestRefresh();

private:
    void initUI();
    void initConnections();
    void updateStyles();
    void updateTable();
    void updateStatistics();
    
    struct Statistics {
        int totalTrades;
        int winCount;
        int lossCount;
        double totalProfit;
        double totalLoss;
        double totalCommission;
        double winRate;
        double profitFactor;
    };
    
    Statistics calculateStatistics() const;

    // UI Components
    QTableWidget *m_tradeTable;
    QPushButton *m_refreshBtn;
    QPushButton *m_exportBtn;
    QComboBox *m_directionFilterCombo;
    QComboBox *m_typeFilterCombo;
    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    
    // Statistics labels
    QLabel *m_totalTradesLabel;
    QLabel *m_winCountLabel;
    QLabel *m_lossCountLabel;
    QLabel *m_winRateLabel;
    QLabel *m_totalProfitLabel;
    QLabel *m_totalLossLabel;
    QLabel *m_profitFactorLabel;
    
    // Data
    QVector<TradeRecord> m_records;
};

#endif // TRADEHISTORYPAGE_H
