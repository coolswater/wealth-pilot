/**
 * @file NewsSentimentAnalyzer.cpp
 * @brief 新闻情感分析器实现
 */

#include "NewsSentimentAnalyzer.h"
#include "utils/Logger.h"
#include <QRegularExpression>
#include <QTextStream>
#include <algorithm>

NewsSentimentAnalyzer* NewsSentimentAnalyzer::instance()
{
    static NewsSentimentAnalyzer* inst = new NewsSentimentAnalyzer();
    return inst;
}

NewsSentimentAnalyzer::NewsSentimentAnalyzer(QObject* parent)
    : QObject(parent)
{
}

bool NewsSentimentAnalyzer::initialize()
{
    if (m_initialized) return true;

    LOG_INFO("Initializing News Sentiment Analyzer");

    // 初始化正面词汇词典
    m_positiveWords = {
        {QStringLiteral("上涨"), 2.0},
        {QStringLiteral("增长"), 1.8},
        {QStringLiteral("盈利"), 2.0},
        {QStringLiteral("利好"), 2.5},
        {QStringLiteral("突破"), 1.5},
        {QStringLiteral("创新高"), 2.0},
        {QStringLiteral("业绩大增"), 2.5},
        {QStringLiteral("超预期"), 2.0},
        {QStringLiteral("并购"), 1.5},
        {QStringLiteral("分红"), 1.2},
        {QStringLiteral("回购"), 1.5},
        {QStringLiteral("增持"), 1.3},
        {QStringLiteral("涨停"), 2.5},
        {QStringLiteral("牛市"), 2.0},
        {QStringLiteral("反弹"), 1.0},
        {QStringLiteral("回暖"), 1.2},
        {QStringLiteral("优化"), 0.8},
        {QStringLiteral("扩张"), 1.0},
        {QStringLiteral("领先"), 1.0},
        {QStringLiteral("突破"), 1.5}
    };

    // 初始化负面词汇词典
    m_negativeWords = {
        {QStringLiteral("下跌"), -2.0},
        {QStringLiteral("亏损"), -2.5},
        {QStringLiteral("利空"), -2.5},
        {QStringLiteral("暴跌"), -3.0},
        {QStringLiteral("跌停"), -3.0},
        {QStringLiteral("减持"), -1.5},
        {QStringLiteral("清仓"), -2.0},
        {QStringLiteral("爆雷"), -3.0},
        {QStringLiteral("违约"), -3.0},
        {QStringLiteral("诉讼"), -2.0},
        {QStringLiteral("调查"), -1.8},
        {QStringLiteral("处罚"), -2.0},
        {QStringLiteral("退市"), -3.5},
        {QStringLiteral("熊市"), -2.0},
        {QStringLiteral("下滑"), -1.5},
        {QStringLiteral("萎缩"), -1.5},
        {QStringLiteral("裁员"), -2.0},
        {QStringLiteral("关闭"), -1.5},
        {QStringLiteral("风险"), -0.8},
        {QStringLiteral("预警"), -1.0}
    };

    // 停用词
    m_stopWords = {
        QStringLiteral("的"), QStringLiteral("了"), QStringLiteral("在"),
        QStringLiteral("是"), QStringLiteral("我"), QStringLiteral("有"),
        QStringLiteral("和"), QStringLiteral("就"), QStringLiteral("不"),
        QStringLiteral("人"), QStringLiteral("都"), QStringLiteral("一"),
        QStringLiteral("一个"), QStringLiteral("上"), QStringLiteral("也"),
        QStringLiteral("很"), QStringLiteral("到"), QStringLiteral("说"),
        QStringLiteral("要"), QStringLiteral("去"), QStringLiteral("你"),
        QStringLiteral("会"), QStringLiteral("着"), QStringLiteral("没有"),
        QStringLiteral("看"), QStringLiteral("好"), QStringLiteral("自己")
    };

    // 金融关键词
    m_financialKeywords = {
        QStringLiteral("业绩"), QStringLiteral("营收"), QStringLiteral("利润"),
        QStringLiteral("市值"), QStringLiteral("估值"), QStringLiteral("PE"),
        QStringLiteral("PB"), QStringLiteral("ROE"), QStringLiteral("毛利率"),
        QStringLiteral("净利率"), QStringLiteral("现金流"), QStringLiteral("资产负债率"),
        QStringLiteral("分红"), QStringLiteral("回购"), QStringLiteral("增发"),
        QStringLiteral("并购"), QStringLiteral("重组"), QStringLiteral("IPO"),
        QStringLiteral("研报"), QStringLiteral("评级"), QStringLiteral("目标价")
    };

    m_initialized = true;
    LOG_INFO("News Sentiment Analyzer initialized");
    return true;
}

SentimentResult NewsSentimentAnalyzer::analyzeSentiment(const QString& text)
{
    SentimentResult result;

    if (text.isEmpty()) {
        return result;
    }

    // 计算情感分数
    double positiveScore = 0.0;
    double negativeScore = 0.0;
    int positiveCount = 0;
    int negativeCount = 0;

    // 匹配正面词汇
    for (auto it = m_positiveWords.begin(); it != m_positiveWords.end(); ++it) {
        if (text.contains(it.key())) {
            positiveScore += it.value();
            positiveCount++;
        }
    }

    // 匹配负面词汇
    for (auto it = m_negativeWords.begin(); it != m_negativeWords.end(); ++it) {
        if (text.contains(it.key())) {
            negativeScore += it.value();
            negativeCount++;
        }
    }

    // 计算总分数
    double totalScore = positiveScore + negativeScore;

    // 确定情感类型
    if (totalScore > 0.5) {
        result.sentiment = SentimentType::Positive;
        result.impactScore = qMin(totalScore, 10.0);
    } else if (totalScore < -0.5) {
        result.sentiment = SentimentType::Negative;
        result.impactScore = qMax(totalScore, -10.0);
    } else {
        result.sentiment = SentimentType::Neutral;
        result.impactScore = totalScore;
    }

    // 计算置信度
    int totalMatches = positiveCount + negativeCount;
    if (totalMatches > 0) {
        double maxPossibleScore = qMax(positiveScore, -negativeScore);
        result.confidence = qMin(totalMatches / 10.0, 1.0);
    }

    // 提取关键词
    result.keywords = extractKeywords(text, 5);

    // 生成摘要
    result.summary = generateSummary(text);

    // 生成风险提示
    result.riskHint = QStringLiteral("基于情感分析的建议");

    return result;
}

QVector<SentimentResult> NewsSentimentAnalyzer::analyzeBatch(const QVector<QString>& texts)
{
    QVector<SentimentResult> results;
    results.reserve(texts.size());

    for (const QString& text : texts) {
        results.append(analyzeSentiment(text));
    }

    return results;
}

QVector<QString> NewsSentimentAnalyzer::extractKeywords(const QString& text, int maxCount)
{
    QVector<QString> keywords;

    // 简单的关键词提取：匹配金融关键词
    for (const QString& keyword : m_financialKeywords) {
        if (text.contains(keyword)) {
            keywords.append(keyword);
            if (keywords.size() >= maxCount) {
                break;
            }
        }
    }

    // 如果金融关键词不足，提取高频词
    if (keywords.size() < maxCount) {
        QStringList words = text.split(QRegularExpression(QStringLiteral("[\\s,，。！？、；：\"'（）【】《》]+")));

        QMap<QString, int> wordCount;
        for (const QString& word : words) {
            if (word.length() >= 2 && !m_stopWords.contains(word)) {
                wordCount[word]++;
            }
        }

        // 排序并取前N个
        QVector<QPair<QString, int>> sortedWords;
        for (auto it = wordCount.begin(); it != wordCount.end(); ++it) {
            sortedWords.append({it.key(), it.value()});
        }

        std::sort(sortedWords.begin(), sortedWords.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });

        for (int i = 0; i < qMin(maxCount - keywords.size(), sortedWords.size()); ++i) {
            if (!keywords.contains(sortedWords[i].first)) {
                keywords.append(sortedWords[i].first);
            }
        }
    }

    return keywords;
}

QString NewsSentimentAnalyzer::generateSummary(const QString& text, int maxLength)
{
    if (text.length() <= maxLength) {
        return text;
    }

    // 简单的摘要生成：取前maxLength个字符
    QString summary = text.left(maxLength);

    // 尝试在句号处截断
    int lastPeriod = summary.lastIndexOf(QStringLiteral("。"));
    if (lastPeriod > maxLength / 2) {
        summary = summary.left(lastPeriod + 1);
    } else {
        summary += QStringLiteral("...");
    }

    return summary;
}

QString NewsSentimentAnalyzer::generateRiskHint(const SentimentResult& sentiment, const QString& symbol)
{
    QString hint;

    if (sentiment.sentiment == SentimentType::Negative) {
        if (sentiment.impactScore < -5.0) {
            hint = QStringLiteral("【高风险】%1 相关新闻呈现强烈负面情绪，建议密切关注。").arg(symbol);
        } else if (sentiment.impactScore < -2.0) {
            hint = QStringLiteral("【中风险】%1 相关新闻存在负面因素，请注意风险。").arg(symbol);
        } else {
            hint = QStringLiteral("【提示】%1 相关新闻略有负面情绪。").arg(symbol);
        }
    } else if (sentiment.sentiment == SentimentType::Positive) {
        if (sentiment.impactScore > 5.0) {
            hint = QStringLiteral("【利好】%1 相关新闻呈现强烈正面情绪，可关注机会。").arg(symbol);
        } else {
            hint = QStringLiteral("【积极】%1 相关新闻整体偏正面。").arg(symbol);
        }
    } else {
        hint = QStringLiteral("【中性】%1 相关新闻情绪平稳。").arg(symbol);
    }

    return hint;
}

double NewsSentimentAnalyzer::calculateImpactScore(const SentimentResult& sentiment, const NewsItem& news)
{
    double baseScore = sentiment.impactScore;

    // 根据新闻类型调整
    double typeMultiplier = 1.0;
    if (news.category == QStringLiteral("公告")) {
        typeMultiplier = 1.5;
    } else if (news.category == QStringLiteral("财报")) {
        typeMultiplier = 2.0;
    } else if (news.category == QStringLiteral("研报")) {
        typeMultiplier = 1.3;
    }

    // 根据阅读量调整
    double readMultiplier = 1.0;
    if (news.readCount > 10000) {
        readMultiplier = 1.5;
    } else if (news.readCount > 5000) {
        readMultiplier = 1.3;
    }

    return baseScore * typeMultiplier * readMultiplier;
}
