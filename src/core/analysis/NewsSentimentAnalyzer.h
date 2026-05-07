/**
 * @file NewsSentimentAnalyzer.h
 * @brief 新闻情感分析器 - AI驱动的新闻情感分析
 *
 * @details 提供新闻情感分析功能：
 * - 新闻文本情感分析（正面/负面/中性）
 * - 关键词提取
 * - 影响力评估
 * - 风险提示生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef NEWSSENTIMENTANALYZER_H
#define NEWSSENTIMENTANALYZER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QDateTime>

/**
 * @brief 情感类型
 */
enum class SentimentType {
    Positive,   ///< 正面
    Negative,   ///< 负面
    Neutral     ///< 中性
};

/**
 * @brief 新闻情感分析结果
 */
struct SentimentResult {
    SentimentType sentiment = SentimentType::Neutral;  ///< 情感类型
    double confidence = 0.0;                           ///< 置信度（0-1）
    double impactScore = 0.0;                          ///< 影响力分数（-10到+10）
    QVector<QString> keywords;                         ///< 关键词
    QString summary;                                   ///< 摘要
    QString riskHint;                                  ///< 风险提示
};

/**
 * @brief 新闻条目
 */
struct NewsItem {
    QString id;                    ///< 新闻ID
    QString title;                 ///< 标题
    QString content;               ///< 内容
    QString source;                ///< 来源
    QDateTime publishTime;         ///< 发布时间
    QString category;              ///< 分类（新闻/公告/财报/研报）
    QVector<QString> relatedSymbols; ///< 相关股票代码
    SentimentResult sentiment;     ///< 情感分析结果
    int readCount = 0;             ///< 阅读量
    int commentCount = 0;          ///< 评论数
    bool isRead = false;           ///< 是否已读
    bool isImportant = false;      ///< 是否重要
};

/**
 * @brief 社交媒体热度
 */
struct SocialHeatData {
    QString symbol;                ///< 股票代码
    QString platform;              ///< 平台（雪球/微博/东方财富）
    int mentionCount = 0;          ///< 提及量
    int previousCount = 0;         ///< 上次提及量
    double changePercent = 0.0;    ///< 变化百分比
    double sentimentScore = 0.0;   ///< 情感分数
    QDateTime updateTime;          ///< 更新时间
};

/**
 * @brief 新闻情感分析器
 */
class NewsSentimentAnalyzer : public QObject
{
    Q_OBJECT

public:
    static NewsSentimentAnalyzer* instance();

    /**
     * @brief 初始化分析器
     */
    bool initialize();

    /**
     * @brief 分析新闻情感
     */
    SentimentResult analyzeSentiment(const QString& text);

    /**
     * @brief 批量分析新闻
     */
    QVector<SentimentResult> analyzeBatch(const QVector<QString>& texts);

    /**
     * @brief 提取关键词
     */
    QVector<QString> extractKeywords(const QString& text, int maxCount = 10);

    /**
     * @brief 生成摘要
     */
    QString generateSummary(const QString& text, int maxLength = 200);

    /**
     * @brief 生成风险提示
     */
    QString generateRiskHint(const SentimentResult& sentiment, const QString& symbol);

    /**
     * @brief 计算影响力分数
     */
    double calculateImpactScore(const SentimentResult& sentiment, const NewsItem& news);

signals:
    void analysisCompleted(const QString& newsId, const SentimentResult& result);

private:
    explicit NewsSentimentAnalyzer(QObject* parent = nullptr);
    ~NewsSentimentAnalyzer() override = default;

    // 情感词典
    QMap<QString, double> m_positiveWords;
    QMap<QString, double> m_negativeWords;
    QVector<QString> m_stopWords;

    // 金融关键词
    QVector<QString> m_financialKeywords;

    bool m_initialized = false;
};

#endif // NEWSSENTIMENTANALYZER_H