/**
 * @file AIService.h
 * @brief AI 分析服务
 */

#ifndef AISERVICE_H
#define AISERVICE_H

#include "shared/base/Singleton.h"
#include "shared/base/Result.h"

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutex>
#include <functional>

// 简化 Result 类型使用
template<typename T>
using AIResult = WealthPilot::Result<T>;

enum class AIRole {
    User,
    Assistant,
    System
};

struct AIMessage {
    AIRole role;
    QString content;
    QDateTime timestamp;

    static AIMessage user(const QString& content) {
        return { AIRole::User, content, QDateTime::currentDateTime() };
    }

    static AIMessage assistant(const QString& content) {
        return { AIRole::Assistant, content, QDateTime::currentDateTime() };
    }

    static AIMessage system(const QString& content) {
        return { AIRole::System, content, QDateTime::currentDateTime() };
    }
};

struct AIAnalysis {
    QString summary;
    QString suggestion;
    double confidence = 0;
    QStringList keywords;
    QJsonObject rawData;

    static AIAnalysis fromJson(const QJsonObject& json);
};

enum class AIProvider {
    None,
    OpenAI,
    Claude,
    Local,
    Custom
};

struct AIConfig {
    AIProvider provider = AIProvider::None;
    QString apiKey;
    QString baseUrl;
    QString model = "gpt-4";
    int maxTokens = 2048;
    double temperature = 0.7;
    int timeout = 30000;
};

class AIService : public QObject, public Singleton<AIService>
{
    Q_OBJECT
    friend class Singleton<AIService>;

public:
    bool initialize();
    void shutdown();

    void setConfig(const AIConfig& config);
    AIConfig config() const;
    void setSystemPrompt(const QString& prompt);

    void chat(const QString& message,
              std::function<void(AIResult<QString>)> callback);

    AIResult<QString> chatSync(const QString& message, int timeoutMs = 30000);

    void chatStream(const QString& message,
                    std::function<void(const QString& chunk)> onChunk,
                    std::function<void()> onComplete);

    void clearHistory();

    void analyzePortfolio(const QJsonObject& positions,
                          std::function<void(AIResult<AIAnalysis>)> callback);

    void analyzeStock(const QString& code, const QJsonObject& data,
                      std::function<void(AIResult<AIAnalysis>)> callback);

    void analyzeMarketSentiment(const QJsonObject& marketData,
                                std::function<void(AIResult<AIAnalysis>)> callback);

    void quickAsk(const QString& question,
                  std::function<void(AIResult<QString>)> callback);

    void getInvestmentAdvice(const QString& context,
                             std::function<void(AIResult<QString>)> callback);

signals:
    void streamChunk(const QString& chunk);
    void responseComplete(const QString& fullResponse);
    void errorOccurred(const QString& error);

private:
    AIService();
    ~AIService() override;

    QJsonObject buildRequest(const QString& message);
    QJsonArray buildMessagesArray(const QString& userMessage);
    QString parseResponse(const QByteArray& data);

    void sendRequest(const QJsonObject& request,
                     std::function<void(AIResult<QString>)> callback);

    AIConfig m_config;
    QString m_systemPrompt;
    QList<AIMessage> m_history;
    int m_maxHistory = 20;
    bool m_initialized = false;

    // 缓存相关
    int m_maxHistorySize = 10;
    bool m_cacheEnabled = true;
    int m_requestCacheTimeout = 60000;

    struct RequestCacheEntry {
        QString response;
        qint64 expiryTime;
    };

    QMap<QString, RequestCacheEntry> m_requestCache;
    QMutex m_mutex;

    // 缓存管理
    void setMaxHistorySize(int size);
    void setCacheEnabled(bool enabled);
    void setCacheTimeout(int timeoutMs);
    void clearRequestCache();
    void cleanupRequestCache();
    QString generateCacheKey(const QString& message);
    QVariant getCachedResponse(const QString& key);
    void setCachedResponse(const QString& key, const QString& response);

    static const QString DEFAULT_SYSTEM_PROMPT;
};

#endif // AISERVICE_H
