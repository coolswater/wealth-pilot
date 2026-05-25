/**
 * @file ConditionOrderPage.h
 * @brief 条件单页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 条件单管理（止损、止盈、追踪止损等）
 * - 条件触发监控
 * - 条件单历史查询
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅条件单数据
 * - 自动生命周期管理
 * - 实时条件触发
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef CONDITIONORDERPAGE_H
#define CONDITIONORDERPAGE_H

#include "presentation/components/DataHubPageBase.h"
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QVector>
#include <QDateTime>

namespace WealthPilot {

/**
 * @brief 条件单页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅条件单列表（condition:orders）
 * - 订阅条件触发事件（condition:triggered）
 * - 页面销毁时自动取消订阅
 */
class ConditionOrderPage : public DataHubPageBase
{
    Q_OBJECT

public:
    /**
     * @brief 条件类型枚举
     */
    enum class ConditionType {
        StopLoss,       ///< 止损
        TakeProfit,     ///< 止盈
        TrailingStop,   ///< 追踪止损
        PriceTrigger,   ///< 价格触发
        TimeTrigger     ///< 时间触发
    };

    /**
     * @brief 条件单状态枚举
     */
    enum class ConditionOrderStatus {
        Pending,        ///< 待触发
        Triggered,      ///< 已触发
        Cancelled,      ///< 已取消
        Expired         ///< 已过期
    };

    /**
     * @brief 条件单结构
     */
    struct ConditionOrder {
        QString orderId;                    ///< 订单ID
        QString instrumentId;               ///< 合约代码
        ConditionType conditionType;        ///< 条件类型
        ConditionOrderStatus status;        ///< 状态
        double triggerPrice = 0.0;          ///< 触发价格
        double orderPrice = 0.0;            ///< 下单价格
        int quantity = 0;                   ///< 数量
    };

    explicit ConditionOrderPage(QWidget* parent = nullptr);
    ~ConditionOrderPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return "conditionOrder"; }
    QString pageName() const override { return QStringLiteral("条件单"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 条件单数据
     * 3. 加载初始数据
     */
    void initializePage() override;

public slots:
    // ========== UI 交互槽函数 ==========

    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onRefreshClicked();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅条件单列表（condition:orders）
     * 2. 订阅条件触发事件（condition:triggered）
     * 3. 回调函数中更新表格
     */
    void setupDataHubSubscriptions();

    // ========== 数据更新 ==========

    void updateTable();

    // ========== UI 组件 ==========
    QTableWidget* m_table = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_editBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QPushButton* m_refreshBtn = nullptr;

    // ========== 数据存储 ==========
    QVector<ConditionOrder> m_orders;
};

} // namespace WealthPilot

#endif // CONDITIONORDERPAGE_H