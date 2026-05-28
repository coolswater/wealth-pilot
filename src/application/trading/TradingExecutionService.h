/**
 * @file TradingExecutionService.h
 * @brief 交易执行服务 - 协调 OrderManager 和交易接口
 */

#ifndef TRADING_EXECUTION_SERVICE_H
#define TRADING_EXECUTION_SERVICE_H

#include <QObject>
#include "shared/base/Singleton.h"

namespace WealthPilot {
namespace Application {

/**
 * @brief 交易执行服务 - 协调 Domain 和 Infrastructure
 */
class TradingExecutionService : public QObject, public Singleton<TradingExecutionService>
{
    Q_OBJECT
    friend class Singleton<TradingExecutionService>;

public:
    /**
     * @brief 执行下单
     */
    void placeOrder(const QString& symbol, int direction, double price, int volume);

    /**
     * @brief 撤单
     */
    void cancelOrder(const QString& orderId);

    /**
     * @brief 查询持仓
     */
    void queryPositions();

signals:
    void orderPlaced(const QString& orderId);
    void orderFilled(const QString& orderId, double price, int volume);
    void orderCancelled(const QString& orderId);
    void errorOccurred(const QString& message);

private:
    TradingExecutionService();
    ~TradingExecutionService();

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Application
} // namespace WealthPilot

#endif // TRADING_EXECUTION_SERVICE_H
