/////////////////////////////////////////////////////////////////////////
///@file CtpTradingSpi.h
///@brief 交易SPI实现 - 处理交易回报和事件
/////////////////////////////////////////////////////////////////////////

#ifndef CTPTRADINGSPI_H
#define CTPTRADINGSPI_H

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QHash>
#include <QtCore/QDateTime>
#include "../service/CTPService.h"

// CTP SDK 使用未命名参数作为虚函数占位符，禁用警告
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "external/ctp/ThostFtdcTraderApi.h"
#pragma GCC diagnostic pop

// 关键前向声明CTP结构体（避免头文件污染）
struct CThostFtdcInvestorPositionField;
struct CThostFtdcRspInfoField;
struct CThostFtdcInputOrderField;

namespace CTP {

/**
 * @brief 交易SPI类 - 处理交易回报、成交、持仓等
 */
class CtpTradingSpi : public QObject, public CThostFtdcTraderSpi {
    Q_OBJECT
public:
    explicit CtpTradingSpi(QObject *parent = nullptr);
    ~CtpTradingSpi() override;

    void createApi(const QString& flowPath);
    void registerFront(const QString& address);
    void init();
    void release();

    // 认证登录
    void authenticate(const QString& brokerId, const QString& userId,
                      const QString& appId, const QString& authCode);
    void login(const QString& brokerId, const QString& userId,
               const QString& password);
    void logout();

    // 交易操作
    std::optional<OrderRef> insertOrder(const OrderInfo& order);
    void cancelOrder(const OrderRef& orderRef);

    // 查询操作
    void queryAccount();
    void queryPositions(const QString& instrument = QString());
    void queryOrders();
    void queryTrades();
    void queryInstruments(const QString& exchangeId = QString());  ///< 查询合约
    void confirmSettlement();  ///< 确认结算单

    // CTP SPI回调（仅列出关键回调，其余在cpp中实现）
    void OnFrontConnected() override;
    void OnFrontDisconnected(int nReason) override;
    void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField,
                           CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                          CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRtnOrder(CThostFtdcOrderField *pOrder) override;
    void OnRtnTrade(CThostFtdcTradeField *pTrade) override;
    void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                                CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition,
                                  CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
                            CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryOrder(CThostFtdcOrderField *pOrder,
                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryTrade(CThostFtdcTradeField *pTrade,
                       CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, 
                                    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

signals:
    void connected();
    void disconnected(int reason);
    void authenticated(bool success, const QString& msg);
    void loginResult(bool success, const QString& msg);
    void orderInserted(const OrderRef& orderRef, bool success, const QString& msg);
    void orderUpdated(const OrderInfo& order);
    void tradeReceived(const TradeInfo& trade);
    void accountInfo(double available, double balance, double margin);
    void positionUpdated(const QString& instrument, int longPos, int shortPos);
    void instrumentQueried(const QString& instrumentId, const QString& exchangeId,
                           const QString& instrumentName, double priceTick, int volumeMultiple);
    void instrumentQueryFinished(int totalCount);
    void settlementConfirmed(bool success, const QString& msg);  ///< 结算单确认结果
    void ordersQueried(const QVector<OrderInfo>& orders);       ///< 订单查询结果
    void tradesQueried(const QVector<TradeInfo>& trades);       ///< 成交查询结果
    void error(int requestId, int errorId, const QString& msg);

private:
    class Impl;
    std::unique_ptr<Impl> d;

    // 转换函数
    static OrderInfo convertOrderField(const CThostFtdcOrderField& field);
    static TradeInfo convertTradeField(const CThostFtdcTradeField& field);
    static OrderStatus convertOrderStatus(char status);
    static Direction convertDirection(char dir);
    static OffsetFlag convertOffsetFlag(char flag);
};

} // namespace CTP

#endif