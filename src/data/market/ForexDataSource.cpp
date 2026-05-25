/**
 * @file ForexDataSource.cpp
 * @brief 外汇数据源实现
 */

#include "ForexDataSource.h"
#include "shared/utils/Logger.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

struct ForexDataSource::Impl {
    QNetworkAccessManager* networkManager = nullptr;
    QTimer* refreshTimer = nullptr;
    DataSource dataSource = DataSource::Sina;
    
    QHash<QString, ForexQuote> quoteCache;
    QStringList watchPairs;
    
    int requestId = 0;
};

ForexDataSource* ForexDataSource::instance()
{
    static ForexDataSource instance;
    return &instance;
}

ForexDataSource::ForexDataSource(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->networkManager = new QNetworkAccessManager(this);
    d->refreshTimer = new QTimer(this);
    
    connect(d->refreshTimer, &QTimer::timeout, this, [this]() {
        if (!d->watchPairs.isEmpty()) {
            requestQuotes(d->watchPairs, [](const QVector<ForexQuote>&) {});
        }
    });
    
    LOG_DEBUG("ForexDataSource initialized");
}

ForexDataSource::~ForexDataSource()
{
    stopAutoRefresh();
}

void ForexDataSource::setDataSource(DataSource source)
{
    d->dataSource = source;
}

void ForexDataSource::requestQuote(const QString& baseCurrency, const QString& quoteCurrency,
                                   std::function<void(const ForexQuote&)> callback)
{
    QString pair = baseCurrency + "/" + quoteCurrency;
    
    // 构建请求URL
    QString url;
    switch (d->dataSource) {
    case DataSource::Sina:
        // 新浪外汇API
        url = QString("https://hq.sinajs.cn/list/forex_%1%2")
              .arg(baseCurrency.toLower(), quoteCurrency.toLower());
        break;
    case DataSource::EastMoney:
        // 东方财富外汇API
        url = QString("https://push2.eastmoney.com/api/qt/stock/get"
                     "?secid=100.%1%2&fields=f43,f44,f45,f46,f47,f48,f49,f50,f51,f52,f55")
              .arg(baseCurrency, quoteCurrency);
        break;
    case DataSource::BOC:
        // 中国银行外汇牌价（需要解析HTML）
        url = "https://www.boc.cn/sourcedb/whpj/";
        break;
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    
    QNetworkReply* reply = d->networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, pair, callback]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR(QString("Forex request failed: %1").arg(reply->errorString()));
            emit error(reply->errorString());
            return;
        }
        
        QByteArray data = reply->readAll();
        ForexQuote quote;
        
        switch (d->dataSource) {
        case DataSource::Sina:
            quote = parseSinaResponse(pair, data);
            break;
        case DataSource::EastMoney:
            quote = parseEastMoneyResponse(pair, data);
            break;
        case DataSource::BOC:
            quote = parseBOCResponse(pair, data);
            break;
        }
        
        if (quote.isValid()) {
            d->quoteCache[pair] = quote;
            emit quoteUpdated(quote);
            callback(quote);
        }
    });
}

void ForexDataSource::requestQuotes(const QStringList& pairs,
                                    std::function<void(const QVector<ForexQuote>&)> callback)
{
    d->watchPairs = pairs;
    
    QVector<ForexQuote> results;
    results.reserve(pairs.size());
    
    int pendingCount = pairs.size();
    
    for (const QString& pair : pairs) {
        QStringList parts = pair.split("/");
        if (parts.size() != 2) continue;
        
        requestQuote(parts[0], parts[1], [this, &results, &pendingCount, callback](const ForexQuote& quote) {
            results.append(quote);
            pendingCount--;
            
            if (pendingCount == 0) {
                emit quotesUpdated(results);
                callback(results);
            }
        });
    }
}

ForexQuote ForexDataSource::cachedQuote(const QString& pair) const
{
    return d->quoteCache.value(pair);
}

double ForexDataSource::convert(double amount, const QString& from, const QString& to)
{
    if (from == to) return amount;
    
    // 尝试直接汇率
    QString directPair = from + "/" + to;
    if (d->quoteCache.contains(directPair)) {
        return amount * d->quoteCache[directPair].rate;
    }
    
    // 尝试反向汇率
    QString reversePair = to + "/" + from;
    if (d->quoteCache.contains(reversePair)) {
        return amount / d->quoteCache[reversePair].rate;
    }
    
    // 通过USD中转
    QString toUsdPair = from + "/USD";
    QString fromUsdPair = "USD/" + to;
    
    if (d->quoteCache.contains(toUsdPair) && d->quoteCache.contains(fromUsdPair)) {
        double usdAmount = amount * d->quoteCache[toUsdPair].rate;
        return usdAmount * d->quoteCache[fromUsdPair].rate;
    }
    
    LOG_WARNING(QString("Cannot convert %1 to %2: no exchange rate available").arg(from, to));
    return 0.0;
}

void ForexDataSource::startAutoRefresh(int intervalMs)
{
    d->refreshTimer->start(intervalMs);
    LOG_INFO(QString("Forex auto refresh started, interval: %1ms").arg(intervalMs));
}

void ForexDataSource::stopAutoRefresh()
{
    d->refreshTimer->stop();
    LOG_INFO("Forex auto refresh stopped");
}

// ========== 数据解析 ==========

ForexQuote ForexDataSource::parseSinaResponse(const QString& pair, const QByteArray& data)
{
    ForexQuote quote;
    quote.pair = pair;
    
    // 解析新浪外汇数据格式
    // var hq_str_forex_usdcny="美元人民币,7.2456,7.2450,7.2462,..."
    QString text = QString::fromUtf8(data);
    // 简化解析：直接查找数据
    int start = text.indexOf("hq_str_forex_");
    if (start > 0) {
        int quoteStart = text.indexOf("\"", start);
        int quoteEnd = text.indexOf("\"", quoteStart + 1);
        if (quoteStart > 0 && quoteEnd > quoteStart) {
            QString data = text.mid(quoteStart + 1, quoteEnd - quoteStart - 1);
            QStringList parts = data.split(",");
            if (parts.size() >= 4) {
                quote.rate = parts[1].toDouble();
                quote.bid = parts[2].toDouble();
                quote.ask = parts[3].toDouble();
                quote.updateTime = QDateTime::currentDateTime();
                
                QStringList currencies = pair.split("/");
                if (currencies.size() == 2) {
                    quote.baseCurrency = currencies[0];
                    quote.quoteCurrency = currencies[1];
                }
            }
        }
    }
    
    return quote;
}

ForexQuote ForexDataSource::parseEastMoneyResponse(const QString& pair, const QByteArray& data)
{
    ForexQuote quote;
    quote.pair = pair;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return quote;
    }
    
    QJsonObject root = doc.object();
    QJsonObject dataObj = root["data"].toObject();
    
    if (!dataObj.isEmpty()) {
        quote.rate = dataObj["f43"].toDouble() / 1000.0;  // 最新价
        quote.high24h = dataObj["f44"].toDouble() / 1000.0;
        quote.low24h = dataObj["f45"].toDouble() / 1000.0;
        quote.updateTime = QDateTime::currentDateTime();
        
        QStringList currencies = pair.split("/");
        if (currencies.size() == 2) {
            quote.baseCurrency = currencies[0];
            quote.quoteCurrency = currencies[1];
        }
    }
    
    return quote;
}

ForexQuote ForexDataSource::parseBOCResponse(const QString& pair, const QByteArray& data)
{
    ForexQuote quote;
    quote.pair = pair;
    
    // 中国银行外汇牌价需要解析HTML
    // 这里简化处理，实际需要HTML解析
    QString html = QString::fromUtf8(data);
    
    // 查找汇率表格
    QRegularExpression regex(QString(R"(%1.*?(\d+\.\d+).*(\d+\.\d+))").arg(pair.split("/")[0]));
    QRegularExpressionMatch match = regex.match(html);
    
    if (match.hasMatch()) {
        quote.bid = match.captured(1).toDouble();
        quote.ask = match.captured(2).toDouble();
        quote.rate = (quote.bid + quote.ask) / 2;
        quote.updateTime = QDateTime::currentDateTime();
        
        QStringList currencies = pair.split("/");
        if (currencies.size() == 2) {
            quote.baseCurrency = currencies[0];
            quote.quoteCurrency = currencies[1];
        }
    }
    
    return quote;
}
