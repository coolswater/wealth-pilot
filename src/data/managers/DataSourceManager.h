/**
 * @file DataSourceManager.h
 * @brief 数据源管理器 - 管理多数据源切换和故障转移
 *
 * @details 功能：
 * - 多数据源管理：支持配置多个数据源
 * - 自动切换：当主数据源失败时自动切换到备用源
 * - 故障转移：记录故障状态，自动恢复
 * - 健康检查：定期检查数据源可用性
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATASOURCEMANAGER_H
#define DATASOURCEMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QDateTime>
#include <functional>
#include <memory>

/**
 * @brief 数据源状态
 */
enum class DataSourceStatus {
    Healthy,    ///< 健康
    Degraded,   ///< 降级
    Failed,     ///< 失败
    Unknown     ///< 未知
};

/**
 * @brief 数据源信息
 */
struct DataSourceInfo {
    QString name;               ///< 数据源名称
    QString type;               ///< 数据源类型（stock, forex, crypto, fund）
    int priority = 0;           ///< 优先级（数字越小优先级越高）
    DataSourceStatus status = DataSourceStatus::Unknown;
    int failCount = 0;          ///< 连续失败次数
    int successCount = 0;       ///< 连续成功次数
    QDateTime lastSuccess;      ///< 最后成功时间
    QDateTime lastFailure;      ///< 最后失败时间
    int latencyMs = 0;          ///< 平均延迟（毫秒）
};

/**
 * @brief 数据源管理器
 */
class DataSourceManager : public QObject
{
    Q_OBJECT

public:
    static DataSourceManager* instance();

    /**
     * @brief 注册数据源
     * @param type 数据源类型
     * @param name 数据源名称
     * @param priority 优先级
     */
    void registerDataSource(const QString& type, const QString& name, int priority = 0);

    /**
     * @brief 获取最佳数据源
     * @param type 数据源类型
     * @return 最佳数据源名称
     */
    QString getBestSource(const QString& type) const;

    /**
     * @brief 报告成功
     * @param type 数据源类型
     * @param name 数据源名称
     * @param latencyMs 响应延迟
     */
    void reportSuccess(const QString& type, const QString& name, int latencyMs = 0);

    /**
     * @brief 报告失败
     * @param type 数据源类型
     * @param name 数据源名称
     */
    void reportFailure(const QString& type, const QString& name);

    /**
     * @brief 获取数据源信息
     */
    DataSourceInfo getSourceInfo(const QString& type, const QString& name) const;

    /**
     * @brief 获取所有数据源状态
     */
    QHash<QString, DataSourceInfo> getAllSources(const QString& type) const;

    /**
     * @brief 启用/禁用数据源
     */
    void setSourceEnabled(const QString& type, const QString& name, bool enabled);

    /**
     * @brief 设置故障阈值
     * @param threshold 连续失败多少次后切换
     */
    void setFailureThreshold(int threshold);

    /**
     * @brief 设置恢复阈值
     * @param threshold 连续成功多少次后恢复
     */
    void setRecoveryThreshold(int threshold);

    /**
     * @brief 启动健康检查
     * @param intervalMs 检查间隔
     */
    void startHealthCheck(int intervalMs = 300000);

    /**
     * @brief 停止健康检查
     */
    void stopHealthCheck();

signals:
    /**
     * @brief 数据源切换信号
     */
    void sourceSwitched(const QString& type, const QString& fromSource, const QString& toSource);

    /**
     * @brief 数据源状态变化信号
     */
    void sourceStatusChanged(const QString& type, const QString& name, DataSourceStatus status);

    /**
     * @brief 所有数据源失败信号
     */
    void allSourcesFailed(const QString& type);

private:
    DataSourceManager(QObject* parent = nullptr);
    ~DataSourceManager() override;

    // 禁止拷贝
    DataSourceManager(const DataSourceManager&) = delete;
    DataSourceManager& operator=(const DataSourceManager&) = delete;

    void updateSourceStatus(const QString& type, const QString& name);
    void checkHealth();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // DATASOURCEMANAGER_H
