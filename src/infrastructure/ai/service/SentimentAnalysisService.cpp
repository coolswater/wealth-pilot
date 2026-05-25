/**
 * @file SentimentAnalysisService.cpp
 * @brief 情绪分析服务实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "SentimentAnalysisService.h"
#include "AIService.h"
#include "shared/utils/Logger.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include <QRegularExpression>

namespace WealthPilot
{

struct SentimentAnalysisService::Impl
{
    QString storagePath;
    QMap<QString, SentimentIndex> sentimentIndices;
    QStringList watchList;
    QJsonObject analysisParams;
};

SentimentAnalysisService::SentimentAnalysisService(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    d->storagePath = appDataPath + "/sentiment";
    QDir dir(d->storagePath);
    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    LOG_DEBUG("SentimentAnalysisService created");
}

SentimentAnalysisService::~SentimentAnalysisService()
{
    LOG_DEBUG("SentimentAnalysisService destroyed");
}

void SentimentAnalysisService::analyzeNews(const NewsItem& news,
                                           std::function<void(const SentimentResult&)> callback)
{
    QString text = news.title + "\n" + news.content;
    analyzeTextWithAI(text, [this, news, callback](const SentimentResult& result)
    {
        for (const auto& stockCode : news.relatedStocks)
        {
            updateSentimentIndex(stockCode, result);
        }
        callback(result);
    });
}

void SentimentAnalysisService::analyzeNewsBatch(const QList<NewsItem>& newsList,
                                                std::function<void(const QList<SentimentResult>&)> callback)
{
    QList<SentimentResult> results;
    int pending = newsList.size();

    for (const auto& news : newsList)
    {
        analyzeNews(news, [&results, &pending, callback](const SentimentResult& result)
        {
            results.append(result);
            pending--;

            if (pending == 0)
            {
                callback(results);
            }
        });
    }
}

void SentimentAnalysisService::getStockNewsSentiment(const QString& stockCode,
                                                     std::function<void(const SentimentResult&)> callback)
{
    QString prompt = QString(QStringLiteral(
        "请分析股票 %1 最近的市场情绪：\n\n"
        "请从以下角度分析：\n"
        "1. 新闻报道情绪\n"
        "2. 市场关注度\n"
        "3. 投资者情绪\n\n"
        "请给出情绪评分（-1到1）和简要分析。"
    )).arg(stockCode);

    AIService::instance()->chat(prompt, [this, stockCode, callback](Result<QString> result)
    {
        if (result.isError())
        {
            emit sentimentAlert(stockCode, result.errorMessage());
            callback(SentimentResult());
            return;
        }

        SentimentResult sentiment;
        sentiment.summary = result.value();
        sentiment.score = 0;
        sentiment.sentiment = SentimentType::Neutral;
        sentiment.confidence = 0.7;
        sentiment.analyzedAt = QDateTime::currentDateTime();

        updateSentimentIndex(stockCode, sentiment);
        emit analysisCompleted(stockCode, sentiment);

        callback(sentiment);
    });
}

void SentimentAnalysisService::getMarketSentiment(std::function<void(const SentimentResult&)> callback)
{
    QString prompt = QStringLiteral(
        "请分析当前 A 股市场的整体情绪：\n\n"
        "请从以下角度分析：\n"
        "1. 大盘走势情绪\n"
        "2. 板块轮动情况\n"
        "3. 资金流向情绪\n"
        "4. 投资者信心\n\n"
        "请给出市场情绪评分（-1到1）和简要分析。"
    );

    AIService::instance()->chat(prompt, [this, callback](Result<QString> result)
    {
        if (result.isError())
        {
            callback(SentimentResult());
            return;
        }

        SentimentResult sentiment;
        sentiment.summary = result.value();
        sentiment.score = 0;
        sentiment.sentiment = SentimentType::Neutral;
        sentiment.confidence = 0.6;
        sentiment.analyzedAt = QDateTime::currentDateTime();

        emit marketSentimentUpdated(sentiment);
        callback(sentiment);
    });
}

void SentimentAnalysisService::getSectorSentiment(const QString& sectorName,
                                                  std::function<void(const SentimentResult&)> callback)
{
    QString prompt = QString(QStringLiteral(
        "请分析 %1 板块的市场情绪：\n\n"
        "请分析该板块的：\n"
        "1. 政策面情绪\n"
        "2. 基本面情绪\n"
        "3. 资金面情绪\n\n"
        "请给出情绪评分（-1到1）和简要分析。"
    )).arg(sectorName);

    AIService::instance()->chat(prompt, [this, callback](Result<QString> result)
    {
        if (result.isError())
        {
            callback(SentimentResult());
            return;
        }

        SentimentResult sentiment;
        sentiment.summary = result.value();
        sentiment.score = 0;
        sentiment.sentiment = SentimentType::Neutral;
        sentiment.confidence = 0.65;
        sentiment.analyzedAt = QDateTime::currentDateTime();

        callback(sentiment);
    });
}

void SentimentAnalysisService::getStockSentimentIndex(const QString& stockCode,
                                                      std::function<void(const SentimentIndex&)> callback)
{
    if (d->sentimentIndices.contains(stockCode))
    {
        callback(d->sentimentIndices[stockCode]);
    }
    else
    {
        // 计算新的情绪指标
        getStockNewsSentiment(stockCode, [this, stockCode, callback](const SentimentResult& result)
        {
            SentimentIndex index;
            index.stockCode = stockCode;
            index.currentScore = result.score;
            index.updatedAt = QDateTime::currentDateTime();

            SentimentPoint point;
            point.timestamp = QDateTime::currentDateTime();
            point.score = result.score;
            point.positive = result.positive;
            point.negative = result.negative;
            point.sampleCount = 1;
            index.history.append(point);

            d->sentimentIndices[stockCode] = index;
            callback(index);
        });
    }
}

void SentimentAnalysisService::analyzeSocialMedia(const QString& platform,
                                                  const QString& stockCode,
                                                  std::function<void(const SentimentResult&)> callback)
{
    QString prompt = QString(QStringLiteral(
        "请分析 %1 平台上股票 %2 的社交媒体情绪：\n\n"
        "请分析用户讨论的：\n"
        "1. 情绪倾向\n"
        "2. 热门话题\n"
        "3. 关注焦点\n\n"
        "请给出情绪评分（-1到1）。"
    )).arg(platform, stockCode);

    AIService::instance()->chat(prompt, [this, callback](Result<QString> result)
    {
        if (result.isError())
        {
            callback(SentimentResult());
            return;
        }

        SentimentResult sentiment;
        sentiment.summary = result.value();
        sentiment.score = 0;
        sentiment.sentiment = SentimentType::Neutral;
        sentiment.confidence = 0.5;
        sentiment.analyzedAt = QDateTime::currentDateTime();

        callback(sentiment);
    });
}

void SentimentAnalysisService::getSocialHotspots(std::function<void(const QList<QString>&)> callback)
{
    // TODO: 实现社交媒体热点获取
    callback(QList<QString>());
}

void SentimentAnalysisService::getSentimentTrend(const QString& stockCode,
                                                 const QDateTime& from,
                                                 const QDateTime& to,
                                                 std::function<void(const QList<SentimentPoint>&)> callback)
{
    if (!d->sentimentIndices.contains(stockCode))
    {
        callback(QList<SentimentPoint>());
        return;
    }

    const auto& index = d->sentimentIndices[stockCode];
    QList<SentimentPoint> trend;

    for (const auto& point : index.history)
    {
        if (point.timestamp >= from && point.timestamp <= to)
        {
            trend.append(point);
        }
    }

    callback(trend);
}

void SentimentAnalysisService::getSentimentAlerts(std::function<void(const QList<QString>&)> callback)
{
    // TODO: 实现情绪预警获取
    callback(QList<QString>());
}

void SentimentAnalysisService::setAnalysisParams(const QJsonObject& params)
{
    d->analysisParams = params;
}

QJsonObject SentimentAnalysisService::getAnalysisParams() const
{
    return d->analysisParams;
}

void SentimentAnalysisService::analyzeTextWithAI(const QString& text,
                                                 std::function<void(const SentimentResult&)> callback)
{
    QString prompt = QString(QStringLiteral(
        "请分析以下文本的情绪：\n\n"
        "%1\n\n"
        "请返回 JSON 格式的分析结果：\n"
        "{\n"
        "  \"score\": 情绪得分(-1到1),\n"
        "  \"positive\": 正面情绪占比(0到1),\n"
        "  \"negative\": 负面情绪占比(0到1),\n"
        "  \"neutral\": 中性情绪占比(0到1),\n"
        "  \"keywords\": [\"关键词1\", \"关键词2\"],\n"
        "  \"summary\": \"简要分析\"\n"
        "}"
    )).arg(text.left(1000));

    AIService::instance()->chat(prompt, [this, callback](Result<QString> result)
    {
        if (result.isError())
        {
            callback(SentimentResult());
            return;
        }

        QString response = result.value();
        QRegularExpression jsonRegex("\\{[\\s\\S]*\\}");
        QRegularExpressionMatch match = jsonRegex.match(response);

        SentimentResult sentiment;
        sentiment.analyzedAt = QDateTime::currentDateTime();

        if (match.hasMatch())
        {
            QString jsonStr = match.captured(0);
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            QJsonObject json = doc.object();

            sentiment.score = json["score"].toDouble();
            sentiment.positive = json["positive"].toDouble();
            sentiment.negative = json["negative"].toDouble();
            sentiment.neutral = json["neutral"].toDouble();
            sentiment.summary = json["summary"].toString();
            sentiment.sentiment = scoreToType(sentiment.score);
            sentiment.confidence = 0.8;

            QJsonArray keywordsArray = json["keywords"].toArray();
            for (const auto& k : keywordsArray)
            {
                sentiment.keywords.append(k.toString());
            }
        }
        else
        {
            sentiment.summary = response;
            sentiment.sentiment = SentimentType::Neutral;
            sentiment.confidence = 0.5;
        }

        callback(sentiment);
    });
}

void SentimentAnalysisService::updateSentimentIndex(const QString& stockCode, const SentimentResult& result)
{
    SentimentIndex index;

    if (d->sentimentIndices.contains(stockCode))
    {
        index = d->sentimentIndices[stockCode];
    }

    index.stockCode = stockCode;
    index.currentScore = result.score;
    index.updatedAt = QDateTime::currentDateTime();

    SentimentPoint point;
    point.timestamp = QDateTime::currentDateTime();
    point.score = result.score;
    point.positive = result.positive;
    point.negative = result.negative;
    point.sampleCount = 1;
    index.history.append(point);

    // 保留最近90天数据
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-90);
    index.history.erase(
        std::remove_if(index.history.begin(), index.history.end(),
                       [cutoff](const SentimentPoint& p) { return p.timestamp < cutoff; }),
        index.history.end());

    // 计平均值
    if (!index.history.isEmpty())
    {
        double sum7d = 0, sum30d = 0;
        int count7d = 0, count30d = 0;

        QDateTime now = QDateTime::currentDateTime();
        for (const auto& p : index.history)
        {
            int daysAgo = p.timestamp.daysTo(now);
            if (daysAgo <= 7)
            {
                sum7d += p.score;
                count7d++;
            }
            if (daysAgo <= 30)
            {
                sum30d += p.score;
                count30d++;
            }
        }

        index.avgScore7d = count7d > 0 ? sum7d / count7d : 0;
        index.avgScore30d = count30d > 0 ? sum30d / count30d : 0;

        if (index.avgScore30d != 0)
        {
            index.trend = (index.avgScore7d - index.avgScore30d) / qAbs(index.avgScore30d);
            index.trend = qBound(-1.0, index.trend, 1.0);
        }

        if (index.trend > 0.1)
            index.trendDirection = QStringLiteral("上升");
        else if (index.trend < -0.1)
            index.trendDirection = QStringLiteral("下降");
        else
            index.trendDirection = QStringLiteral("平稳");
    }

    d->sentimentIndices[stockCode] = index;
}

SentimentType SentimentAnalysisService::scoreToType(double score)
{
    if (score < -0.6) return SentimentType::VeryNegative;
    if (score < -0.2) return SentimentType::Negative;
    if (score < 0.2) return SentimentType::Neutral;
    if (score < 0.6) return SentimentType::Positive;
    return SentimentType::VeryPositive;
}

} // namespace WealthPilot