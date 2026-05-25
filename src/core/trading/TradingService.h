/**
 * @file TradingService.h
 * @brief 交易服务 - 统一交易系统入口
 *
 * @details 功能：
 * - 整合订单管理、持仓管理、风控系统
 * - 对接 CTP 交易接口
 * - 提供统一的交易 API
 * - 管理交易生命周期
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef TRADINGSERVICE_H
#define TRADINGSERVICE_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <memory>
#include "TradingTypes.h"
#include "RiskController.h"

// 包含 CTP 服务头文件
#include "infrastructure/ctp/service/CTPService.h"

/**
 * @brief 交易服务 - 统一交易系统入口
 * @details 单例模式，整合所有交易相关组件
 * 
 * @example
 * @code
 * // 初始化
 * TradingService::instance().initialize();
 * 
 * // 下单
 * OrderRequest request;
 * request.instrumentId = "cu2505";
 * request.direction = TradeDirection::Buy;
 * request.price = 75000.0;
 * request.volume = 1;
 * 
 * QString orderId = TradingService::instance().submitOrder(request);
 * @endcode
 */
class TradingService : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static TradingService& instance();

    /**
     * @brief 初始化交易服务
     * @return 是否成功
     */
    bool initialize();

    /**
     * @brief 关闭交易服务
     */
    void shutdown();

    /**
     * @brief 设置 CTP 服务
     * @param ctpService CTP服务实例
     */
    void setCtpService(CTP::CTPService* ctpService);

    // ========== 交易操作 ==========

    /**
     * @brief 提交订单
     * @param request 订单请求
     * @return 订单ID，失败返回空
     */
    QString submitOrder(const OrderRequest& request);

    /**
     * @brief 撤销订单
     * @param orderId 订单ID
     * @return 是否成功
     */
    bool cancelOrder(const QString& orderId);

    /**
     * @brief 批量撤销订单
     * @param orderIds 订单ID列表
     * @return 成功撤销的数量
     */
    int cancelOrders(const QVector<QString>& orderIds);

    /**
     * @brief 设置止损止盈
     * @param instrumentId 合约代码
     * @param stopLoss 止损价
     * @param takeProfit 止盈价
     * @return 是否成功
     */
    bool setStopLossTakeProfit(const QString& instrumentId, 
                                double stopLoss, 
                                double takeProfit);

    /**
     * @brief 设置条件单
     * @param condition 条件单信息
     * @return 条件单ID
     */
    QString setConditionOrder(const ConditionOrder& condition);

    // ========== 查询接口 ==========

    /**
     * @brief 获取订单
     * @param orderId 订单ID
     */
    std::optional<OrderInfo> getOrder(const QString& orderId) const;

    /**
     * @brief 获取所有活动订单
     */
    QVector<OrderInfo> getActiveOrders() const;

    /**
     * @brief 获取持仓
     * @param instrumentId 合约代码
     * @param direction 持仓方向
     */
    std::optional<PositionInfo> getPosition(const QString& instrumentId, 
                                            PositionDirection direction) const;

    /**
     * @brief 获取所有持仓
     */
    QVector<PositionInfo> getPositions() const;

    /**
     * @brief 获取账户信息
     */
    AccountInfo getAccountInfo() const;

    /**
     * @brief 获取总盈亏
     */
    double getTotalProfit() const;

    /**
     * @brief 获取风险等级
     */
    int getRiskLevel() const;

    // ========== 风控接口 ==========

    /**
     * @brief 检查订单是否通过风控
     * @param request 订单请求
     * @return 检查结果
     */
    RiskCheckResult checkOrder(const OrderRequest& request);

    /**
     * @brief 获取风险报告
     */
    struct RiskReport {
        int riskLevel = 0;
        double totalRisk = 0.0;
        QVector<QString> warnings;
        QVector<QString> suggestions;
    };
    RiskReport getRiskReport() const;

signals:
    /**
     * @brief 订单已提交
     */
    void orderSubmitted(const QString& orderId);

    /**
     * @brief 订单已接受
     */
    void orderAccepted(const QString& orderId);

    /**
     * @brief 订单已拒绝
     */
    void orderRejected(const QString& orderId, const QString& reason);

    /**
     * @brief 订单已成交
     */
    void orderFilled(const QString& orderId, const TradeRecord& trade);

    /**
     * @brief 订单已撤销
     */
    void orderCancelled(const QString& orderId);

    /**
     * @brief 持仓已更新
     */
    void positionUpdated(const PositionInfo& position);

    /**
     * @brief 盈亏已更新
     */
    void profitUpdated(double totalProfit);

    /**
     * @brief 风险等级变化
     */
    void riskLevelChanged(int level);

    /**
     * @brief 风控警告
     */
    void riskWarning(const QString& warning);

    /**
     * @brief 交易日志
     */
    void tradingLog(const QString& message, int level);

private slots:
    /**
     * @brief 处理 CTP 订单更新
     */
    void onCtpOrderUpdated(const CTP::OrderInfo& order);

    /**
     * @brief 处理 CTP 成交回报
     */
    void onCtpTradeReceived(const CTP::TradeInfo& trade);

    /**
     * @brief 处理 CTP 账户信息
     */
    void onCtpAccountInfo(double available, double balance);

    /**
     * @brief 处理 CTP 持仓信息
     */
    void onCtpPositionReceived(const QString& instrument, int longPos, int shortPos);

private:
    // 私有构造函数（单例）
    TradingService(QObject* parent = nullptr);
    ~TradingService() override;
    Q_DISABLE_COPY(TradingService)

    // 内部方法
    void connectSignals();
    void disconnectSignals();
    OrderRequest convertToOrderRequest(const CTP::OrderInfo& ctpOrder);
    PositionInfo convertToPositionInfo(const QString& instrument, int longPos, int shortPos);
    void updatePositionFromTrade(const TradeRecord& trade);

    // PIMPL 实现
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // TRADINGSERVICE_H
