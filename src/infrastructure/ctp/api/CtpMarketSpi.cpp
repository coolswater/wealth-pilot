/////////////////////////////////////////////////////////////////////////
///@file CtpMarketSpi.cpp
///@brief Market SPI implementation - PIMPL细节
/////////////////////////////////////////////////////////////////////////

// CTP SDK 使用未命名参数作为虚函数占位符，禁用警告
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "external/ctp/ThostFtdcMdApi.h"
#pragma GCC diagnostic pop
#include "../service/CTPService.h"
#include "CtpDataBuffer.h"
#include "CtpMarketSpi.h"
#include "CtpApiLoader.h"  // 动态加载器
#include "shared/utils/Logger.h"

#include <QtCore/QString>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
    // Implementation

namespace CTP {

// PIMPL 实现 - 使用 QScopedPointer 避免不完整类型
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

    // 创建缓冲6ms刷新，约60fps，降低UI更新频率
    d->buffer = new BatchBuffer<MarketData>(200, 16, this);

    // 在构造函数或Initialize函数中设置回调
    d->buffer->setCallback([this](const std::vector<MarketData>& data) {
        // 处理数据
        // 如果需要切换到主线程，使用 QMetaObject::invokeMethod
        QMetaObject::invokeMethod(this, [this, &data]() {
            // 批量发射到主业务逻辑
            for (const auto& item : data) {
                emit marketDataReceived(item);
            }
        }, Qt::QueuedConnection);
    });
}

CtpMarketSpi::~CtpMarketSpi() {
    LOG_INFO("CtpMarketSpi destructor - stopping worker thread...");
    
    // 停止 MarketWorker 线程（如果正在运行）
    if (d->worker && d->worker->isRunning()) {
        LOG_INFO("Stopping MarketWorker thread...");
        d->worker->quit();  // 退出事件循环
        
        if (!d->worker->wait(5000)) {  // 等待最多5秒
            LOG_WARNING("MarketWorker did not stop gracefully, terminating...");
            d->worker->terminate();  // 强制终止
            d->worker->wait();  // 等待终止完成
        }
        
        LOG_INFO("MarketWorker thread stopped");
    }
    
    // 释放 CTP API（如果存在）
    if (d->api) {
        LOG_INFO("Releasing CTP Market API...");
        d->api->RegisterSpi(nullptr);
        d->api->Release();
        d->api = nullptr;
        LOG_INFO("CTP Market API released");
    }
    
    // QScopedPointer 会自动清理
    LOG_INFO("CtpMarketSpi destructor complete");
}

void CtpMarketSpi::createApi(const QString& flowPath, bool isUdp, bool isMulticast) {
    QMutexLocker locker(&d->apiMutex);

    LOG_INFO(QString("CtpMarketSpi::createApi() called, flowPath= %1").arg(flowPath));

    // 使用动态加载器创建 API（解MinGW �?MSVC 兼容性问题）
    QByteArray path = flowPath.toLocal8Bit();
    d->api = CtpApiLoader::instance().createMdApi(
        path.constData(), isUdp, isMulticast, true);  // true=生产模式

    if (d->api) {
        d->api->RegisterSpi(this);
        LOG_INFO(QString("CTP Market API created successfully, version: %1").arg(CtpApiLoader::instance().getMdApiVersion()));
    } else {
        LOG_ERROR(QString("Failed to create CTP Market API - this is likely a DLL loading issue! \n\r"
                          "Please ensure thostmduserapi_se.dll is in the application directory"));
    }
}

void CtpMarketSpi::registerFront(const QString& address) {
    QMutexLocker locker(&d->apiMutex);
    if (d->api) {
        QByteArray addr = address.toLocal8Bit();
        LOG_INFO(QString("CtpMarketSpi::registerFront() called, address= %1").arg(address));
        d->api->RegisterFront(addr.data());
    } else {
        LOG_DEBUG(QString("CtpMarketSpi::registerFront() failed: API not created"));
    }
}

void CtpMarketSpi::init() {
    QMutexLocker locker(&d->apiMutex);
    if (d->api) {
        LOG_INFO("CtpMarketSpi::init() called, starting CTP connection...");
        d->api->Init();
        LOG_INFO("CtpMarketSpi::init() completed, CTP thread started");
    } else {
        LOG_ERROR("CtpMarketSpi::init() failed: API not created");
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

    LOG_INFO(QString("CtpMarketSpi::subscribeMarketData() - Subscribing %1 instruments: %2")
             .arg(instruments.size()).arg(instruments.join(", ")));

    // 转换为CTP需要的char**
    QList<QByteArray> byteArrays;
    std::vector<char*> ptrs;

    for (const auto& inst : instruments) {
        byteArrays.append(inst.toLocal8Bit());
        ptrs.push_back(byteArrays.last().data());
    }

    int result = d->api->SubscribeMarketData(ptrs.data(), static_cast<int>(ptrs.size()));
    LOG_INFO(QString("SubscribeMarketData API returned: %1").arg(result));
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
    LOG_INFO("CtpMarketSpi::OnFrontConnected() - CTP server connected!");
    emit connected();
}

void CtpMarketSpi::OnFrontDisconnected(int nReason) {
    LOG_DEBUG(QString("=== CtpMarketSpi::OnFrontDisconnected() CALLED, reason= %1 ===").arg(nReason));
    LOG_WARNING(QString("CtpMarketSpi::OnFrontDisconnected() - reason: %1").arg(nReason));
    emit disconnected(nReason);
}

void CtpMarketSpi::OnHeartBeatWarning(int nTimeLapse) {
    emit heartbeatWarning(nTimeLapse);
}

void CtpMarketSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                  CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {
    bool success = (pRspInfo && pRspInfo->ErrorID == 0);
    QString msg = success ? "Login成功" :
                      (pRspInfo ? QString::fromLocal8Bit(pRspInfo->ErrorMsg) : "未知Error");

    LOG_INFO(QString("CtpMarketSpi::OnRspUserLogin() - success: %1, msg: %2, ErrorID: %3")
             .arg(success).arg(msg).arg(pRspInfo ? pRspInfo->ErrorID : -1));

    emit loginResult(success, msg);
}

void CtpMarketSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument,
                                      CThostFtdcRspInfoField *pRspInfo,
                                      int nRequestID, bool bIsLast) {
    QString instrumentId = pSpecificInstrument ? QString::fromLocal8Bit(pSpecificInstrument->InstrumentID) : "unknown";
    int errorId = pRspInfo ? pRspInfo->ErrorID : 0;

    // 订阅响应处理
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        QString errorMsg = QString::fromLocal8Bit(pRspInfo->ErrorMsg);
        LOG_ERROR(QString("Subscribe failed: %1").arg(errorMsg));
        emit error(nRequestID, pRspInfo->ErrorID, errorMsg);
    }
}

void CtpMarketSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
    if (!pDepthMarketData) return;
    auto data = convertDepthMarketData(*pDepthMarketData);

    // 使用批量缓冲或直接发送
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
    data.InstrumentID = QString::fromLocal8Bit(field.InstrumentID);

    // Implementation
    QString timeStr = QString("%1.%2")
                          .arg(QString::fromLocal8Bit(field.UpdateTime))
                          .arg(field.UpdateMillisec, 3, 10, QChar('0'));

    data.UpdateTime = QDateTime::currentDateTime();
    data.UpdateTime.setTime(QTime::fromString(timeStr, "hh:mm:ss.zzz"));

    data.lastPrice = field.LastPrice;
    data.BidPrice1 = field.BidPrice1;
    data.BidVolume1 = field.BidVolume1;
    data.AskPrice1 = field.AskPrice1;
    data.AskVolume1 = field.AskVolume1;
    data.OpenPrice = field.OpenPrice;
    data.HighestPrice = field.HighestPrice;
    data.LowestPrice = field.LowestPrice;
    data.Volume = field.Volume;
    data.OpenInterest = field.OpenInterest;
    data.preSettlementPrice = field.PreSettlementPrice;

    return data;
}

} // namespace CTP
