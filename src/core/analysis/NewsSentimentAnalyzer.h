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
#include "core/types/NewsTypes.h"

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::NewsItem;
using WealthPilot::SentimentType;
using WealthPilot::NewsSentimentResult;
using WealthPilot::SocialHeatData;

// 为了兼容旧代码，保留 NewsSentimentType 别名
using NewsSentimentType = SentimentType;

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
    NewsSentimentResult analyzeSentiment(const QString& text);

    /**
     * @brief 批量分析新闻
     */
    QVector<NewsSentimentResult> analyzeBatch(const QVector<QString>& texts);

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
    QString generateRiskHint(const NewsSentimentResult& sentiment, const QString& symbol);

    /**
     * @brief 计算影响力分数
     */
    double calculateImpactScore(const NewsSentimentResult& sentiment, const NewsItem& news);

signals:
    void analysisCompleted(const QString& newsId, const NewsSentimentResult& result);

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