#ifndef CONDITIONORDERPAGE_H
#define CONDITIONORDERPAGE_H

#include "core/base/BasePage.h"
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QVector>
#include <QDateTime>

/**
 * @brief ConditionOrderPage - Condition order management page
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
        double triggerPrice;
        double orderPrice;
        int quantity;
        bool isBuy;
        bool isOpen;
        QDateTime createTime;
        QString remark;
    };

    explicit ConditionOrderPage(QWidget *parent = nullptr);
    ~ConditionOrderPage();
    
    // BasePage interface
    QString pageId() const override { return "conditionOrder"; }
    QString pageName() const override { return QStringLiteral("条件单"); }
    void initializePage() override;
    void onPageActivated(const QVariantMap &params) override;

    void addConditionOrder(const ConditionOrder &order);
    void setConditionOrders(const QVector<ConditionOrder> &orders);
    ConditionOrder getSelectedOrder() const;

public slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onCancelClicked();
    void onRefreshClicked();
    void onFilterChanged();
    void onSelectionChanged();

signals:
    void conditionOrderAdded(const ConditionOrder &order);
    void conditionOrderRemoved(const QString &orderId);
    void conditionOrderCancelled(const QString &orderId);
    void requestRefresh();

private:
    void initUI();
    void initConnections();
    void updateStyles();
    void updateTable();
    void updateSummary();
    
    QString conditionTypeToString(ConditionType type) const;
    QString statusToString(ConditionOrderStatus status) const;
    QColor statusColor(ConditionOrderStatus status) const;

    QTableWidget *m_orderTable;
    QPushButton *m_addBtn;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_refreshBtn;
    QComboBox *m_statusFilterCombo;
    QComboBox *m_typeFilterCombo;
    
    QLabel *m_totalCountLabel;
    QLabel *m_pendingCountLabel;
    QLabel *m_triggeredCountLabel;
    
    QVector<ConditionOrder> m_orders;
};

#endif // CONDITIONORDERPAGE_H
