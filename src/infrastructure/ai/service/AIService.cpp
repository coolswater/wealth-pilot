/**
 * @file AIService.cpp
 * @brief AI 分析服务实现
 *
 * @details 本服务提供以下核心功能：
 * - AI对话交互（chat/chatSync/chatStream）
 * - 投资组合分析（analyzePortfolio）
 * - 股票分析（analyzeStock）
 * - 市场情绪分析（analyzeMarketSentiment）
 * - 快速问答（quickAsk）
 * - 投资建议（getInvestmentAdvice）
 *
 * 支持多种AI提供商：OpenAI、Claude、本地模型
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "AIService.h"
#include "../../network/NetworkManager.h"
#include "infrastructure/config/ConfigManager.h"
#include "shared/utils/Logger.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QUuid>

const QString AIService::DEFAULT_SYSTEM_PROMPT = R"(
你是一个专业的金融投资助理，名�?WealthPilot AI�?
你的职责是：
1. 分析用户的投资组合，提供专业的投资建�?
2. 解读市场行情和财经新�?
3. 回答用户关于投资、理财的问题
4. 提供风险提示和投资教�?

请注意：
- 回答要专业、准确、有依据
- 对于不确定的信息，要明确告知
- 不要提供具体的买卖建议，只提供分析和参�?
- 使用简洁明了的语言
)";

AIAnalysis AIAnalysis::fromJson(const QJsonObject& json)
{
    AIAnalysis a;
    a.summary = json["summary"].toString();
    a.suggestion = json["suggestion"].toString();
    a.confidence = json["confidence"].toDouble();
    a.rawData = json;

    QJsonArray keywords = json["keywords"].toArray();
    for (const auto& k : keywords) {
        a.keywords.append(k.toString());
    }

    return a;
}

/**
 * @brief 默认构造函数
 * 初始化AI服务配置
 */
AIService::AIService()
    : m_maxHistorySize(10)
    , m_cacheEnabled(true)
    , m_requestCacheTimeout(60000) // 1分钟缓存
{
}

AIService::~AIService()
{
    shutdown();
}

/**
 * @brief 初始化AI服务
 * @return true 初始化成功，false 初始化失败
 *
 * 从配置管理器加载API密钥、提供商设置、模型配置等
 */
bool AIService::initialize()
{
    if (m_initialized) return m_initialized;

    // 使用新的配置键
    QString apiKey = ConfigManager::instance()->getSecure("secure/ai_api_key");
    QString provider = ConfigManager::instance()->getString("ai/provider", "openai");
    QString model = ConfigManager::instance()->getString("ai/model", "gpt-4");
    QString apiUrl = ConfigManager::instance()->getString("ai/api_url", "https://api.openai.com/v1");

    if (!apiKey.isEmpty()) {
        m_config.apiKey = apiKey;
        m_config.provider = (provider == "openai") ? AIProvider::OpenAI :
                           (provider == "claude" || provider == "anthropic") ? AIProvider::Claude :
                           (provider == "local") ? AIProvider::Local : AIProvider::Custom;
        m_config.model = model;
        m_config.baseUrl = apiUrl;
    }

    m_systemPrompt = DEFAULT_SYSTEM_PROMPT;
    m_initialized = true;

    LOG_INFO("AIService initialized");

    return m_initialized;
}

/**
 * @brief 设置历史消息最大数量
 * @param size 最大历史消息数
 *
 * 超过此数量的历史消息将被清理，用于控制上下文长度
 */
void AIService::setMaxHistorySize(int size)
{
    m_maxHistorySize = size;
    clearHistory();
}

/**
 * @brief 设置是否启用响应缓存
 * @param enabled true启用，false禁用
 *
 * 启用后相同请求会返回缓存结果，减少API调用
 */
void AIService::setCacheEnabled(bool enabled)
{
    m_cacheEnabled = enabled;
    if (!enabled) {
        clearRequestCache();
    }
}

/**
 * @brief 设置缓存超时时间
 * @param timeoutMs 超时时间（毫秒）
 *
 * 缓存条目超过此时间后失效
 */
void AIService::setCacheTimeout(int timeoutMs)
{
    m_requestCacheTimeout = timeoutMs;
    clearRequestCache();
}

/**
 * @brief 关闭AI服务
 * 清理历史消息和缓存，释放资源
 */
void AIService::shutdown()
{
    clearHistory();
    clearRequestCache();
    m_initialized = false;
    LOG_INFO("AIService shutdown");
}

/**
 * @brief 设置AI服务配置
 * @param config 配置对象，包含API密钥、模型、基础URL等
 *
 * 同时将敏感信息保存到安全配置存储
 */
void AIService::setConfig(const AIConfig& config)
{
    m_config = config;

    if (!config.apiKey.isEmpty()) {
        ConfigManager::instance()->setSecure("secure/ai_api_key", config.apiKey);
    }
    if (!config.model.isEmpty()) {
        ConfigManager::instance()->set("ai/model", config.model);
    }
}

AIConfig AIService::config() const
{
    return m_config;
}

void AIService::setSystemPrompt(const QString& prompt)
{
    m_systemPrompt = prompt;
}

/**
 * @brief 发送聊天消息（异步）
 * @param message 用户消息
 * @param callback 结果回调函数
 *
 * 支持缓存，相同消息会返回缓存结果
 * 自动管理对话历史
 */
void AIService::chat(const QString& message, std::function<void(AIResult<QString>)> callback)
{
    if (m_config.provider == AIProvider::None || m_config.apiKey.isEmpty()) {
        callback(AIResult<QString>::error("AI service not configured"));
        return;
    }

    // 检查缓存
    if (m_cacheEnabled) {
        QString cacheKey = generateCacheKey(message);
        QVariant cached = getCachedResponse(cacheKey);
        if (cached.isValid()) {
            QString cachedResponse = cached.toString();
            m_history.append(AIMessage::assistant(cachedResponse));
            callback(AIResult<QString>::ok(cachedResponse));
            return;
        }
    }

    m_history.append(AIMessage::user(message));

    QJsonObject request = buildRequest(message);

    sendRequest(request, [this, callback, message, cacheKey = generateCacheKey(message)](AIResult<QString> result) {
        if (result.isOk()) {
            QString response = result.value();
            m_history.append(AIMessage::assistant(response));

            // 缓存响应
            if (m_cacheEnabled) {
                setCachedResponse(cacheKey, response);
            }

            callback(result);
        } else {
            callback(result);
        }
    });
}

/**
 * @brief 发送聊天消息（同步）
 * @param message 用户消息
 * @param timeoutMs 超时时间（毫秒）
 * @return AI响应结果
 *
 * 阻塞等待响应，适用于需要同步处理的场景
 */
AIResult<QString> AIService::chatSync(const QString& message, int timeoutMs)
{
    QString resultValue;
    QString errorMessage;
    bool success = false;
    QEventLoop loop;

    chat(message, [&](AIResult<QString> r) {
        if (r.isOk()) {
            resultValue = r.value();
            success = true;
        } else {
            errorMessage = r.errorMessage();
        }
        loop.quit();
    });

    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeoutMs);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec();

    if (!timer.isActive()) {
        return AIResult<QString>::error("AI request timeout");
    }

    if (success) {
        return AIResult<QString>::ok(resultValue);
    } else {
        return AIResult<QString>::error(errorMessage);
    }
}

/**
 * @brief 发送聊天消息（流式）
 * @param message 用户消息
 * @param onChunk 收到数据块时的回调
 * @param onComplete 完成时的回调
 *
 * 适用于需要实时显示响应的场景
 */
void AIService::chatStream(const QString& message,
                           std::function<void(const QString&)> onChunk,
                           std::function<void()> onComplete)
{
    chat(message, [onChunk, onComplete](AIResult<QString> result) {
        if (result.isOk()) {
            onChunk(result.value());
        }
        onComplete();
    });
}

// ==================== 请求缓存 ====================

void AIService::clearRequestCache()
{
    QMutexLocker locker(&m_mutex);
    m_requestCache.clear();
}

void AIService::cleanupRequestCache()
{
    QMutexLocker locker(&m_mutex);

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    auto it = m_requestCache.begin();

    while (it != m_requestCache.end()) {
        if (it.value().expiryTime < now || m_requestCache.size() > 100) {
            it = m_requestCache.erase(it);
        } else {
            ++it;
        }
    }
}

QString AIService::generateCacheKey(const QString& message)
{
    // 使用消息的哈希作为缓存键
    return QString("chat_%1").arg(QString::number(qHash(message)));
}

QVariant AIService::getCachedResponse(const QString& key)
{
    QMutexLocker locker(&m_mutex);

    if (!m_requestCache.contains(key)) {
        return QVariant();
    }

    RequestCacheEntry& entry = m_requestCache[key];
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (entry.expiryTime < now) {
        m_requestCache.remove(key);
        return QVariant();
    }

    return entry.response;
}

void AIService::setCachedResponse(const QString& key, const QString& response)
{
    QMutexLocker locker(&m_mutex);

    RequestCacheEntry entry;
    entry.response = response;
    entry.expiryTime = QDateTime::currentMSecsSinceEpoch() + m_requestCacheTimeout;

    m_requestCache[key] = entry;

    // 限制缓存大小
    if (m_requestCache.size() > 100) {
        auto it = m_requestCache.begin();
        m_requestCache.erase(it);
    }
}

void AIService::clearHistory()
{
    m_history.clear();
    clearRequestCache();
}

/**
 * @brief 分析投资组合
 * @param positions 持仓数据（JSON格式）
 * @param callback 分析结果回调
 *
 * 调用AI分析持仓结构、风险评估、优化建议等
 */
void AIService::analyzePortfolio(const QJsonObject& positions,
                                  std::function<void(AIResult<AIAnalysis>)> callback)
{
    QString prompt = QString("请分析以下投资组合，并提供投资建议：\n\n%1")
        .arg(QString::fromUtf8(QJsonDocument(positions).toJson()));

    chat(prompt, [callback](AIResult<QString> result) {
        if (result.isError()) {
            callback(AIResult<AIAnalysis>::error(result.errorMessage()));
            return;
        }

        AIAnalysis analysis;
        analysis.summary = result.value();
        analysis.confidence = 0.8;
        callback(AIResult<AIAnalysis>::ok(analysis));
    });
}

/**
 * @brief 分析单只股票
 * @param code 股票代码
 * @param data 股票数据（JSON格式）
 * @param callback 分析结果回调
 *
 * 提供技术分析、基本面分析、投资建议
 */
void AIService::analyzeStock(const QString& code, const QJsonObject& data,
                              std::function<void(AIResult<AIAnalysis>)> callback)
{
    QString prompt = QString("请分析股�?%1 的以下数据：\n\n%2\n\n请提供：\n1. 技术分析\n2. 基本面分析\n3. 投资建议")
        .arg(code)
        .arg(QString::fromUtf8(QJsonDocument(data).toJson()));

    chat(prompt, [callback](AIResult<QString> result) {
        if (result.isError()) {
            callback(AIResult<AIAnalysis>::error(result.errorMessage()));
            return;
        }

        AIAnalysis analysis;
        analysis.summary = result.value();
        analysis.confidence = 0.75;
        callback(AIResult<AIAnalysis>::ok(analysis));
    });
}

/**
 * @brief 分析市场情绪
 * @param marketData 市场数据（JSON格式）
 * @param callback 分析结果回调
 *
 * 基于市场数据判断当前市场情绪状态
 */
void AIService::analyzeMarketSentiment(const QJsonObject& marketData,
                                        std::function<void(AIResult<AIAnalysis>)> callback)
{
    QString prompt = QString("请分析以下市场数据，判断当前市场情绪：\n\n%1")
        .arg(QString::fromUtf8(QJsonDocument(marketData).toJson()));

    chat(prompt, [callback](AIResult<QString> result) {
        if (result.isError()) {
            callback(AIResult<AIAnalysis>::error(result.errorMessage()));
            return;
        }

        AIAnalysis analysis;
        analysis.summary = result.value();
        analysis.confidence = 0.7;
        callback(AIResult<AIAnalysis>::ok(analysis));
    });
}

/**
 * @brief 快速问答
 * @param question 问题内容
 * @param callback 回答回调
 *
 * 简单的问答接口，不携带上下文
 */
void AIService::quickAsk(const QString& question,
                          std::function<void(AIResult<QString>)> callback)
{
    chat(question, callback);
}

/**
 * @brief 获取投资建议
 * @param context 上下文信息
 * @param callback 建议回调
 *
 * 基于用户提供的上下文给出投资建议
 */
void AIService::getInvestmentAdvice(const QString& context,
                                     std::function<void(AIResult<QString>)> callback)
{
    QString prompt = QString("基于以下情况，请给出投资建议：\n\n%1").arg(context);
    chat(prompt, callback);
}

QJsonObject AIService::buildRequest(const QString& message)
{
    QJsonObject request;
    request["model"] = m_config.model;
    request["messages"] = buildMessagesArray(message);
    request["max_tokens"] = m_config.maxTokens;
    request["temperature"] = m_config.temperature;
    request["stream"] = false;
    return request;
}

QJsonArray AIService::buildMessagesArray(const QString& userMessage)
{
    QJsonArray messages;

    if (!m_systemPrompt.isEmpty()) {
        QJsonObject sysMsg;
        sysMsg["role"] = "system";
        sysMsg["content"] = m_systemPrompt;
        messages.append(sysMsg);
    }

    int startIdx = qMax(0, m_history.size() - m_maxHistory);
    for (int i = startIdx; i < m_history.size(); ++i) {
        QJsonObject msg;
        msg["role"] = (m_history[i].role == AIRole::User) ? "user" : "assistant";
        msg["content"] = m_history[i].content;
        messages.append(msg);
    }

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = userMessage;
    messages.append(userMsg);

    return messages;
}

QString AIService::parseResponse(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject json = doc.object();

    QJsonArray choices = json["choices"].toArray();
    if (!choices.isEmpty()) {
        return choices[0].toObject()["message"].toObject()["content"].toString();
    }

    if (json.contains("content")) {
        QJsonArray content = json["content"].toArray();
        if (!content.isEmpty()) {
            return content[0].toObject()["text"].toString();
        }
    }

    return QString();
}

void AIService::sendRequest(const QJsonObject& request,
                             std::function<void(AIResult<QString>)> callback)
{
    QString url;
    QMap<QString, QString> headers;

    switch (m_config.provider) {
    case AIProvider::OpenAI:
        url = m_config.baseUrl.isEmpty() ? "https://api.openai.com/v1/chat/completions"
                                         : m_config.baseUrl + "/chat/completions";
        headers["Authorization"] = QString("Bearer %1").arg(m_config.apiKey);
        break;

    case AIProvider::Claude:
        url = m_config.baseUrl.isEmpty() ? "https://api.anthropic.com/v1/messages"
                                         : m_config.baseUrl + "/messages";
        headers["x-api-key"] = m_config.apiKey;
        headers["anthropic-version"] = "2023-06-01";
        break;

    case AIProvider::Local:
        url = m_config.baseUrl.isEmpty() ? "http://localhost:11434/api/chat"
                                         : m_config.baseUrl;
        break;

    default:
        url = m_config.baseUrl;
        if (!m_config.apiKey.isEmpty()) {
            headers["Authorization"] = QString("Bearer %1").arg(m_config.apiKey);
        }
    }

    headers["Content-Type"] = "application/json";

    QByteArray data = QJsonDocument(request).toJson();

    NetworkManager::instance()->postAsync(url, data,
        [this, callback](NetResult<QByteArray> result) {
            if (result.isError()) {
                callback(AIResult<QString>::error(result.errorMessage()));
                return;
            }

            QString content = parseResponse(result.value());

            if (content.isEmpty()) {
                callback(AIResult<QString>::error("Failed to parse response"));
                return;
            }

            emit responseComplete(content);
            callback(AIResult<QString>::ok(content));
        }, headers);
}
