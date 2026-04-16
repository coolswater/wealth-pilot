/**
 * @file EnvironmentConfig.cpp
 * @brief 多环境配置管理器实现
 */

#include "EnvironmentConfig.h"
#include "../utils/Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QElapsedTimer>

QMap<Environment, QString> EnvironmentConfig::s_envNames = {
    {Environment::Development, "development"},
    {Environment::Testing, "testing"},
    {Environment::Staging, "staging"},
    {Environment::Production, "production"}
};

EnvironmentConfig::EnvironmentConfig()
    : QObject(nullptr)
    , m_currentEnv(Environment::Development)
    , m_settings(new QSettings(
        QSettings::IniFormat,
        QSettings::UserScope,
        QCoreApplication::organizationName().isEmpty() ? "Hexd" : QCoreApplication::organizationName(),
        QCoreApplication::applicationName().isEmpty() ? "WealthPilot" : QCoreApplication::applicationName()
    ))
{
}

EnvironmentConfig::~EnvironmentConfig()
{
    if (m_settings) {
        m_settings->sync();
        delete m_settings;
    }
}

bool EnvironmentConfig::initialize()
{
    QElapsedTimer timer;
    timer.start();
    
    QMutexLocker locker(&m_mutex);
    
    // 加载环境配置
    loadEnvironments();
    
    // 加载当前环境
    QString envStr = m_settings->value("environment/current", "development").toString();
    if (envStr == "development") m_currentEnv = Environment::Development;
    else if (envStr == "testing") m_currentEnv = Environment::Testing;
    else if (envStr == "staging") m_currentEnv = Environment::Staging;
    else if (envStr == "production") m_currentEnv = Environment::Production;
    
    // 更新缓存
    updateCache();
    
    LOG_INFO(QString("EnvironmentConfig initialized in %1ms, current: %2")
        .arg(timer.elapsed()).arg(environmentName(m_currentEnv)));
    
    return true;
}

Environment EnvironmentConfig::currentEnvironment() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentEnv;
}

void EnvironmentConfig::setCurrentEnvironment(Environment env)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_currentEnv == env) {
        return;
    }
    
    m_currentEnv = env;
    
    // 保存设置
    m_settings->setValue("environment/current", environmentName(env));
    m_settings->sync();
    
    // 更新缓存
    updateCache();
    
    LOG_INFO(QString("Environment changed to: %1").arg(environmentName(env)));

    emit environmentChanged(env);
}

QString EnvironmentConfig::environmentName(Environment env) const
{
    return s_envNames.value(env, "unknown");
}

const EnvironmentSettings* EnvironmentConfig::currentSettings() const
{
    QMutexLocker locker(&m_mutex);
    auto it = m_environments.find(m_currentEnv);
    if (it != m_environments.end()) {
        return &it.value();
    }
    static const EnvironmentSettings empty;
    return &empty;
}

EnvironmentSettings EnvironmentConfig::getSettings(Environment env) const
{
    QMutexLocker locker(&m_mutex);
    return m_environments.value(env);
}

void EnvironmentConfig::setSettings(Environment env, const EnvironmentSettings& settings)
{
    QMutexLocker locker(&m_mutex);
    
    if (!validateSettings(settings)) {
        LOG_WARNING(QString("Invalid settings for environment: %1").arg(environmentName(env)));
        return;
    }
    
    m_environments[env] = settings;
    saveEnvironments();
    
    // 如果修改的是当前环境，更新缓存
    if (env == m_currentEnv) {
        updateCache();
    }
    
    LOG_INFO(QString("Settings updated for environment: %1").arg(environmentName(env)));
}

QVariant EnvironmentConfig::getValue(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&m_mutex);
    
    // 性能优化：先检查缓存
    if (m_configCache.contains(key)) {
        return m_configCache[key];
    }
    
    // 从设置中读取
    QVariant value = m_settings->value(key, defaultValue);
    if (value.isValid()) {
        // 缓存结果
        const_cast<EnvironmentConfig*>(this)->m_configCache[key] = value;
    }
    
    return value;
}

void EnvironmentConfig::setValue(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&m_mutex);
    
    m_settings->setValue(key, value);
    m_configCache[key] = value;
    
    LOG_DEBUG(QString("Config set: %1 = %2").arg(key, value.toString()));

    emit configUpdated(key);
}

void EnvironmentConfig::reload()
{
    QMutexLocker locker(&m_mutex);
    
    m_settings->sync();
    loadEnvironments();
    updateCache();
    
    LOG_INFO("Configuration reloaded");
}

bool EnvironmentConfig::exportConfig(const QString& filePath)
{
    QMutexLocker locker(&m_mutex);
    
    QJsonObject root;
    root["currentEnvironment"] = environmentName(m_currentEnv);
    
    // 导出所有环境配置
    QJsonObject envsObj;
    for (auto it = m_environments.begin(); it != m_environments.end(); ++it) {
        QJsonObject envObj;
        envObj["name"] = it.value().name;
        envObj["apiUrl"] = it.value().apiUrl;
        envObj["ctpMarketFront"] = it.value().ctpMarketFront;
        envObj["ctpTradeFront"] = it.value().ctpTradeFront;
        envObj["ctpBrokerId"] = it.value().ctpBrokerId;
        envObj["aiProvider"] = it.value().aiProvider;
        envObj["aiModel"] = it.value().aiModel;
        envObj["requestTimeout"] = it.value().requestTimeout;
        envObj["retryCount"] = it.value().retryCount;
        envObj["enableDebugLog"] = it.value().enableDebugLog;
        envObj["enableCache"] = it.value().enableCache;
        envObj["cacheExpireTime"] = it.value().cacheExpireTime;
        envsObj[environmentName(it.key())] = envObj;
    }
    root["environments"] = envsObj;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file for export: %1").arg(filePath));
        return false;
    }
    
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    
    LOG_INFO(QString("Configuration exported to: %1").arg(filePath));
    return true;
}

bool EnvironmentConfig::importConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open file for import: %1").arg(filePath));
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        LOG_ERROR("Invalid configuration file format");
        return false;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QJsonObject root = doc.object();
    
    // 导入环境配置
    if (root.contains("environments")) {
        QJsonObject envsObj = root["environments"].toObject();
        for (auto it = envsObj.begin(); it != envsObj.end(); ++it) {
            QString envName = it.key();
            Environment env;
            if (envName == "development") env = Environment::Development;
            else if (envName == "testing") env = Environment::Testing;
            else if (envName == "staging") env = Environment::Staging;
            else if (envName == "production") env = Environment::Production;
            else continue;
            
            QJsonObject envObj = it.value().toObject();
            EnvironmentSettings settings;
            settings.name = envObj["name"].toString();
            settings.apiUrl = envObj["apiUrl"].toString();
            settings.ctpMarketFront = envObj["ctpMarketFront"].toString();
            settings.ctpTradeFront = envObj["ctpTradeFront"].toString();
            settings.ctpBrokerId = envObj["ctpBrokerId"].toString();
            settings.aiProvider = envObj["aiProvider"].toString();
            settings.aiModel = envObj["aiModel"].toString();
            settings.requestTimeout = envObj["requestTimeout"].toInt(30000);
            settings.retryCount = envObj["retryCount"].toInt(3);
            settings.enableDebugLog = envObj["enableDebugLog"].toBool(true);
            settings.enableCache = envObj["enableCache"].toBool(true);
            settings.cacheExpireTime = envObj["cacheExpireTime"].toInt(300);
            
            if (validateSettings(settings)) {
                m_environments[env] = settings;
            }
        }
    }
    
    saveEnvironments();
    updateCache();
    
    LOG_INFO(QString("Configuration imported from: %1").arg(filePath));
    return true;
}

void EnvironmentConfig::loadEnvironments()
{
    // 开发环境
    EnvironmentSettings devSettings;
    devSettings.name = "Development";
    devSettings.apiUrl = "http://localhost:8080/api";
    devSettings.ctpMarketFront = "tcp://180.168.146.187:10131";
    devSettings.ctpTradeFront = "tcp://180.168.146.187:10130";
    devSettings.ctpBrokerId = "9999";
    devSettings.aiProvider = "openai";
    devSettings.aiModel = "gpt-4";
    devSettings.requestTimeout = 30000;
    devSettings.retryCount = 3;
    devSettings.enableDebugLog = true;
    devSettings.enableCache = true;
    devSettings.cacheExpireTime = 60;
    m_environments[Environment::Development] = devSettings;
    
    // 测试环境
    EnvironmentSettings testSettings;
    testSettings.name = "Testing";
    testSettings.apiUrl = "https://test-api.wealthpilot.com/api";
    testSettings.ctpMarketFront = "tcp://180.168.146.187:10131";
    testSettings.ctpTradeFront = "tcp://180.168.146.187:10130";
    testSettings.ctpBrokerId = "9999";
    testSettings.aiProvider = "openai";
    testSettings.aiModel = "gpt-4";
    testSettings.requestTimeout = 30000;
    testSettings.retryCount = 3;
    testSettings.enableDebugLog = true;
    testSettings.enableCache = true;
    testSettings.cacheExpireTime = 300;
    m_environments[Environment::Testing] = testSettings;
    
    // 生产环境
    EnvironmentSettings prodSettings;
    prodSettings.name = "Production";
    prodSettings.apiUrl = "https://api.wealthpilot.com/api";
    prodSettings.ctpMarketFront = "tcp://180.168.146.187:10131";
    prodSettings.ctpTradeFront = "tcp://180.168.146.187:10130";
    prodSettings.ctpBrokerId = "9999";
    prodSettings.aiProvider = "openai";
    prodSettings.aiModel = "gpt-4";
    prodSettings.requestTimeout = 15000;
    prodSettings.retryCount = 5;
    prodSettings.enableDebugLog = false;
    prodSettings.enableCache = true;
    prodSettings.cacheExpireTime = 600;
    m_environments[Environment::Production] = prodSettings;
    
    // 从配置文件加载覆盖
    m_settings->beginGroup("environments");
    for (const QString& envKey : m_settings->childGroups()) {
        Environment env;
        if (envKey == "development") env = Environment::Development;
        else if (envKey == "testing") env = Environment::Testing;
        else if (envKey == "staging") env = Environment::Staging;
        else if (envKey == "production") env = Environment::Production;
        else continue;
        
        m_settings->beginGroup(envKey);
        auto& settings = m_environments[env];
        if (m_settings->contains("apiUrl")) settings.apiUrl = m_settings->value("apiUrl").toString();
        if (m_settings->contains("ctpMarketFront")) settings.ctpMarketFront = m_settings->value("ctpMarketFront").toString();
        if (m_settings->contains("ctpTradeFront")) settings.ctpTradeFront = m_settings->value("ctpTradeFront").toString();
        if (m_settings->contains("ctpBrokerId")) settings.ctpBrokerId = m_settings->value("ctpBrokerId").toString();
        if (m_settings->contains("aiProvider")) settings.aiProvider = m_settings->value("aiProvider").toString();
        if (m_settings->contains("aiModel")) settings.aiModel = m_settings->value("aiModel").toString();
        if (m_settings->contains("requestTimeout")) settings.requestTimeout = m_settings->value("requestTimeout").toInt();
        if (m_settings->contains("retryCount")) settings.retryCount = m_settings->value("retryCount").toInt();
        if (m_settings->contains("enableDebugLog")) settings.enableDebugLog = m_settings->value("enableDebugLog").toBool();
        if (m_settings->contains("enableCache")) settings.enableCache = m_settings->value("enableCache").toBool();
        if (m_settings->contains("cacheExpireTime")) settings.cacheExpireTime = m_settings->value("cacheExpireTime").toInt();
        m_settings->endGroup();
    }
    m_settings->endGroup();
}

void EnvironmentConfig::saveEnvironments()
{
    m_settings->beginGroup("environments");
    for (auto it = m_environments.begin(); it != m_environments.end(); ++it) {
        QString envKey = environmentName(it.key());
        m_settings->beginGroup(envKey);
        const auto& settings = it.value();
        m_settings->setValue("name", settings.name);
        m_settings->setValue("apiUrl", settings.apiUrl);
        m_settings->setValue("ctpMarketFront", settings.ctpMarketFront);
        m_settings->setValue("ctpTradeFront", settings.ctpTradeFront);
        m_settings->setValue("ctpBrokerId", settings.ctpBrokerId);
        m_settings->setValue("aiProvider", settings.aiProvider);
        m_settings->setValue("aiModel", settings.aiModel);
        m_settings->setValue("requestTimeout", settings.requestTimeout);
        m_settings->setValue("retryCount", settings.retryCount);
        m_settings->setValue("enableDebugLog", settings.enableDebugLog);
        m_settings->setValue("enableCache", settings.enableCache);
        m_settings->setValue("cacheExpireTime", settings.cacheExpireTime);
        m_settings->endGroup();
    }
    m_settings->endGroup();
    m_settings->sync();
}

void EnvironmentConfig::updateCache()
{
    m_configCache.clear();
    
    const auto& settings = m_environments[m_currentEnv];
    
    // 缓存常用配置
    m_configCache["apiUrl"] = settings.apiUrl;
    m_configCache["ctpMarketFront"] = settings.ctpMarketFront;
    m_configCache["ctpTradeFront"] = settings.ctpTradeFront;
    m_configCache["ctpBrokerId"] = settings.ctpBrokerId;
    m_configCache["aiProvider"] = settings.aiProvider;
    m_configCache["aiModel"] = settings.aiModel;
    m_configCache["requestTimeout"] = settings.requestTimeout;
    m_configCache["retryCount"] = settings.retryCount;
    m_configCache["enableDebugLog"] = settings.enableDebugLog;
    m_configCache["enableCache"] = settings.enableCache;
    m_configCache["cacheExpireTime"] = settings.cacheExpireTime;
}

bool EnvironmentConfig::validateSettings(const EnvironmentSettings& settings) const
{
    // 基本验证
    if (settings.apiUrl.isEmpty()) return false;
    if (settings.requestTimeout <= 0) return false;
    if (settings.retryCount < 0) return false;
    
    return true;
}
