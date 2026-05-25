/**
 * @file EnvironmentConfig.h
 * @brief 环境配置管理器 - 支持开发/测试/生产环境切换
 *
 * @details 主要功能：
 * - 环境配置管理
 * - 环境切换与验证
 * - 配置继承和覆盖
 * - 热更新支持
 * - 环境优化与用户配置
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef ENVIRONMENTCONFIG_H
#define ENVIRONMENTCONFIG_H

#include "shared/base/Singleton.h"
#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QMutex>
#include <QSettings>
#include <memory>

/**
 * @brief 环境类型
 */
enum class Environment {
    Development,    ///< 开发环境
    Testing,        ///< 测试环境
    Staging,        ///< 预发布环境
    Production      ///< 生产环境
};

/**
 * @brief 环境配置结构
 */
struct EnvironmentSettings {
    QString name;                       ///< 环境名称
    QString apiUrl;                     ///< API服务URL
    QString ctpMarketFront;            ///< CTP行情前置地址
    QString ctpTradeFront;             ///< CTP交易前置地址
    QString ctpBrokerId;               ///< CTP经纪商代码
    QString aiProvider;                 ///< AI服务提供商
    QString aiModel;                    ///< AI模型
    int requestTimeout = 30000;         ///< 请求超时（毫秒）
    int retryCount = 3;                 ///< 重试次数
    bool enableDebugLog = true;         ///< 是否启用调试日志
    bool enableCache = true;            ///< 是否启用缓存
    int cacheExpireTime = 300;          ///< 缓存过期时间（秒）
    
    EnvironmentSettings() = default;
};

/**
 * @brief 环境配置管理器
 */
class EnvironmentConfig : public QObject, public Singleton<EnvironmentConfig>
{
    Q_OBJECT
    friend class Singleton<EnvironmentConfig>;

public:
    /**
     * @brief 获取当前环境
     */
    Environment currentEnvironment() const;

    /**
     * @brief 设置当前环境
     * @param env 目标环境
     * @return 是否切换成功
     */
    bool setEnvironment(Environment env);

    /**
     * @brief 获取当前环境配置
     */
    const EnvironmentSettings* currentSettings() const;

    /**
     * @brief 获取指定环境配置
     */
    std::optional<EnvironmentSettings> getSettings(Environment env) const;

    /**
     * @brief 设置环境配置
     */
    void setSettings(Environment env, const EnvironmentSettings& settings);

    /**
     * @brief 获取配置值
     */
    QVariant get(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /**
     * @brief 设置配置值
     */
    void set(const QString& key, const QVariant& value);

    /**
     * @brief 重新加载配置
     */
    void reload();

    /**
     * @brief 导出配置
     */
    bool exportConfig(const QString& filePath);

    /**
     * @brief 导入配置
     */
    bool importConfig(const QString& filePath);

    /**
     * @brief 获取环境名称
     */
    static QString environmentName(Environment env);

    /**
     * @brief 从名称解析环境
     */
    static Environment environmentFromName(const QString& name);

signals:
    /**
     * @brief 环境切换信号
     */
    void environmentChanged(Environment newEnv);

    /**
     * @brief 配置更新信号
     */
    void configUpdated(const QString& key);

private:
    EnvironmentConfig();
    ~EnvironmentConfig();

    void loadEnvironments();
    void updateCache();
    void saveSettings();

    Environment m_currentEnv;                              ///< 当前环境
    QMap<Environment, EnvironmentSettings> m_environments; ///< 所有环境配置
    std::unique_ptr<QSettings> m_settings;                 ///< QSettings实例
    QMap<QString, QVariant> m_configCache;                 ///< 配置缓存
    mutable QMutex m_mutex;                                ///< 线程安全锁
    
    static QMap<Environment, QString> s_envNames;          ///< 环境名称映射
};

#endif // ENVIRONMENTCONFIG_H