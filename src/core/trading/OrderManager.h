/**
 * @file OrderManager.h
 * @brief 订单管理器 - 统一管理所有订单的生命周期
 *
 * @details 功能：
 * - 订单提交、撤销、修改
 * - 订单状态跟踪
 * - 成交记录管理
 * - 条件单管理
 * - 止损止盈管理
 *
 * @details 性能优化：
 * - 使用哈希表快速查找订单
 * - 信号驱动更新，避免轮询
 * - 异步下单，不阻塞UI
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ORDERMANAGER_H
#define ORDERMANAGER_H

#include <QObject>
#include <QHash>
#include <QMap>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include <memory>
#include "TradingTypes.h"

// 前向声明
class CTPTradingSpi;

/**
 * @brief 订单管理器
 * @details 单例模式，统一管理所有订单
 * 
 * @example
 * @code
 * // 提交订单
 * OrderRequest request;
 * request.instrumentId = "IF2501";
 * request.direction = TradeDirection::Buy;
 * request.volume = 1;
 * request.price = 3850.0;
 * 
 * QString orderId = OrderManager::instance().submitOrder(request);
 * 
 * // 撤销订单
 * OrderManager::instance().cancelOrder(orderId);
 * @endcode
 */
class OrderManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static OrderManager& instance();

    /**
     * @brief 初始化订单管理器
     * @param tradingSpi CTP交易SPI指针
     * @return 是否成功
     */
    bool initialize(CTPTradingSpi* tradingSpi);

    /**
     * @brief 关闭订单管理器
     */
    void shutdown();

    // ========== 订单操作 ==========

    /**
     * @brief 提交订单
     * @param request 订单请求
     * @return 订单ID，失败返回空字符串
     */
    QString submitOrder(const OrderRequest& request);

    /**
     * @brief 撤销订单
     * @param orderId 订单ID
     * @return 是否成功提交撤单请求
     */
    bool cancelOrder(const QString& orderId);

    /**
     * @brief 批量撤销订单
     * @param orderIds 订单ID列表
     * @return 成功撤销的数量
     */
    int cancelOrders(const QStringList& orderIds);

    /**
     * @brief 修改订单
     * @param orderId 订单ID
     * @param newPrice 新价格
     * @param newVolume 新数量
     * @return 是否成功
     */
    bool modifyOrder(const QString& orderId, double newPrice, int newVolume);

    // ========== 条件单操作 ==========

    /**
     * @brief 添加条件单
     * @param condition 条件单信息
     * @return 条件单ID
     */
    QString addConditionOrder(const ConditionOrder& condition);

    /**
     * @brief 删除条件单
     * @param conditionId 条件单ID
     * @return 是否成功
     */
    bool removeConditionOrder(const QString& conditionId);

    /**
     * @brief 激活/停用条件单
     * @param conditionId 条件单ID
     * @param active 是否激活
     */
    void setConditionOrderActive(const QString& conditionId, bool active);

    // ========== 止损止盈操作 ==========

    /**
     * @brief 设置止损止盈
     * @param stop 止损止盈信息
     * @return 止损止盈ID
     */
    QString setStopLossTakeProfit(const StopLossTakeProfit& stop);

    /**
     * @brief 更新止损止盈
     * @param stopId 止损止盈ID
     * @param stop 新的止损止盈信息
     * @return 是否成功
     */
    bool updateStopLossTakeProfit(const QString& stopId, const StopLossTakeProfit& stop);

    /**
     * @brief 删除止损止盈
     * @param stopId 止损止盈ID
     * @return 是否成功
     */
    bool removeStopLossTakeProfit(const QString& stopId);

    // ========== 查询接口 ==========

    /**
     * @brief 获取订单信息
     * @param orderId 订单ID
     * @return 订单信息，不存在返回空
     */
    std::optional<OrderInfo> getOrder(const QString& orderId) const;

    /**
     * @brief 获取所有活跃订单
     * @return 活跃订单列表
     */
    QVector<OrderInfo> getActiveOrders() const;

    /**
     * @brief 获取指定合约的活跃订单
     * @param instrumentId 合约代码
     * @return 活跃订单列表
     */
    QVector<OrderInfo> getActiveOrders(const QString& instrumentId) const;

    /**
     * @brief 获取历史订单
     * @param from 开始时间
     * @param to 结束时间
     * @return 历史订单列表
     */
    QVector<OrderInfo> getHistoryOrders(const QDateTime& from, const QDateTime& to) const;

    /**
     * @brief 获取成交记录
     * @param orderId 订单ID（可选，为空返回所有）
     * @return 成交记录列表
     */
    QVector<TradeRecord> getTradeRecords(const QString& orderId = QString()) const;

    /**
     * @brief 获取条件单列表
     * @return 条件单列表
     */
    QVector<ConditionOrder> getConditionOrders() const;

    /**
     * @brief 获取止损止盈列表
     * @param instrumentId 合约代码（可选）
     * @return 止损止盈列表
     */
    QVector<StopLossTakeProfit> getStopLossTakeProfits(const QString& instrumentId = QString()) const;

    /**
     * @brief 更新订单的CTP引用ID
     * @param orderId 内部订单ID
     * @param ctpOrderId CTP返回的订单引用
     */
    void updateOrderId(const QString& orderId, const QString& ctpOrderId);

    /**
     * @brief 更新订单状态
     * @param orderId 订单ID
     * @param status 新状态
     * @param filledVolume 已成交数量
     */
    void updateOrderStatus(const QString& orderId, OrderStatus status, int filledVolume = 0);

    /**
     * @brief 记录成交
     * @param trade 成交记录
     */
    void recordTrade(const TradeRecord& trade);

    /**
     * @brief 添加止损止盈规则
     * @param sltp 止损止盈信息
     */
    void addStopLossTakeProfit(const StopLossTakeProfit& sltp);

    /**
     * @brief 获取订单统计
     */
    struct OrderStats {
        int totalOrders = 0;         ///< 总订单数
        int activeOrders = 0;        ///< 活跃订单数
        int filledOrders = 0;        ///< 已成交订单数
        int cancelledOrders = 0;     ///< 已撤销订单数
        int rejectedOrders = 0;      ///< 已拒绝订单数
        double totalTurnover = 0.0;  ///< 总成交金额
        double totalCommission = 0.0; ///< 总手续费
    };
    OrderStats getStats() const;

signals:
    /**
     * @brief 订单已提交
     * @param orderId 订单ID
     */
    void orderSubmitted(const QString& orderId);

    /**
     * @brief 订单状态更新
     * @param order 订单信息
     */
    void orderUpdated(const OrderInfo& order);

    /**
     * @brief 订单已成交
     * @param orderId 订单ID
     * @param trade 成交记录
     */
    void orderFilled(const QString& orderId, const TradeRecord& trade);

    /**
     * @brief 订单已撤销
     * @param orderId 订单ID
     */
    void orderCancelled(const QString& orderId);

    /**
     * @brief 订单被拒绝
     * @param orderId 订单ID
     * @param reason 拒绝原因
     */
    void orderRejected(const QString& orderId, const QString& reason);

    /**
     * @brief 条件单触发
     * @param conditionId 条件单ID
     * @param orderId 触发后的订单ID
     */
    void conditionTriggered(const QString& conditionId, const QString& orderId);

    /**
     * @brief 止损止盈触发
     * @param stopId 止损止盈ID
     * @param orderId 触发后的订单ID
     */
    void stopLossTakeProfitTriggered(const QString& stopId, const QString& orderId);

    /**
     * @brief 错误发生
     * @param errorCode 错误码
     * @param errorMsg 错误信息
     */
    void errorOccurred(int errorCode, const QString& errorMsg);

public slots:
    /**
     * @brief 处理行情更新（用于条件单检查）
     * @param instrumentId 合约代码
     * @param lastPrice 最新价
     */
    void onMarketDataUpdated(const QString& instrumentId, double lastPrice);

private slots:
    /**
     * @brief 处理CTP订单回报
     */
    void onCtpOrderReturn(const OrderInfo& order);

    /**
     * @brief 处理CTP成交回报
     */
    void onCtpTradeReturn(const TradeRecord& trade);

    /**
     * @brief 处理CTP报单错误
     */
    void onCtpOrderError(const QString& requestId, int errorCode, const QString& errorMsg);

    /**
     * @brief 检查条件单
     */
    void checkConditionOrders();

    /**
     * @brief 检查止损止盈
     */
    void checkStopLossTakeProfit();

private:
    // 私有构造函数（单例）
    OrderManager(QObject* parent = nullptr);
    ~OrderManager() override;
    Q_DISABLE_COPY(OrderManager)

    // 内部方法
    QString generateOrderId();
    void updateOrderCache(const OrderInfo& order);
    void addTradeRecord(const TradeRecord& trade);
    void checkConditionOrder(const ConditionOrder& condition, double price);
    void checkStopLossTakeProfit(const StopLossTakeProfit& stop, double price);

    // PIMPL 实现
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // ORDERMANAGER_H
