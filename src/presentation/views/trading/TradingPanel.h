/**
 * @file TradingPanel.h
 * @brief 交易面板 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 提供快捷下单入口
 * - 连接 OrderDialog 与 TradingService
 * - 显示订单状态
 * - 管理止损止盈
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅交易数据
 * - 自动生命周期管理
 * - 订单状态实时更新
 *
 * @author WealthPilot Team
 * @version 2.0.0
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
#include "core/trading/TradingTypes.h"
#include "presentation/components/OrderDialog.h"
#include "core/datahub/DataHub.h"

// 使用 DataHub 命名空间
namespace DataHubNS = WealthPilot::DataHub;

/**
 * @brief 交易面板组件
 *
 * @details 使用 DataHub 订阅：
 * - 账户资金（account:balance）
 * - 持仓信息（position:*）
 * - 订单状态（order:*）
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
    // ========== UI 交互槽函数 ==========

    void onBuyOpenClicked();
    void onSellOpenClicked();
    void onBuyCloseClicked();
    void onSellCloseClicked();
    void onOrderDialogSubmitted(const OrderDialog::OrderParams &params);
    void onOrderDialogCancelled();

    // ========== 交易服务槽函数 ==========

    void onTradingServiceOrderSubmitted(const QString &orderId);
    void onTradingServiceOrderFilled(const QString &orderId);
    void onTradingServiceOrderRejected(const QString &orderId, const QString &reason);
    void onTradingServiceTradingLog(const QString &log, int level);

private:
    // ========== UI 初始化 ==========

    void initUI();
    void initConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅账户资金（account:balance）
     * 2. 订阅持仓信息（position:*）
     * 3. 订阅订单状态（order:*）
     */
    void setupDataHubSubscriptions();

    // ========== 数据更新 ==========

    void updateAccountInfo();
    void updatePositionInfo();
    void updateOrderTable();
    void showOrderDialog(PositionDirection direction, OpenCloseFlag openClose);

    // ========== UI 组件 ==========
    QLabel *m_instrumentLabel = nullptr;
    QLabel *m_priceLabel = nullptr;
    QLabel *m_availableLabel = nullptr;

    QPushButton *m_buyOpenBtn = nullptr;
    QPushButton *m_sellOpenBtn = nullptr;
    QPushButton *m_buyCloseBtn = nullptr;
    QPushButton *m_sellCloseBtn = nullptr;

    QTableWidget *m_orderTable = nullptr;
    QLabel *m_statusLabel = nullptr;

    // ========== DataHub 相关 ==========

    /**
     * @brief DataHub 实例引用
     */
    DataHubNS::DataHub& m_dataHub;

    // 订单对话框
    OrderDialog *m_orderDialog = nullptr;

    // ========== 当前合约信息 ==========
    QString m_instrumentId;
    QString m_instrumentName;
    double m_lastPrice = 0.0;
    double m_tickSize = 0.01;
    int m_volumeMultiple = 1;
    double m_marginRatio = 0.1;

    // ========== 当前持仓 ==========
    int m_longPosition = 0;
    int m_shortPosition = 0;
};

#endif // TRADINGPANEL_H