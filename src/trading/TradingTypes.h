/**
 * @file TradingTypes.h
 * @brief 交易系统统一类型定义
 *
 * @details 定义：
 * - 订单数据结构
 * - 持仓数据结构
 * - 交易方向枚举
 * - 订单状态枚举
 * - 风控规则结构
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef TRADINGTYPES_H
#define TRADINGTYPES_H

#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMap>
#include <QVariant>
#include <QRandomGenerator>
#include <QUuid>

// 包含市场类型定义，避免重复定义
#include "core/types/MarketTypes.h"

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::TradeDirection;
using WealthPilot::OrderStatus;

// ============================================================================
// 枚举定义（扩展 MarketTypes.h 中的定义）
// ============================================================================

/**
 * @brief 订单类型枚举
 */
enum class OrderType {
    Market = 0,         ///< 市价单
    Limit = 1,          ///< 限价单
    Stop = 2,           ///< 止损单
    StopLimit = 3,      ///< 止损限价单
    Iceberg = 4,        ///< 冰山单
    Unknown = 99        ///< 未知
};

/**
 * @brief 持仓方向枚举
 */
enum class PositionDirection {
    Long = 0,           ///< 多头持仓
    Short = 1,          ///< 空头持仓
    Net = 2,            ///< 净持仓
    Unknown = 99        ///< 未知
};

/**
 * @brief 开平标志枚举
 */
enum class OpenCloseFlag {
    Open = 0,           ///< 开仓
    Close = 1,          ///< 平仓
    CloseToday = 2,     ///< 平今
    CloseYesterday = 3, ///< 平昨
    Unknown = 99        ///< 未知
};

/**
 * @brief 投机套保标志枚举
 */
enum class HedgeFlag {
    Speculation = 0,    ///< 投机
    Arbitrage = 1,      ///< 套利
    Hedge = 2,          ///< 套保
    Unknown = 99        ///< 未知
};

/**
 * @brief 条件单类型枚举
 */
enum class ConditionType {
    PriceAbove = 0,     ///< 价格高于
    PriceBelow = 1,     ///< 价格低于
    TimeReach = 2,      ///< 时间到达
    Unknown = 99        ///< 未知
};

/**
 * @brief 触发条件枚举
 */
enum class TriggerCondition {
    Immediately = 0,    ///< 立即触发
    StopLoss = 1,       ///< 止损触发
    TakeProfit = 2,     ///< 止盈触发
    TrailingStop = 3,   ///< 移动止损触发
    Unknown = 99        ///< 未知
};

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 订单请求结构
 * @details 用于下单时传递的参数
 */
struct OrderRequest {
    QString requestId;              ///< 请求ID（客户端生成）
    QString instrumentId;           ///< 合约代码
    QString exchangeId;             ///< 交易所代码
    TradeDirection direction;       ///< 买卖方向
    OpenCloseFlag openClose;        ///< 开平标志
    OrderType orderType;            ///< 订单类型
    HedgeFlag hedgeFlag;            ///< 投机套保标志
    double price = 0.0;             ///< 委托价格（市价单填0）
    int volume = 0;                 ///< 委托数量
    double stopPrice = 0.0;         ///< 止损价格（止损单用）
    QDateTime expireTime;           ///< 过期时间（日内单用）
    QString strategyId;             ///< 策略ID（可选）
    QString remark;                 ///< 备注（可选）
    
    /**
     * @brief 验证订单请求有效性
     * @return 错误信息，空字符串表示有效
     */
    QString validate() const {
        if (instrumentId.isEmpty()) {
            return QStringLiteral("合约代码不能为空");
        }
        if (volume <= 0) {
            return QStringLiteral("委托数量必须大于0");
        }
        if (orderType == OrderType::Limit && price <= 0) {
            return QStringLiteral("限价单价格必须大于0");
        }
        return QString();
    }
};

/**
 * @brief 订单信息结构
 * @details 订单的完整信息，包括状态、成交等
 */
struct OrderInfo {
    QString orderId;                ///< 订单ID（交易所返回）
    QString requestId;              ///< 请求ID（客户端生成）
    QString instrumentId;           ///< 合约代码
    QString exchangeId;             ///< 交易所代码
    TradeDirection direction;       ///< 买卖方向
    OpenCloseFlag openClose;        ///< 开平标志
    OrderType orderType;            ///< 订单类型
    HedgeFlag hedgeFlag;            ///< 投机套保标志
    OrderStatus status;             ///< 订单状态
    
    double price = 0.0;             ///< 委托价格
    int volume = 0;                 ///< 委托数量
    double avgPrice = 0.0;          ///< 成交均价
    int filledVolume = 0;           ///< 已成交数量
    int remainingVolume = 0;        ///< 剩余数量
    
    double commission = 0.0;        ///< 手续费
    double turnover = 0.0;          ///< 成交金额
    
    QDateTime submitTime;           ///< 提交时间
    QDateTime updateTime;           ///< 更新时间
    
    QString brokerId;               ///< 经纪公司代码
    QString investorId;             ///< 投资者代码
    QString userId;                 ///< 用户代码
    
    int frontId = 0;                ///< 前置机ID
    int sessionId = 0;              ///< 会话ID
    int orderRef = 0;               ///< 订单引用
    
    QString errorMsg;               ///< 错误信息
    QString strategyId;             ///< 策略ID
    QString remark;                 ///< 备注
    
    /**
     * @brief 判断订单是否活跃（未完成）
     */
    bool isActive() const {
        return status == OrderStatus::Pending ||
               status == OrderStatus::Submitted ||
               status == OrderStatus::Accepted ||
               status == OrderStatus::PartialFilled;
    }
    
    /**
     * @brief 判断订单是否已完成
     */
    bool isFinished() const {
        return status == OrderStatus::Filled ||
               status == OrderStatus::Cancelled ||
               status == OrderStatus::Rejected;
    }
};

/**
 * @brief 成交记录结构
 */
struct TradeRecord {
    QString tradeId;                ///< 成交ID
    QString orderId;                ///< 订单ID
    QString instrumentId;           ///< 合约代码
    QString exchangeId;             ///< 交易所代码
    
    TradeDirection direction;       ///< 买卖方向
    OpenCloseFlag openClose;        ///< 开平标志
    
    double price = 0.0;             ///< 成交价格
    int volume = 0;                 ///< 成交数量
    double turnover = 0.0;          ///< 成交金额
    double commission = 0.0;        ///< 手续费
    
    QDateTime tradeTime;            ///< 成交时间
    
    QString brokerId;               ///< 经纪公司代码
    QString investorId;             ///< 投资者代码
    QString userId;                 ///< 用户代码
};

/**
 * @brief 持仓信息结构
 */
struct PositionInfo {
    QString instrumentId;           ///< 合约代码
    QString exchangeId;             ///< 交易所代码
    PositionDirection direction;    ///< 持仓方向
    
    int volume = 0;                 ///< 持仓数量
    int frozenVolume = 0;           ///< 冻结数量
    int availableVolume = 0;        ///< 可用数量
    
    double avgPrice = 0.0;          ///< 持仓均价
    double marketPrice = 0.0;       ///< 最新价
    double marketValue = 0.0;       ///< 市值
    
    double profit = 0.0;            ///< 浮动盈亏
    double todayProfit = 0.0;       ///< 今日盈亏
    double realizedProfit = 0.0;    ///< 已实现盈亏
    
    double margin = 0.0;            ///< 占用保证金
    double frozenMargin = 0.0;      ///< 冻结保证金
    double availableMargin = 0.0;   ///< 可用保证金
    
    int todayVolume = 0;            ///< 今仓数量
    int yesterdayVolume = 0;        ///< 昨仓数量
    
    QDateTime updateTime;           ///< 更新时间
    
    QString brokerId;               ///< 经纪公司代码
    QString investorId;             ///< 投资者代码
    
    /**
     * @brief 计算盈亏比例
     */
    double profitRatio() const {
        if (avgPrice <= 0) return 0.0;
        return (marketPrice - avgPrice) / avgPrice * 100.0;
    }
    
    /**
     * @brief 计算可平仓数量
     */
    int closeableVolume() const {
        return volume - frozenVolume;
    }
};

/**
 * @brief 账户资金信息结构
 */
struct AccountInfo {
    QString accountId;              ///< 账户ID
    QString brokerId;               ///< 经纪公司代码
    QString investorId;             ///< 投资者代码
    
    double preBalance = 0.0;        ///< 上日余额
    double balance = 0.0;           ///< 当前余额
    double available = 0.0;         ///< 可用资金
    double withdraw = 0.0;          ///< 可取资金
    
    double margin = 0.0;            ///< 占用保证金
    double frozenMargin = 0.0;      ///< 冻结保证金
    double frozenCommission = 0.0;  ///< 冻结手续费
    
    double commission = 0.0;        ///< 手续费
    double closeProfit = 0.0;       ///< 平仓盈亏
    double positionProfit = 0.0;    ///< 持仓盈亏
    double todayProfit = 0.0;       ///< 今日盈亏
    
    double deposit = 0.0;           ///< 入金
    double withdrawAmount = 0.0;    ///< 出金
    
    double credit = 0.0;            ///< 信用额度
    double collateral = 0.0;        ///< 质押金额
    
    QDateTime updateTime;           ///< 更新时间
    
    /**
     * @brief 计算总资产
     */
    double totalAssets() const {
        return balance + credit + collateral;
    }
    
    /**
     * @brief 计算风险度
     */
    double riskLevel() const {
        if (balance <= 0) return 0.0;
        return margin / balance * 100.0;
    }
    
    /**
     * @brief 计算收益率
     */
    double returnRate() const {
        if (preBalance <= 0) return 0.0;
        return (balance - preBalance) / preBalance * 100.0;
    }
};

/**
 * @brief 条件单结构
 */
struct ConditionOrder {
    QString conditionId;            ///< 条件单ID
    QString instrumentId;           ///< 合约代码
    QString exchangeId;             ///< 交易所代码
    
    ConditionType conditionType;    ///< 条件类型
    double triggerPrice = 0.0;      ///< 触发价格
    QDateTime triggerTime;          ///< 触发时间
    
    OrderRequest orderRequest;      ///< 触发后的订单请求
    
    bool isActive = true;           ///< 是否激活
    bool isTriggered = false;       ///< 是否已触发
    QDateTime createTime;           ///< 创建时间
    QDateTime triggerTimeActual;    ///< 实际触发时间
    QString triggeredOrderId;       ///< 触发后的订单ID
    
    QString remark;                 ///< 备注
};

/**
 * @brief 止损止盈单结构
 */
struct StopLossTakeProfit {
    QString stopId;                 ///< 止损止盈ID
    QString instrumentId;           ///< 合约代码
    PositionDirection direction;    ///< 持仓方向
    
    double stopLossPrice = 0.0;     ///< 止损价格
    double takeProfitPrice = 0.0;   ///< 止盈价格
    
    double trailingStopPercent = 0.0; ///< 移动止损比例
    double trailingStopPrice = 0.0;   ///< 移动止损价格
    
    int volume = 0;                 ///< 数量（0表示全部）
    bool isActive = true;           ///< 是否激活
    
    QDateTime createTime;           ///< 创建时间
    QDateTime updateTime;           ///< 更新时间
};

/**
 * @brief 风控规则结构
 */
struct RiskRule {
    QString ruleId;                 ///< 规则ID
    QString name;                   ///< 规则名称
    QString description;            ///< 规则描述
    
    double maxPositionSize = 0.0;   ///< 最大持仓金额
    int maxPositionCount = 0;       ///< 最大持仓数量
    double maxDailyLoss = 0.0;      ///< 日最大亏损
    double maxSingleLoss = 0.0;     ///< 单笔最大亏损
    double maxLeverage = 0.0;       ///< 最大杠杆
    double maxMarginRatio = 0.0;    ///< 最大保证金比例
    double maxDrawdown = 0.0;       ///< 最大回撤
    
    bool allowNightTrading = true;  ///< 允许夜盘交易
    bool allowReverseTrade = true;  ///< 允许反向交易
    
    bool isEnabled = true;          ///< 是否启用
    QDateTime createTime;           ///< 创建时间
    QDateTime updateTime;           ///< 更新时间
};

/**
 * @brief 交易日志结构
 */
struct TradingLog {
    QString logId;                  ///< 日志ID
    QDateTime time;                 ///< 时间
    QString type;                   ///< 类型（Order/Trade/Position/Risk）
    QString level;                  ///< 级别（Info/Warning/Error）
    QString message;                ///< 消息内容
    QString instrumentId;           ///< 合约代码（可选）
    QString orderId;                ///< 订单ID（可选）
    QString tradeId;                ///< 成交ID（可选）
    QVariantMap details;            ///< 详细信息
};

// ============================================================================
// 工具函数
// ============================================================================

namespace TradingUtils {
    /**
     * @brief 交易方向转字符串
     */
    inline QString directionToString(TradeDirection dir) {
        switch (dir) {
            case TradeDirection::Buy: return QStringLiteral("买入");
            case TradeDirection::Sell: return QStringLiteral("卖出");
            case TradeDirection::BuyClose: return QStringLiteral("平空");
            case TradeDirection::SellClose: return QStringLiteral("平多");
            default: return QStringLiteral("未知");
        }
    }
    
    /**
     * @brief 订单状态转字符串
     */
    inline QString statusToString(OrderStatus status) {
        switch (status) {
            case OrderStatus::Pending: return QStringLiteral("待提交");
            case OrderStatus::Submitted: return QStringLiteral("已提交");
            case OrderStatus::Accepted: return QStringLiteral("已接受");
            case OrderStatus::Rejected: return QStringLiteral("已拒绝");
            case OrderStatus::PartialFilled: return QStringLiteral("部分成交");
            case OrderStatus::Filled: return QStringLiteral("全部成交");
            case OrderStatus::Cancelled: return QStringLiteral("已撤销");
            default: return QStringLiteral("未知");
        }
    }
    
    /**
     * @brief 开平标志转字符串
     */
    inline QString openCloseToString(OpenCloseFlag flag) {
        switch (flag) {
            case OpenCloseFlag::Open: return QStringLiteral("开仓");
            case OpenCloseFlag::Close: return QStringLiteral("平仓");
            case OpenCloseFlag::CloseToday: return QStringLiteral("平今");
            case OpenCloseFlag::CloseYesterday: return QStringLiteral("平昨");
            default: return QStringLiteral("未知");
        }
    }
    
    /**
     * @brief 持仓方向转字符串
     */
    inline QString positionDirToString(PositionDirection dir) {
        switch (dir) {
            case PositionDirection::Long: return QStringLiteral("多");
            case PositionDirection::Short: return QStringLiteral("空");
            case PositionDirection::Net: return QStringLiteral("净");
            default: return QStringLiteral("未知");
        }
    }
    
    /**
     * @brief 生成唯一请求ID
     */
    inline QString generateRequestId() {
        return QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz") + 
               QString::number(QRandomGenerator::global()->bounded(10000));
    }
}

// ============================================================================
// Q_DECLARE_METATYPE（仅声明新增类型）
// ============================================================================

Q_DECLARE_METATYPE(OrderRequest)
Q_DECLARE_METATYPE(OrderInfo)
Q_DECLARE_METATYPE(TradeRecord)
Q_DECLARE_METATYPE(PositionInfo)
Q_DECLARE_METATYPE(AccountInfo)
Q_DECLARE_METATYPE(ConditionOrder)
Q_DECLARE_METATYPE(StopLossTakeProfit)
Q_DECLARE_METATYPE(RiskRule)
Q_DECLARE_METATYPE(TradingLog)

Q_DECLARE_METATYPE(OrderType)
Q_DECLARE_METATYPE(PositionDirection)
Q_DECLARE_METATYPE(OpenCloseFlag)
Q_DECLARE_METATYPE(HedgeFlag)
Q_DECLARE_METATYPE(ConditionType)
Q_DECLARE_METATYPE(TriggerCondition)

#endif // TRADINGTYPES_H
