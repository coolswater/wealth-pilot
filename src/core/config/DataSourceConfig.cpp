/**
 * @file DataSourceConfig.cpp
 * @brief 数据源配置加载器实现
 */

#include "DataSourceConfig.h"
#include "utils/Logger.h"
#include <QJsonDocument>
#include <QFile>

namespace WealthPilot {

DataSourceConfig& DataSourceConfig::instance()
{
    static DataSourceConfig inst;
    return inst;
}

bool DataSourceConfig::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open data source config: %1").arg(filePath));
        emit configError(QString("Cannot open file: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        emit configError(error.errorString());
        return false;
    }

    m_config = doc.object();
    m_lastFilePath = filePath;

    // 解析全局设置
    if (m_config.contains("settings")) {
        QJsonObject settings = m_config["settings"].toObject();
        m_settings.healthCheckInterval = settings["healthCheckInterval"].toInt(300000);
        m_settings.maxFailCount = settings["maxFailCount"].toInt(3);
        m_settings.recoveryThreshold = settings["recoveryThreshold"].toInt(5);
        m_settings.requestTimeout = settings["requestTimeout"].toInt(10000);
        m_settings.retryCount = settings["retryCount"].toInt(2);
    }

    LOG_INFO(QString("Data source config loaded: %1").arg(filePath));
    emit configLoaded();
    return true;
}

QVector<DataSourceConfigItem> DataSourceConfig::getDataSources(const QString& type) const
{
    QVector<DataSourceConfigItem> result;

    if (!m_config.contains("dataSources")) {
        return result;
    }

    QJsonObject dataSources = m_config["dataSources"].toObject();
    if (!dataSources.contains(type)) {
        return result;
    }

    QJsonArray sources = dataSources[type].toArray();
    for (const auto& source : sources) {
        QJsonObject obj = source.toObject();
        DataSourceConfigItem item;
        item.name = obj["name"].toString();
        item.type = obj["type"].toString();
        item.priority = obj["priority"].toInt(0);
        item.enabled = obj["enabled"].toBool(true);
        item.apiUrl = obj["apiUrl"].toString();
        item.wsUrl = obj["wsUrl"].toString();
        item.description = obj["description"].toString();
        result.append(item);
    }

    return result;
}

DataSourceConfigItem DataSourceConfig::getBestDataSource(const QString& type) const
{
    auto sources = getDataSources(type);
    if (sources.isEmpty()) {
        return DataSourceConfigItem();
    }

    // 按优先级排序，返回优先级最高且启用的数据源
    std::sort(sources.begin(), sources.end(), [](const DataSourceConfigItem& a, const DataSourceConfigItem& b) {
        return a.priority < b.priority;
    });

    for (const auto& source : sources) {
        if (source.enabled) {
            return source;
        }
    }

    return sources.first();
}

} // namespace WealthPilot