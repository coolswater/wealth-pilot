/**
 * @file ConditionOrderEngine.h
 * @brief 条件单引擎 - 监控条件单触发
 *
 * @details 功能：
 * - 价格条件监控
 * - 时间条件监控
 * - 自动触发下单
 * - 预警通知
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef CONDITIONORDERENGINE_H
#define CONDITIONORDERENGINE_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QMutex>
#include <memory>
#include "core/trading/TradingTypes.h"

/**
 * @brief 条件单引擎
 */
class ConditionOrderEngine : public QObject
{
    Q_OBJECT

public:
    static ConditionOrderEngine& instance();

    bool initialize();
    void shutdown();

    // ========== 条件单管理 ==========

    /**
     * @brief 添加条件单
     * @param condition 条件单信息
     * @return 条件单ID
     */
    QString addConditionOrder(const ConditionOrder& condition);

    /**
     * @brief 删除条件单
     */
    bool removeConditionOrder(const QString& conditionId);

    /**
     * @brief 暂停/恢复条件单
     */
    void setConditionOrderActive(const QString& conditionId, bool active);

    /**
     * @brief 获取所有条件单
     */
    QVector<ConditionOrder> getConditionOrders() const;

    /**
     * @brief 获取指定合约的条件单
     */
    QVector<ConditionOrder> getConditionOrders(const QString& instrumentId) const;

    // ========== 价格监控 ==========

    /**
     * @brief 更新行情价格
     */
    void updatePrice(const QString& instrumentId, double lastPrice);

    /**
     * @brief 设置检查间隔
     */
    void setCheckInterval(int intervalMs);

signals:
    void conditionTriggered(const QString& conditionId, const QString& orderId);
    void conditionAdded(const QString& conditionId);
    void conditionRemoved(const QString& conditionId);
    void triggerFailed(const QString& conditionId, const QString& reason);

private slots:
    void onCheckTimer();
    void onOrderSubmitted(const QString& orderId);
    void onOrderRejected(const QString& orderId, const QString& reason);

private:
    ConditionOrderEngine(QObject* parent = nullptr);
    ~ConditionOrderEngine() override;
    Q_DISABLE_COPY(ConditionOrderEngine)

    void checkCondition(const ConditionOrder& condition, double price);
    void triggerCondition(ConditionOrder& condition);
    void saveConditionOrders();
    void loadConditionOrders();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // CONDITIONORDERENGINE_H
