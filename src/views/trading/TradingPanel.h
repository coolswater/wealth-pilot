/**
 * @file TradingPanel.h
 * @brief 交易面板 - 整合下单对话框与交易服务
 *
 * @details 功能：
 * - 提供快捷下单入口
 * - 连接 OrderDialog 与 TradingService
 * - 显示订单状态
 * - 管理止损止盈
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef TRADINGPANEL_H
#define TRADINGPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "trading/TradingTypes.h"
#include "ui/components/OrderDialog.h"

/**
 * @brief 交易面板组件
 */
class TradingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TradingPanel(QWidget *parent = nullptr);
    ~TradingPanel();

    /**
     * @brief 设置当前合约
     */
    void setCurrentInstrument(const QString &instrumentId,
                              const QString &instrumentName,
                              double lastPrice,
                              double tickSize,
                              int volumeMultiple,
                              double marginRatio);

    /**
     * @brief 刷新数据
     */
    void refresh();

signals:
    /**
     * @brief 订单提交成功
     */
    void orderSubmitted(const QString &orderId);

    /**
     * @brief 订单状态变化
     */
    void orderStatusChanged(const QString &orderId, const QString &status);

private slots:
    void onBuyOpenClicked();
    void onSellOpenClicked();
    void onBuyCloseClicked();
    void onSellCloseClicked();
    void onOrderDialogSubmitted(const OrderDialog::OrderParams &params);
    void onOrderDialogCancelled();
    void onTradingServiceOrderSubmitted(const QString &orderId);
    void onTradingServiceOrderFilled(const QString &orderId);
    void onTradingServiceOrderRejected(const QString &orderId, const QString &reason);
    void onTradingServiceTradingLog(const QString &log, int level);

private:
    void initUI();
    void initConnections();
    void updateAccountInfo();
    void updatePositionInfo();
    void updateOrderTable();
    void showOrderDialog(PositionDirection direction, OpenCloseFlag openClose);

    // UI 组件
    QLabel *m_instrumentLabel;
    QLabel *m_priceLabel;
    QLabel *m_availableLabel;

    QPushButton *m_buyOpenBtn;
    QPushButton *m_sellOpenBtn;
    QPushButton *m_buyCloseBtn;
    QPushButton *m_sellCloseBtn;

    QTableWidget *m_orderTable;
    QLabel *m_statusLabel;

    // 订单对话框
    OrderDialog *m_orderDialog;

    // 当前合约信息
    QString m_instrumentId;
    QString m_instrumentName;
    double m_lastPrice = 0.0;
    double m_tickSize = 0.01;
    int m_volumeMultiple = 1;
    double m_marginRatio = 0.1;

    // 当前持仓
    int m_longPosition = 0;
    int m_shortPosition = 0;
};

#endif // TRADINGPANEL_H
