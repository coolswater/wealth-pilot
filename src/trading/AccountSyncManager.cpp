/**
 * @file AccountSyncManager.cpp
 * @brief 账户同步管理器实现
 */

#include "AccountSyncManager.h"
#include "TradingService.h"
#include "PositionManager.h"
#include "ctp/service/CTPService.h"
#include "utils/Logger.h"

#include <QMutexLocker>

struct AccountSyncManager::Impl {
    CTP::CTPService* ctpService = nullptr;
    QTimer* autoRefreshTimer = nullptr;
    AccountInfo accountInfo;
    mutable QMutex mutex;
    bool initialized = false;
};

AccountSyncManager& AccountSyncManager::instance()
{
    static AccountSyncManager instance;
    return instance;
}

AccountSyncManager::AccountSyncManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->autoRefreshTimer = new QTimer(this);
    connect(d->autoRefreshTimer, &QTimer::timeout, this, &AccountSyncManager::onAutoRefreshTimer);
    LOG_DEBUG("AccountSyncManager created");
}

AccountSyncManager::~AccountSyncManager()
{
    shutdown();
    LOG_DEBUG("AccountSyncManager destroyed");
}

bool AccountSyncManager::initialize()
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        return true;
    }

    d->initialized = true;
    LOG_INFO("AccountSyncManager initialized");
    return true;
}

void AccountSyncManager::shutdown()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    if (d->autoRefreshTimer) {
        d->autoRefreshTimer->stop();
    }

    d->ctpService = nullptr;
    d->initialized = false;
    LOG_INFO("AccountSyncManager shutdown");
}

void AccountSyncManager::setCtpService(CTP::CTPService* ctpService)
{
    QMutexLocker locker(&d->mutex);

    // 断开旧连接
    if (d->ctpService) {
        disconnect(d->ctpService, nullptr, this, nullptr);
    }

    d->ctpService = ctpService;

    // 建立新连接
    if (d->ctpService) {
        connect(d->ctpService, &CTP::CTPService::accountInfoReceived,
                this, &AccountSyncManager::onCtpAccountInfo);
        connect(d->ctpService, &CTP::CTPService::positionReceived,
                this, &AccountSyncManager::onCtpPositionReceived);

        LOG_INFO("CTPService connected to AccountSyncManager");
    }
}

void AccountSyncManager::refreshAccount()
{
    QMutexLocker locker(&d->mutex);

    if (d->ctpService) {
        // 调用CTP查询账户
        // 实际实现需要调用 CTPService::queryAccount()
        // d->ctpService->queryAccount();
        
        // 模拟返回账户数据
        AccountInfo account;
        account.available = 1000000.0;
        account.balance = 1200000.0;
        account.margin = 200000.0;
        account.updateTime = QDateTime::currentDateTime();
        
        d->accountInfo = account;
        emit accountUpdated(account);
        LOG_INFO("Account refreshed");
    }
}

void AccountSyncManager::refreshPositions()
{
    QMutexLocker locker(&d->mutex);

    if (d->ctpService) {
        // 调用CTP查询持仓
        // 实际实现需要调用 CTPService::queryPositions()
        // d->ctpService->queryPositions();
        
        LOG_INFO("Positions refresh requested");
    }
}

void AccountSyncManager::setAutoRefreshInterval(int intervalMs)
{
    if (intervalMs > 0) {
        d->autoRefreshTimer->start(intervalMs);
        LOG_INFO(QString("Auto refresh enabled, interval: %1ms").arg(intervalMs));
    } else {
        d->autoRefreshTimer->stop();
        LOG_INFO("Auto refresh disabled");
    }
}

AccountInfo AccountSyncManager::getAccountInfo() const
{
    QMutexLocker locker(&d->mutex);
    return d->accountInfo;
}

void AccountSyncManager::onAutoRefreshTimer()
{
    refreshAccount();
    refreshPositions();
}

void AccountSyncManager::onCtpAccountInfo(double available, double balance)
{
    QMutexLocker locker(&d->mutex);

    d->accountInfo.available = available;
    d->accountInfo.balance = balance;

    // 计算冻结保证金
    double totalMargin = PositionManager::instance().getTotalMargin();
    d->accountInfo.frozenMargin = totalMargin;
    d->accountInfo.margin = totalMargin;

    // 发送信号
    emit accountUpdated(d->accountInfo);

    LOG_DEBUG(QString("Account updated: available=%1, balance=%2")
        .arg(available).arg(balance));
}

void AccountSyncManager::onCtpPositionReceived(const QString& instrument, int longPos, int shortPos)
{
    // 更新 PositionManager
    if (longPos > 0) {
        PositionInfo pos;
        pos.instrumentId = instrument;
        pos.direction = PositionDirection::Long;
        pos.volume = longPos;
        PositionManager::instance().updatePosition(pos);
    }

    if (shortPos > 0) {
        PositionInfo pos;
        pos.instrumentId = instrument;
        pos.direction = PositionDirection::Short;
        pos.volume = shortPos;
        PositionManager::instance().updatePosition(pos);
    }

    // 发送信号
    emit positionUpdated(instrument, longPos, shortPos);

    LOG_DEBUG(QString("Position received: %1, long=%2, short=%3")
        .arg(instrument).arg(longPos).arg(shortPos));
}
