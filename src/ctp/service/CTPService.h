/////////////////////////////////////////////////////////////////////////
///@file CTPService.h
///@brief CTP客户端对外接�?- PIMPL模式封装
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

// C++17 强类型定�?
using InstrumentID = QString;
using OrderRef = QString;
using Price = double;
using Volume = int;

/**
 * @brief 行情数据结构（Qt友好封装�?
 */
struct MarketData {
    QDateTime TradingDay;               // 交易�?
    QString ExchangeID;                 // 交易所代码
    InstrumentID InstrumentID;          // 合约代码
    QDateTime UpdateTime;               // 更新时间
    Price lastPrice{0.0};              // 最新价
    Price BidPrice1{0.0};              // 买一�?
    Volume BidVolume1{0};              // 买一�?
    Price AskPrice1{0.0};              // 卖一�?
    Volume AskVolume1{0};              // 卖一�?
    Volume Volume{0};                  // 成交�?
    Price OpenInterest{0.0};           // 持仓�?
    Price preSettlementPrice{0.0};     // 昨结�?
    Price PreClosePrice{0.0};           // 昨收盘价
    Price PreOpenInterest{0.0};         // 昨持仓量
    Price OpenPrice{0.0};               // 开盘价
    Price HighestPrice{0.0};            // 最高价
    Price LowestPrice{0.0};            // 最低价
    Price Turnover{0.0};                // 成交�?
    Price ClosePrice{0.0};              // 收盘�?
    Price SettlementPrice{0.0};         // 结算�?
    Price UpperLimitPrice{0.0};         // 涨停板价
    Price LowerLimitPrice{0.0};         // 跌停板价
    Price AveragePrice{0.0};         // 当日均价
};

/**
 * @brief 订单状态枚�?
 */
enum class OrderStatus {
    Unknown,
    AllTraded,              // 全部成交
    PartTradedQueueing,     // 部分成交还在队列�?
    PartTradedNotQueueing,  // 部分成交不在队列�?
    NoTradeQueueing,        // 未成交还在队列中
    NoTradeNotQueueing,     // 未成交不在队列中
    Canceled                // 撤单
};

/**
 * @brief 委托方向
 */
enum class Direction {
    Buy,    // �?
    Sell    // �?
};

/**
 * @brief 开平标�?
 */
enum class OffsetFlag {
    Open,           // 开�?
    Close,          // 平仓
    CloseToday,     // 平今
    CloseYesterday  // 平昨
};

/**
 * @brief 订单信息结构
 */
struct OrderInfo {
    InstrumentID instrumentId;
    OrderRef orderRef;
    Direction direction;
    OffsetFlag offset;
    Price price{0.0};
    Volume totalVolume{0};
    Volume tradedVolume{0};
    OrderStatus status{OrderStatus::Unknown};
    QString statusMsg;
    QDateTime insertTime;
};

/**
 * @brief 成交信息结构
 */
struct TradeInfo {
    InstrumentID instrumentId;
    OrderRef orderRef;
    QString tradeId;
    Direction direction;
    OffsetFlag offset;
    Price price{0.0};
    Volume volume{0};
    QDateTime tradeTime;
};

/**
 * @brief CTP客户端主�?- PIMPL实现
 * @details 对外隐藏CTP API实现细节，提供线程安全的Qt风格接口
 */
class CTPService : public QObject {
    Q_OBJECT
public:
    explicit CTPService(QObject *parent = nullptr);
    ~CTPService() override;

    // 禁止拷贝（资源管理语义）
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
     * @param brokerId 经纪公司代码
     * @param userId 用户代码
     * @param password 密码
     * @param appId 应用代码（CTP6.6.1+需要）
     * @param authCode 认证码（CTP6.6.1+需要）
     */
    void setCredentials(const QString& brokerId, const QString& userId,
                        const QString& password, const QString& appId = "",
                        const QString& authCode = "");

    /////////////////////////////////////////////////////////////////////////
    /// 配置管理器集�?
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 从配置管理器加载配置并连�?
     * @param brokerId 服务商ID，为空则使用当前选中的服务商
     * @return 是否成功加载配置
     */
    bool loadConfigAndConnect(const QString& brokerId = QString());

    /**
     * @brief 切换服务商并重新连接
     * @param brokerId 新的服务商ID
     * @return 是否成功切换
     */
    bool switchBroker(const QString& brokerId);

    /**
     * @brief 获取当前服务商配�?
     */
    std::optional<CTPBrokerConfig> currentBrokerConfig() const;

    /**
     * @brief 获取当前服务商ID
     */
    QString currentBrokerId() const;

    /////////////////////////////////////////////////////////////////////////
    /// 连接管理
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 初始化并连接（非阻塞，异步完成）
     * @details 启动后会发射 loginFinished 信号
     */
    void setupConnections();

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 是否已登�?
     */
    bool isLoggedIn() const;

    /**
     * @brief 获取当前交易�?
     */
    QString tradingDay() const;

    /////////////////////////////////////////////////////////////////////////
    /// 行情接口（批量缓冲优化）
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 订阅行情（支持批量订阅，内部缓冲优化�?
     * @param instruments 合约代码列表，如 ["cu2505", "ag2506"]
     * @param useBuffer 是否使用批量缓冲（默认true，降低CPU占用�?
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
     * @return 本地订单引用（用于后续撤单）
     */
    std::optional<OrderRef> insertOrder(const OrderInfo& order);

    /**
     * @brief 撤销委托�?
     * @param orderRef 订单引用
     */
    void cancelOrder(const OrderRef& orderRef);

    /**
     * @brief 查询账户资金（异步，结果通过 signal 返回�?
     */
    void queryTradingAccount();

    /**
     * @brief 查询持仓（异步）
     */
    void queryPositions();

    /**
     * @brief 查询合约列表（异步）
     * @param exchangeId 交易所代码，为空则查询所有交易所
     */
    void queryInstruments(const QString& exchangeId = QString());

    /**
     * @brief 确认结算单（查询合约前需要）
     */
    void confirmSettlement();

signals:
    /////////////////////////////////////////////////////////////////////////
    /// 连接状态信�?
    /////////////////////////////////////////////////////////////////////////

    void marketConnected();                 // 行情前置连接成功
    void marketDisconnected(int reason);    // 行情前置断开
    void tradingConnected();                // 交易前置连接成功
    void tradingDisconnected(int reason);   // 交易前置断开
    void marketLoginFinished(bool success, const QString& errorMsg);  // 行情登录完成
    void tradingLoginFinished(bool success, const QString& errorMsg); // 交易登录完成
    void loginFinished(bool success, const QString& errorMsg);  // 兼容旧信号（行情登录�?
    void heartbeatWarning(int timeLapse);   // 心跳超时警告

    /////////////////////////////////////////////////////////////////////////
    /// 行情数据信号（批量缓冲优化）
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 批量深度行情推送（已解耦并缓冲�?
     * @param dataList 行情数据列表（批量到达）
     */
    void marketDataBatchReceived(const QList<MarketData>& dataList);

    /**
     * @brief 单个行情推送（高频场景�?
     */
    void marketDataReceived(const MarketData& data);

    /////////////////////////////////////////////////////////////////////////
    /// 交易回报信号
    /////////////////////////////////////////////////////////////////////////

    void orderUpdated(const OrderInfo& order);      // 订单状态更�?
    void tradeReceived(const TradeInfo& trade);     // 成交回报
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
    std::unique_ptr<Impl> d;  // C++17 unique_ptr支持不完整类�?

    // 内部初始化（C++17 if constexpr优化�?
    void initializeSpi();
};

} // namespace CTP

#endif // CTPService_H
