/**
 * @file TradingTypes.h
 * @brief 交易相关类型定义
 * @details 统一管理订单、持仓、账户等交易相关类型
 * @author WealthPilot Team
 * @date 2026-01-01
 */

#pragma once

#include <QString>
#include <QDateTime>
#include <QVariant>
#include <optional>

namespace WealthPilot {

/**
 * @brief 订单方向枚举
 */
enum class OrderDirection
{
    Buy,    ///< 买入
    Sell    ///< 卖出
};

/**
 * @brief 订单类型枚举
 */
enum class OrderType
{
    Market,       ///< 市价单
    Limit,        ///< 限价单
    Stop,         ///< 止损单
    StopLimit,    ///< 止损限价单
    TrailingStop  ///< 追踪止损单
};

/**
 * @brief 订单状态枚举
 */
enum class OrderStatus
{
    Pending,      ///< 待提交
    Submitted,    ///< 已提交
    Partial,      ///< 部分成交
    Filled,       ///< 完全成交
    Cancelled,    ///< 已撤销
    Rejected,     ///< 已拒绝
    Expired       ///< 已过期
};

/**
 * @brief 订单有效类型枚举
 */
enum class TimeInForce
{
    Day,          ///< 当日有效
    GTC,          ///< 撤销前有效
    IOC,          ///< 立即成交或撤销
    FOK           ///< 全部成交或撤销
};

/**
 * @brief 持仓结构
 */
struct Position
{
    QString symbol;           ///< 证券代码
    QString name;             ///< 证券名称
    qint64 quantity = 0;      ///< 持仓数量
    double avgPrice = 0.0;    ///< 平均成本
    double marketValue = 0.0; ///< 市值
    double profitLoss = 0.0;  ///< 盈亏
    double profitLossPercent = 0.0; ///< 盈亏比例
    double available = 0.0;   ///< 可用数量
    
    bool isValid() const { return !symbol.isEmpty() && quantity != 0; }
};

/**
 * @brief 订单结构
 */
struct Order
{
    QString orderId;          ///< 订单ID
    QString symbol;           ///< 证券代码
    OrderDirection direction = OrderDirection::Buy;
    OrderType type = OrderType::Limit;
    OrderStatus status = OrderStatus::Pending;
    TimeInForce timeInForce = TimeInForce::Day;
    double price = 0.0;       ///< 委托价格
    qint64 quantity = 0;      ///< 委托数量
    qint64 filledQuantity = 0; ///< 成交数量
    double filledPrice = 0.0;  ///< 成交均价
    QDateTime createTime;      ///< 创建时间
    QDateTime updateTime;      ///< 更新时间
    QString message;           ///< 消息
    
    bool isValid() const { return !orderId.isEmpty() && !symbol.isEmpty(); }
    
    bool isActive() const {
        return status == OrderStatus::Pending || 
               status == OrderStatus::Submitted || 
               status == OrderStatus::Partial;
    }
};

/**
 * @brief 账户信息结构
 */
struct AccountInfo
{
    QString accountId;        ///< 账户ID
    QString accountName;      ///< 账户名称
    double totalAssets = 0.0; ///< 总资产
    double availableCash = 0.0; ///< 可用资金
    double marketValue = 0.0; ///< 持仓市值
    double profitLoss = 0.0;  ///< 当日盈亏
    double profitLossPercent = 0.0; ///< 当日盈亏比例
    double marginUsed = 0.0;  ///< 已用保证金
    double marginAvailable = 0.0; ///< 可用保证金
    QDateTime updateTime;     ///< 更新时间
    
    bool isValid() const { return !accountId.isEmpty(); }
};

/**
 * @brief 交易记录结构
 */
struct TradeRecord
{
    QString tradeId;          ///< 成交ID
    QString orderId;          ///< 订单ID
    QString symbol;           ///< 证券代码
    OrderDirection direction = OrderDirection::Buy;
    double price = 0.0;       ///< 成交价格
    qint64 quantity = 0;      ///< 成交数量
    double commission = 0.0;  ///< 手续费
    QDateTime tradeTime;      ///< 成交时间
    
    bool isValid() const { return !tradeId.isEmpty(); }
};

/**
 * @brief 获取订单方向显示名称
 */
inline QString orderDirectionToString(OrderDirection direction)
{
    return direction == OrderDirection::Buy ? QStringLiteral("买入") : QStringLiteral("卖出");
}

/**
 * @brief 获取订单类型显示名称
 */
inline QString orderTypeToString(OrderType type)
{
    switch (type) {
        case OrderType::Market:       return QStringLiteral("市价单");
        case OrderType::Limit:        return QStringLiteral("限价单");
        case OrderType::Stop:         return QStringLiteral("止损单");
        case OrderType::StopLimit:    return QStringLiteral("止损限价单");
        case OrderType::TrailingStop: return QStringLiteral("追踪止损单");
        default: return QStringLiteral("未知");
    }
}

/**
 * @brief 获取订单状态显示名称
 */
inline QString orderStatusToString(OrderStatus status)
{
    switch (status) {
        case OrderStatus::Pending:   return QStringLiteral("待提交");
        case OrderStatus::Submitted: return QStringLiteral("已提交");
        case OrderStatus::Partial:   return QStringLiteral("部分成交");
        case OrderStatus::Filled:    return QStringLiteral("完全成交");
        case OrderStatus::Cancelled: return QStringLiteral("已撤销");
        case OrderStatus::Rejected:  return QStringLiteral("已拒绝");
        case OrderStatus::Expired:   return QStringLiteral("已过期");
        default: return QStringLiteral("未知");
    }
}

} // namespace WealthPilot