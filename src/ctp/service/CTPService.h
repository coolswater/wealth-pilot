/////////////////////////////////////////////////////////////////////////
///@file CTPService.h
///@brief CTP客户端服务类 - PIMPL模式封装
///@author CTP Service
///@date 2026-04-01
/////////////////////////////////////////////////////////////////////////

#ifndef CTPService_H
#define CTPService_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <memory>
#include <functional>
#include <optional>
#include "../config/CTPConfigManager.h"

// 前向声明CTP结构体（避免暴露头文件细节）
struct CThostFtdcDepthMarketDataField;
struct CThostFtdcOrderField;
struct CThostFtdcTradeField;
struct CThostFtdcRspUserLoginField;
struct CThostFtdcRspInfoField;

namespace CTP {

// C++17 强类型别名
using InstrumentID = QString;
using OrderRef = QString;
using Price = double;
using Volume = int;

/**
 * @brief 行情数据结构（Qt友好封装）
 */
struct MarketData {
    QDateTime TradingDay;               ///< 交易日
    QString ExchangeID;                 ///< 交易所代码
    InstrumentID InstrumentID;          ///< 合约代码
    QDateTime UpdateTime;               ///< 更新时间
    Price lastPrice{0.0};              ///< 最新价
    Price BidPrice1{0.0};              ///< 买一价
    Volume BidVolume1{0};              ///< 买一量
    Price AskPrice1{0.0};              ///< 卖一价
    Volume AskVolume1{0};              ///< 卖一量
    Volume Volume{0};                  ///< 成交量
    Price OpenInterest{0.0};           ///< 持仓量
    Price preSettlementPrice{0.0};     ///< 昨结算
    Price PreClosePrice{0.0};           ///< 昨收盘价
    Price PreOpenInterest{0.0};         ///< 昨持仓量
    Price OpenPrice{0.0};               ///< 开盘价
    Price HighestPrice{0.0};            ///< 最高价
    Price LowestPrice{0.0};            ///< 最低价
    Price Turnover{0.0};                ///< 成交额
    Price ClosePrice{0.0};              ///< 收盘价
    Price SettlementPrice{0.0};         ///< 结算价
    Price UpperLimitPrice{0.0};         ///< 涨停板价
    Price LowerLimitPrice{0.0};         ///< 跌停板价
    Price AveragePrice{0.0};            ///< 均价
};

/**
 * @brief 订单状态枚举
 */
enum class OrderStatus {
    Unknown,
    AllTraded,              ///< 全部成交
    PartTradedQueueing,     ///< 部分成交还在队列中
    PartTradedNotQueueing,  ///< 部分成交不在队列中
    NoTradeQueueing,        ///< 未成交还在队列中
    NoTradeNotQueueing,     ///< 未成交不在队列中
    Canceled                ///< 已撤销
};

/**
 * @brief 委托方向
 */
enum class Direction {
    Buy,    ///< 买入
    Sell    ///< 卖出
};

/**
 * @brief 开平标志
 */
enum class OffsetFlag {
    Open,           ///< 开仓
    Close,          ///< 平仓
    CloseToday,     ///< 平今
    CloseYesterday  ///< 平昨
};

/**
 * @brief 订单信息结构
 */
struct OrderInfo {
    InstrumentID instrumentId;      ///< 合约代码
    OrderRef orderRef;              ///< 报单引用
    Direction direction;            ///< 方向
    OffsetFlag offset;              ///< 开平标志
    Price price{0.0};              ///< 价格
    Volume totalVolume{0};         ///< 总数量
    Volume tradedVolume{0};        ///< 已成交数量
    OrderStatus status{OrderStatus::Unknown};  ///< 状态
    QString statusMsg;              ///< 状态消息
    QDateTime insertTime;           ///< 委托时间
};

/**
 * @brief 成交信息结构
 */
struct TradeInfo {
    InstrumentID instrumentId;      ///< 合约代码
    OrderRef orderRef;              ///< 报单引用
    QString tradeId;                ///< 成交编号
    Direction direction;            ///< 方向
    OffsetFlag offset;              ///< 开平标志
    Price price{0.0};              ///< 成交价格
    Volume volume{0};              ///< 成交数量
    QDateTime tradeTime;            ///< 成交时间
};

/**
 * @brief CTP客户端服务类 - PIMPL实现
 * @details 封装原生CTP API实现细节，提供线程安全的Qt接口
 */
class CTPService : public QObject {
    Q_OBJECT
public:
    explicit CTPService(QObject *parent = nullptr);
    ~CTPService() override;

    // 禁止复制（保证资源唯一性）
    CTPService(const CTPService&) = delete;
    CTPService& operator=(const CTPService&) = delete;
    CTPService(CTPService&&) = delete;
    CTPService& operator=(CTPService&&) = delete;

    /////////////////////////////////////////////////////////////////////////
    /// 配置接口
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 设置行情前置地址
     * @param frontAddr 行情前置地址，如 "tcp://180.168.146.187:10131"
     */
    void setMarketFrontAddress(const QString& frontAddr);

    /**
     * @brief 设置交易前置地址
     * @param frontAddr 交易前置地址，如 "tcp://180.168.146.187:10101"
     */
    void setTradingFrontAddress(const QString& frontAddr);

    /**
     * @brief 设置认证信息
     * @param brokerId 经纪商代码
     * @param userId 用户代码
     * @param password 密码
     * @param appId 应用代码（CTP6.6.1+需要）
     * @param authCode 认证码（CTP6.6.1+需要）
     */
    void setCredentials(const QString& brokerId, const QString& userId,
                        const QString& password, const QString& appId = "",
                        const QString& authCode = "");

    /////////////////////////////////////////////////////////////////////////
    /// 经纪商配置管理
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 加载配置并连接（使用配置管理器参数）
     * @param brokerId 经纪商ID，为空时使用当前选中的服务器
     * @return 是否成功开始连接
     */
    bool loadConfigAndConnect(const QString& brokerId = QString());

    /**
     * @brief 切换经纪商（断开并重连）
     * @param brokerId 新的经纪商ID
     * @return 是否成功切换
     */
    bool switchBroker(const QString& brokerId);

    /**
     * @brief 获取当前经纪商配置
     */
    std::optional<CTPBrokerConfig> currentBrokerConfig() const;

    /**
     * @brief 获取当前经纪商ID
     */
    QString currentBrokerId() const;

    /////////////////////////////////////////////////////////////////////////
    /// 连接管理
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 开始建立连接（行情和交易，异步完成）
     * @details 完成后会发送 loginFinished 信号
     */
    void setupConnections();

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 是否已登录
     */
    bool isLoggedIn() const;

    /**
     * @brief 获取当前交易日
     */
    QString tradingDay() const;

    /////////////////////////////////////////////////////////////////////////
    /// 行情接口（支持批量订阅优化）
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 订阅行情（支持批量，内部有缓冲优化）
     * @param instruments 合约代码列表，如 ["cu2505", "ag2506"]
     * @param useBuffer 是否使用缓冲区（默认true），减少CPU占用
     */
    void subscribeMarketData(const QList<InstrumentID>& instruments, bool useBuffer = true);

    /**
     * @brief 取消订阅行情
     */
    void unsubscribeMarketData(const QList<InstrumentID>& instruments);

    /////////////////////////////////////////////////////////////////////////
    /// 交易接口
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 发送委托单（异步）
     * @param order 订单信息
     * @return 订单引用，用于后续跟踪
     */
    std::optional<OrderRef> insertOrder(const OrderInfo& order);

    /**
     * @brief 撤销委托单
     * @param orderRef 报单引用
     */
    void cancelOrder(const OrderRef& orderRef);

    /**
     * @brief 查询账户资金（异步，结果通过 signal 返回）
     */
    void queryTradingAccount();

    /**
     * @brief 查询持仓（异步）
     */
    void queryPositions();

    /**
     * @brief 查询合约列表（异步）
     * @param exchangeId 交易所代码，为空查询所有交易所
     */
    void queryInstruments(const QString& exchangeId = QString());

    /**
     * @brief 确认结算单（查询合约前需要）
     */
    void confirmSettlement();

signals:
    /////////////////////////////////////////////////////////////////////////
    /// 连接状态信号
    /////////////////////////////////////////////////////////////////////////

    void marketConnected();                 ///< 行情前置连接成功
    void marketDisconnected(int reason);    ///< 行情前置断开
    void tradingConnected();                ///< 交易前置连接成功
    void tradingDisconnected(int reason);   ///< 交易前置断开
    void marketLoginFinished(bool success, const QString& errorMsg);  ///< 行情登录结果
    void tradingLoginFinished(bool success, const QString& errorMsg); ///< 交易登录结果
    void loginFinished(bool success, const QString& errorMsg);  ///< 综合结果信号（两个登录都完成）
    void heartbeatWarning(int timeLapse);   ///< 心跳超时警告

    /////////////////////////////////////////////////////////////////////////
    /// 行情数据信号（支持批量推送优化）
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 行情数据批量到达（高频场景优化）
     * @param dataList 行情数据列表（批量处理）
     */
    void marketDataBatchReceived(const QList<MarketData>& dataList);

    /**
     * @brief 行情数据到达（低频场景，逐条推送）
     */
    void marketDataReceived(const MarketData& data);

    /////////////////////////////////////////////////////////////////////////
    /// 交易相关信号
    /////////////////////////////////////////////////////////////////////////

    void orderUpdated(const OrderInfo& order);      ///< 订单状态更新
    void tradeReceived(const TradeInfo& trade);     ///< 成交回报
    void positionReceived(const QString& instrument, int longPos, int shortPos);
    void accountInfoReceived(double available, double balance);

    /////////////////////////////////////////////////////////////////////////
    /// 合约查询信号
    /////////////////////////////////////////////////////////////////////////

    void instrumentQueried(const QString& instrumentId, const QString& exchangeId,
                           const QString& instrumentName, double priceTick, int volumeMultiple);
    void instrumentQueryFinished(int totalCount);
    void settlementConfirmed(bool success, const QString& msg);

    /////////////////////////////////////////////////////////////////////////
    /// 错误信号
    /////////////////////////////////////////////////////////////////////////

    void errorOccurred(int requestId, int errorId, const QString& errorMsg);

private:
    // PIMPL实现指针
    class Impl;
    std::unique_ptr<Impl> d;  ///< C++17 unique_ptr支持不完整类型

    // 内部方法初始化（C++17 if constexpr优化）
    void initializeSpi();
};

} // namespace CTP

#endif // CTPService_H