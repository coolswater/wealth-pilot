/////////////////////////////////////////////////////////////////////////
///@file CTPService.cpp
///@brief PIMPL实现 - 组装行情与交易SPI
/////////////////////////////////////////////////////////////////////////

#include "CTPService.h"
#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <core/CtpMarketSpi.h>
#include <core/CtpTradingSpi.h>
#include <QTimer>

namespace CTP {

class CTPService::Impl {
public:
    // 双SPI架构
    CtpMarketSpi* marketSpi{nullptr};
    CtpTradingSpi* tradingSpi{nullptr};

    // 独立线程（CTP API线程安全要求）
    QThread* marketThread{nullptr};
    QThread* tradingThread{nullptr};

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
    // 创建线程和SPI（C++17 结构化绑定不适用此处，使用传统方式）

    // 确保流文件目录存在
    QDir().mkpath("market_flow");
    QDir().mkpath("trading_flow");

    // 1. 启动行情线程
    d->marketThread = new QThread(this);
    d->marketSpi = new CtpMarketSpi();
    d->marketSpi->moveToThread(d->marketThread);

    // 行情信号连接
    connect(d->marketSpi, &CtpMarketSpi::connected, this, &CTPService::marketConnected);
    connect(d->marketSpi, &CtpMarketSpi::disconnected, this, &CTPService::marketDisconnected);
    connect(d->marketSpi, &CtpMarketSpi::marketDataReceived,
            this, &CTPService::marketDataReceived);
    connect(d->marketSpi, &CtpMarketSpi::marketDataReceived,
            this, [this](const MarketData& data) {
                // 批量缓冲：收集到列表后批量发射
                static QList<MarketData> batch;
                batch.append(data);
                if (batch.size() >= 50) {  // 每50条批量发射一次
                    emit marketDataBatchReceived(batch);
                    batch.clear();
                }
            });

    d->marketThread->start();

    // 在行情线程中初始化API
    QMetaObject::invokeMethod(d->marketSpi, [this]() {
        d->marketSpi->createApi("market_flow/");
        d->marketSpi->registerFront(d->marketFront);
        d->marketSpi->init();
    }, Qt::QueuedConnection);

    // 2. 启动交易线程
    d->tradingThread = new QThread(this);
    d->tradingSpi = new CtpTradingSpi();
    d->tradingSpi->moveToThread(d->tradingThread);

    connect(d->tradingSpi, &CtpTradingSpi::connected, this, &CTPService::tradingConnected);
    connect(d->tradingSpi, &CtpTradingSpi::loginResult,
            this, [this](bool success, const QString& msg) {
                d->isLoggedIn = success;
                emit loginFinished(success, msg);
            });
    connect(d->tradingSpi, &CtpTradingSpi::orderUpdated,
            this, &CTPService::orderUpdated);
    connect(d->tradingSpi, &CtpTradingSpi::tradeReceived,
            this, &CTPService::tradeReceived);
    connect(d->tradingSpi, &CtpTradingSpi::accountInfo,
            this, &CTPService::accountInfoReceived);

    d->tradingThread->start();

    QMetaObject::invokeMethod(d->tradingSpi, [this]() {
        d->tradingSpi->createApi("trading_flow/");
        d->tradingSpi->registerFront(d->tradingFront);
        d->tradingSpi->init();
    }, Qt::QueuedConnection);

    // 连接成功后自动登录（延迟执行，确保OnFrontConnected已触发）
    QTimer::singleShot(2000, this, [this]() {
        // 先认证（CTP6.6.1+要求）
        if (!d->appId.isEmpty() && !d->authCode.isEmpty()) {
            QMetaObject::invokeMethod(d->tradingSpi, [this]() {
                d->tradingSpi->authenticate(d->brokerId, d->userId,
                                            d->appId, d->authCode);
            }, Qt::QueuedConnection);

            // 认证后登录
            QTimer::singleShot(1000, this, [this]() {
                QMetaObject::invokeMethod(d->tradingSpi, [this]() {
                    d->tradingSpi->login(d->brokerId, d->userId, d->password);
                }, Qt::QueuedConnection);
            });
        } else {
            // 旧版直接登录
            QMetaObject::invokeMethod(d->tradingSpi, [this]() {
                d->tradingSpi->login(d->brokerId, d->userId, d->password);
            }, Qt::QueuedConnection);
        }

        // 行情登录（通常无需认证）
        QMetaObject::invokeMethod(d->marketSpi, [this]() {
            d->marketSpi->login(d->brokerId, d->userId, d->password);
        }, Qt::QueuedConnection);
    });
}

void CTPService::disconnect() {
    if (d->marketSpi) {
        QMetaObject::invokeMethod(d->marketSpi, &CtpMarketSpi::release,
                                  Qt::BlockingQueuedConnection);
    }
    if (d->tradingSpi) {
        QMetaObject::invokeMethod(d->tradingSpi, &CtpTradingSpi::release,
                                  Qt::BlockingQueuedConnection);
    }

    if (d->marketThread) {
        d->marketThread->quit();
        d->marketThread->wait(3000);
    }
    if (d->tradingThread) {
        d->tradingThread->quit();
        d->tradingThread->wait(3000);
    }

    d->isLoggedIn = false;
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

    QMetaObject::invokeMethod(d->marketSpi, [this, instruments]() {
        d->marketSpi->subscribeMarketData(instruments);
    }, Qt::QueuedConnection);
}

void CTPService::unsubscribeMarketData(const QList<InstrumentID>& instruments) {
    if (!d->marketSpi) return;

    QMetaObject::invokeMethod(d->marketSpi, [this, instruments]() {
        d->marketSpi->unsubscribeMarketData(instruments);
    }, Qt::QueuedConnection);
}

std::optional<OrderRef> CTPService::insertOrder(const OrderInfo& order) {
    if (!d->tradingSpi || !d->isLoggedIn) return std::nullopt;

    // 跨线程调用（C++17 std::optional返回值处理）
    std::optional<OrderRef> result;
    QMetaObject::invokeMethod(d->tradingSpi, [this, &result, order]() {
        result = d->tradingSpi->insertOrder(order);
    }, Qt::BlockingQueuedConnection);

    return result;
}

void CTPService::cancelOrder(const OrderRef& orderRef) {
    if (!d->tradingSpi || !d->isLoggedIn) return;

    QMetaObject::invokeMethod(d->tradingSpi, [this, orderRef]() {
        d->tradingSpi->cancelOrder(orderRef);
    }, Qt::QueuedConnection);
}

void CTPService::queryTradingAccount() {
    if (!d->tradingSpi || !d->isLoggedIn) return;

    QMetaObject::invokeMethod(d->tradingSpi, &CtpTradingSpi::queryAccount,
                              Qt::QueuedConnection);
}

void CTPService::queryPositions() {
    if (!d->tradingSpi || !d->isLoggedIn) return;

    QMetaObject::invokeMethod(d->tradingSpi, [this]() {
        d->tradingSpi->queryPositions(QString());  // 或传入实际合约代码
    }, Qt::QueuedConnection);
}

} // namespace CTP
