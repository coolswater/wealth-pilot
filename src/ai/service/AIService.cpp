/**
 * @file AIService.cpp
 * @brief AI 分析服务实现
 */

#include "AIService.h"
#include "../../network/NetworkManager.h"
#include "../../core/config/ConfigManager.h"
#include "../../utils/Logger.h"

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

void AIService::setMaxHistorySize(int size)
{
    m_maxHistorySize = size;
    clearHistory();
}

void AIService::setCacheEnabled(bool enabled)
{
    m_cacheEnabled = enabled;
    if (!enabled) {
        clearRequestCache();
    }
}

void AIService::setCacheTimeout(int timeoutMs)
{
    m_requestCacheTimeout = timeoutMs;
    clearRequestCache();
}

void AIService::shutdown()
{
    clearHistory();
    clearRequestCache();
    m_initialized = false;
    LOG_INFO("AIService shutdown");
}

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

void AIService::chat(const QString& message, std::function<void(Result<QString>)> callback)
{
    if (m_config.provider == AIProvider::None || m_config.apiKey.isEmpty()) {
        callback(Result<QString>::err(ErrorCode::AiError, "AI service not configured"));
        return;
    }

    // 检查缓存
    if (m_cacheEnabled) {
        QString cacheKey = generateCacheKey(message);
        QVariant cached = getCachedResponse(cacheKey);
        if (cached.isValid()) {
            QString cachedResponse = cached.toString();
            m_history.append(AIMessage::assistant(cachedResponse));
            callback(Result<QString>::ok(cachedResponse));
            return;
        }
    }

    m_history.append(AIMessage::user(message));

    QJsonObject request = buildRequest(message);

    sendRequest(request, [this, callback, message, cacheKey = generateCacheKey(message)](Result<QString> result) {
        if (result.isOk()) {
            QString response = result.unwrap();
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

Result<QString> AIService::chatSync(const QString& message, int timeoutMs)
{
    QString resultValue;
    ErrorCode errorCode = ErrorCode::Success;
    QString errorMessage;
    bool success = false;
    QEventLoop loop;

    chat(message, [&](Result<QString> r) {
        if (r.isOk()) {
            resultValue = r.unwrap();
            success = true;
        } else {
            errorCode = r.errorCode();
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
        return Result<QString>::err(ErrorCode::NetworkTimeout, "AI request timeout");
    }

    if (success) {
        return Result<QString>::ok(resultValue);
    } else {
        return Result<QString>::err(errorCode, errorMessage);
    }
}

void AIService::chatStream(const QString& message,
                           std::function<void(const QString&)> onChunk,
                           std::function<void()> onComplete)
{
    chat(message, [onChunk, onComplete](Result<QString> result) {
        if (result.isOk()) {
            onChunk(result.unwrap());
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

void AIService::analyzePortfolio(const QJsonObject& positions,
                                  std::function<void(Result<AIAnalysis>)> callback)
{
    QString prompt = QString("请分析以下投资组合，并提供投资建议：\n\n%1")
        .arg(QString::fromUtf8(QJsonDocument(positions).toJson()));

    chat(prompt, [callback](Result<QString> result) {
        if (result.isError()) {
            callback(Result<AIAnalysis>::fromError(result.error()));
            return;
        }

        AIAnalysis analysis;
        analysis.summary = result.unwrap();
        analysis.confidence = 0.8;
        callback(Result<AIAnalysis>::ok(analysis));
    });
}

void AIService::analyzeStock(const QString& code, const QJsonObject& data,
                              std::function<void(Result<AIAnalysis>)> callback)
{
    QString prompt = QString("请分析股�?%1 的以下数据：\n\n%2\n\n请提供：\n1. 技术分析\n2. 基本面分析\n3. 投资建议")
        .arg(code)
        .arg(QString::fromUtf8(QJsonDocument(data).toJson()));

    chat(prompt, [callback](Result<QString> result) {
        if (result.isError()) {
            callback(Result<AIAnalysis>::fromError(result.error()));
            return;
        }

        AIAnalysis analysis;
        analysis.summary = result.unwrap();
        analysis.confidence = 0.75;
        callback(Result<AIAnalysis>::ok(analysis));
    });
}

void AIService::analyzeMarketSentiment(const QJsonObject& marketData,
                                        std::function<void(Result<AIAnalysis>)> callback)
{
    QString prompt = QString("请分析以下市场数据，判断当前市场情绪：\n\n%1")
        .arg(QString::fromUtf8(QJsonDocument(marketData).toJson()));

    chat(prompt, [callback](Result<QString> result) {
        if (result.isError()) {
            callback(Result<AIAnalysis>::fromError(result.error()));
            return;
        }

        AIAnalysis analysis;
        analysis.summary = result.unwrap();
        analysis.confidence = 0.7;
        callback(Result<AIAnalysis>::ok(analysis));
    });
}

void AIService::quickAsk(const QString& question,
                          std::function<void(Result<QString>)> callback)
{
    chat(question, callback);
}

void AIService::getInvestmentAdvice(const QString& context,
                                     std::function<void(Result<QString>)> callback)
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
                             std::function<void(Result<QString>)> callback)
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
        [this, callback](Result<QByteArray> result) {
            if (result.isError()) {
                callback(Result<QString>::err(result.errorCode(), result.errorMessage()));
                return;
            }

            QString content = parseResponse(result.unwrap());

            if (content.isEmpty()) {
                callback(Result<QString>::err(ErrorCode::AiParseError, "Failed to parse response"));
                return;
            }

            emit responseComplete(content);
            callback(Result<QString>::ok(content));
        }, headers);
}
