/////////////////////////////////////////////////////////////////////////
///@file CtpMarketSpi.cpp
///@brief 行情SPI实现 - PIMPL细节
/////////////////////////////////////////////////////////////////////////
#include "external/ctp/ThostFtdcMdApi.h"
#include "services/CTPService.h"
#include "CtpDataBuffer.h"
#include "CtpMarketSpi.h"
#include "CtpApiLoader.h"  // 动态加载器

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QScopedPointer>       // 关键：替换 <memory>

namespace CTP {

// PIMPL 实现 - 使用 QScopedPointer 避免不完整类型问题
class CtpMarketSpi::Impl {
public:
    CThostFtdcMdApi* api{nullptr};
    QMutex apiMutex;
    BatchBuffer<MarketData>* buffer{nullptr};
    bool useBuffer{true};
    int requestId{0};

    // 修复：QScopedPointer 自动管理内存，且支持前向声明
    QScopedPointer<MarketWorker> worker;
};

CtpMarketSpi::CtpMarketSpi(QObject *parent)
    : QObject(parent)
    , d(new Impl) {

    // 创建缓冲（16ms刷新，约60fps，降低UI更新频率）
    d->buffer = new BatchBuffer<MarketData>(200, 16, this);

    // 在构造函数或初始化函数中设置回调
    d->buffer->setCallback([=](const std::vector<MarketData>& data) {
        // 处理数据
        // 如果需要切换到主线程，使用 QMetaObject::invokeMethod
        QMetaObject::invokeMethod(this, [=]() {
            // 批量发射到主业务逻辑
            for (const auto& item : data) {
                emit marketDataReceived(item);
            }
        }, Qt::QueuedConnection);
    });
}

CtpMarketSpi::~CtpMarketSpi() = default;  // C++17 默认析构

void CtpMarketSpi::createApi(const QString& flowPath, bool isUdp, bool isMulticast) {
    QMutexLocker locker(&d->apiMutex);

    // 使用动态加载器创建 API（解决 MinGW 与 MSVC 兼容性问题）
    QByteArray path = flowPath.toLocal8Bit();
    d->api = CtpApiLoader::instance().createMdApi(
        path.constData(), isUdp, isMulticast, true);  // true=生产模式

    if (d->api) {
        d->api->RegisterSpi(this);
        qDebug() << "CTP Market API created successfully, version:" 
                 << CtpApiLoader::instance().getMdApiVersion();
    } else {
        qWarning() << "Failed to create CTP Market API";
    }
}

void CtpMarketSpi::registerFront(const QString& address) {
    QMutexLocker locker(&d->apiMutex);
    if (d->api) {
        QByteArray addr = address.toLocal8Bit();
        d->api->RegisterFront(addr.data());
    }
}

void CtpMarketSpi::init() {
    QMutexLocker locker(&d->apiMutex);
    if (d->api) {
        d->api->Init();
    }
}

void CtpMarketSpi::release() {
    QMutexLocker locker(&d->apiMutex);
    if (d->api) {
        d->api->RegisterSpi(nullptr);
        d->api->Release();
        d->api = nullptr;
    }
}

void CtpMarketSpi::login(const QString& brokerId, const QString& userId,
                         const QString& password) {
    QMutexLocker locker(&d->apiMutex);
    if (!d->api) return;

    CThostFtdcReqUserLoginField req{};

    // C++17 安全字符串拷贝
    strncpy(req.BrokerID, brokerId.toLocal8Bit().constData(),
            sizeof(req.BrokerID) - 1);
    strncpy(req.UserID, userId.toLocal8Bit().constData(),
            sizeof(req.UserID) - 1);
    strncpy(req.Password, password.toLocal8Bit().constData(),
            sizeof(req.Password) - 1);

    d->api->ReqUserLogin(&req, ++d->requestId);
}

void CtpMarketSpi::subscribeMarketData(const QList<QString>& instruments) {
    QMutexLocker locker(&d->apiMutex);
    if (!d->api || instruments.isEmpty()) return;

    // 转换为CTP需要的char**
    QList<QByteArray> byteArrays;
    std::vector<char*> ptrs;

    for (const auto& inst : instruments) {
        byteArrays.append(inst.toLocal8Bit());
        ptrs.push_back(byteArrays.last().data());
    }

    d->api->SubscribeMarketData(ptrs.data(), static_cast<int>(ptrs.size()));
}

void CtpMarketSpi::unsubscribeMarketData(const QList<QString>& instruments) {
    QMutexLocker locker(&d->apiMutex);
    if (!d->api || instruments.isEmpty()) return;

    QList<QByteArray> byteArrays;
    std::vector<char*> ptrs;

    for (const auto& inst : instruments) {
        byteArrays.append(inst.toLocal8Bit());
        ptrs.push_back(byteArrays.last().data());
    }

    d->api->UnSubscribeMarketData(ptrs.data(), static_cast<int>(ptrs.size()));
}

// CTP回调实现
void CtpMarketSpi::OnFrontConnected() {
    emit connected();
}

void CtpMarketSpi::OnFrontDisconnected(int nReason) {
    emit disconnected(nReason);
}

void CtpMarketSpi::OnHeartBeatWarning(int nTimeLapse) {
    emit heartbeatWarning(nTimeLapse);
}

void CtpMarketSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                  CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    bool success = (pRspInfo && pRspInfo->ErrorID == 0);
    QString msg = success ? "登录成功" :
                      (pRspInfo ? QString::fromLocal8Bit(pRspInfo->ErrorMsg) : "未知错误");

    emit loginResult(success, msg);
}

void CtpMarketSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                      CThostFtdcRspInfoField *pRspInfo,
                                      int nRequestID, bool bIsLast) {
    // 订阅响应处理
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        emit error(nRequestID, pRspInfo->ErrorID,
                   QString::fromLocal8Bit(pRspInfo->ErrorMsg));
    }
}

void CtpMarketSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
    if (!pDepthMarketData) return;

    auto data = convertDepthMarketData(*pDepthMarketData);

    // 使用批量缓冲或直接发射
    if (d->buffer && d->useBuffer) {
        d->buffer->push(std::move(data));  // C++17 move语义
    } else {
        emit marketDataReceived(data);
    }
}

void CtpMarketSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo,
                              int nRequestID, bool bIsLast) {
    if (pRspInfo) {
        emit error(nRequestID, pRspInfo->ErrorID,
                   QString::fromLocal8Bit(pRspInfo->ErrorMsg));
    }
}

MarketData CtpMarketSpi::convertDepthMarketData(
    const CThostFtdcDepthMarketDataField& field) {

    MarketData data;
    data.instrumentId = QString::fromLocal8Bit(field.InstrumentID);

    // 解析时间（格式：HH:MM:SS.mmm）
    QString timeStr = QString("%1.%2")
                          .arg(QString::fromLocal8Bit(field.UpdateTime))
                          .arg(field.UpdateMillisec, 3, 10, QChar('0'));

    data.updateTime = QDateTime::currentDateTime();
    data.updateTime.setTime(QTime::fromString(timeStr, "hh:mm:ss.zzz"));

    data.lastPrice = field.LastPrice;
    data.bidPrice1 = field.BidPrice1;
    data.bidVolume1 = field.BidVolume1;
    data.askPrice1 = field.AskPrice1;
    data.askVolume1 = field.AskVolume1;
    data.volume = field.Volume;
    data.openInterest = field.OpenInterest;
    data.preSettlementPrice = field.PreSettlementPrice;

    return data;
}

} // namespace CTP
