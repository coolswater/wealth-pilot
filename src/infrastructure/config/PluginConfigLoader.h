/**
 * @file PluginConfigLoader.h
 * @brief 插件配置加载器
 *
 * @details 功能：
 * - 加载 CTP 配置
 * - 加载 AI 配置
 * - 支持配置热重载
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PLUGINCONFIGLOADER_H
#define PLUGINCONFIGLOADER_H

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace WealthPilot {

/**
 * @brief CTP 配置结构
 */
struct CTPConfig {
    // 行情配置
    struct MarketConfig {
        QString frontAddress;
        QString brokerId;
        QString userId;
        QString password;
        QString appId;
        QString authCode;
        bool enabled = false;
        QString description;
    } market;

    // 交易配置
    struct TradeConfig {
        QString frontAddress;
        QString brokerId;
        QString userId;
        QString password;
        QString appId;
        QString authCode;
        bool enabled = false;
        QString description;
    } trade;

    // 设置
    struct Settings {
        int reconnectInterval = 5000;
        int maxReconnectAttempts = 10;
        int heartbeatInterval = 60000;
        int dataBufferSize = 200;
        QString logLevel = "info";
    } settings;

    // 订阅配置
    struct Subscriptions {
        QStringList defaultSymbols;
        bool autoSubscribe = true;
    } subscriptions;
};

/**
 * @brief AI 配置结构
 */
struct AIConfig {
    QString provider = "zhipu";

    // 模型配置
    struct ModelConfig {
        QString model;
        int maxTokens = 4096;
        double temperature = 0.7;
        double topP = 0.9;
    } chat, analysis;

    // API Keys
    struct ApiKeys {
        QString zhipu;
        QString openai;
        QString anthropic;
    } apiKeys;

    // 设置
    struct Settings {
        int requestTimeout = 30000;
        int maxRetries = 3;
        int retryDelay = 1000;
        bool cacheEnabled = true;
        int cacheTTL = 3600;
    } settings;

    // 提示词
    struct Prompts {
        QString systemPrompt;
        QString analysisPrompt;
        QString riskPrompt;
    } prompts;

    // 功能开关
    struct Features {
        bool marketAnalysis = true;
        bool riskAssessment = true;
        bool portfolioOptimization = true;
        bool sentimentAnalysis = true;
        bool newsSummary = true;
    } features;
};

/**
 * @brief 插件配置加载器
 */
class PluginConfigLoader : public QObject
{
    Q_OBJECT

public:
    static PluginConfigLoader& instance();

    /**
     * @brief 加载 CTP 配置
     */
    bool loadCTPConfig(const QString& filePath);
    CTPConfig getCTPConfig() const { return m_ctpConfig; }

    /**
     * @brief 加载 AI 配置
     */
    bool loadAIConfig(const QString& filePath);
    AIConfig getAIConfig() const { return m_aiConfig; }

    /**
     * @brief 获取配置目录
     */
    QString getConfigDir() const { return m_configDir; }
    void setConfigDir(const QString& dir) { m_configDir = dir; }

signals:
    void ctpConfigLoaded();
    void aiConfigLoaded();
    void configError(const QString& error);

private:
    PluginConfigLoader() = default;
    ~PluginConfigLoader() = default;
    PluginConfigLoader(const PluginConfigLoader&) = delete;
    PluginConfigLoader& operator=(const PluginConfigLoader&) = delete;

    QString m_configDir = "config";
    CTPConfig m_ctpConfig;
    AIConfig m_aiConfig;
};

} // namespace WealthPilot

#endif // PLUGINCONFIGLOADER_H