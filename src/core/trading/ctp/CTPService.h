/**
 * @file CTPService.h
 * @brief CTP 服务占位实现
 * 
 * @note 此文件为临时占位，CTP 完整实现需要安装 CTP API
 */

#ifndef CTPSERVICE_H
#define CTPSERVICE_H

#include "shared/base/Singleton.h"
#include <QObject>
#include <QString>

namespace CTP {

// 方向枚举
enum class Direction {
    Buy,
    Sell
};

// 开平标志
enum class OffsetFlag {
    Open,
    Close,
    CloseToday,
    CloseYesterday
};

// 订单信息
struct OrderInfo {
    QString instrumentId;
    Direction direction = Direction::Buy;
    OffsetFlag offset = OffsetFlag::Open;
    double price = 0.0;
    int totalVolume = 0;
    QString orderRef;
};

// 成交信息
struct TradeInfo {
    QString instrumentId;
    double price = 0.0;
    int volume = 0;
};

/**
 * @brief CTP 服务 - 占位实现
 */
class CTPService : public QObject, public Singleton<CTPService>
{
    Q_OBJECT

public:
    /**
     * @brief 下单
     */
    std::optional<QString> insertOrder(const OrderInfo& order) {
        Q_UNUSED(order)
        // 占位实现：返回空表示失败
        return std::nullopt;
    }

    /**
     * @brief 撤单
     */
    bool cancelOrder(const QString& orderId) {
        Q_UNUSED(orderId)
        return false;
    }

signals:
    void orderUpdated(const OrderInfo& info);
    void tradeReceived(const TradeInfo& info);
    void positionReceived(const QString& symbol, int volume, int available);
    void accountInfoReceived(double available, double margin);
};

} // namespace CTP

#endif // CTPSERVICE_H
