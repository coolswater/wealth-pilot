/**
 * @file AccountSyncService.cpp
 * @brief 账户同步服务实现
 */

#include "AccountSyncService.h"
#include "MultiAccountManager.h"
#include "utils/Logger.h"
#include <QElapsedTimer>

namespace WealthPilot {

AccountSyncService& AccountSyncService::instance()
{
    static AccountSyncService inst;
    return inst;
}

AccountSyncService::AccountSyncService()
    : m_autoSyncTimer(new QTimer(this))
{
    connect(m_autoSyncTimer, &QTimer::timeout, this, &AccountSyncService::onAutoSyncTimer);
}

AccountSyncService::~AccountSyncService()
{
    stopAutoSync();
}

void AccountSyncService::initialize(const AccountSyncConfig& config)
{
    m_config = config;
    m_initialized = true;
    LOG_INFO("AccountSyncService initialized");
}

void AccountSyncService::startAutoSync()
{
    if (!m_initialized) {
        LOG_WARNING("AccountSyncService not initialized");
        return;
    }

    if (!m_autoSyncTimer->isActive()) {
        m_autoSyncTimer->start(m_config.autoSyncInterval);
        LOG_INFO(QString("Auto sync started, interval: %1ms").arg(m_config.autoSyncInterval));

        // 启动时同步
        if (m_config.syncOnStartup) {
            syncAllAccounts();
        }
    }
}

void AccountSyncService::stopAutoSync()
{
    if (m_autoSyncTimer->isActive()) {
        m_autoSyncTimer->stop();
        LOG_INFO("Auto sync stopped");
    }
}

void AccountSyncService::syncAccount(const QString& accountId)
{
    if (m_syncStatuses[accountId] == SyncStatus::Syncing) {
        LOG_DEBUG(QString("Account %1 is already syncing").arg(accountId));
        return;
    }

    performSync(accountId);
}

void AccountSyncService::syncAllAccounts()
{
    auto accounts = MultiAccountManager::instance()->getAllAccounts();
    LOG_INFO(QString("Syncing %1 accounts").arg(accounts.size()));

    for (const auto& account : accounts) {
        if (account.enabled) {
            syncAccount(account.id);
        }
    }
}

SyncStatus AccountSyncService::getSyncStatus(const QString& accountId) const
{
    return m_syncStatuses.value(accountId, SyncStatus::Idle);
}

SyncResult AccountSyncService::getSyncResult(const QString& accountId) const
{
    return m_syncResults.value(accountId);
}

QHash<QString, SyncResult> AccountSyncService::getAllSyncResults() const
{
    return m_syncResults;
}

void AccountSyncService::updateConfig(const AccountSyncConfig& config)
{
    m_config = config;

    // 更新定时器间隔
    if (m_autoSyncTimer->isActive()) {
        m_autoSyncTimer->setInterval(m_config.autoSyncInterval);
    }

    LOG_INFO("AccountSyncService config updated");
}

void AccountSyncService::onAutoSyncTimer()
{
    LOG_DEBUG("Auto sync triggered");
    syncAllAccounts();
}

void AccountSyncService::performSync(const QString& accountId)
{
    m_syncStatuses[accountId] = SyncStatus::Syncing;
    emit syncStarted(accountId);

    QElapsedTimer timer;
    timer.start();

    SyncResult result;
    result.accountId = accountId;
    result.timestamp = QDateTime::currentDateTime();

    // 获取账户信息
    auto accountInfo = MultiAccountManager::instance()->getAccount(accountId);
    if (accountInfo.id.isEmpty()) {
        result.status = SyncStatus::Failed;
        result.errorMessage = "Account not found";
        result.syncTime = timer.elapsed();
        m_syncStatuses[accountId] = SyncStatus::Failed;
        m_syncResults[accountId] = result;
        emit syncCompleted(accountId, result);
        LOG_ERROR(QString("Sync failed: account %1 not found").arg(accountId));
        return;
    }

    // 模拟同步过程（实际应用中需要调用券商 API）
    bool syncSuccess = true;
    QString errorMsg;

    try {
        // TODO: 实际的同步逻辑
        // 1. 同步余额
        if (m_config.syncBalance) {
            emit syncProgress(accountId, 25);
            // 调用券商 API 获取余额
        }

        // 2. 同步持仓
        if (m_config.syncPositions) {
            emit syncProgress(accountId, 50);
            // 调用券商 API 获取持仓
        }

        // 3. 同步订单
        if (m_config.syncOrders) {
            emit syncProgress(accountId, 75);
            // 调用券商 API 获取订单
        }

        // 4. 同步成交
        if (m_config.syncTrades) {
            emit syncProgress(accountId, 100);
            // 调用券商 API 获取成交
        }

    } catch (const std::exception& e) {
        syncSuccess = false;
        errorMsg = QString::fromStdString(e.what());
    }

    result.syncTime = timer.elapsed();
    result.status = syncSuccess ? SyncStatus::Success : SyncStatus::Failed;
    result.errorMessage = errorMsg;

    m_syncStatuses[accountId] = result.status;
    m_syncResults[accountId] = result;

    emit syncCompleted(accountId, result);

    LOG_INFO(QString("Sync %1 for account %2, time: %3ms")
            .arg(syncSuccess ? "completed" : "failed")
            .arg(accountId)
            .arg(result.syncTime));
}

} // namespace WealthPilot