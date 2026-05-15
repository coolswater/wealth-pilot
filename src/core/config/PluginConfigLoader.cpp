/**
 * @file PluginConfigLoader.cpp
 * @brief 插件配置加载器实现
 */

#include "PluginConfigLoader.h"
#include "utils/Logger.h"
#include <QJsonDocument>
#include <QFile>

namespace WealthPilot {

PluginConfigLoader& PluginConfigLoader::instance()
{
    static PluginConfigLoader inst;
    return inst;
}

bool PluginConfigLoader::loadCTPConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open CTP config: %1").arg(filePath));
        emit configError(QString("Cannot open CTP config: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("CTP config JSON parse error: %1").arg(error.errorString()));
        emit configError(error.errorString());
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("ctp")) {
        LOG_ERROR("CTP config missing 'ctp' section");
        emit configError("Missing 'ctp' section");
        return false;
    }

    QJsonObject ctp = root["ctp"].toObject();

    // 解析行情配置
    if (ctp.contains("market")) {
        QJsonObject market = ctp["market"].toObject();
        m_ctpConfig.market.frontAddress = market["frontAddress"].toString();
        m_ctpConfig.market.brokerId = market["brokerId"].toString();
        m_ctpConfig.market.userId = market["userId"].toString();
        m_ctpConfig.market.password = market["password"].toString();
        m_ctpConfig.market.appId = market["appId"].toString();
        m_ctpConfig.market.authCode = market["authCode"].toString();
        m_ctpConfig.market.enabled = market["enabled"].toBool(false);
        m_ctpConfig.market.description = market["description"].toString();
    }

    // 解析交易配置
    if (ctp.contains("trade")) {
        QJsonObject trade = ctp["trade"].toObject();
        m_ctpConfig.trade.frontAddress = trade["frontAddress"].toString();
        m_ctpConfig.trade.brokerId = trade["brokerId"].toString();
        m_ctpConfig.trade.userId = trade["userId"].toString();
        m_ctpConfig.trade.password = trade["password"].toString();
        m_ctpConfig.trade.appId = trade["appId"].toString();
        m_ctpConfig.trade.authCode = trade["authCode"].toString();
        m_ctpConfig.trade.enabled = trade["enabled"].toBool(false);
        m_ctpConfig.trade.description = trade["description"].toString();
    }

    // 解析设置
    if (ctp.contains("settings")) {
        QJsonObject settings = ctp["settings"].toObject();
        m_ctpConfig.settings.reconnectInterval = settings["reconnectInterval"].toInt(5000);
        m_ctpConfig.settings.maxReconnectAttempts = settings["maxReconnectAttempts"].toInt(10);
        m_ctpConfig.settings.heartbeatInterval = settings["heartbeatInterval"].toInt(60000);
        m_ctpConfig.settings.dataBufferSize = settings["dataBufferSize"].toInt(200);
        m_ctpConfig.settings.logLevel = settings["logLevel"].toString("info");
    }

    // 解析订阅配置
    if (ctp.contains("subscriptions")) {
        QJsonObject subs = ctp["subscriptions"].toObject();
        QJsonArray symbols = subs["defaultSymbols"].toArray();
        for (const auto& sym : symbols) {
            m_ctpConfig.subscriptions.defaultSymbols.append(sym.toString());
        }
        m_ctpConfig.subscriptions.autoSubscribe = subs["autoSubscribe"].toBool(true);
    }

    LOG_INFO(QString("CTP config loaded: %1").arg(filePath));
    emit ctpConfigLoaded();
    return true;
}

bool PluginConfigLoader::loadAIConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open AI config: %1").arg(filePath));
        emit configError(QString("Cannot open AI config: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("AI config JSON parse error: %1").arg(error.errorString()));
        emit configError(error.errorString());
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("ai")) {
        LOG_ERROR("AI config missing 'ai' section");
        emit configError("Missing 'ai' section");
        return false;
    }

    QJsonObject ai = root["ai"].toObject();

    m_aiConfig.provider = ai["provider"].toString("zhipu");

    // 解析模型配置
    if (ai.contains("models")) {
        QJsonObject models = ai["models"].toObject();

        if (models.contains("chat")) {
            QJsonObject chat = models["chat"].toObject();
            m_aiConfig.chat.model = chat["model"].toString("glm-4");
            m_aiConfig.chat.maxTokens = chat["maxTokens"].toInt(4096);
            m_aiConfig.chat.temperature = chat["temperature"].toDouble(0.7);
            m_aiConfig.chat.topP = chat["topP"].toDouble(0.9);
        }

        if (models.contains("analysis")) {
            QJsonObject analysis = models["analysis"].toObject();
            m_aiConfig.analysis.model = analysis["model"].toString("glm-4-flash");
            m_aiConfig.analysis.maxTokens = analysis["maxTokens"].toInt(2048);
            m_aiConfig.analysis.temperature = analysis["temperature"].toDouble(0.3);
            m_aiConfig.analysis.topP = analysis["topP"].toDouble(0.95);
        }
    }

    // 解析 API Keys
    if (ai.contains("apiKeys")) {
        QJsonObject keys = ai["apiKeys"].toObject();
        m_aiConfig.apiKeys.zhipu = keys["zhipu"].toString();
        m_aiConfig.apiKeys.openai = keys["openai"].toString();
        m_aiConfig.apiKeys.anthropic = keys["anthropic"].toString();
    }

    // 解析设置
    if (ai.contains("settings")) {
        QJsonObject settings = ai["settings"].toObject();
        m_aiConfig.settings.requestTimeout = settings["requestTimeout"].toInt(30000);
        m_aiConfig.settings.maxRetries = settings["maxRetries"].toInt(3);
        m_aiConfig.settings.retryDelay = settings["retryDelay"].toInt(1000);
        m_aiConfig.settings.cacheEnabled = settings["cacheEnabled"].toBool(true);
        m_aiConfig.settings.cacheTTL = settings["cacheTTL"].toInt(3600);
    }

    // 解析提示词
    if (ai.contains("prompts")) {
        QJsonObject prompts = ai["prompts"].toObject();
        m_aiConfig.prompts.systemPrompt = prompts["systemPrompt"].toString();
        m_aiConfig.prompts.analysisPrompt = prompts["analysisPrompt"].toString();
        m_aiConfig.prompts.riskPrompt = prompts["riskPrompt"].toString();
    }

    // 解析功能开关
    if (ai.contains("features")) {
        QJsonObject features = ai["features"].toObject();
        m_aiConfig.features.marketAnalysis = features["marketAnalysis"].toBool(true);
        m_aiConfig.features.riskAssessment = features["riskAssessment"].toBool(true);
        m_aiConfig.features.portfolioOptimization = features["portfolioOptimization"].toBool(true);
        m_aiConfig.features.sentimentAnalysis = features["sentimentAnalysis"].toBool(true);
        m_aiConfig.features.newsSummary = features["newsSummary"].toBool(true);
    }

    LOG_INFO(QString("AI config loaded: %1").arg(filePath));
    emit aiConfigLoaded();
    return true;
}

} // namespace WealthPilot