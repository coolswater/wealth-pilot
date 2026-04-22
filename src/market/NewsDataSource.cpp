/**
 * @file NewsDataSource.cpp
 * @brief 新闻数据源实现 - 对接华尔街见闻API
 */

#include "NewsDataSource.h"
#include "utils/Logger.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

NewsDataSource::NewsDataSource(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &NewsDataSource::onNetworkReply);
    LOG_DEBUG("NewsDataSource created");
}

NewsDataSource::~NewsDataSource()
{
}

void NewsDataSource::requestNews(Channel channel, int limit)
{
    // 华尔街见闻API
    QString url = QString("https://api-one.wallstcn.com/apiv1/content/articles?channel=%1&limit=%2")
        .arg(channelToString(channel))
        .arg(limit);

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    request.setRawHeader("Referer", "https://wallstreetcn.com/");
    request.setRawHeader("Accept", "application/json");

    m_networkManager->get(request);
    LOG_INFO(QString("Requesting news: %1").arg(url));
}

QVector<NewsItem> NewsDataSource::cachedNews() const
{
    return m_cachedNews;
}

void NewsDataSource::onNetworkReply(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        LOG_ERROR(QString("News network error: %1").arg(error));
        emit errorOccurred(error);
        return;
    }

    QByteArray data = reply->readAll();
    LOG_DEBUG(QString("News reply received: %1 bytes").arg(data.size()));

    parseWallStNews(data);
}

QString NewsDataSource::channelToString(Channel channel) const
{
    switch (channel) {
    case Channel::Global: return "global-channel";
    case Channel::AShares: return "a-shares-channel";
    case Channel::USStocks: return "us-shares-channel";
    case Channel::Forex: return "forex-channel";
    case Channel::Commodities: return "commodities-channel";
    }
    return "global-channel";
}

void NewsDataSource::parseWallStNews(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        emit errorOccurred(error.errorString());
        return;
    }

    QJsonObject root = doc.object();
    int code = root["code"].toInt();

    if (code != 20000) {
        LOG_ERROR(QString("API error code: %1").arg(code));
        emit errorOccurred(QString("API error: %1").arg(code));
        return;
    }

    QJsonObject dataObj = root["data"].toObject();
    QJsonArray items = dataObj["items"].toArray();

    m_cachedNews.clear();

    for (const QJsonValue& val : items) {
        QJsonObject item = val.toObject();

        NewsItem news;
        news.id = QString::number(item["id"].toInt());
        news.title = item["title"].toString();
        news.content = item["content_short"].toString();
        news.url = item["uri"].toString();
        news.timestamp = item["display_time"].toVariant().toLongLong();
        news.publishTime = QDateTime::fromSecsSinceEpoch(news.timestamp);

        // 作者
        QJsonObject author = item["author"].toObject();
        news.author = author["display_name"].toString();

        // 图片
        QJsonObject image = item["image"].toObject();
        news.imageUrl = image["uri"].toString();

        // 分类
        QJsonArray categories = item["categories"].toArray();
        for (const QJsonValue& cat : categories) {
            news.categories.append(cat.toString());
        }

        // 来源
        news.source = QStringLiteral("华尔街见闻");

        m_cachedNews.append(news);
    }

    LOG_INFO(QString("Parsed %1 news items").arg(m_cachedNews.size()));
    emit newsReceived(m_cachedNews);
}
