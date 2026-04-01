/////////////////////////////////////////////////////////////////////////
///@file CtpTradingSpi.cpp
///@brief 交易SPI实现 - 订单生命周期管理
/////////////////////////////////////////////////////////////////////////

#include "CtpTradingSpi.h"
#include "CtpApiLoader.h"  // 动态加载器
#include <QtCore/QDebug>
#include <cstring>

namespace CTP {

class CtpTradingSpi::Impl {
public:
    CThostFtdcTraderApi* api{nullptr};
    QMutex apiMutex;
    int requestId{0};
    QString frontId;
    QString sessionId;
    QString brokerId;
    QString userId;

    // C++17 unordered_map替代QHash提升性能
    std::unordered_map<std::string, OrderInfo> pendingOrders;
};

CtpTradingSpi::CtpTradingSpi(QObject *parent)
    : QObject(parent)
    , d(new Impl) {}

CtpTradingSpi::~CtpTradingSpi() {
    release();
}

void CtpTradingSpi::createApi(const QString& flowPath) {
    QMutexLocker locker(&d->apiMutex);
    QByteArray path = flowPath.toLocal8Bit();

    // 使用动态加载器创建 API（解决 MinGW 与 MSVC 兼容性问题）
    d->api = CtpApiLoader::instance().createTraderApi(path.constData(), true);

    if (d->api) {
        d->api->RegisterSpi(this);
        LOG_INFO(QString("CTP Trader API created successfully, version: %1").arg(CtpApiLoader::instance().getTraderApiVersion()));
    } else {
        LOG_ERROR("Failed to create CTP Trader API");
    }
}

void CtpTradingSpi::registerFront(const QString& address) {
    QMutexLocker locker(&d->apiMutex);
    if (d->api) {
        QByteArray addr = address.toLocal8Bit();
        d->api->RegisterFront(addr.data());
    }
}

void CtpTradingSpi::init() {
    QMutexLocker locker(&d->apiMutex);
    if (d->api) {
        // 订阅私有流（重传模式）
        d->api->SubscribePrivateTopic(THOST_TERT_RESUME);
        d->api->SubscribePublicTopic(THOST_TERT_RESUME);
        d->api->Init();
    }
}

void CtpTradingSpi::release() {
    QMutexLocker locker(&d->apiMutex);
    if (d->api) {
        d->api->RegisterSpi(nullptr);
        d->api->Release();
        d->api = nullptr;
    }
}

void CtpTradingSpi::authenticate(const QString& brokerId, const QString& userId,
                                 const QString& appId, const QString& authCode) {
    QMutexLocker locker(&d->apiMutex);
    if (!d->api) return;

    CThostFtdcReqAuthenticateField req{};
    strncpy(req.BrokerID, brokerId.toLocal8Bit().constData(), sizeof(req.BrokerID) - 1);
    strncpy(req.UserID, userId.toLocal8Bit().constData(), sizeof(req.UserID) - 1);
    strncpy(req.AppID, appId.toLocal8Bit().constData(), sizeof(req.AppID) - 1);
    strncpy(req.AuthCode, authCode.toLocal8Bit().constData(), sizeof(req.AuthCode) - 1);

    d->api->ReqAuthenticate(&req, ++d->requestId);
}

void CtpTradingSpi::login(const QString& brokerId, const QString& userId,
                          const QString& password) {
    QMutexLocker locker(&d->apiMutex);
    if (!d->api) return;

    d->brokerId = brokerId;
    d->userId = userId;

    CThostFtdcReqUserLoginField req{};
    strncpy(req.BrokerID, brokerId.toLocal8Bit().constData(), sizeof(req.BrokerID) - 1);
    strncpy(req.UserID, userId.toLocal8Bit().constData(), sizeof(req.UserID) - 1);
    strncpy(req.Password, password.toLocal8Bit().constData(), sizeof(req.Password) - 1);

    d->api->ReqUserLogin(&req, ++d->requestId);
}

std::optional<OrderRef> CtpTradingSpi::insertOrder(const OrderInfo& order) {
    QMutexLocker locker(&d->apiMutex);
    if (!d->api) return std::nullopt;

    CThostFtdcInputOrderField req{};

    // 填充字段
    strncpy(req.BrokerID, d->brokerId.toLocal8Bit().constData(),
            sizeof(req.BrokerID) - 1);
    strncpy(req.InvestorID, d->userId.toLocal8Bit().constData(),
            sizeof(req.InvestorID) - 1);
    strncpy(req.InstrumentID, order.instrumentId.toLocal8Bit().constData(),
            sizeof(req.InstrumentID) - 1);

    // 生成订单引用（C++17 string拼接优化）
    QString orderRefStr = QString("%1_%2")
                              .arg(QDateTime::currentDateTime().toString("hhmmss"))
                              .arg(++d->requestId);
    QByteArray orderRefBa = orderRefStr.toLocal8Bit();
    strncpy(req.OrderRef, orderRefBa.constData(), sizeof(req.OrderRef) - 1);

    req.Direction = order.direction == Direction::Buy ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;

    // 开平标志转换
    switch (order.offset) {
    case OffsetFlag::Open: req.CombOffsetFlag[0] = THOST_FTDC_OF_Open; break;
    case OffsetFlag::Close: req.CombOffsetFlag[0] = THOST_FTDC_OF_Close; break;
    case OffsetFlag::CloseToday: req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday; break;
    case OffsetFlag::CloseYesterday: req.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday; break;
    }

    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;  // 投机
    req.LimitPrice = order.price;
    req.VolumeTotalOriginal = order.totalVolume;
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;  // 限价
    req.TimeCondition = THOST_FTDC_TC_GFD;  // 当日有效
    req.VolumeCondition = THOST_FTDC_VC_AV;  // 任何数量
    req.ContingentCondition = THOST_FTDC_CC_Immediately;  // 立即

    int ret = d->api->ReqOrderInsert(&req, ++d->requestId);
    if (ret == 0) {
        return orderRefStr;
    }
    return std::nullopt;
}

void CtpTradingSpi::cancelOrder(const OrderRef& orderRef) {
    QMutexLocker locker(&d->apiMutex);
    if (!d->api) return;

    CThostFtdcInputOrderActionField req{};
    strncpy(req.BrokerID, d->brokerId.toLocal8Bit().constData(),
            sizeof(req.BrokerID) - 1);
    strncpy(req.InvestorID, d->userId.toLocal8Bit().constData(),
            sizeof(req.InvestorID) - 1);
    strncpy(req.OrderRef, orderRef.toLocal8Bit().constData(),
            sizeof(req.OrderRef) - 1);
    req.ActionFlag = THOST_FTDC_AF_Delete;  // 删除

    d->api->ReqOrderAction(&req, ++d->requestId);
}

void CtpTradingSpi::queryAccount() {
    QMutexLocker locker(&d->apiMutex);
    if (!d->api) return;

    CThostFtdcQryTradingAccountField req{};
    strncpy(req.BrokerID, d->brokerId.toLocal8Bit().constData(),
            sizeof(req.BrokerID) - 1);
    strncpy(req.InvestorID, d->userId.toLocal8Bit().constData(),
            sizeof(req.InvestorID) - 1);

    d->api->ReqQryTradingAccount(&req, ++d->requestId);
}

void CtpTradingSpi::queryPositions(const QString& instrument)
{
    // 如果 instrument 为空字符串，查询所有持仓
    CThostFtdcQryInvestorPositionField req{};
    strcpy(req.BrokerID, d->brokerId.toUtf8().constData());
    strcpy(req.InvestorID, d->userId.toUtf8().constData());

    if (!instrument.isEmpty()) {
        strcpy(req.InstrumentID, instrument.toUtf8().constData());
    }

    int ret = d->api->ReqQryInvestorPosition(&req, ++d->requestId);
    if (ret != 0) {
        emit error(d->requestId, ret, "查询持仓请求发送失败");
    }
}

// SPI回调实现
void CtpTradingSpi::OnFrontConnected() {
    emit connected();
}

void CtpTradingSpi::OnFrontDisconnected(int nReason) {
    emit disconnected(nReason);
}

void CtpTradingSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField,
                                      CThostFtdcRspInfoField *pRspInfo,
                                      int nRequestID, bool bIsLast) {
    bool success = (pRspInfo && pRspInfo->ErrorID == 0);
    QString msg = success ? "认证成功" :
                      (pRspInfo ? QString::fromLocal8Bit(pRspInfo->ErrorMsg) : "认证失败");
    emit authenticated(success, msg);
}

void CtpTradingSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) {
    bool success = (pRspInfo && pRspInfo->ErrorID == 0);
    if (success && pRspUserLogin) {
        d->frontId = QString::number(pRspUserLogin->FrontID);
        d->sessionId = QString::number(pRspUserLogin->SessionID);
    }
    QString msg = success ? "登录成功" :
                      (pRspInfo ? QString::fromLocal8Bit(pRspInfo->ErrorMsg) : "登录失败");
    emit loginResult(success, msg);
}

void CtpTradingSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                                     CThostFtdcRspInfoField *pRspInfo,
                                     int nRequestID, bool bIsLast) {
    bool success = (pRspInfo && pRspInfo->ErrorID == 0);
    QString orderRef = pInputOrder ? QString::fromLocal8Bit(pInputOrder->OrderRef) : "";
    QString msg = success ? "报单成功" :
                      (pRspInfo ? QString::fromLocal8Bit(pRspInfo->ErrorMsg) : "报单失败");
    emit orderInserted(orderRef, success, msg);
}

void CtpTradingSpi::OnRtnOrder(CThostFtdcOrderField *pOrder) {
    if (!pOrder) return;
    auto order = convertOrderField(*pOrder);
    emit orderUpdated(order);
}

void CtpTradingSpi::OnRtnTrade(CThostFtdcTradeField *pTrade) {
    if (!pTrade) return;
    auto trade = convertTradeField(*pTrade);
    emit tradeReceived(trade);
}

void CtpTradingSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount,
                                           CThostFtdcRspInfoField *pRspInfo,
                                           int nRequestID, bool bIsLast) {
    if (pTradingAccount && pRspInfo && pRspInfo->ErrorID == 0) {
        emit accountInfo(pTradingAccount->Available,
                         pTradingAccount->Balance,
                         pTradingAccount->CurrMargin);
    }
}

void CtpTradingSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{

}

void CtpTradingSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
                                                CThostFtdcRspInfoField *pRspInfo,
                                                int nRequestID, bool bIsLast) {
    bool success = (pRspInfo && pRspInfo->ErrorID == 0);
    QString msg = success ? "结算单确认成功" :
                      (pRspInfo ? QString::fromLocal8Bit(pRspInfo->ErrorMsg) : "结算单确认失败");

    LOG_INFO(QString("OnRspSettlementInfoConfirm: %1").arg(msg));
}

OrderInfo CtpTradingSpi::convertOrderField(const CThostFtdcOrderField& field) {
    OrderInfo info;
    info.instrumentId = QString::fromLocal8Bit(field.InstrumentID);
    info.orderRef = QString::fromLocal8Bit(field.OrderRef);
    info.direction = convertDirection(field.Direction);
    // ... 其他字段转换
    info.status = convertOrderStatus(field.OrderStatus);
    info.statusMsg = QString::fromLocal8Bit(field.StatusMsg);
    info.totalVolume = field.VolumeTotalOriginal;
    info.tradedVolume = field.VolumeTraded;
    info.insertTime = QDateTime::currentDateTime();
    return info;
}

TradeInfo CtpTradingSpi::convertTradeField(const CThostFtdcTradeField& field) {
    TradeInfo info;
    info.instrumentId = QString::fromLocal8Bit(field.InstrumentID);
    info.orderRef = QString::fromLocal8Bit(field.OrderRef);
    info.tradeId = QString::fromLocal8Bit(field.TradeID);
    info.direction = convertDirection(field.Direction);
    info.offset = convertOffsetFlag(field.OffsetFlag);
    info.price = field.Price;
    info.volume = field.Volume;
    info.tradeTime = QDateTime::currentDateTime();
    info.tradeTime.setTime(QTime::fromString(
        QString::fromLocal8Bit(field.TradeTime), "hh:mm:ss"));
    return info;
}

OrderStatus CtpTradingSpi::convertOrderStatus(char status) {
    switch (status) {
    case THOST_FTDC_OST_AllTraded: return OrderStatus::AllTraded;
    case THOST_FTDC_OST_PartTradedQueueing: return OrderStatus::PartTradedQueueing;
    case THOST_FTDC_OST_PartTradedNotQueueing: return OrderStatus::PartTradedNotQueueing;
    case THOST_FTDC_OST_NoTradeQueueing: return OrderStatus::NoTradeQueueing;
    case THOST_FTDC_OST_NoTradeNotQueueing: return OrderStatus::NoTradeNotQueueing;
    case THOST_FTDC_OST_Canceled: return OrderStatus::Canceled;
    default: return OrderStatus::Unknown;
    }
}

Direction CtpTradingSpi::convertDirection(char dir) {
    return dir == THOST_FTDC_D_Buy ? Direction::Buy : Direction::Sell;
}

OffsetFlag CtpTradingSpi::convertOffsetFlag(char flag) {
    switch (flag) {
    case THOST_FTDC_OF_Open: return OffsetFlag::Open;
    case THOST_FTDC_OF_Close: return OffsetFlag::Close;
    case THOST_FTDC_OF_CloseToday: return OffsetFlag::CloseToday;
    case THOST_FTDC_OF_CloseYesterday: return OffsetFlag::CloseYesterday;
    default: return OffsetFlag::Open;
    }
}

} // namespace Ctp
