/**
 * @file AccountSyncManager.h
 * @brief 账户同步管理器 - 同步CTP账户数据到UI
 *
 * @details 功能：
 * - 监听CTP账户回报
 * - 更新TradingService账户信息
 * - 更新UI显示
 * - 定时刷新机制
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ACCOUNTSYNCMANAGER_H
#define ACCOUNTSYNCMANAGER_H

#include <QObject>
#include <QTimer>
#include <memory>
#include "trading/TradingTypes.h"

// 前向声明
namespace CTP {
    class CTPService;
}

/**
 * @brief 账户同步管理器
 */
class AccountSyncManager : public QObject
{
    Q_OBJECT

public:
    static AccountSyncManager& instance();

    bool initialize();
    void shutdown();

    void setCtpService(CTP::CTPService* ctpService);

    /**
     * @brief 请求刷新账户数据
     */
    void refreshAccount();

    /**
     * @brief 请求刷新持仓数据
     */
    void refreshPositions();

    /**
     * @brief 设置自动刷新间隔
     * @param intervalMs 间隔毫秒（0表示禁用）
     */
    void setAutoRefreshInterval(int intervalMs);

    /**
     * @brief 获取当前账户信息
     */
    AccountInfo getAccountInfo() const;

signals:
    void accountUpdated(const AccountInfo& account);
    void positionUpdated(const QString& instrumentId, int longPos, int shortPos);
    void syncError(const QString& error);

private slots:
    void onAutoRefreshTimer();
    void onCtpAccountInfo(double available, double balance);
    void onCtpPositionReceived(const QString& instrument, int longPos, int shortPos);

private:
    AccountSyncManager(QObject* parent = nullptr);
    ~AccountSyncManager() override;
    Q_DISABLE_COPY(AccountSyncManager)

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // ACCOUNTSYNCMANAGER_H
