/**
 * @file ICTPPlugin.h
 * @brief CTP插件接口 - 定义CTP服务的插件接口
 *
 * @details 功能：
 * - 行情订阅
 * - 交易接口
 * - 账户查询
 * - 订单管理
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef ICTPPLUGIN_H
#define ICTPPLUGIN_H

#include "IPlugin.h"
#include <QString>
#include <QMap>
#include <QDateTime>

/**
 * @brief 行情数据结构
 */
struct MarketData {
    QString instrumentId;       // 合约代码
    QString exchangeId;         // 交易所代码
    double lastPrice;           // 最新价
    double bidPrice1;           // 买一价
    int bidVolume1;             // 买一量
    double askPrice1;           // 卖一价
    int askVolume1;             // 卖一量
    double openPrice;           // 开盘价
    double highestPrice;        // 最高价
    double lowestPrice;         // 最低价
    int volume;                 // 成交量
    double turnover;            // 成交金额
    double openInterest;        // 持仓量
    double upperLimitPrice;     // 涨停价
    double lowerLimitPrice;     // 跌停价
    QDateTime updateTime;       // 更新时间
};

/**
 * @brief 订单数据结构
 */
struct OrderData {
    QString orderId;            // 订单ID
    QString instrumentId;       // 合约代码
    QString direction;          // 方向（买/卖）
    QString offsetFlag;         // 开平标志
    double price;               // 价格
    int volume;                 // 数量
    int volumeTraded;           // 已成交数量
    QString status;             // 状态
    QDateTime insertTime;       // 委托时间
};

/**
 * @brief 账户数据结构
 */
struct AccountData {
    QString accountId;          // 账户ID
    double available;           // 可用资金
    double balance;             // 总资产
    double margin;              // 保证金
    double profit;              // 浮动盈亏
    double closeProfit;         // 平仓盈亏
};

/**
 * @brief CTP插件接口
 */
class ICTPPlugin : public IPlugin
{
    Q_OBJECT

public:
    virtual ~ICTPPlugin() = default;

    // ========== 连接管理 ==========

    /**
     * @brief 连接到CTP服务器
     */
    virtual bool connect(const QString& brokerId, 
                        const QString& userId, 
                        const QString& password,
                        const QString& marketFront,
                        const QString& tradeFront) = 0;

    /**
     * @brief 断开连接
     */
    virtual void disconnect() = 0;

    /**
     * @brief 是否已连接
     */
    virtual bool isConnected() const = 0;

    // ========== 行情接口 ==========

    /**
     * @brief 订阅行情
     */
    virtual bool subscribeMarketData(const QStringList& instruments) = 0;

    /**
     * @brief 取消订阅
     */
    virtual void unsubscribeMarketData(const QStringList& instruments) = 0;

    /**
     * @brief 获取最新行情
     */
    virtual MarketData getMarketData(const QString& instrumentId) const = 0;

    /**
     * @brief 获取所有行情
     */
    virtual QMap<QString, MarketData> getAllMarketData() const = 0;

    // ========== 交易接口 ==========

    /**
     * @brief 下单
     */
    virtual QString sendOrder(const QString& instrumentId,
                             const QString& direction,
                             const QString& offsetFlag,
                             double price,
                             int volume) = 0;

    /**
     * @brief 撤单
     */
    virtual bool cancelOrder(const QString& orderId) = 0;

    /**
     * @brief 查询订单
     */
    virtual QList<OrderData> queryOrders() = 0;

    /**
     * @brief 查询成交
     */
    virtual QList<OrderData> queryTrades() = 0;

    // ========== 账户接口 ==========

    /**
     * @brief 查询账户
     */
    virtual AccountData queryAccount() = 0;

    /**
     * @brief 查询持仓
     */
    virtual QList<AccountData> queryPositions() = 0;

signals:
    /**
     * @brief 连接成功信号
     */
    void connected();

    /**
     * @brief 连接断开信号
     */
    void disconnected();

    /**
     * @brief 行情更新信号
     */
    void marketDataUpdated(const MarketData& data);

    /**
     * @brief 订单状态更新信号
     */
    void orderUpdated(const OrderData& order);

    /**
     * @brief 账户更新信号
     */
    void accountUpdated(const AccountData& account);
};

Q_DECLARE_INTERFACE(ICTPPlugin, "com.wealthpilot.ICTPPlugin/2.0")

#endif // ICTPPLUGIN_H
