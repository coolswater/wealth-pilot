/**
 * @file CTPService.h
 * @brief CTP期货行情和交易服务 - Simnow模拟交易平台对接
 *
 * 功能：
 * - 连接Simnow行情和交易服务器
 * - 订阅期货行情
 * - 模拟交易（下单、撤单）
 * - 持仓和资金查询
 *
 * Simnow环境配置：
 * - 行情前置: tcp://180.168.146.187:10131
 * - 交易前置: tcp://180.168.146.187:10130
 * - 经纪商代码: 9999
 */
#pragma once

#include <QObject>
#include <QTimer>
#include <QMap>
#include <memory>

// 期货行情数据结构
struct FuturesQuote {
    QString instrumentId;      // 合约代码
    QString exchangeId;        // 交易所代码
    double lastPrice = 0;      // 最新价
    double preSettlementPrice = 0;  // 昨结算价
    double preClosePrice = 0;  // 昨收盘价
    double openPrice = 0;      // 开盘价
    double highestPrice = 0;   // 最高价
    double lowestPrice = 0;    // 最低价
    int volume = 0;            // 成交量
    double turnover = 0;       // 成交金额
    int openInterest = 0;      // 持仓量
    double bidPrice1 = 0;      // 买一价
    int bidVolume1 = 0;        // 买一量
    double askPrice1 = 0;      // 卖一价
    int askVolume1 = 0;        // 卖一量
    QString updateTime;        // 更新时间
    int updateMillisec = 0;    // 更新毫秒
};

// 订单请求结构
struct OrderRequest {
    QString instrumentId;
    char direction;            // '0'=买, '1'=卖
    char offsetFlag;           // '0'=开仓, '1'=平仓
    double price;
    int volume;
    char priceType = '2';      // '1'=任意价, '2'=限价
    char timeCondition = '3';  // '1'=立即完成，否则撤销, '3'=当日有效
};

// 订单响应结构
struct OrderResponse {
    QString orderId;
    QString instrumentId;
    int status;                // 0=未知, 1=未成交, 2=部分成交, 3=全部成交, 4=已撤销
    double price;
    int volume;
    int tradedVolume;
    QString insertTime;
    QString errorMsg;
};

// Simnow配置
struct SimnowConfig {
    QString marketFront = "tcp://180.168.146.187:10131";  // 行情前置
    QString tradeFront = "tcp://180.168.146.187:10130";   // 交易前置
    QString brokerId = "9999";                            // 经纪商代码
    QString userId;                                       // 用户ID
    QString password;                                     // 密码
    QString authCode;                                     // 认证码（如果需要）
    QString appId = "simnow_client_test";                 // 应用ID
};

class CTPService : public QObject
{
    Q_OBJECT
public:
    static CTPService* instance();

    bool initialize();
    void shutdown();

    // 配置
    void setConfig(const SimnowConfig& config);
    SimnowConfig config() const;

    // 连接管理
    bool connectMarket();
    bool connectTrade();
    void disconnect();
    bool isMarketConnected() const;
    bool isTradeConnected() const;

    // 登录
    bool login(const QString& userId, const QString& password);
    void logout();
    bool isLoggedIn() const;

    // 行情订阅
    void subscribeMarketData(const QStringList& instruments);
    void unsubscribeMarketData(const QStringList& instruments);
    QStringList subscribedInstruments() const;

    // 交易接口
    QString sendOrder(const OrderRequest& request);
    bool cancelOrder(const QString& orderId);
    void queryPositions();
    void queryAccount();
    void queryOrders();
    void queryTrades();

    // 行情数据获取
    FuturesQuote getQuote(const QString& instrumentId) const;
    QMap<QString, FuturesQuote> getAllQuotes() const;

signals:
    void marketConnected();
    void marketDisconnected();
    void tradeConnected();
    void tradeDisconnected();
    void loggedIn();
    void loggedOut();
    void errorOccurred(const QString& error);

    void marketDataReceived(const FuturesQuote& quote);
    void orderResponseReceived(const OrderResponse& response);
    void positionUpdated(const QString& instrumentId, int position, double avgPrice);
    void accountUpdated(double balance, double available, double margin);

private slots:
    void onReconnectTimer();
    void onHeartbeatTimer();

private:
    explicit CTPService(QObject* parent = nullptr);
    ~CTPService();

    // CTP回调处理（在实际实现中会连接到CTP API）
    void onMarketData(const FuturesQuote& quote);
    void onOrderResponse(const OrderResponse& response);
    void onTradeResponse(const QString& orderId, int tradedVolume, double tradedPrice);
    void onError(const QString& error);

    class Impl;
    std::unique_ptr<Impl> d;

    static CTPService* s_instance;
};
