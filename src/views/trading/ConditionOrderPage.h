/**
 * @file ConditionOrderPage.h
 * @brief 条件单页面
 */

#ifndef CONDITIONORDERPAGE_H
#define CONDITIONORDERPAGE_H

#include "ui/components/BasePage.h"
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QVector>
#include <QDateTime>

namespace WealthPilot {

/**
 * @brief 条件单页面
 */
class ConditionOrderPage : public BasePage
{
    Q_OBJECT

public:
    enum class ConditionType { StopLoss, TakeProfit, TrailingStop, PriceTrigger, TimeTrigger };
    enum class ConditionOrderStatus { Pending, Triggered, Cancelled, Expired };
    
    struct ConditionOrder {
        QString orderId;
        QString instrumentId;
        ConditionType conditionType;
        ConditionOrderStatus status;
        double triggerPrice = 0.0;
        double orderPrice = 0.0;
        int quantity = 0;
    };

    explicit ConditionOrderPage(QWidget* parent = nullptr);
    ~ConditionOrderPage();

    QString pageId() const override { return "conditionOrder"; }
    QString pageName() const override { return QStringLiteral("条件单"); }
    void initializePage() override;

public slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onRefreshClicked();

private:
    void setupUI();
    void updateTable();

    QTableWidget* m_table = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_editBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;
};



} // namespace WealthPilot

#endif
