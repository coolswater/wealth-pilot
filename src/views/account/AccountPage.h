#ifndef ACCOUNTPAGE_H
#define ACCOUNTPAGE_H

#include "core/base/BasePage.h"
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QVector>
#include <QDateTime>

/**
 * @brief AccountPage - Account and fund management page
 */
class AccountPage : public BasePage
{
    Q_OBJECT

public:
    explicit AccountPage(QWidget *parent = nullptr);
    ~AccountPage();
    
    // BasePage interface
    QString pageId() const override { return "account"; }
    QString pageName() const override { return QStringLiteral("账户资金"); }
    void initializePage() override;
    void onPageActivated(const QVariantMap &params) override;

    // Update account data
    void setAccountData(double balance, double available, double margin, 
                        double frozenMargin, double commission, double closeProfit,
                        double positionProfit);
    
    // Fund flow record
    struct FundFlowRecord {
        QDateTime time;
        QString type;       // "Deposit", "Withdraw", "Profit", "Loss", "Commission", "Transfer"
        double amount;
        double balance;
        QString remark;
    };
    
    void addFundFlowRecord(const FundFlowRecord &record);
    void setFundFlowRecords(const QVector<FundFlowRecord> &records);
    
    // Update statistics
    void setStatistics(double totalProfit, double totalLoss, double totalCommission,
                       double maxDrawdown, double winRate);

public slots:
    void refreshData();
    void onDateRangeChanged();
    void onExportClicked();

signals:
    void requestRefresh();
    void requestExport(const QString &format);

private:
    void initUI();
    void initConnections();
    void updateStyles();
    void updateSummary();
    void updateFundFlowTable();
    void updateStatistics();
    
    // Create summary card
    QFrame* createSummaryCard(const QString &title, const QString &value, 
                               const QString &change = "", bool isUp = true);
    
    // UI Components - Summary
    QLabel *m_balanceLabel;
    QLabel *m_availableLabel;
    QLabel *m_marginLabel;
    QLabel *m_frozenMarginLabel;
    QLabel *m_commissionLabel;
    QLabel *m_closeProfitLabel;
    QLabel *m_positionProfitLabel;
    
    // UI Components - Fund Flow Table
    QTableWidget *m_fundFlowTable;
    QComboBox *m_typeFilterCombo;
    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    QPushButton *m_refreshBtn;
    QPushButton *m_exportBtn;
    
    // UI Components - Statistics
    QLabel *m_totalProfitLabel;
    QLabel *m_totalLossLabel;
    QLabel *m_totalCommissionLabel;
    QLabel *m_maxDrawdownLabel;
    QLabel *m_winRateLabel;
    
    // Data
    double m_balance;
    double m_available;
    double m_margin;
    double m_frozenMargin;
    double m_commission;
    double m_closeProfit;
    double m_positionProfit;
    
    double m_totalProfit;
    double m_totalLoss;
    double m_totalCommission;
    double m_maxDrawdown;
    double m_winRate;
    
    QVector<FundFlowRecord> m_fundFlowRecords;
};

#endif // ACCOUNTPAGE_H
