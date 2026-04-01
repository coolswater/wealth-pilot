/////////////////////////////////////////////////////////////////////////
///@file CTPService.cpp
///@brief PIMPL实现 - 组装行情与交易SPI
/////////////////////////////////////////////////////////////////////////

#include "CTPService.h"
#include "utils/Logger.h"
#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <core/CtpMarketSpi.h>
#include <core/CtpTradingSpi.h>
#include <QTimer>

namespace CTP {

class CTPService::Impl {
public:
    // 双SPI架构
    CtpMarketSpi* marketSpi{nullptr};
    CtpTradingSpi* tradingSpi{nullptr};

    // 配置
    QString marketFront;
    QString tradingFront;
    QString brokerId;
    QString userId;
    QString password;
    QString appId;
    QString authCode;

    bool isLoggedIn{false};
};

CTPService::CTPService(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Impl>()) {  // C++17 make_unique
}

CTPService::~CTPService() {
    disconnect();
}

void CTPService::setMarketFrontAddress(const QString& frontAddr) {
    d->marketFront = frontAddr;
}

void CTPService::setTradingFrontAddress(const QString& frontAddr) {
    d->tradingFront = frontAddr;
}

void CTPService::setCredentials(const QString& brokerId, const QString& userId,
                               const QString& password, const QString& appId,
                               const QString& authCode) {
    d->brokerId = brokerId;
    d->userId = userId;
    d->password = password;
    d->appId = appId;
    d->authCode = authCode;
}

void CTPService::setupConnections() {
    LOG_INFO("CTPService::setupConnections() called");

    // 创建线程和SPI
    // 注意：CTP API 内部有自己的线程，不需要 moveToThread
    // CTP 的回调会在 CTP 内部线程中触发，我们只需要确保信号槽连接正确

    // 确保流文件目录存在（使用绝对路径）
    QString appPath = QCoreApplication::applicationDirPath();
    QString marketFlowPath = appPath + "/market_flow";
    QString tradingFlowPath = appPath + "/trading_flow";
    
    QDir().mkpath(marketFlowPath);
    QDir().mkpath(tradingFlowPath);
    
    LOG_INFO(QString("Flow directories created: %1, %2").arg(marketFlowPath).arg(tradingFlowPath));

    // 1. 创建行情 SPI（不要移动到 QThread！）
    d->marketSpi = new CtpMarketSpi(this);  // 设置 parent，确保生命周期

    // 行情信号连接（使用 Qt::DirectConnection 确保在 CTP 线程中立即发射）
    connect(d->marketSpi, &CtpMarketSpi::connected, this, [this]() {
        LOG_INFO("CTPService: marketSpi::connected signal received");
        emit marketConnected();
    }, Qt::DirectConnection);

    connect(d->marketSpi, &CtpMarketSpi::disconnected, this, [this](int reason) {
        LOG_WARNING(QString("CTPService: marketSpi::disconnected signal received, reason=%1").arg(reason));
        emit marketDisconnected(reason);
    }, Qt::DirectConnection);

    connect(d->marketSpi, &CtpMarketSpi::marketDataReceived,
            this, &CTPService::marketDataReceived, Qt::DirectConnection);
    
    connect(d->marketSpi, &CtpMarketSpi::marketDataReceived,
            this, [this](const MarketData& data) {
                // 批量缓冲：收集到列表后批量发射
                static QList<MarketData> batch;
                batch.append(data);
                if (batch.size() >= 50) {  // 每50条批量发射一次
                    emit marketDataBatchReceived(batch);
                    batch.clear();
                }
            }, Qt::DirectConnection);

    connect(d->marketSpi, &CtpMarketSpi::loginResult, this,
            [this](bool success, const QString& msg) {
                LOG_INFO(QString("CTPService: marketSpi::loginResult success=%1 msg=%2").arg(success).arg(msg));
                emit loginFinished(success, msg);
            }, Qt::DirectConnection);

    connect(d->marketSpi, &CtpMarketSpi::error, this,
            [this](int reqId, int errorId, const QString& msg) {
                LOG_ERROR(QString("CTPService: marketSpi::error reqId=%1 errorId=%2 msg=%3").arg(reqId).arg(errorId).arg(msg));
                emit errorOccurred(reqId, errorId, msg);
            }, Qt::DirectConnection);

    LOG_INFO("Market SPI created and signals connected");

    // 直接初始化API（不需要 QMetaObject::invokeMethod）
    LOG_INFO(QString("Creating market API with flow path: %1/, front: %2").arg(marketFlowPath).arg(d->marketFront));
    d->marketSpi->createApi(marketFlowPath.toLocal8Bit() + "/");
    d->marketSpi->registerFront(d->marketFront);
    d->marketSpi->init();
    LOG_INFO("Market API init() called");

    // 2. 创建交易 SPI（同样不要移动到 QThread）
    d->tradingSpi = new CtpTradingSpi(this);

    connect(d->tradingSpi, &CtpTradingSpi::connected, this, &CTPService::tradingConnected, Qt::DirectConnection);
    connect(d->tradingSpi, &CtpTradingSpi::loginResult,
            this, [this](bool success, const QString& msg) {
                d->isLoggedIn = success;
                emit loginFinished(success, msg);
            }, Qt::DirectConnection);
    connect(d->tradingSpi, &CtpTradingSpi::orderUpdated,
            this, &CTPService::orderUpdated, Qt::DirectConnection);
    connect(d->tradingSpi, &CtpTradingSpi::tradeReceived,
            this, &CTPService::tradeReceived, Qt::DirectConnection);
    connect(d->tradingSpi, &CtpTradingSpi::accountInfo,
            this, &CTPService::accountInfoReceived, Qt::DirectConnection);

    LOG_INFO("Trader SPI created and signals connected");

    // 直接初始化交易 API
    LOG_INFO(QString("Creating trader API with flow path: %1/, front: %2").arg(tradingFlowPath).arg(d->tradingFront));
    d->tradingSpi->createApi(tradingFlowPath.toLocal8Bit() + "/");
    d->tradingSpi->registerFront(d->tradingFront);
    d->tradingSpi->init();
    LOG_INFO("Trader API init() called");

    // 连接成功后自动登录（延迟执行，确保OnFrontConnected已触发）
    QTimer::singleShot(2000, this, [this]() {
        LOG_INFO("Auto-login timer triggered after 2 seconds");
        
        // 行情登录（通常无需认证）
        if (d->marketSpi) {
            LOG_INFO(QString("Calling marketSpi::login, brokerId: %1 userId: %2").arg(d->brokerId).arg(d->userId));
            d->marketSpi->login(d->brokerId, d->userId, d->password);
        }

        // 交易登录
        if (d->tradingSpi) {
            // 先认证（CTP6.6.1+要求）
            if (!d->appId.isEmpty() && !d->authCode.isEmpty()) {
                LOG_INFO("Authenticating with AppID and AuthCode...");
                d->tradingSpi->authenticate(d->brokerId, d->userId, d->appId, d->authCode);

                // 认证后登录
                QTimer::singleShot(1000, this, [this]() {
                    LOG_INFO("Logging in to trader after authentication...");
                    d->tradingSpi->login(d->brokerId, d->userId, d->password);
                });
            } else {
                // 旧版直接登录
                LOG_INFO("Direct login to trader (no auth required)...");
                d->tradingSpi->login(d->brokerId, d->userId, d->password);
            }
        }
    });
}

void CTPService::disconnect() {
    if (d->marketSpi) {
        d->marketSpi->release();
    }
    if (d->tradingSpi) {
        d->tradingSpi->release();
    }

    d->isLoggedIn = false;
    LOG_INFO("CTPService disconnected");
}

bool CTPService::isLoggedIn() const {
    return d->isLoggedIn;
}

QString CTPService::tradingDay() const {
    // 可通过tradingSpi获取
    return QString();
}

void CTPService::subscribeMarketData(const QList<InstrumentID>& instruments, bool useBuffer) {
    if (!d->marketSpi) return;
    d->marketSpi->subscribeMarketData(instruments);
    LOG_INFO(QString("Subscribed to %1 instruments").arg(instruments.size()));
}

void CTPService::unsubscribeMarketData(const QList<InstrumentID>& instruments) {
    if (!d->marketSpi) return;
    d->marketSpi->unsubscribeMarketData(instruments);
}

std::optional<OrderRef> CTPService::insertOrder(const OrderInfo& order) {
    if (!d->tradingSpi || !d->isLoggedIn) return std::nullopt;
    return d->tradingSpi->insertOrder(order);
}

void CTPService::cancelOrder(const OrderRef& orderRef) {
    if (!d->tradingSpi || !d->isLoggedIn) return;
    d->tradingSpi->cancelOrder(orderRef);
}

void CTPService::queryTradingAccount() {
    if (!d->tradingSpi || !d->isLoggedIn) return;
    d->tradingSpi->queryAccount();
}

void CTPService::queryPositions() {
    if (!d->tradingSpi || !d->isLoggedIn) return;
    d->tradingSpi->queryPositions(QString());
}

} // namespace CTP
