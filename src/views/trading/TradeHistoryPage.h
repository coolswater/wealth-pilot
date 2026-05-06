/**
 * @file TradeHistoryPage.h
 * @brief 交易历史页面
 */

#ifndef TRADEHISTORYPAGE_H
#define TRADEHISTORYPAGE_H

#include "ui/components/BasePage.h"
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QVector>
#include <QDateTime>

namespace WealthPilot {

/**
 * @brief 交易历史页面
 */
class TradeHistoryPage : public BasePage
{
    Q_OBJECT

public:
    struct TradeRecord {
        QString tradeId;
        QString orderId;
        QString instrumentId;
        QString instrumentName;
        QDateTime tradeTime;
        double price = 0.0;
        int quantity = 0;
    };

    explicit TradeHistoryPage(QWidget* parent = nullptr);
    ~TradeHistoryPage();

    QString pageId() const override { return "tradeHistory"; }
    QString pageName() const override { return QStringLiteral("交易历史"); }
    void initializePage() override;

public slots:
    void onRefreshClicked();
    void onFilterChanged();
    void onExportClicked();

private:
    void setupUI();
    void setupConnections();
    void updateTable();

    QTableWidget* m_table;
    QPushButton* m_refreshBtn;
    QPushButton* m_exportBtn;
    QComboBox* m_filterCombo;
    QDateEdit* m_startDate;
    QDateEdit* m_endDate;
    QVector<TradeRecord> m_records;
};



} // namespace WealthPilot

#endif
