/**
 * @file AccountSyncService.h
 * @brief 账户同步服务
 *
 * @details 功能：
 * - 多账户数据同步
 * - 定期更新账户状态
 * - 支持手动和自动同步
 * - 同步状态监控
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ACCOUNTSYNCSERVICE_H
#define ACCOUNTSYNCSERVICE_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QString>
#include <QDateTime>

namespace WealthPilot {

/**
 * @brief 同步状态
 */
enum class SyncStatus {
    Idle,       ///< 空闲
    Syncing,    ///< 同步中
    Success,    ///< 成功
    Failed,     ///< 失败
    Timeout     ///< 超时
};

/**
 * @brief 同步结果
 */
struct SyncResult {
    QString accountId;      ///< 账户ID
    SyncStatus status;      ///< 同步状态
    QString errorMessage;   ///< 错误信息
    qint64 syncTime = 0;    ///< 同步耗时 (ms)
    QDateTime timestamp;    ///< 时间戳
};

/**
 * @brief 账户同步配置
 */
struct AccountSyncConfig {
    int autoSyncInterval = 300000;      ///< 自动同步间隔 (ms), 默认5分钟
    int syncTimeout = 30000;           ///< 同步超时 (ms)
    int retryCount = 3;                ///< 重试次数
    int retryDelay = 1000;             ///< 重试延迟 (ms)
    bool syncOnStartup = true;         ///< 启动时同步
    bool syncBalance = true;           ///< 同步余额
    bool syncPositions = true;         ///< 同步持仓
    bool syncOrders = true;            ///< 同步订单
    bool syncTrades = true;            ///< 同步成交
};

/**
 * @brief 账户同步服务
 */
class AccountSyncService : public QObject
{
    Q_OBJECT

public:
    static AccountSyncService& instance();

    /**
     * @brief 初始化服务
     */
    void initialize(const AccountSyncConfig& config);

    /**
     * @brief 启动自动同步
     */
    void startAutoSync();

    /**
     * @brief 停止自动同步
     */
    void stopAutoSync();

    /**
     * @brief 同步单个账户
     * @param accountId 账户ID
     */
    void syncAccount(const QString& accountId);

    /**
     * @brief 同步所有账户
     */
    void syncAllAccounts();

    /**
     * @brief 获取同步状态
     */
    SyncStatus getSyncStatus(const QString& accountId) const;

    /**
     * @brief 获取同步结果
     */
    SyncResult getSyncResult(const QString& accountId) const;

    /**
     * @brief 获取所有同步结果
     */
    QHash<QString, SyncResult> getAllSyncResults() const;

    /**
     * @brief 获取配置
     */
    AccountSyncConfig getConfig() const { return m_config; }

    /**
     * @brief 更新配置
     */
    void updateConfig(const AccountSyncConfig& config);

signals:
    /**
     * @brief 同步开始
     */
    void syncStarted(const QString& accountId);

    /**
     * @brief 同步完成
     */
    void syncCompleted(const QString& accountId, const SyncResult& result);

    /**
     * @brief 同步进度
     */
    void syncProgress(const QString& accountId, int progress);

    /**
     * @brief 所有账户同步完成
     */
    void allSyncCompleted(const QHash<QString, SyncResult>& results);

private slots:
    void onAutoSyncTimer();

private:
    AccountSyncService();
    ~AccountSyncService();
    AccountSyncService(const AccountSyncService&) = delete;
    AccountSyncService& operator=(const AccountSyncService&) = delete;

    void performSync(const QString& accountId);

    AccountSyncConfig m_config;
    QTimer* m_autoSyncTimer = nullptr;
    QHash<QString, SyncResult> m_syncResults;
    QHash<QString, SyncStatus> m_syncStatuses;
    bool m_initialized = false;
};

} // namespace WealthPilot

#endif // ACCOUNTSYNCSERVICE_H