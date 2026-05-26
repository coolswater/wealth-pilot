/**
 * @file MultiAccountManager.cpp
 * @brief 多账户管理器实现
 */

#include "MultiAccountManager.h"
#include "data/DataStorageService.h"
#include "data/datahub/DataHub.h"
#include "shared/utils/Logger.h"
#include <QUuid>
#include <QTimer>
#include <QSettings>
#include <algorithm>

MultiAccountManager* MultiAccountManager::instance()
{
    static MultiAccountManager* inst = new MultiAccountManager();
    return inst;
}

MultiAccountManager::MultiAccountManager(QObject* parent)
    : QObject(parent)
{
    // 加载保存的账户
    QSettings settings("WealthPilot", "Accounts");
    QString currentId = settings.value("currentAccount").toString();
    if (!currentId.isEmpty()) {
        m_currentAccountId = currentId;
    }

    LOG_INFO("MultiAccountManager initialized");
}

bool MultiAccountManager::addAccount(const MultiAccountInfo& account)
{
    if (m_accounts.contains(account.id)) {
        LOG_WARNING(QString("Account already exists: %1").arg(account.id));
        return false;
    }

    MultiAccountInfo newAccount = account;
    if (newAccount.id.isEmpty()) {
        newAccount.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    newAccount.updateTime = QDateTime::currentDateTime();

    m_accounts[newAccount.id] = newAccount;

    emit accountAdded(newAccount);
    LOG_INFO(QString("Account added: %1 (%2)").arg(newAccount.name).arg(newAccount.id));

    return true;
}

bool MultiAccountManager::updateAccount(const MultiAccountInfo& account)
{
    if (!m_accounts.contains(account.id)) {
        LOG_WARNING(QString("Account not found: %1").arg(account.id));
        return false;
    }

    MultiAccountInfo updated = account;
    updated.updateTime = QDateTime::currentDateTime();
    m_accounts[account.id] = updated;

    emit accountUpdated(updated);
    LOG_INFO(QString("Account updated: %1").arg(account.id));

    return true;
}

bool MultiAccountManager::removeAccount(const QString& accountId)
{
    if (!m_accounts.contains(accountId)) {
        return false;
    }

    // 从所有分组中移除
    for (auto it = m_groups.begin(); it != m_groups.end(); ++it) {
        it.value().accountIds.removeAll(accountId);
        updateGroupStats(it.key());
    }

    m_accounts.remove(accountId);

    // 如果是当前账户，切换到第一个可用账户
    if (m_currentAccountId == accountId) {
        if (!m_accounts.isEmpty()) {
            setCurrentAccount(m_accounts.begin().key());
        } else {
            m_currentAccountId.clear();
        }
    }

    emit accountRemoved(accountId);
    LOG_INFO(QString("Account removed: %1").arg(accountId));

    return true;
}

MultiAccountInfo MultiAccountManager::getAccount(const QString& accountId) const
{
    return m_accounts.value(accountId);
}

QVector<MultiAccountInfo> MultiAccountManager::getAllAccounts() const
{
    return m_accounts.values();
}

QVector<MultiAccountInfo> MultiAccountManager::getAccountsByType(AccountType type) const
{
    QVector<MultiAccountInfo> result;
    for (const auto& account : m_accounts) {
        if (account.type == type) {
            result.append(account);
        }
    }
    return result;
}

void MultiAccountManager::setCurrentAccount(const QString& accountId)
{
    if (!m_accounts.contains(accountId)) {
        LOG_WARNING(QString("Account not found: %1").arg(accountId));
        return;
    }

    m_currentAccountId = accountId;

    // 保存当前账户
    QSettings settings("WealthPilot", "Accounts");
    settings.setValue("currentAccount", accountId);

    emit currentAccountChanged(accountId);
    LOG_INFO(QString("Current account changed: %1").arg(accountId));
}

MultiAccountInfo MultiAccountManager::currentAccount() const
{
    return m_accounts.value(m_currentAccountId);
}

bool MultiAccountManager::createGroup(const QString& groupId, const QString& name)
{
    QString id = groupId.isEmpty() ?
        QUuid::createUuid().toString(QUuid::WithoutBraces) : groupId;

    if (m_groups.contains(id)) {
        LOG_WARNING(QString("Group already exists: %1").arg(id));
        return false;
    }

    AccountGroup group;
    group.id = id;
    group.name = name;
    m_groups[id] = group;

    emit groupChanged(id);
    LOG_INFO(QString("Group created: %1 (%2)").arg(name).arg(id));

    return true;
}

bool MultiAccountManager::deleteGroup(const QString& groupId)
{
    if (!m_groups.contains(groupId)) {
        return false;
    }

    // 清除账户的分组标记
    for (const QString& accountId : m_groups[groupId].accountIds) {
        if (m_accounts.contains(accountId)) {
            m_accounts[accountId].group.clear();
        }
    }

    m_groups.remove(groupId);

    emit groupChanged(groupId);
    LOG_INFO(QString("Group deleted: %1").arg(groupId));

    return true;
}

bool MultiAccountManager::addToGroup(const QString& accountId, const QString& groupId)
{
    if (!m_accounts.contains(accountId) || !m_groups.contains(groupId)) {
        return false;
    }

    // 从旧分组移除
    QString oldGroup = m_accounts[accountId].group;
    if (!oldGroup.isEmpty() && m_groups.contains(oldGroup)) {
        m_groups[oldGroup].accountIds.removeAll(accountId);
        updateGroupStats(oldGroup);
    }

    // 添加到新分组
    m_groups[groupId].accountIds.append(accountId);
    m_accounts[accountId].group = groupId;
    updateGroupStats(groupId);

    emit groupChanged(groupId);
    LOG_INFO(QString("Account %1 added to group %2").arg(accountId).arg(groupId));

    return true;
}

bool MultiAccountManager::removeFromGroup(const QString& accountId, const QString& groupId)
{
    if (!m_groups.contains(groupId)) {
        return false;
    }

    m_groups[groupId].accountIds.removeAll(accountId);
    if (m_accounts.contains(accountId)) {
        m_accounts[accountId].group.clear();
    }

    updateGroupStats(groupId);

    emit groupChanged(groupId);
    LOG_INFO(QString("Account %1 removed from group %2").arg(accountId).arg(groupId));

    return true;
}

AccountGroup MultiAccountManager::getGroup(const QString& groupId) const
{
    return m_groups.value(groupId);
}

QVector<AccountGroup> MultiAccountManager::getAllGroups() const
{
    return m_groups.values();
}

double MultiAccountManager::calculateTotalAsset() const
{
    double total = 0;
    for (const auto& account : m_accounts) {
        total += account.totalAsset;
    }
    return total;
}

double MultiAccountManager::calculateTotalProfit() const
{
    double total = 0;
    for (const auto& account : m_accounts) {
        total += account.profit;
    }
    return total;
}

MultiAccountManager::AccountStats MultiAccountManager::getStats() const
{
    AccountStats stats;
    stats.totalAccounts = m_accounts.size();

    for (const auto& account : m_accounts) {
        stats.totalAsset += account.totalAsset;
        stats.totalProfit += account.profit;

        if (account.profit > 0) {
            stats.profitAccounts++;
        } else if (account.profit < 0) {
            stats.lossAccounts++;
        }
    }

    // 计算总盈亏比例（基于总资产）
    if (stats.totalAsset > stats.totalProfit) {
        stats.profitPercent = stats.totalProfit / (stats.totalAsset - stats.totalProfit);
    }

    // 不在 const 函数中 emit
    // emit statsUpdated(stats);
    return stats;
}

void MultiAccountManager::refreshAccount(const QString& accountId)
{
    if (!m_accounts.contains(accountId)) {
        return;
    }

    // 从数据源刷新账户数据
    MultiAccountInfo& account = m_accounts[accountId];
    
    auto* storage = DataStorageService::instance();
    AccountInfo accountData = storage->getAccountInfo(account.accountId);
    
    if (accountData.isValid()) {
        account.balance = accountData.totalAsset;
        account.available = accountData.availableCash;
        account.marketValue = accountData.positionValue;
        account.updateTime = QDateTime::currentDateTime();
    }

    emit accountUpdated(account);
    LOG_DEBUG(QString("Account refreshed: %1").arg(accountId));
}

void MultiAccountManager::refreshAllAccounts()
{
    for (const QString& accountId : m_accounts.keys()) {
        refreshAccount(accountId);
    }

    getStats();
    LOG_INFO("All accounts refreshed");
}

void MultiAccountManager::setAutoRefresh(bool enabled, int intervalMs)
{
    m_autoRefresh = enabled;
    m_refreshInterval = intervalMs;

    if (enabled) {
        // 使用 DataHub 统一调度，不再使用独立定时器
        auto* dataHub = DataHub::instance();
        if (dataHub) {
            dataHub->registerDataProducer("account_refresh", [this]() {
                refreshAllAccounts();
            }, intervalMs);
        }
        LOG_INFO(QString("Auto refresh enabled via DataHub, interval: %1ms").arg(intervalMs));
    } else {
        LOG_INFO("Auto refresh disabled");
    }
}

void MultiAccountManager::updateGroupStats(const QString& groupId)
{
    if (!m_groups.contains(groupId)) {
        return;
    }

    AccountGroup& group = m_groups[groupId];
    group.totalAsset = 0;
    group.totalProfit = 0;

    for (const QString& accountId : group.accountIds) {
        if (m_accounts.contains(accountId)) {
            group.totalAsset += m_accounts[accountId].totalAsset;
            group.totalProfit += m_accounts[accountId].profit;
        }
    }
}