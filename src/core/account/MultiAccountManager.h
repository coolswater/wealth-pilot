/**
 * @file MultiAccountManager.h
 * @brief 多账户管理器 - 企业级功能
 *
 * @details 功能：
 * - 多账户管理
 * - 账户切换
 * - 统一视图
 * - 账户分组
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef MULTIACCOUNTMANAGER_H
#define MULTIACCOUNTMANAGER_H

#include <QObject>
#include <QVector>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QVariant>

/**
 * @brief 账户类型
 */
enum class AccountType {
    Stock,              ///< 股票账户
    Futures,            ///< 期货账户
    Forex,              ///< 外汇账户
    Crypto,             ///< 数字货币账户
    Fund                ///< 基金账户
};

/**
 * @brief 多账户信息（账户管理）
 */
struct MultiAccountInfo {
    QString id;                 ///< 账户ID
    QString name;               ///< 账户名称
    AccountType type;           ///< 账户类型
    QString broker;             ///< 券商/经纪商
    QString accountNumber;      ///< 账号
    double balance = 0.0;       ///< 余额
    double available = 0.0;     ///< 可用资金
    double marketValue = 0.0;   ///< 持仓市值
    double totalAsset = 0.0;    ///< 总资产
    double profit = 0.0;        ///< 盈亏
    double profitPercent = 0.0; ///< 盈亏比例
    QDateTime updateTime;       ///< 更新时间
    QString group;              ///< 分组
    bool enabled = true;        ///< 是否启用
    QVariantMap config;         ///< 配置信息
};

/**
 * @brief 账户组
 */
struct AccountGroup {
    QString id;                 ///< 组ID
    QString name;               ///< 组名
    QStringList accountIds;     ///< 账户ID列表
    double totalAsset = 0.0;    ///< 总资产
    double totalProfit = 0.0;   ///< 总盈亏
};

/**
 * @brief 多账户管理器
 *
 * 提供企业级多账户管理：
 * - 多账户管理
 * - 统一视图
 * - 账户分组
 * - 快速切换
 */
class MultiAccountManager : public QObject {
    Q_OBJECT

public:
    static MultiAccountManager* instance();

    // ========== 账户管理 ==========

    /**
     * @brief 添加账户
     */
    bool addAccount(const MultiAccountInfo& account);

    /**
     * @brief 更新账户
     */
    bool updateAccount(const MultiAccountInfo& account);

    /**
     * @brief 删除账户
     */
    bool removeAccount(const QString& accountId);

    /**
     * @brief 获取账户
     */
    MultiAccountInfo getAccount(const QString& accountId) const;

    /**
     * @brief 获取所有账户
     */
    QVector<MultiAccountInfo> getAllAccounts() const;

    /**
     * @brief 获取账户列表（按类型）
     */
    QVector<MultiAccountInfo> getAccountsByType(AccountType type) const;

    /**
     * @brief 获取账户数量
     */
    int getAccountCount() const { return m_accounts.size(); }

    // ========== 当前账户 ==========

    /**
     * @brief 设置当前账户
     */
    void setCurrentAccount(const QString& accountId);

    /**
     * @brief 获取当前账户
     */
    QString currentAccountId() const { return m_currentAccountId; }

    /**
     * @brief 获取当前账户信息
     */
    MultiAccountInfo currentAccount() const;

    // ========== 账户分组 ==========

    /**
     * @brief 创建分组
     */
    bool createGroup(const QString& groupId, const QString& name);

    /**
     * @brief 删除分组
     */
    bool deleteGroup(const QString& groupId);

    /**
     * @brief 添加账户到分组
     */
    bool addToGroup(const QString& accountId, const QString& groupId);

    /**
     * @brief 从分组移除账户
     */
    bool removeFromGroup(const QString& accountId, const QString& groupId);

    /**
     * @brief 获取分组
     */
    AccountGroup getGroup(const QString& groupId) const;

    /**
     * @brief 获取所有分组
     */
    QVector<AccountGroup> getAllGroups() const;

    // ========== 统计 ==========

    /**
     * @brief 计算总资产
     */
    double calculateTotalAsset() const;

    /**
     * @brief 计算总盈亏
     */
    double calculateTotalProfit() const;

    /**
     * @brief 获取账户统计
     */
    struct AccountStats {
        int totalAccounts = 0;
        double totalAsset = 0.0;
        double totalProfit = 0.0;
        double profitPercent = 0.0;
        int profitAccounts = 0;
        int lossAccounts = 0;
    };
    AccountStats getStats() const;

    // ========== 数据同步 ==========

    /**
     * @brief 刷新账户数据
     */
    void refreshAccount(const QString& accountId);

    /**
     * @brief 刷新所有账户
     */
    void refreshAllAccounts();

    /**
     * @brief 设置自动刷新
     */
    void setAutoRefresh(bool enabled, int intervalMs = 30000);

signals:
    /**
     * @brief 账户添加信号
     */
    void accountAdded(const MultiAccountInfo& account);

    /**
     * @brief 账户更新信号
     */
    void accountUpdated(const MultiAccountInfo& account);

    /**
     * @brief 账户删除信号
     */
    void accountRemoved(const QString& accountId);

    /**
     * @brief 当前账户变化
     */
    void currentAccountChanged(const QString& accountId);

    /**
     * @brief 分组变化
     */
    void groupChanged(const QString& groupId);

    /**
     * @brief 统计更新
     */
    void statsUpdated(const AccountStats& stats);

private:
    explicit MultiAccountManager(QObject* parent = nullptr);
    ~MultiAccountManager() override = default;

    void updateGroupStats(const QString& groupId);

    QHash<QString, MultiAccountInfo> m_accounts;
    QHash<QString, AccountGroup> m_groups;
    QString m_currentAccountId;

    bool m_autoRefresh = false;
    int m_refreshInterval = 30000;
};

#endif // MULTIACCOUNTMANAGER_H