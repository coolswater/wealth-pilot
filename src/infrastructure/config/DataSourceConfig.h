/**
 * @file DataSourceConfig.h
 * @brief 数据源配置加载器
 *
 * @details 功能：
 * - 从 JSON 文件加载数据源配置
 * - 支持多数据源配置
 * - 支持动态切换数据源
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATASOURCECONFIG_H
#define DATASOURCECONFIG_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QVector>

namespace WealthPilot {

/**
 * @brief 单个数据源配置
 */
struct DataSourceConfigItem {
    QString name;           ///< 数据源名称
    QString type;           ///< 数据源类型 (stock, forex, crypto, fund)
    int priority = 0;       ///< 优先级
    bool enabled = true;    ///< 是否启用
    QString apiUrl;         ///< API URL
    QString wsUrl;          ///< WebSocket URL (可选)
    QString description;    ///< 描述
};

/**
 * @brief 数据源全局设置
 */
struct DataSourceSettings {
    int healthCheckInterval = 300000;   ///< 健康检查间隔 (ms)
    int maxFailCount = 3;               ///< 最大失败次数
    int recoveryThreshold = 5;          ///< 恢复阈值
    int requestTimeout = 10000;         ///< 请求超时 (ms)
    int retryCount = 2;                 ///< 重试次数
};

/**
 * @brief 数据源配置管理器
 */
class DataSourceConfig : public QObject
{
    Q_OBJECT

public:
    static DataSourceConfig& instance();

    /**
     * @brief 从文件加载配置
     * @param filePath 配置文件路径
     * @return 是否加载成功
     */
    bool loadFromFile(const QString& filePath);

    /**
     * @brief 获取指定类型的数据源列表
     * @param type 数据源类型
     * @return 数据源配置列表
     */
    QVector<DataSourceConfigItem> getDataSources(const QString& type) const;

    /**
     * @brief 获取最佳数据源
     * @param type 数据源类型
     * @return 最佳数据源配置
     */
    DataSourceConfigItem getBestDataSource(const QString& type) const;

    /**
     * @brief 获取全局设置
     */
    DataSourceSettings getSettings() const { return m_settings; }

    /**
     * @brief 重新加载配置
     */
    bool reload() { return loadFromFile(m_lastFilePath); }

signals:
    void configLoaded();
    void configError(const QString& error);

private:
    DataSourceConfig() = default;
    ~DataSourceConfig() = default;
    DataSourceConfig(const DataSourceConfig&) = delete;
    DataSourceConfig& operator=(const DataSourceConfig&) = delete;

    QString m_lastFilePath;
    QJsonObject m_config;
    DataSourceSettings m_settings;
};

} // namespace WealthPilot

#endif // DATASOURCECONFIG_H