/**
 * @file NewsDataSource.cpp
 * @brief 新闻数据源实现
 */

#include "NewsDataSource.h"
#include "utils/Logger.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

NewsDataSource* NewsDataSource::instance()
{
    static NewsDataSource* inst = new NewsDataSource();
    return inst;
}

NewsDataSource::NewsDataSource(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_updateTimer(new QTimer(this))
{
    connect(m_updateTimer, &QTimer::timeout, this, &NewsDataSource::onPeriodicUpdate);
}

NewsDataSource::~NewsDataSource()
{
    m_updateTimer->stop();
}

bool NewsDataSource::initialize()
{
    if (m_initialized) return true;

    LOG_INFO("Initializing News Data Source");

    // 初始化情感分析器
    NewsSentimentAnalyzer::instance()->initialize();

    m_initialized = true;
    LOG_INFO("News Data Source initialized");
    return true;
}

void NewsDataSource::requestNews(const QString& symbol, int count)
{
    Q_UNUSED(count);

    // 模拟新闻数据（实际应从API获取）
    QVector<NewsItem> newsList;

    // 生成模拟新闻
    NewsItem item1;
    item1.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item1.title = QStringLiteral("%1 业绩大增，股价创新高").arg(symbol.isEmpty() ? "市场" : symbol);
    item1.content = QStringLiteral("最新财报显示，公司营收同比增长30%，净利润增长25%，超出市场预期。分析师普遍看好后市表现。");
    item1.source = QStringLiteral("财经快讯");
    item1.publishTime = QDateTime::currentDateTime().addSecs(-3600);
    item1.category = QStringLiteral("新闻");
    item1.relatedSymbols.append(symbol);
    item1.readCount = 5234;
    item1.commentCount = 128;

    // 分析情感
    item1.sentiment = NewsSentimentAnalyzer::instance()->analyzeSentiment(item1.title + item1.content);
    item1.isImportant = item1.sentiment.impactScore > 3.0;

    newsList.append(item1);

    NewsItem item2;
    item2.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item2.title = QStringLiteral("机构调研：%1 获多家机构关注").arg(symbol.isEmpty() ? "某公司" : symbol);
    item2.content = QStringLiteral("近期多家知名机构对公司进行调研，重点关注公司新产品研发进展和市场拓展计划。");
    item2.source = QStringLiteral("机构动态");
    item2.publishTime = QDateTime::currentDateTime().addSecs(-7200);
    item2.category = QStringLiteral("新闻");
    item2.relatedSymbols.append(symbol);
    item2.readCount = 3421;
    item2.commentCount = 89;

    item2.sentiment = NewsSentimentAnalyzer::instance()->analyzeSentiment(item2.title + item2.content);
    item2.isImportant = item2.sentiment.impactScore > 3.0;

    newsList.append(item2);

    // 缓存
    QString cacheKey = symbol.isEmpty() ? QStringLiteral("all") : symbol;
    m_newsCache[cacheKey] = newsList;

    emit newsUpdated(symbol, newsList);
}

void NewsDataSource::requestAnnouncements(const QString& symbol, int count)
{
    Q_UNUSED(count);

    QVector<NewsItem> announcements;

    NewsItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.title = QStringLiteral("%1 关于回购公司股份的进展公告").arg(symbol);
    item.content = QStringLiteral("截至本公告日，公司已累计回购股份100万股，占公司总股本的0.5%，支付资金总额5000万元。");
    item.source = QStringLiteral("公司公告");
    item.publishTime = QDateTime::currentDateTime().addDays(-1);
    item.category = QStringLiteral("公告");
    item.relatedSymbols.append(symbol);
    item.readCount = 8921;
    item.commentCount = 256;

    item.sentiment = NewsSentimentAnalyzer::instance()->analyzeSentiment(item.title + item.content);
    item.isImportant = true;

    announcements.append(item);

    emit announcementsUpdated(symbol, announcements);
}

void NewsDataSource::requestFinancialReports(const QString& symbol, int count)
{
    Q_UNUSED(count);

    QVector<NewsItem> reports;

    NewsItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.title = QStringLiteral("%1 2024年年度报告").arg(symbol);
    item.content = QStringLiteral("报告期内，公司实现营业收入100亿元，同比增长15%；归属于上市公司股东的净利润10亿元，同比增长20%。");
    item.source = QStringLiteral("定期报告");
    item.publishTime = QDateTime::currentDateTime().addDays(-30);
    item.category = QStringLiteral("财报");
    item.relatedSymbols.append(symbol);
    item.readCount = 15234;
    item.commentCount = 512;

    item.sentiment = NewsSentimentAnalyzer::instance()->analyzeSentiment(item.title + item.content);
    item.isImportant = true;

    reports.append(item);

    emit financialReportsUpdated(symbol, reports);
}

void NewsDataSource::requestResearchReports(const QString& symbol, int count)
{
    Q_UNUSED(count);

    QVector<NewsItem> reports;

    NewsItem item;
    item.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    item.title = QStringLiteral("券商研报：给予%1\"买入\"评级").arg(symbol);
    item.content = QStringLiteral("目标价25元，上涨空间20%。公司基本面良好，行业地位稳固，建议重点关注。");
    item.source = QStringLiteral("券商研报");
    item.publishTime = QDateTime::currentDateTime().addDays(-7);
    item.category = QStringLiteral("研报");
    item.relatedSymbols.append(symbol);
    item.readCount = 6543;
    item.commentCount = 189;

    item.sentiment = NewsSentimentAnalyzer::instance()->analyzeSentiment(item.title + item.content);
    item.isImportant = true;

    reports.append(item);

    emit researchReportsUpdated(symbol, reports);
}

void NewsDataSource::requestSocialHeat(const QString& symbol)
{
    // 模拟社交媒体热度数据
    SocialHeatData heat;
    heat.symbol = symbol;
    heat.platform = QStringLiteral("雪球");
    heat.mentionCount = 1256;
    heat.previousCount = 980;
    heat.changePercent = (heat.mentionCount - heat.previousCount) * 100.0 / heat.previousCount;
    heat.sentimentScore = 0.65; // 0-1, 0.5为中性
    heat.updateTime = QDateTime::currentDateTime();

    m_socialHeatCache[symbol] = heat;

    emit socialHeatUpdated(symbol, heat);
}

void NewsDataSource::subscribeSymbols(const QVector<QString>& symbols)
{
    for (const QString& symbol : symbols) {
        if (!m_subscribedSymbols.contains(symbol)) {
            m_subscribedSymbols.append(symbol);
        }
    }

    // 启动定时更新
    if (!m_updateTimer->isActive()) {
        m_updateTimer->start(m_updateInterval * 1000);
    }

    LOG_INFO(QString("Subscribed to %1 symbols for news updates").arg(symbols.size()));
}

void NewsDataSource::unsubscribeSymbol(const QString& symbol)
{
    m_subscribedSymbols.removeAll(symbol);

    if (m_subscribedSymbols.isEmpty()) {
        m_updateTimer->stop();
    }
}

QVector<NewsItem> NewsDataSource::getCachedNews(const QString& symbol) const
{
    QString cacheKey = symbol.isEmpty() ? QStringLiteral("all") : symbol;
    return m_newsCache.value(cacheKey);
}

QVector<SocialHeatData> NewsDataSource::getSocialHeat(const QString& symbol) const
{
    QVector<SocialHeatData> result;
    if (m_socialHeatCache.contains(symbol)) {
        result.append(m_socialHeatCache[symbol]);
    }
    return result;
}

void NewsDataSource::setUpdateInterval(int seconds)
{
    m_updateInterval = seconds;
    if (m_updateTimer->isActive()) {
        m_updateTimer->setInterval(seconds * 1000);
    }
}

void NewsDataSource::onNetworkReply(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        LOG_ERROR(QString("Network error: %1").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QString url = reply->url().toString();

    // 根据URL判断响应类型
    if (url.contains(QStringLiteral("news"))) {
        QVector<NewsItem> news = parseNewsResponse(data);
        // 处理新闻
    } else if (url.contains(QStringLiteral("social"))) {
        SocialHeatData heat = parseSocialHeatResponse(data);
        // 处理社交热度
    }

    reply->deleteLater();
}

void NewsDataSource::onPeriodicUpdate()
{
    // 定期更新订阅股票的新闻
    for (const QString& symbol : m_subscribedSymbols) {
        requestNews(symbol, 20);
        requestSocialHeat(symbol);
    }
}

void NewsDataSource::fetchNewsFromAPI(const QString& symbol, const QString& category)
{
    // 使用新浪财经API获取新闻
    QString url = QString("https://newsapi.sina.cn/api/news/list?symbol=%1&category=%2")
        .arg(symbol, category);

    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, symbol]()
    {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            LOG_ERROR(QString("News API error: %1").arg(reply->errorString()));
            return;
        }

        QByteArray data = reply->readAll();
        QVector<NewsItem> newsList = parseNewsResponse(data);

        if (!newsList.isEmpty())
        {
            m_newsCache[symbol] = newsList;
            emit newsUpdated(symbol, newsList);
        }
    });
}

void NewsDataSource::fetchSocialHeatFromAPI(const QString& symbol)
{
    // 使用东方财富API获取社交热度
    QString url = QString("https://emweb.eastmoney.com/PC_HSF10/NewFinance/Index?type=web&code=%1").arg(symbol);

    QNetworkRequest request((QUrl(url)));
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, symbol]()
    {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError)
        {
            LOG_ERROR(QString("Social heat API error: %1").arg(reply->errorString()));
            return;
        }

        // 解析热度数据
        QByteArray data = reply->readAll();
        // TODO: 实现热度解析逻辑
        Q_UNUSED(data);
    });
}

QVector<NewsItem> NewsDataSource::parseNewsResponse(const QByteArray& data)
{
    QVector<NewsItem> newsList;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return newsList;
    }

    QJsonArray array = doc.array();
    for (const QJsonValue& value : array) {
        QJsonObject obj = value.toObject();

        NewsItem item;
        item.id = obj[QStringLiteral("id")].toString();
        item.title = obj[QStringLiteral("title")].toString();
        item.content = obj[QStringLiteral("content")].toString();
        item.source = obj[QStringLiteral("source")].toString();
        item.publishTime = QDateTime::fromString(
            obj[QStringLiteral("publishTime")].toString(), Qt::ISODate);
        item.category = obj[QStringLiteral("category")].toString();
        item.readCount = obj[QStringLiteral("readCount")].toInt();

        // 分析情感
        item.sentiment = NewsSentimentAnalyzer::instance()->analyzeSentiment(
            item.title + item.content);

        newsList.append(item);
    }

    return newsList;
}

SocialHeatData NewsDataSource::parseSocialHeatResponse(const QByteArray& data)
{
    SocialHeatData heat;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return heat;
    }

    QJsonObject obj = doc.object();
    heat.symbol = obj[QStringLiteral("symbol")].toString();
    heat.platform = obj[QStringLiteral("platform")].toString();
    heat.mentionCount = obj[QStringLiteral("mentionCount")].toInt();
    heat.previousCount = obj[QStringLiteral("previousCount")].toInt();
    heat.changePercent = obj[QStringLiteral("changePercent")].toDouble();
    heat.sentimentScore = obj[QStringLiteral("sentimentScore")].toDouble();
    heat.updateTime = QDateTime::currentDateTime();

    return heat;
}

void NewsDataSource::checkAndPushImportantNews(const NewsItem& news)
{
    if (news.isImportant && news.sentiment.impactScore > 5.0) {
        emit importantNewsPush(news);
    }
}
