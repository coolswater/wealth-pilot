/**
 * @file EnvironmentConfig.cpp
 * @brief 多环境配置管理器实现
 */

#include "EnvironmentConfig.h"
#include "shared/utils/Logger.h"
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
    LOG_DEBUG("EnvironmentConfig created");
}

EnvironmentConfig::~EnvironmentConfig()
{
    saveSettings();
    LOG_DEBUG("EnvironmentConfig destroyed");
}

Environment EnvironmentConfig::currentEnvironment() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentEnv;
}

bool EnvironmentConfig::setEnvironment(Environment env)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_environments.contains(env)) {
        m_currentEnv = env;
        updateCache();
        
        LOG_INFO(QString("Environment changed to: %1").arg(environmentName(env)));

        emit environmentChanged(env);
        return true;
    }
    
    LOG_WARNING(QString("Failed to set environment: %1").arg(environmentName(env)));
    return false;
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

std::optional<EnvironmentSettings> EnvironmentConfig::getSettings(Environment env) const
{
    QMutexLocker locker(&m_mutex);
    auto it = m_environments.find(env);
    if (it != m_environments.end()) {
        return it.value();
    }
    return std::nullopt;
}

void EnvironmentConfig::setSettings(Environment env, const EnvironmentSettings& settings)
{
    QMutexLocker locker(&m_mutex);
    m_environments[env] = settings;
    saveSettings();
}

QVariant EnvironmentConfig::get(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&m_mutex);
    
    if (m_configCache.contains(key)) {
        return m_configCache[key];
    }
    
    QVariant value = m_settings->value(key, defaultValue);
    m_configCache[key] = value;
    
    return value;
}

void EnvironmentConfig::set(const QString& key, const QVariant& value)
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
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file for export: %1").arg(filePath));
        return false;
    }
    
    QJsonObject root;
    
    // 导出环境配置
    QJsonObject envsObj;
    for (auto it = m_environments.begin(); it != m_environments.end(); ++it) {
        QJsonObject envObj;
        const EnvironmentSettings& settings = it.value();
        envObj["name"] = settings.name;
        envObj["apiUrl"] = settings.apiUrl;
        envObj["ctpMarketFront"] = settings.ctpMarketFront;
        envObj["ctpTradeFront"] = settings.ctpTradeFront;
        envObj["ctpBrokerId"] = settings.ctpBrokerId;
        envObj["aiProvider"] = settings.aiProvider;
        envObj["aiModel"] = settings.aiModel;
        envObj["requestTimeout"] = settings.requestTimeout;
        envObj["retryCount"] = settings.retryCount;
        envObj["enableDebugLog"] = settings.enableDebugLog;
        envObj["enableCache"] = settings.enableCache;
        envObj["cacheExpireTime"] = settings.cacheExpireTime;
        envsObj[environmentName(it.key())] = envObj;
    }
    root["environments"] = envsObj;
    root["currentEnvironment"] = environmentName(m_currentEnv);
    
    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
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
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("Failed to parse config file: %1").arg(error.errorString()));
        return false;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QJsonObject root = doc.object();
    
    // 导入环境配置
    QJsonObject envsObj = root["environments"].toObject();
    for (const QString& envName : envsObj.keys()) {
        Environment env = environmentFromName(envName);
        QJsonObject envObj = envsObj[envName].toObject();
        
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
        
        m_environments[env] = settings;
    }
    
    // 设置当前环境
    QString currentEnvName = root["currentEnvironment"].toString();
    m_currentEnv = environmentFromName(currentEnvName);
    
    updateCache();
    saveSettings();
    
    LOG_INFO(QString("Configuration imported from: %1").arg(filePath));
    return true;
}

QString EnvironmentConfig::environmentName(Environment env)
{
    return s_envNames.value(env, "unknown");
}

Environment EnvironmentConfig::environmentFromName(const QString& name)
{
    for (auto it = s_envNames.begin(); it != s_envNames.end(); ++it) {
        if (it.value() == name) {
            return it.key();
        }
    }
    return Environment::Development;
}

void EnvironmentConfig::loadEnvironments()
{
    // 默认开发环境配置
    EnvironmentSettings devSettings;
    devSettings.name = "Development";
    devSettings.apiUrl = "http://localhost:8080";
    devSettings.ctpMarketFront = "tcp://180.168.146.187:10131";
    devSettings.ctpTradeFront = "tcp://180.168.146.187:10101";
    devSettings.ctpBrokerId = "9999";
    devSettings.aiProvider = "openai";
    devSettings.aiModel = "gpt-4";
    m_environments[Environment::Development] = devSettings;
    
    // 测试环境配置
    EnvironmentSettings testSettings;
    testSettings.name = "Testing";
    testSettings.apiUrl = "http://test.example.com";
    testSettings.ctpMarketFront = "tcp://218.202.237.33:10212";
    testSettings.ctpTradeFront = "tcp://218.202.237.33:10202";
    testSettings.ctpBrokerId = "9999";
    testSettings.aiProvider = "openai";
    testSettings.aiModel = "gpt-4";
    m_environments[Environment::Testing] = testSettings;
    
    // 预发布环境配置
    EnvironmentSettings stagingSettings;
    stagingSettings.name = "Staging";
    stagingSettings.apiUrl = "https://staging.example.com";
    stagingSettings.ctpMarketFront = "tcp://180.168.146.187:10131";
    stagingSettings.ctpTradeFront = "tcp://180.168.146.187:10101";
    stagingSettings.ctpBrokerId = "9999";
    stagingSettings.aiProvider = "openai";
    stagingSettings.aiModel = "gpt-4";
    m_environments[Environment::Staging] = stagingSettings;
    
    // 生产环境配置
    EnvironmentSettings prodSettings;
    prodSettings.name = "Production";
    prodSettings.apiUrl = "https://api.example.com";
    prodSettings.ctpMarketFront = "";
    prodSettings.ctpTradeFront = "";
    prodSettings.ctpBrokerId = "";
    prodSettings.aiProvider = "openai";
    prodSettings.aiModel = "gpt-4";
    prodSettings.enableDebugLog = false;
    m_environments[Environment::Production] = prodSettings;
    
    // 从设置中加载覆盖
    if (m_settings) {
        QString currentEnv = m_settings->value("currentEnvironment", "development").toString();
        m_currentEnv = environmentFromName(currentEnv);
    }
}

void EnvironmentConfig::updateCache()
{
    m_configCache.clear();
    
    if (m_settings) {
        for (const QString& key : m_settings->allKeys()) {
            m_configCache[key] = m_settings->value(key);
        }
    }
}

void EnvironmentConfig::saveSettings()
{
    if (m_settings) {
        m_settings->setValue("currentEnvironment", environmentName(m_currentEnv));
        m_settings->sync();
    }
}
