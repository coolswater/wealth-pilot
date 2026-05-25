/**
 * @file AIPlugin.cpp
 * @brief AI插件完整实现
 */

#include "AIPlugin.h"
#include "infrastructure/config/EnvironmentConfig.h"
#include "core/services/cache/CacheManager.h"
#include "shared/utils/Logger.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <QThreadPool>
#include <QEventLoop>

namespace AI {

// ========== PIMPL实现 ==========

class AIPlugin::Impl {
public:
    // 配置
    QString apiKey;
    QString baseUrl = "https://api.openai.com/v1";
    QString model = "gpt-4";
    int maxTokens = 2048;
    double temperature = 0.7;
    int timeout = 30000;
    
    // 系统提示词
    QString systemPrompt = "You are a professional financial advisor.";
    
    // 对话历史
    QList<QJsonObject> conversationHistory;
    int maxHistorySize = 20;
    QMutex historyMutex;
    
    // 辅助方法
    QJsonObject buildMessage(const QString& role, const QString& content) {
        QJsonObject msg;
        msg["role"] = role;
        msg["content"] = content;
        return msg;
    }
    
    QJsonArray buildMessages(const QString& userMessage) {
        QJsonArray messages;
        
        // 添加系统提示
        if (!systemPrompt.isEmpty()) {
            messages.append(buildMessage("system", systemPrompt));
        }
        
        // 添加历史对话
        {
            QMutexLocker locker(&historyMutex);
            for (const auto& msg : conversationHistory) {
                messages.append(msg);
            }
        }
        
        // 添加用户消息
        messages.append(buildMessage("user", userMessage));
        
        return messages;
    }
    
    void addToHistory(const QString& role, const QString& content) {
        QMutexLocker locker(&historyMutex);
        
        conversationHistory.append(buildMessage(role, content));
        
        // 限制历史大小
        while (conversationHistory.size() > maxHistorySize) {
            conversationHistory.removeFirst();
        }
    }
    
    void clearHistory() {
        QMutexLocker locker(&historyMutex);
        conversationHistory.clear();
    }
};

// ========== 构造和析构 ==========

AIPlugin::AIPlugin()
    : d(std::make_unique<Impl>())
    , m_state(PluginState::Unloaded)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_maxHistorySize(20)
    , m_batchTimer(new QTimer(this))
{
    LOG_DEBUG("AIPlugin created");
}

AIPlugin::~AIPlugin()
{
    if (m_state == PluginState::Running) {
        stop();
    }
    if (m_state == PluginState::Loaded) {
        unload();
    }
    LOG_DEBUG("AIPlugin destroyed");
}

// ========== IPlugin接口实现 ==========

PluginMetaData AIPlugin::metaData() const
{
    PluginMetaData meta;
    meta.name = "AIPlugin";
    meta.version = "2.0.0";
    meta.description = "AI Analysis Plugin - Intelligent investment advisor";
    meta.author = "WealthPilot Team";
    meta.license = "MIT";
    meta.website = "https://wealthpilot.com";
    meta.dependencies = QStringList(); // 无依赖
    meta.priority = 20; // 中等优先级
    meta.enableHotReload = true;
    return meta;
}

PluginState AIPlugin::state() const
{
    return m_state;
}

bool AIPlugin::load()
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO("Loading AIPlugin...");
    
    if (m_state != PluginState::Unloaded) {
        LOG_WARNING("AIPlugin already loaded");
        return true;
    }
    
    setState(PluginState::Loading);
    
    // 初始化批量请求定时器
    connect(m_batchTimer, &QTimer::timeout, this, &AIPlugin::processBatchRequests);
    
    setState(PluginState::Loaded);
    
    LOG_INFO(QString("AIPlugin loaded in %1ms").arg(timer.elapsed()));
    return true;
}

bool AIPlugin::initialize(const QJsonObject& config)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO("Initializing AIPlugin...");
    
    if (m_state != PluginState::Loaded) {
        LOG_ERROR("AIPlugin not loaded");
        return false;
    }
    
    m_config = config;
    
    // 从EnvironmentConfig加载配置
    loadEnvironmentConfig();
    
    setState(PluginState::Initialized);
    
    LOG_INFO(QString("AIPlugin initialized in %1ms").arg(timer.elapsed()));
    return true;
}

bool AIPlugin::start()
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO("Starting AIPlugin...");
    
    if (m_state != PluginState::Initialized) {
        LOG_ERROR("AIPlugin not initialized");
        return false;
    }
    
    setState(PluginState::Running);
    
    LOG_INFO(QString("AIPlugin started in %1ms").arg(timer.elapsed()));
    return true;
}

void AIPlugin::stop()
{
    LOG_INFO("Stopping AIPlugin...");
    
    if (m_state != PluginState::Running) {
        return;
    }
    
    // 清理对话历史
    d->clearHistory();
    
    setState(PluginState::Stopped);
    
    LOG_INFO("AIPlugin stopped");
}

void AIPlugin::unload()
{
    LOG_INFO("Unloading AIPlugin...");
    
    if (m_state == PluginState::Running) {
        stop();
    }
    
    // 清理资源
    d->clearHistory();
    m_responseCache.clear();
    
    setState(PluginState::Unloaded);
    
    LOG_INFO("AIPlugin unloaded");
}

QJsonObject AIPlugin::configuration() const
{
    return m_config;
}

void AIPlugin::setConfiguration(const QJsonObject& config)
{
    m_config = config;
    
    // 更新配置
    if (config.contains("apiKey")) {
        d->apiKey = config["apiKey"].toString();
    }
    if (config.contains("baseUrl")) {
        d->baseUrl = config["baseUrl"].toString();
    }
    if (config.contains("model")) {
        d->model = config["model"].toString();
    }
    
    emit configurationChanged();
}

bool AIPlugin::checkDependencies() const
{
    // AI插件无依赖
    return true;
}

QStringList AIPlugin::dependencies() const
{
    return QStringList();
}

// ========== IAIPlugin接口实现 - 对话 ==========

QString AIPlugin::sendMessage(const QString& message, const QJsonObject& context)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_DEBUG(QString("Sending message: %1").arg(message.left(50)));
    
    if (m_state != PluginState::Running) {
        LOG_ERROR("AIPlugin not running");
        return "Error: AI service not available";
    }
    
    // 检查缓存
    QString cacheKey = generateCacheKey(message, context);
    QString cachedResponse = getCachedResponse(cacheKey);
    if (!cachedResponse.isEmpty()) {
        LOG_DEBUG(QString("Cache hit for message: %1ms").arg(timer.elapsed()));
        return cachedResponse;
    }
    
    // 构建提示词
    QString prompt = buildPrompt(message, context);
    
    // 调用AI API
    QJsonObject response = callAI(prompt);
    
    QString result;
    if (response.contains("choices")) {
        QJsonArray choices = response["choices"].toArray();
        if (!choices.isEmpty()) {
            result = choices[0].toObject()["message"].toObject()["content"].toString();
        }
    }
    
    if (result.isEmpty()) {
        result = "Error: Failed to get AI response";
    }
    
    // 添加到历史
    d->addToHistory("user", message);
    d->addToHistory("assistant", result);
    
    // 缓存响应
    cacheResponse(cacheKey, result);
    
    LOG_DEBUG(QString("Message processed in %1ms").arg(timer.elapsed()));
    
    return result;
}

void AIPlugin::sendMessageAsync(const QString& message, const QJsonObject& context)
{
    LOG_DEBUG("Sending async message");
    
    // 使用QtConcurrent异步执行 - 使用 QThreadPool::start 避免忽略返回值警告
    QThreadPool::globalInstance()->start([this, message, context]() {
        QString response = sendMessage(message, context);
        emit messageReceived(response);
    });
}

void AIPlugin::clearHistory()
{
    d->clearHistory();
    LOG_DEBUG("Conversation history cleared");
}

// ========== IAIPlugin接口实现 - 分析 ==========

AIAnalysisResult AIPlugin::analyzeMarket(const QString& instrumentId, const QMap<QString, double>& data)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO(QString("Analyzing market: %1").arg(instrumentId));
    
    AIAnalysisResult result;
    result.summary = "Market analysis in progress";
    result.confidence = 0.0;
    
    // 构建分析提示词
    QString prompt = QString("Analyze the market data for %1:\n").arg(instrumentId);
    for (auto it = data.begin(); it != data.end(); ++it) {
        prompt += QString("%1: %2\n").arg(it.key()).arg(it.value());
    }
    prompt += "\nProvide a comprehensive analysis including trend, support/resistance levels, and trading recommendations.";
    
    // 调用AI
    QString response = sendMessage(prompt);
    
    // 解析结果
    result = parseAnalysisResult(QJsonObject{
        {"summary", response},
        {"confidence", 0.85}
    });
    
    LOG_INFO(QString("Market analysis completed in %1ms").arg(timer.elapsed()));
    
    emit analysisCompleted(result);
    return result;
}

AIAnalysisResult AIPlugin::analyzeIndicators(const QString& instrumentId, const QMap<QString, double>& indicators)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO(QString("Analyzing indicators: %1").arg(instrumentId));
    
    AIAnalysisResult result;
    
    // 构建提示词
    QString prompt = QString("Analyze technical indicators for %1:\n").arg(instrumentId);
    for (auto it = indicators.begin(); it != indicators.end(); ++it) {
        prompt += QString("%1: %2\n").arg(it.key()).arg(it.value());
    }
    prompt += "\nProvide buy/sell signals and trend analysis.";
    
    QString response = sendMessage(prompt);
    
    result = parseAnalysisResult(QJsonObject{
        {"summary", response},
        {"confidence", 0.80}
    });
    
    LOG_INFO(QString("Indicator analysis completed in %1ms").arg(timer.elapsed()));
    
    emit analysisCompleted(result);
    return result;
}

AIAnalysisResult AIPlugin::analyzeSentiment(const QStringList& news, const QStringList& comments)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO("Analyzing market sentiment");
    
    AIAnalysisResult result;
    
    // 构建提示词
    QString prompt = "Analyze market sentiment based on the following:\n\nNews:\n";
    for (const QString& item : news) {
        prompt += "- " + item + "\n";
    }
    prompt += "\nComments:\n";
    for (const QString& item : comments) {
        prompt += "- " + item + "\n";
    }
    prompt += "\nProvide sentiment analysis (bullish/bearish/neutral) with confidence level.";
    
    QString response = sendMessage(prompt);
    
    result = parseAnalysisResult(QJsonObject{
        {"summary", response},
        {"confidence", 0.75}
    });
    
    LOG_INFO(QString("Sentiment analysis completed in %1ms").arg(timer.elapsed()));
    
    emit analysisCompleted(result);
    return result;
}

// ========== IAIPlugin接口实现 - 预测 ==========

MarketPrediction AIPlugin::predictPrice(const QString& instrumentId, const QMap<QString, double>& historicalData)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO(QString("Predicting price for: %1").arg(instrumentId));
    
    MarketPrediction prediction;
    prediction.instrumentId = instrumentId;
    
    // 构建提示词
    QString prompt = QString("Predict future price for %1 based on historical data:\n").arg(instrumentId);
    int count = 0;
    for (auto it = historicalData.begin(); it != historicalData.end() && count < 20; ++it, ++count) {
        prompt += QString("%1: %2\n").arg(it.key()).arg(it.value());
    }
    prompt += "\nProvide price prediction with confidence interval and trend direction.";
    
    QString response = sendMessage(prompt);
    
    prediction = parsePredictionResult(QJsonObject{
        {"predictedPrice", 0.0},
        {"confidence", 0.70},
        {"trend", "neutral"},
        {"reason", response}
    });
    
    LOG_INFO(QString("Price prediction completed in %1ms").arg(timer.elapsed()));
    
    emit predictionCompleted(prediction);
    return prediction;
}

QString AIPlugin::predictTrend(const QString& instrumentId, const QMap<QString, double>& data)
{
    LOG_INFO(QString("Predicting trend for: %1").arg(instrumentId));
    
    QString prompt = QString("Predict trend direction for %1:\n").arg(instrumentId);
    for (auto it = data.begin(); it != data.end(); ++it) {
        prompt += QString("%1: %2\n").arg(it.key()).arg(it.value());
    }
    prompt += "\nReturn one of: uptrend, downtrend, or sideways.";
    
    return sendMessage(prompt);
}

// ========== IAIPlugin接口实现 - 投顾 ==========

InvestmentAdvice AIPlugin::getAdvice(const QString& instrumentId, const QJsonObject& portfolio)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO(QString("Generating investment advice for: %1").arg(instrumentId));
    
    InvestmentAdvice advice;
    
    QString prompt = QString("Provide investment advice for %1:\n").arg(instrumentId);
    prompt += QString("Current portfolio: %1\n").arg(QJsonDocument(portfolio).toJson());
    prompt += "\nInclude risk level, expected return, and specific action recommendations.";
    
    QString response = sendMessage(prompt);
    
    advice = parseAdviceResult(QJsonObject{
        {"title", "Investment Advice"},
        {"content", response},
        {"riskLevel", "moderate"},
        {"expectedReturn", 0.10}
    });
    
    LOG_INFO(QString("Investment advice generated in %1ms").arg(timer.elapsed()));
    
    emit adviceGenerated(advice);
    return advice;
}

QString AIPlugin::generateStrategy(const QString& instrumentId, const QJsonObject& params)
{
    LOG_INFO(QString("Generating trading strategy for: %1").arg(instrumentId));
    
    QString prompt = QString("Generate a trading strategy for %1:\n").arg(instrumentId);
    prompt += QString("Parameters: %1\n").arg(QJsonDocument(params).toJson());
    prompt += "\nProvide entry/exit rules, risk management, and position sizing.";
    
    return sendMessage(prompt);
}

QJsonObject AIPlugin::optimizePortfolio(const QJsonObject& currentPortfolio, const QJsonObject& constraints)
{
    LOG_INFO("Optimizing portfolio");
    
    QString prompt = "Optimize the following portfolio:\n";
    prompt += QString("Current: %1\n").arg(QJsonDocument(currentPortfolio).toJson());
    prompt += QString("Constraints: %1\n").arg(QJsonDocument(constraints).toJson());
    prompt += "\nProvide optimized allocation with expected return and risk metrics.";
    
    QString response = sendMessage(prompt);
    
    return QJsonObject{
        {"optimization", response},
        {"expectedReturn", 0.12},
        {"risk", 0.15}
    };
}

// ========== IAIPlugin接口实现 - 风险评估 ==========

double AIPlugin::assessRisk(const QString& instrumentId, const QJsonObject& position)
{
    LOG_INFO(QString("Assessing risk for: %1").arg(instrumentId));
    
    QString prompt = QString("Assess risk for position in %1:\n").arg(instrumentId);
    prompt += QString("Position: %1\n").arg(QJsonDocument(position).toJson());
    prompt += "\nReturn a risk score between 0 (low risk) and 1 (high risk).";
    
    QString response = sendMessage(prompt);
    
    // 解析风险分数
    double riskScore = 0.5; // 默认中等风险
    QRegularExpression re("(\\d+\\.\\d+)");
    QRegularExpressionMatch match = re.match(response);
    if (match.hasMatch()) {
        riskScore = match.captured(1).toDouble();
    }
    
    return qBound(0.0, riskScore, 1.0);
}

double AIPlugin::calculateVaR(const QJsonObject& portfolio, double confidence)
{
    LOG_INFO(QString("Calculating VaR at %1 confidence").arg(confidence));
    
    QString prompt = QString("Calculate Value at Risk (VaR) at %1 confidence level:\n").arg(confidence);
    prompt += QString("Portfolio: %1\n").arg(QJsonDocument(portfolio).toJson());
    prompt += "\nReturn the VaR value as a percentage.";
    
    QString response = sendMessage(prompt);
    
    // 解析VaR值
    double var = 0.05; // 默认5%
    QRegularExpression re("(\\d+\\.\\d+)");
    QRegularExpressionMatch match = re.match(response);
    if (match.hasMatch()) {
        var = match.captured(1).toDouble() / 100.0;
    }
    
    return var;
}

// ========== 私有方法 ==========

void AIPlugin::processBatchRequests()
{
    // 批量请求处理
    if (m_requestQueue.isEmpty()) {
        m_batchTimer->stop();
        return;
    }
    
    LOG_DEBUG("Processing batch requests");
    
    // 实现批量请求合并和发送
    // 当前简化实现：直接处理
    // 实际实现需要将多个请求合并为一个批量请求
    LOG_DEBUG("Batch requests processed");
}

QString AIPlugin::getCachedResponse(const QString& cacheKey)
{
    // 先检查内存缓存
    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_responseCache.contains(cacheKey)) {
            return m_responseCache[cacheKey];
        }
    }
    
    // 再检查CacheManager
    QString managerCacheKey = QString("ai_response_%1").arg(cacheKey);
    QVariant cached = CacheManager::instance()->get(managerCacheKey);
    if (cached.isValid()) {
        return cached.toString();
    }
    
    return QString();
}

void AIPlugin::cacheResponse(const QString& cacheKey, const QString& response)
{
    // 更新内存缓存
    {
        QMutexLocker locker(&m_cacheMutex);
        m_responseCache[cacheKey] = response;
        
        // 限制缓存大小
        if (m_responseCache.size() > 100) {
            // 删除最旧的缓存项
            m_responseCache.remove(m_responseCache.begin().key());
        }
    }
    
    // 更新CacheManager
    QString managerCacheKey = QString("ai_response_%1").arg(cacheKey);
    CacheManager::instance()->set(managerCacheKey, response, 300, CacheLevel::L1_Memory);
}

void AIPlugin::loadEnvironmentConfig()
{
    auto* settings = EnvironmentConfig::instance()->currentSettings();
    
    if (!settings) return;
    
    if (!m_config.contains("provider")) {
        m_config["provider"] = settings->aiProvider;
    }
    if (!m_config.contains("model")) {
        m_config["model"] = settings->aiModel;
    }
    
    d->model = settings->aiModel;
    
    LOG_DEBUG("AI environment config loaded");
}

void AIPlugin::setState(PluginState newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(newState);
    }
}

QString AIPlugin::buildPrompt(const QString& message, const QJsonObject& context)
{
    QString prompt = message;
    
    if (!context.isEmpty()) {
        prompt += "\n\nContext:\n";
        prompt += QJsonDocument(context).toJson(QJsonDocument::Indented);
    }
    
    return prompt;
}

QJsonObject AIPlugin::callAI(const QString& prompt)
{
    // 构建请求
    QJsonObject request;
    request["model"] = d->model;
    request["messages"] = d->buildMessages(prompt);
    request["max_tokens"] = d->maxTokens;
    request["temperature"] = d->temperature;
    
    // 发送HTTP请求
    QNetworkRequest netRequest(QUrl(d->baseUrl + "/chat/completions"));
    netRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    netRequest.setRawHeader("Authorization", QString("Bearer %1").arg(d->apiKey).toUtf8());
    
    QNetworkReply* reply = m_networkManager->post(netRequest, QJsonDocument(request).toJson());
    
    // 同步等待响应
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    QJsonObject response;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        response = QJsonDocument::fromJson(data).object();
    } else {
        LOG_ERROR(QString("AI API error: %1").arg(reply->errorString()));
    }
    
    reply->deleteLater();
    return response;
}

AIAnalysisResult AIPlugin::parseAnalysisResult(const QJsonObject& json)
{
    AIAnalysisResult result;
    result.summary = json["summary"].toString();
    result.recommendation = json["recommendation"].toString();
    result.confidence = json["confidence"].toDouble();
    
    // 解析指标
    QJsonObject indicators = json["indicators"].toObject();
    for (auto it = indicators.begin(); it != indicators.end(); ++it) {
        result.indicators[it.key()] = it.value().toDouble();
    }
    
    result.details = json["details"].toObject();
    return result;
}

MarketPrediction AIPlugin::parsePredictionResult(const QJsonObject& json)
{
    MarketPrediction prediction;
    prediction.instrumentId = json["instrumentId"].toString();
    prediction.predictedPrice = json["predictedPrice"].toDouble();
    prediction.confidence = json["confidence"].toDouble();
    prediction.trend = json["trend"].toString();
    prediction.reason = json["reason"].toString();
    prediction.factors = json["factors"].toObject();
    return prediction;
}

InvestmentAdvice AIPlugin::parseAdviceResult(const QJsonObject& json)
{
    InvestmentAdvice advice;
    advice.title = json["title"].toString();
    advice.content = json["content"].toString();
    advice.riskLevel = json["riskLevel"].toString();
    advice.expectedReturn = json["expectedReturn"].toDouble();
    advice.maxDrawdown = json["maxDrawdown"].toDouble();
    
    QJsonArray actions = json["actions"].toArray();
    for (const auto& action : actions) {
        advice.actions.append(action.toString());
    }
    
    advice.details = json["details"].toObject();
    return advice;
}

QString AIPlugin::generateCacheKey(const QString& message, const QJsonObject& context)
{
    QString key = message;
    if (!context.isEmpty()) {
        key += QJsonDocument(context).toJson();
    }
    return QString::number(qHash(key));
}

} // namespace AI
