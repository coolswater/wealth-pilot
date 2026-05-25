/**
 * @file NewsTypes.cpp
 * @brief 新闻类型定义的实现
 */

#include "NewsTypes.h"
#include <QJsonDocument>
#include <QJsonArray>

namespace WealthPilot {

// ============================================================================
// SentimentResult 实现
// ============================================================================

SentimentResult SentimentResult::fromJson(const QJsonObject& json)
{
    SentimentResult result;
    result.positive = json["positive"].toDouble();
    result.negative = json["negative"].toDouble();
    result.neutral = json["neutral"].toDouble();
    result.score = json["score"].toDouble();
    result.sentiment = static_cast<SentimentType>(json["type"].toInt());
    result.confidence = json["confidence"].toDouble();
    result.summary = json["summary"].toString();
    result.analyzedAt = QDateTime::fromString(json["analyzedAt"].toString(), Qt::ISODate);
    result.impactScore = json["impactScore"].toDouble();
    result.riskHint = json["riskHint"].toString();
    result.sentimentLabel = json["sentimentLabel"].toString();

    QJsonArray keywordsArray = json["keywords"].toArray();
    for (const auto& k : keywordsArray)
    {
        result.keywords.append(k.toString());
    }

    return result;
}

QJsonObject SentimentResult::toJson() const
{
    QJsonObject json;
    json["positive"] = positive;
    json["negative"] = negative;
    json["neutral"] = neutral;
    json["score"] = score;
    json["type"] = static_cast<int>(sentiment);
    json["confidence"] = confidence;
    json["summary"] = summary;
    json["analyzedAt"] = analyzedAt.toString(Qt::ISODate);
    json["impactScore"] = impactScore;
    json["riskHint"] = riskHint;
    json["sentimentLabel"] = sentimentLabel;

    QJsonArray keywordsArray;
    for (const auto& k : keywords)
    {
        keywordsArray.append(k);
    }
    json["keywords"] = keywordsArray;

    return json;
}

// ============================================================================
// SentimentPoint 实现
// ============================================================================

SentimentPoint SentimentPoint::fromJson(const QJsonObject& json)
{
    SentimentPoint point;
    point.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
    point.score = json["score"].toDouble();
    point.positive = json["positive"].toDouble();
    point.negative = json["negative"].toDouble();
    point.sampleCount = json["sampleCount"].toInt();
    return point;
}

QJsonObject SentimentPoint::toJson() const
{
    QJsonObject json;
    json["timestamp"] = timestamp.toString(Qt::ISODate);
    json["score"] = score;
    json["positive"] = positive;
    json["negative"] = negative;
    json["sampleCount"] = sampleCount;
    return json;
}

// ============================================================================
// SentimentIndex 实现
// ============================================================================

SentimentIndex SentimentIndex::fromJson(const QJsonObject& json)
{
    SentimentIndex index;
    index.stockCode = json["stockCode"].toString();
    index.currentScore = json["currentScore"].toDouble();
    index.avgScore7d = json["avgScore7d"].toDouble();
    index.avgScore30d = json["avgScore30d"].toDouble();
    index.trend = json["trend"].toDouble();
    index.trendDirection = json["trendDirection"].toString();
    index.updatedAt = QDateTime::fromString(json["updatedAt"].toString(), Qt::ISODate);

    QJsonArray historyArray = json["history"].toArray();
    for (const auto& h : historyArray)
    {
        index.history.append(SentimentPoint::fromJson(h.toObject()));
    }

    return index;
}

QJsonObject SentimentIndex::toJson() const
{
    QJsonObject json;
    json["stockCode"] = stockCode;
    json["currentScore"] = currentScore;
    json["avgScore7d"] = avgScore7d;
    json["avgScore30d"] = avgScore30d;
    json["trend"] = trend;
    json["trendDirection"] = trendDirection;
    json["updatedAt"] = updatedAt.toString(Qt::ISODate);

    QJsonArray historyArray;
    for (const auto& h : history)
    {
        historyArray.append(h.toJson());
    }
    json["history"] = historyArray;

    return json;
}

// ============================================================================
// NewsItem 实现
// ============================================================================

NewsItem NewsItem::fromJson(const QJsonObject& json)
{
    NewsItem item;
    item.id = json["id"].toString();
    item.title = json["title"].toString();
    item.summary = json["summary"].toString();
    item.content = json["content"].toString();
    item.source = json["source"].toString();
    item.author = json["author"].toString();
    item.url = QUrl(json["url"].toString());
    item.publishedAt = QDateTime::fromString(json["publishedAt"].toString(), Qt::ISODate);
    item.fetchTime = QDateTime::fromString(json["fetchTime"].toString(), Qt::ISODate);
    item.category = json["category"].toString();
    item.sentimentScore = json["sentimentScore"].toDouble();
    item.sentimentLabel = json["sentimentLabel"].toString();
    item.readCount = json["readCount"].toInt();
    item.commentCount = json["commentCount"].toInt();
    item.likeCount = json["likeCount"].toInt();
    item.heatScore = json["heatScore"].toDouble();
    item.isImportant = json["isImportant"].toBool();
    item.impactScore = json["impactScore"].toDouble();
    
    if (json.contains("sentiment"))
    {
        item.sentiment = SentimentResult::fromJson(json["sentiment"].toObject());
    }

    QJsonArray stocksArray = json["relatedStocks"].toArray();
    for (const auto& s : stocksArray)
    {
        item.relatedStocks.append(s.toString());
    }

    return item;
}

QJsonObject NewsItem::toJson() const
{
    QJsonObject json;
    json["id"] = id;
    json["title"] = title;
    json["summary"] = summary;
    json["content"] = content;
    json["source"] = source;
    json["author"] = author;
    json["url"] = url.toString();
    json["publishedAt"] = publishedAt.toString(Qt::ISODate);
    json["fetchTime"] = fetchTime.toString(Qt::ISODate);
    json["category"] = category;
    json["sentiment"] = sentiment.toJson();
    json["sentimentScore"] = sentimentScore;
    json["sentimentLabel"] = sentimentLabel;
    json["readCount"] = readCount;
    json["commentCount"] = commentCount;
    json["likeCount"] = likeCount;
    json["heatScore"] = heatScore;
    json["isImportant"] = isImportant;
    json["impactScore"] = impactScore;

    QJsonArray stocksArray;
    for (const auto& s : relatedStocks)
    {
        stocksArray.append(s);
    }
    json["relatedStocks"] = stocksArray;

    return json;
}

} // namespace WealthPilot