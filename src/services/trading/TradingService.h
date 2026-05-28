/**
 * @file TradingService.h
 * @brief 交易服务 - 协调订单和持仓管理
 */

#ifndef TRADING_SERVICE_H
#define TRADING_SERVICE_H

#include <QObject>
#include "shared/base/Singleton.h"

namespace WealthPilot {
namespace Services {

/**
 * @brief 交易服务 - Services 层
 */
class TradingService : public QObject, public Singleton<TradingService>
{
    Q_OBJECT
    friend class Singleton<TradingService>;

public:
    /**
     * @brief 初始化交易服务
     */
    bool initialize();

    /**
     * @brief 关闭交易服务
     */
    void shutdown();

signals:
    void serviceReady();
    void serviceError(const QString& error);

private:
    TradingService();
    ~TradingService();

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Services
} // namespace WealthPilot

#endif // TRADING_SERVICE_H
