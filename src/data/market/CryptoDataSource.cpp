/**
 * @file CryptoDataSource.cpp
 * @brief 数字货币数据源实现
 */

#include "CryptoDataSource.h"
#include "shared/utils/Logger.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

struct CryptoDataSource::Impl {
    QNetworkAccessManager* networkManager = nullptr;
    QTimer* refreshTimer = nullptr;
    DataSource dataSource = DataSource::CoinGecko;
    
    QString apiKey;
    QString apiSecret;
    
    QHash<QString, CryptoQuote> quoteCache;
    QStringList watchSymbols;
    
    // USD/CNY汇率（用于转换价格）
    double usdToCnyRate = 7.24;
};

CryptoDataSource* CryptoDataSource::instance()
{
    static CryptoDataSource instance;
    return &instance;
}

CryptoDataSource::CryptoDataSource(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->networkManager = new QNetworkAccessManager(this);
    d->refreshTimer = new QTimer(this);
    
    connect(d->refreshTimer, &QTimer::timeout, this, [this]() {
        if (!d->watchSymbols.isEmpty()) {
            requestQuotes(d->watchSymbols, [](const QVector<CryptoQuote>&) {});
        }
    });
    
    LOG_DEBUG("CryptoDataSource initialized");
}

CryptoDataSource::~CryptoDataSource()
{
    stopAutoRefresh();
}

void CryptoDataSource::setDataSource(DataSource source)
{
    d->dataSource = source;
}

void CryptoDataSource::setApiKey(const QString& apiKey, const QString& apiSecret)
{
    d->apiKey = apiKey;
    d->apiSecret = apiSecret;
}

void CryptoDataSource::requestQuote(const QString& symbol,
                                    std::function<void(const CryptoQuote&)> callback)
{
    QString url;
    
    switch (d->dataSource) {
    case DataSource::CoinGecko:
        // CoinGecko API（免费）
        url = QString("https://api.coingecko.com/api/v3/simple/price"
                     "?ids=%1&vs_currencies=usd,cny&include_24hr_change=true"
                     "&include_market_cap=true&include_24hr_vol=true")
              .arg(symbol.toLower());
        break;
    case DataSource::Binance:
        // 币安API
        url = QString("https://api.binance.com/api/v3/ticker/24hr?symbol=%1USDT")
              .arg(symbol.toUpper());
        break;
    case DataSource::OKX:
        // OKX API
        url = QString("https://www.okx.com/api/v5/market/ticker?instId=%1-USDT")
              .arg(symbol.toUpper());
        break;
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    
    QNetworkReply* reply = d->networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, symbol, callback]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR(QString("Crypto request failed: %1").arg(reply->errorString()));
            emit error(reply->errorString());
            return;
        }
        
        QByteArray data = reply->readAll();
        CryptoQuote quote;
        
        switch (d->dataSource) {
        case DataSource::CoinGecko:
            quote = parseCoinGeckoResponse(symbol, data);
            break;
        case DataSource::Binance:
            quote = parseBinanceResponse(symbol, data);
            break;
        case DataSource::OKX:
            quote = parseOKXResponse(symbol, data);
            break;
        }
        
        if (quote.isValid()) {
            d->quoteCache[symbol] = quote;
            emit quoteUpdated(quote);
            callback(quote);
        }
    });
}

void CryptoDataSource::requestQuotes(const QStringList& symbols,
                                     std::function<void(const QVector<CryptoQuote>&)> callback)
{
    d->watchSymbols = symbols;
    
    QVector<CryptoQuote> results;
    results.reserve(symbols.size());
    
    int pendingCount = symbols.size();
    
    for (const QString& symbol : symbols) {
        requestQuote(symbol, [this, &results, &pendingCount, callback](const CryptoQuote& quote) {
            results.append(quote);
            pendingCount--;
            
            if (pendingCount == 0) {
                emit quotesUpdated(results);
                callback(results);
            }
        });
    }
}

void CryptoDataSource::requestTopList(int limit,
                                       std::function<void(const QVector<CryptoQuote>&)> callback)
{
    QString url;
    
    switch (d->dataSource) {
    case DataSource::CoinGecko:
        url = QString("https://api.coingecko.com/api/v3/coins/markets"
                     "?vs_currency=usd&order=market_cap_desc&per_page=%1&page=1"
                     "&sparkline=false&price_change_percentage=24h")
              .arg(limit);
        break;
    case DataSource::Binance:
        // 币安需要单独请求每个币种
        url = "https://api.binance.com/api/v3/ticker/24hr";
        break;
    case DataSource::OKX:
        url = QString("https://www.okx.com/api/v5/market/tickers?instType=SPOT");
        break;
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    
    QNetworkReply* reply = d->networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, limit, callback]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR(QString("Crypto top list request failed: %1").arg(reply->errorString()));
            emit error(reply->errorString());
            return;
        }
        
        QByteArray data = reply->readAll();
        QVector<CryptoQuote> quotes;
        
        switch (d->dataSource) {
        case DataSource::CoinGecko:
            quotes = parseCoinGeckoTopList(data, limit);
            break;
        case DataSource::Binance:
            quotes = parseBinanceTopList(data, limit);
            break;
        case DataSource::OKX:
            quotes = parseOKXTopList(data, limit);
            break;
        }
        
        // 更新缓存
        for (const CryptoQuote& quote : quotes) {
            d->quoteCache[quote.symbol] = quote;
        }
        
        emit quotesUpdated(quotes);
        callback(quotes);
    });
}

CryptoQuote CryptoDataSource::cachedQuote(const QString& symbol) const
{
    return d->quoteCache.value(symbol);
}

void CryptoDataSource::startAutoRefresh(int intervalMs)
{
    d->refreshTimer->start(intervalMs);
    LOG_INFO(QString("Crypto auto refresh started, interval: %1ms").arg(intervalMs));
}

void CryptoDataSource::stopAutoRefresh()
{
    d->refreshTimer->stop();
    LOG_INFO("Crypto auto refresh stopped");
}

// ========== 数据解析 ==========

CryptoQuote CryptoDataSource::parseCoinGeckoResponse(const QString& symbol, const QByteArray& data)
{
    CryptoQuote quote;
    quote.symbol = symbol.toUpper();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return quote;
    }
    
    QJsonObject root = doc.object();
    QJsonObject coinData = root[symbol.toLower()].toObject();
    
    if (!coinData.isEmpty()) {
        quote.price = coinData["usd"].toDouble();
        quote.priceCny = coinData["cny"].toDouble();
        quote.change24h = coinData["usd_24h_change"].toDouble();
        quote.marketCap = coinData["usd_market_cap"].toDouble();
        quote.volume24h = coinData["usd_24h_vol"].toDouble();
        quote.updateTime = QDateTime::currentDateTime();
    }
    
    return quote;
}

CryptoQuote CryptoDataSource::parseBinanceResponse(const QString& symbol, const QByteArray& data)
{
    CryptoQuote quote;
    quote.symbol = symbol.toUpper();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return quote;
    }
    
    QJsonObject root = doc.object();
    
    quote.price = root["lastPrice"].toDouble();
    quote.priceCny = quote.price * d->usdToCnyRate;
    quote.change24h = root["priceChangePercent"].toDouble();
    quote.volume24h = root["volume"].toDouble();
    quote.high24h = root["highPrice"].toDouble();
    quote.low24h = root["lowPrice"].toDouble();
    quote.updateTime = QDateTime::currentDateTime();
    
    return quote;
}

CryptoQuote CryptoDataSource::parseOKXResponse(const QString& symbol, const QByteArray& data)
{
    CryptoQuote quote;
    quote.symbol = symbol.toUpper();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return quote;
    }
    
    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();
    
    if (!dataArray.isEmpty()) {
        QJsonObject ticker = dataArray[0].toObject();
        
        quote.price = ticker["last"].toDouble();
        quote.priceCny = quote.price * d->usdToCnyRate;
        quote.high24h = ticker["high"].toDouble();
        quote.low24h = ticker["low"].toDouble();
        quote.volume24h = ticker["vol24h"].toDouble();
        quote.updateTime = QDateTime::currentDateTime();
    }
    
    return quote;
}

QVector<CryptoQuote> CryptoDataSource::parseCoinGeckoTopList(const QByteArray& data, int limit)
{
    QVector<CryptoQuote> quotes;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return quotes;
    }
    
    QJsonArray coins = doc.array();
    
    for (int i = 0; i < coins.size() && i < limit; ++i) {
        QJsonObject coin = coins[i].toObject();
        
        CryptoQuote quote;
        quote.symbol = coin["symbol"].toString().toUpper();
        quote.name = coin["name"].toString();
        quote.price = coin["current_price"].toDouble();
        quote.priceCny = quote.price * d->usdToCnyRate;
        quote.change24h = coin["price_change_percentage_24h"].toDouble();
        quote.marketCap = coin["market_cap"].toDouble();
        quote.volume24h = coin["total_volume"].toDouble();
        quote.high24h = coin["high_24h"].toDouble();
        quote.low24h = coin["low_24h"].toDouble();
        quote.rank = coin["market_cap_rank"].toInt();
        quote.updateTime = QDateTime::currentDateTime();
        
        quotes.append(quote);
    }
    
    return quotes;
}

QVector<CryptoQuote> CryptoDataSource::parseBinanceTopList(const QByteArray& data, int limit)
{
    QVector<CryptoQuote> quotes;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return quotes;
    }
    
    QJsonArray tickers = doc.array();
    
    // 按成交量排序
    QVector<QJsonObject> sortedTickers;
    for (const QJsonValue& val : tickers) {
        sortedTickers.append(val.toObject());
    }
    
    std::sort(sortedTickers.begin(), sortedTickers.end(), 
              [](const QJsonObject& a, const QJsonObject& b) {
                  return a["quoteVolume"].toDouble() > b["quoteVolume"].toDouble();
              });
    
    for (int i = 0; i < sortedTickers.size() && i < limit; ++i) {
        QJsonObject ticker = sortedTickers[i];
        QString symbol = ticker["symbol"].toString();
        
        // 只保留USDT交易对
        if (!symbol.endsWith("USDT")) continue;
        
        CryptoQuote quote;
        quote.symbol = symbol.replace("USDT", "");
        quote.price = ticker["lastPrice"].toDouble();
        quote.priceCny = quote.price * d->usdToCnyRate;
        quote.change24h = ticker["priceChangePercent"].toDouble();
        quote.volume24h = ticker["quoteVolume"].toDouble();
        quote.high24h = ticker["highPrice"].toDouble();
        quote.low24h = ticker["lowPrice"].toDouble();
        quote.updateTime = QDateTime::currentDateTime();
        
        quotes.append(quote);
    }
    
    return quotes;
}

QVector<CryptoQuote> CryptoDataSource::parseOKXTopList(const QByteArray& data, int limit)
{
    QVector<CryptoQuote> quotes;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return quotes;
    }
    
    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();
    
    for (int i = 0; i < dataArray.size() && i < limit; ++i) {
        QJsonObject ticker = dataArray[i].toObject();
        QString instId = ticker["instId"].toString();
        
        // 只保留USDT交易对
        if (!instId.endsWith("-USDT")) continue;
        
        CryptoQuote quote;
        quote.symbol = instId.replace("-USDT", "");
        quote.price = ticker["last"].toDouble();
        quote.priceCny = quote.price * d->usdToCnyRate;
        quote.high24h = ticker["high"].toDouble();
        quote.low24h = ticker["low"].toDouble();
        quote.volume24h = ticker["vol24h"].toDouble();
        quote.updateTime = QDateTime::currentDateTime();
        
        quotes.append(quote);
    }
    
    return quotes;
}