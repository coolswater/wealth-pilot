/**
 * @file SentimentAnalysisService.cpp
 * @brief 情绪分析服务实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "SentimentAnalysisService.h"
#include "AIService.h"
#include "utils/Logger.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QUuid>
#include <QRegularExpression>

namespace WealthPilot
{
    // ============================================================================
    // 数据结构实现
    // ============================================================================

    SentimentResult SentimentResult::fromJson(const QJsonObject& json)
    {
        SentimentResult result;
        result.positive = json["positive"].toDouble();
        result.negative = json["negative"].toDouble();
        result.neutral = json["neutral"].toDouble();
        result.score = json["score"].toDouble();
        result.type = static_cast<SentimentType>(json["type"].toInt());
        result.confidence = json["confidence"].toDouble();
        result.summary = json["summary"].toString();
        result.analyzedAt = QDateTime::fromString(json["analyzedAt"].toString(), Qt::ISODate);

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
        json["type"] = static_cast<int>(type);
        json["confidence"] = confidence;
        json["summary"] = summary;
        json["analyzedAt"] = analyzedAt.toString(Qt::ISODate);

        QJsonArray keywordsArray;
        for (const auto& k : keywords)
        {
            keywordsArray.append(k);
        }
        json["keywords"] = keywordsArray;

        return json;
    }

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

    NewsItem NewsItem::fromJson(const QJsonObject& json)
    {
        NewsItem item;
        item.id = json["id"].toString();
        item.title = json["title"].toString();
        item.content = json["content"].toString();
        item.source = json["source"].toString();
        item.publishedAt = QDateTime::fromString(json["publishedAt"].toString(), Qt::ISODate);
        item.sentiment = SentimentResult::fromJson(json["sentiment"].toObject());

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
        json["content"] = content;
        json["source"] = source;
        json["publishedAt"] = publishedAt.toString(Qt::ISODate);
        json["sentiment"] = sentiment.toJson();

        QJsonArray stocksArray;
        for (const auto& s : relatedStocks)
        {
            stocksArray.append(s);
        }
        json["relatedStocks"] = stocksArray;

        return json;
    }

    // ============================================================================
    // SentimentAnalysisService 实现
    // ============================================================================

    SentimentAnalysisService::SentimentAnalysisService(QObject* parent)
        : QObject(parent)
    {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_storagePath = appDataPath + "/sentiment";
        QDir dir(m_storagePath);
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
                                               std::function<void(const SentimentResult &)> callback)
    {
        // 组合标题和内容进行分析
        QString text = news.title + "\n" + news.content;

        // 使用 AI 分析
        analyzeTextWithAI(text, [this, news, callback](const SentimentResult& result)
        {
            // 更新相关股票的情绪指标
            for (const auto& stockCode : news.relatedStocks)
            {
                updateSentimentIndex(stockCode, result);
            }

            callback(result);
        });
    }

    void SentimentAnalysisService::analyzeNewsBatch(const QList<NewsItem>& newsList,
                                                    std::function<void(const QList<SentimentResult> &)> callback)
    {
        QList<SentimentResult> results;
        int pending = newsList.size();

        for (const auto& news : newsList)
        {
            analyzeNews(news, [this, &results, &pending, callback](const SentimentResult& result)
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
                                                         std::function<void(const SentimentResult &)> callback)
    {
        // TODO: 从数据源获取相关新闻
        // 这里使用 AI 分析
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
                emit errorOccurred(result.errorMessage());
                callback(SentimentResult());
                return;
            }

            SentimentResult sentiment;
            sentiment.summary = result.value();
            sentiment.score = 0; // TODO: 从响应中解析
            sentiment.type = SentimentType::Neutral;
            sentiment.confidence = 0.7;
            sentiment.analyzedAt = QDateTime::currentDateTime();

            updateSentimentIndex(stockCode, sentiment);
            emit sentimentAnalyzed(stockCode, sentiment);

            callback(sentiment);
        });
    }

    void SentimentAnalysisService::getMarketSentiment(std::function < void(const SentimentResult &) > callback)
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

        AIService::instance()->chat(prompt, [callback](Result<QString> result)
        {
            if (result.isError())
            {
                callback(SentimentResult());
                return;
            }

            SentimentResult sentiment;
            sentiment.summary = result.value();
            sentiment.score = 0;
            sentiment.type = SentimentType::Neutral;
            sentiment.confidence = 0.6;
            sentiment.analyzedAt = QDateTime::currentDateTime();

            callback(sentiment);
        });
    }

    void SentimentAnalysisService::getSectorSentiment(const QString& sector,
                                                      std::function<void(const SentimentResult &)> callback)
    {
        QString prompt = QString(QStringLiteral(
            "请分析 %1 板块的市场情绪：\n\n"
            "请分析该板块的：\n"
            "1. 政策面情绪\n"
            "2. 基本面情绪\n"
            "3. 资金面情绪\n\n"
            "请给出情绪评分（-1到1）和简要分析。"
        )).arg(sector);

        AIService::instance()->chat(prompt, [sector, callback](Result<QString> result)
        {
            if (result.isError())
            {
                callback(SentimentResult());
                return;
            }

            SentimentResult sentiment;
            sentiment.summary = result.value();
            sentiment.score = 0;
            sentiment.type = SentimentType::Neutral;
            sentiment.confidence = 0.65;
            sentiment.analyzedAt = QDateTime::currentDateTime();

            callback(sentiment);
        });
    }

    SentimentIndex SentimentAnalysisService::getSentimentIndex(const QString& stockCode) const
    {
        return m_sentimentIndices.value(stockCode);
    }

    QList<SentimentPoint> SentimentAnalysisService::getSentimentTrend(const QString& stockCode, int days) const
    {
        if (!m_sentimentIndices.contains(stockCode))
        {
            return QList<SentimentPoint>();
        }

        const auto& index = m_sentimentIndices[stockCode];
        QDateTime cutoff = QDateTime::currentDateTime().addDays(-days);

        QList<SentimentPoint> trend;
        for (const auto& point : index.history)
        {
            if (point.timestamp >= cutoff)
            {
                trend.append(point);
            }
        }

        return trend;
    }

    void SentimentAnalysisService::calculateSentimentIndex(const QString& stockCode,
                                                           std::function<void(const SentimentIndex &)> callback)
    {
        getStockNewsSentiment(stockCode, [this, stockCode, callback](const SentimentResult& result)
        {
            SentimentIndex index;

            if (m_sentimentIndices.contains(stockCode))
            {
                index = m_sentimentIndices[stockCode];
            }

            index.stockCode = stockCode;
            index.currentScore = result.score;
            index.updatedAt = QDateTime::currentDateTime();

            // 添加历史数据点
            SentimentPoint point;
            point.timestamp = QDateTime::currentDateTime();
            point.score = result.score;
            point.positive = result.positive;
            point.negative = result.negative;
            point.sampleCount = 1;
            index.history.append(point);

            // 保留最近 90 天的数据
            QDateTime cutoff = QDateTime::currentDateTime().addDays(-90);
            index.history.erase(
                std::remove_if(index.history.begin(), index.history.end(),
                               [cutoff](const SentimentPoint& p) { return p.timestamp < cutoff; }),
                index.history.end());

            // 计算平均值
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

                // 计算趋势
                if (index.avgScore7d != 0 && index.avgScore30d != 0)
                {
                    index.trend = (index.avgScore7d - index.avgScore30d) / qAbs(index.avgScore30d);
                    index.trend = qBound(-1.0, index.trend, 1.0);
                }

                // 趋势方向
                if (index.trend > 0.1)
                {
                    index.trendDirection = QStringLiteral("上升");
                }
                else if (index.trend < -0.1)
                {
                    index.trendDirection = QStringLiteral("下降");
                }
                else
                {
                    index.trendDirection = QStringLiteral("平稳");
                }
            }

            m_sentimentIndices[stockCode] = index;
            emit sentimentIndexChanged(stockCode, index);

            callback(index);
        });
    }

    void SentimentAnalysisService::addSentimentWatch(const QString& stockCode)
    {
        if (!m_watchList.contains(stockCode))
        {
            m_watchList.append(stockCode);
            LOG_DEBUG("Added sentiment watch: " + stockCode);
        }
    }

    void SentimentAnalysisService::removeSentimentWatch(const QString& stockCode)
    {
        m_watchList.removeAll(stockCode);
        LOG_DEBUG("Removed sentiment watch: " + stockCode);
    }

    QString SentimentAnalysisService::getSentimentTypeName(SentimentType type)
    {
        switch (type)
        {
        case SentimentType::VeryNegative: return QStringLiteral("非常负面");
        case SentimentType::Negative: return QStringLiteral("负面");
        case SentimentType::Neutral: return QStringLiteral("中性");
        case SentimentType::Positive: return QStringLiteral("正面");
        case SentimentType::VeryPositive: return QStringLiteral("非常正面");
        default: return QStringLiteral("未知");
        }
    }

    SentimentType SentimentAnalysisService::scoreToType(double score)
    {
        if (score < -0.6) return SentimentType::VeryNegative;
        if (score < -0.2) return SentimentType::Negative;
        if (score < 0.2) return SentimentType::Neutral;
        if (score < 0.6) return SentimentType::Positive;
        return SentimentType::VeryPositive;
    }

    SentimentResult SentimentAnalysisService::analyzeTextLocally(const QString& text)
    {
        SentimentResult result;

        // 简单的关键词分析
        QStringList positiveWords = {
            QStringLiteral("上涨"), QStringLiteral("利好"), QStringLiteral("增长"),
            QStringLiteral("突破"), QStringLiteral("创新高"), QStringLiteral("盈利"),
            QStringLiteral("增持"), QStringLiteral("回购"), QStringLiteral("分红")
        };

        QStringList negativeWords = {
            QStringLiteral("下跌"), QStringLiteral("利空"), QStringLiteral("亏损"),
            QStringLiteral("跌破"), QStringLiteral("创新低"), QStringLiteral("减持"),
            QStringLiteral("质押"), QStringLiteral("诉讼"), QStringLiteral("违规")
        };

        int positiveCount = 0, negativeCount = 0;

        for (const auto& word : positiveWords)
        {
            if (text.contains(word)) positiveCount++;
        }

        for (const auto& word : negativeWords)
        {
            if (text.contains(word)) negativeCount++;
        }

        int total = positiveCount + negativeCount;
        if (total > 0)
        {
            result.positive = static_cast<double>(positiveCount) / total;
            result.negative = static_cast<double>(negativeCount) / total;
            result.score = result.positive - result.negative;
        }
        else
        {
            result.neutral = 1.0;
            result.score = 0;
        }

        result.type = scoreToType(result.score);
        result.confidence = 0.5;
        result.analyzedAt = QDateTime::currentDateTime();

        return result;
    }

    void SentimentAnalysisService::analyzeTextWithAI(const QString& text,
                                                     std::function<void(const SentimentResult &)> callback)
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
        )).arg(text.left(1000)); // 限制文本长度

        AIService::instance()->chat(prompt, [callback](Result<QString> result)
        {
            if (result.isError())
            {
                callback(SentimentResult());
                return;
            }

            // 尝试解析 JSON
            QString response = result.value();

            // 提取 JSON 部分
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
                sentiment.type = scoreToType(sentiment.score);
                sentiment.confidence = 0.8;

                QJsonArray keywordsArray = json["keywords"].toArray();
                for (const auto& k : keywordsArray)
                {
                    sentiment.keywords.append(k.toString());
                }
            }
            else
            {
                // 无法解析 JSON，使用默认值
                sentiment.summary = response;
                sentiment.type = SentimentType::Neutral;
                sentiment.confidence = 0.5;
            }

            callback(sentiment);
        });
    }

    void SentimentAnalysisService::updateSentimentIndex(const QString& stockCode, const SentimentResult& result)
    {
        SentimentIndex index;

        if (m_sentimentIndices.contains(stockCode))
        {
            index = m_sentimentIndices[stockCode];
        }

        index.stockCode = stockCode;
        index.currentScore = result.score;
        index.updatedAt = QDateTime::currentDateTime();

        // 添加历史数据点
        SentimentPoint point;
        point.timestamp = QDateTime::currentDateTime();
        point.score = result.score;
        point.positive = result.positive;
        point.negative = result.negative;
        point.sampleCount = 1;
        index.history.append(point);

        m_sentimentIndices[stockCode] = index;
    }
} // namespace WealthPilot