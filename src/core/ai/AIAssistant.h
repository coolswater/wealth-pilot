/**
 * @file AIAssistant.h
 * @brief AI 智能助手 - 自然语言查询与智能推荐
 *
 * @details 功能：
 * - 自然语言查询
 * - 智能推荐
 * - 意图识别
 * - 多轮对话
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef AIASSISTANT_H
#define AIASSISTANT_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QHash>
#include <QVariant>
#include <QDateTime>

/**
 * @brief 用户意图类型
 */
enum class UserIntent {
    QueryStock,         ///< 查询股票
    QueryMarket,        ///< 查询行情
    QueryAccount,       ///< 查询账户
    PlaceOrder,         ///< 下单
    CancelOrder,        ///< 撤单
    AnalyzeStock,       ///< 分析股票
    GetRecommendation,  ///< 获取推荐
    SetAlert,           ///< 设置预警
    GeneralQuestion,    ///< 一般问题
    Unknown             ///< 未知
};

/**
 * @brief 意图识别结果
 */
struct IntentResult {
    UserIntent intent = UserIntent::Unknown;
    double confidence = 0.0;
    QHash<QString, QString> entities;   ///< 实体（股票代码、价格等）
    QString originalQuery;
};

/**
 * @brief AI 回复
 */
struct AIResponse {
    QString text;               ///< 回复文本
    QString action;             ///< 建议动作
    QVariantMap data;           ///< 相关数据
    QVector<QString> suggestions; ///< 建议的后续问题
    bool success = true;
};

/**
 * @brief 对话上下文
 */
struct ConversationContext {
    QString sessionId;
    QVector<QString> history;   ///< 对话历史
    UserIntent lastIntent;
    QString lastSymbol;
    QDateTime lastTime;
};

/**
 * @brief 推荐结果
 */
struct Recommendation {
    QString symbol;             ///< 股票代码
    QString name;               ///< 股票名称
    double score = 0.0;         ///< 推荐分数
    QString reason;             ///< 推荐理由
    QString category;           ///< 分类
    double targetPrice = 0.0;   ///< 目标价
    double stopLoss = 0.0;      ///< 止损价
};

/**
 * @brief AI 智能助手
 *
 * 提供智能交互功能：
 * - 自然语言理解
 * - 智能推荐
 * - 多轮对话
 */
class AIAssistant : public QObject {
    Q_OBJECT

public:
    static AIAssistant* instance();

    // ========== 自然语言查询 ==========

    /**
     * @brief 处理用户输入
     * @param input 用户输入
     * @return AI 回复
     */
    AIResponse processInput(const QString& input);

    /**
     * @brief 异步处理用户输入
     */
    void processInputAsync(const QString& input);

    /**
     * @brief 识别用户意图
     */
    IntentResult recognizeIntent(const QString& input);

    /**
     * @brief 提取实体
     */
    QHash<QString, QString> extractEntities(const QString& input);

    // ========== 智能推荐 ==========

    /**
     * @brief 获取个性化推荐
     */
    QVector<Recommendation> getPersonalizedRecommendations();

    /**
     * @brief 获取市场热点
     */
    QVector<Recommendation> getMarketHotspots();

    /**
     * @brief 获取相似股票
     */
    QVector<Recommendation> getSimilarStocks(const QString& symbol);

    /**
     * @brief 设置用户偏好
     */
    void setUserPreferences(const QHash<QString, QVariant>& preferences);

    // ========== 对话管理 ==========

    /**
     * @brief 开始新对话
     */
    QString startConversation();

    /**
     * @brief 结束对话
     */
    void endConversation(const QString& sessionId);

    /**
     * @brief 获取对话历史
     */
    QVector<QString> getConversationHistory(const QString& sessionId) const;

    /**
     * @brief 清除对话历史
     */
    void clearConversationHistory(const QString& sessionId);

    // ========== 语音支持 ==========

    /**
     * @brief 启用语音输入
     */
    void enableVoiceInput(bool enabled);

    /**
     * @brief 是否启用语音输入
     */
    bool isVoiceInputEnabled() const { return m_voiceInputEnabled; }

    /**
     * @brief 文本转语音
     */
    void speak(const QString& text);

signals:
    /**
     * @brief 回复就绪
     */
    void responseReady(const AIResponse& response);

    /**
     * @brief 推荐更新
     */
    void recommendationsUpdated(const QVector<Recommendation>& recommendations);

    /**
     * @brief 语音识别结果
     */
    void voiceRecognized(const QString& text);

    /**
     * @brief 错误发生
     */
    void errorOccurred(const QString& error);

private:
    explicit AIAssistant(QObject* parent = nullptr);
    ~AIAssistant() override = default;

    AIResponse handleQueryStock(const IntentResult& intent);
    AIResponse handleQueryMarket(const IntentResult& intent);
    AIResponse handleQueryAccount(const IntentResult& intent);
    AIResponse handlePlaceOrder(const IntentResult& intent);
    AIResponse handleAnalyzeStock(const IntentResult& intent);
    AIResponse handleGetRecommendation(const IntentResult& intent);
    AIResponse handleSetAlert(const IntentResult& intent);
    AIResponse handleGeneralQuestion(const IntentResult& intent);

    QString formatStockInfo(const QString& symbol);
    QString formatMarketInfo();
    QString formatAccountInfo();
    QString formatStockAnalysis(const QString& symbol);

    QHash<QString, ConversationContext> m_conversations;
    QString m_currentSession;
    QHash<QString, QVariant> m_userPreferences;
    bool m_voiceInputEnabled = false;

    // 意图关键词映射
    QHash<UserIntent, QVector<QString>> m_intentKeywords;
    void initializeIntentKeywords();
};

#endif // AIASSISTANT_H