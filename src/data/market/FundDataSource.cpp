/**
 * @file FundDataSource.cpp
 * @brief 基金数据源实现
 */

#include "FundDataSource.h"
#include "shared/utils/Logger.h"

#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

struct FundDataSource::Impl {
    QNetworkAccessManager* networkManager = nullptr;
    QTimer* refreshTimer = nullptr;
    DataSource dataSource = DataSource::EastMoney;
    
    QHash<QString, FundQuote> quoteCache;
    QStringList watchCodes;
};

FundDataSource* FundDataSource::instance()
{
    static FundDataSource instance;
    return &instance;
}

FundDataSource::FundDataSource(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->networkManager = new QNetworkAccessManager(this);
    d->refreshTimer = new QTimer(this);
    
    connect(d->refreshTimer, &QTimer::timeout, this, [this]() {
        if (!d->watchCodes.isEmpty()) {
            requestQuotes(d->watchCodes, [](const QVector<FundQuote>&) {});
        }
    });
    
    LOG_DEBUG("FundDataSource initialized");
}

FundDataSource::~FundDataSource()
{
    stopAutoRefresh();
}

void FundDataSource::setDataSource(DataSource source)
{
    d->dataSource = source;
}

void FundDataSource::requestQuote(const QString& code,
                                  std::function<void(const FundQuote&)> callback)
{
    QString url;
    
    switch (d->dataSource) {
    case DataSource::EastMoney:
        // 东方财富基金API
        url = QString("https://fundgz.1234567.com.cn/js/%1.js")
              .arg(code);
        break;
    case DataSource::Danjuan:
        // 蛋卷基金API
        url = QString("https://api.doctorxiong.club/v1/fund/detail?code=%1")
              .arg(code);
        break;
    case DataSource::Sina:
        // 新浪基金API
        url = QString("https://hq.sinajs.cn/list=fu_%1")
              .arg(code);
        break;
    }
    
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                     "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    
    QNetworkReply* reply = d->networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, code, callback]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR(QString("Fund request failed: %1").arg(reply->errorString()));
            emit error(reply->errorString());
            return;
        }
        
        QByteArray data = reply->readAll();
        FundQuote quote;
        
        switch (d->dataSource) {
        case DataSource::EastMoney:
            quote = parseEastMoneyResponse(code, data);
            break;
        case DataSource::Danjuan:
            quote = parseDanjuanResponse(code, data);
            break;
        case DataSource::Sina:
            quote = parseSinaResponse(code, data);
            break;
        }
        
        if (quote.isValid()) {
            d->quoteCache[code] = quote;
            emit quoteUpdated(quote);
            callback(quote);
        }
    });
}

void FundDataSource::requestQuotes(const QStringList& codes,
                                   std::function<void(const QVector<FundQuote>&)> callback)
{
    d->watchCodes = codes;
    
    // 东方财富支持批量请求
    if (d->dataSource == DataSource::EastMoney) {
        QString codesParam = codes.join(",");
        QString url = QString("https://api.doctorxiong.club/v1/fund?code=%1").arg(codesParam);
        
        QNetworkRequest request;
        request.setUrl(QUrl(url));
        QNetworkReply* reply = d->networkManager->get(request);
        
        connect(reply, &QNetworkReply::finished, this, [this, reply, codes, callback]() {
            reply->deleteLater();
            
            if (reply->error() != QNetworkReply::NoError) {
                emit error(reply->errorString());
                return;
            }
            
            QByteArray data = reply->readAll();
            QVector<FundQuote> quotes = parseBatchResponse(data);
            
            for (const FundQuote& quote : quotes) {
                d->quoteCache[quote.code] = quote;
            }
            
            emit quotesUpdated(quotes);
            callback(quotes);
        });
    } else {
        // 其他数据源逐个请求
        QVector<FundQuote> results;
        results.reserve(codes.size());
        int pendingCount = codes.size();
        
        for (const QString& code : codes) {
            requestQuote(code, [this, &results, &pendingCount, callback](const FundQuote& quote) {
                results.append(quote);
                pendingCount--;
                
                if (pendingCount == 0) {
                    emit quotesUpdated(results);
                    callback(results);
                }
            });
        }
    }
}

void FundDataSource::searchFund(const QString& keyword,
                                std::function<void(const QVector<FundQuote>&)> callback)
{
    QString url = QString("https://fundsuggest.eastmoney.com/FundSearch/api/FundSearchAPI.ashx"
                         "?m=1&key=%1&pagesize=20").arg(keyword);
    
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    QNetworkReply* reply = d->networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        
        QByteArray data = reply->readAll();
        QVector<FundQuote> quotes = parseSearchResponse(data);
        
        callback(quotes);
    });
}

void FundDataSource::requestFundList(FundType type, const QString& sortField, int limit,
                                     std::function<void(const QVector<FundQuote>&)> callback)
{
    // 东方财富基金排行API
    QString typeParam;
    switch (type) {
    case FundType::ETF: typeParam = "ETF"; break;
    case FundType::LOF: typeParam = "LOF"; break;
    case FundType::Money: typeParam = "HB"; break;
    case FundType::Bond: typeParam = "ZQ"; break;
    case FundType::Stock: typeParam = "GP"; break;
    case FundType::Mixed: typeParam = "HH"; break;
    case FundType::Index: typeParam = "ZS"; break;
    case FundType::QDII: typeParam = "QDII"; break;
    default: typeParam = "all"; break;
    }
    
    QString sortParam;
    if (sortField == "nav") sortParam = "rzdf";
    else if (sortField == "changePercent") sortParam = "jzzzl";
    else if (sortField == "scale") sortParam = "gm";
    else sortParam = "jzzzl";
    
    QString url = QString("https://fund.eastmoney.com/data/rankhandler.aspx"
                         "?op=ph&dt=kf&ft=%1&rs=&gs=0&sc=%2&st=desc&sd=&ed=&qdii=&tabSubtype=,,,,,&pi=1&pn=%3")
                  .arg(typeParam, sortParam).arg(limit);
    
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    QNetworkReply* reply = d->networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            emit error(reply->errorString());
            return;
        }
        
        QByteArray data = reply->readAll();
        QVector<FundQuote> quotes = parseListResponse(data);
        
        callback(quotes);
    });
}

FundQuote FundDataSource::cachedQuote(const QString& code) const
{
    return d->quoteCache.value(code);
}

void FundDataSource::startAutoRefresh(int intervalMs)
{
    d->refreshTimer->start(intervalMs);
    LOG_INFO(QString("Fund auto refresh started, interval: %1ms").arg(intervalMs));
}

void FundDataSource::stopAutoRefresh()
{
    d->refreshTimer->stop();
    LOG_INFO("Fund auto refresh stopped");
}

// ========== 数据解析 ==========

FundQuote FundDataSource::parseEastMoneyResponse(const QString& code, const QByteArray& data)
{
    FundQuote quote;
    quote.code = code;
    
    // 解析东方财富基金数据格式
    // jsonpgz({"fundcode":"510300","name":"沪深300ETF","jzrq":"2026-04-28","dwjz":"4.123","gsz":"4.125",...})
    QString text = QString::fromUtf8(data);
    QRegularExpression regex(R"(jsonpgz\((\{.*\})\))");
    QRegularExpressionMatch match = regex.match(text);
    
    if (match.hasMatch()) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(match.captured(1).toUtf8(), &error);
        
        if (error.error == QJsonParseError::NoError) {
            QJsonObject obj = doc.object();
            
            quote.name = obj["name"].toString();
            quote.nav = obj["dwjz"].toDouble();
            quote.accNav = obj["gsz"].toDouble();
            quote.lastPrice = obj["gsz"].toDouble();
            quote.changePercent = obj["gszzl"].toDouble();
            quote.navDate = QDate::fromString(obj["jzrq"].toString(), "yyyy-MM-dd");
            quote.updateTime = QDateTime::currentDateTime();
            
            // 根据代码判断基金类型
            quote.type = detectFundType(code);
        }
    }
    
    return quote;
}

FundQuote FundDataSource::parseDanjuanResponse(const QString& code, const QByteArray& data)
{
    FundQuote quote;
    quote.code = code;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return quote;
    }
    
    QJsonObject root = doc.object();
    QJsonObject dataObj = root["data"].toObject();
    
    quote.name = dataObj["name"].toString();
    quote.nav = dataObj["nav"].toDouble();
    quote.accNav = dataObj["accNav"].toDouble();
    quote.changePercent = dataObj["changePercent"].toDouble();
    quote.updateTime = QDateTime::currentDateTime();
    quote.type = detectFundType(code);
    
    return quote;
}

FundQuote FundDataSource::parseSinaResponse(const QString& code, const QByteArray& data)
{
    FundQuote quote;
    quote.code = code;
    
    QString text = QString::fromUtf8(data);
    // 简化解析：直接查找数据
    int start = text.indexOf("hq_str_fu_");
    if (start > 0) {
        int quoteStart = text.indexOf("\"", start);
        int quoteEnd = text.indexOf("\"", quoteStart + 1);
        if (quoteStart > 0 && quoteEnd > quoteStart) {
            QString data = text.mid(quoteStart + 1, quoteEnd - quoteStart - 1);
            QStringList parts = data.split(",");
            if (parts.size() >= 5) {
                quote.name = parts[0];
                quote.lastPrice = parts[1].toDouble();
                quote.changeAmount = parts[2].toDouble();
                quote.changePercent = parts[3].toDouble();
                quote.nav = parts[4].toDouble();
                quote.updateTime = QDateTime::currentDateTime();
                quote.type = detectFundType(code);
            }
        }
    }
    
    return quote;
}

QVector<FundQuote> FundDataSource::parseBatchResponse(const QByteArray& data)
{
    QVector<FundQuote> quotes;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return quotes;
    }
    
    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();
    
    for (const QJsonValue& val : dataArray) {
        QJsonObject obj = val.toObject();
        
        FundQuote quote;
        quote.code = obj["code"].toString();
        quote.name = obj["name"].toString();
        quote.nav = obj["nav"].toDouble();
        quote.accNav = obj["accNav"].toDouble();
        quote.changePercent = obj["changePercent"].toDouble();
        quote.updateTime = QDateTime::currentDateTime();
        quote.type = detectFundType(quote.code);
        
        quotes.append(quote);
    }
    
    return quotes;
}

QVector<FundQuote> FundDataSource::parseSearchResponse(const QByteArray& data)
{
    QVector<FundQuote> quotes;
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return quotes;
    }
    
    QJsonObject root = doc.object();
    QJsonArray dataArray = root["datas"].toArray();
    
    for (const QJsonValue& val : dataArray) {
        QJsonObject obj = val.toObject();
        
        FundQuote quote;
        quote.code = obj["CODE"].toString();
        quote.name = obj["NAME"].toString();
        quote.type = detectFundType(quote.code);
        
        quotes.append(quote);
    }
    
    return quotes;
}

QVector<FundQuote> FundDataSource::parseListResponse(const QByteArray& data)
{
    QVector<FundQuote> quotes;
    
    QString text = QString::fromUtf8(data);
    // 解析东方财富排行数据
    QRegularExpression regex(R"(datas:\[(.*)\])");
    QRegularExpressionMatch match = regex.match(text);
    
    if (match.hasMatch()) {
        QStringList items = match.captured(1).split("},{");
        
        for (const QString& item : items) {
            QStringList fields = item.split(",");
            if (fields.size() >= 10) {
                FundQuote quote;
                quote.code = fields[0].remove("\"").remove("{").remove("}");
                quote.name = fields[1].remove("\"");
                quote.nav = fields[3].toDouble();
                quote.accNav = fields[4].toDouble();
                quote.changePercent = fields[5].toDouble();
                quote.scale = fields[8].toDouble() / 100000000; // 转换为亿元
                quote.updateTime = QDateTime::currentDateTime();
                quote.type = detectFundType(quote.code);
                
                quotes.append(quote);
            }
        }
    }
    
    return quotes;
}

FundType FundDataSource::detectFundType(const QString& code)
{
    // 根据基金代码判断类型
    if (code.startsWith("51") || code.startsWith("15")) {
        return FundType::ETF;
    } else if (code.startsWith("16")) {
        return FundType::LOF;
    } else if (code.startsWith("000") || code.startsWith("001")) {
        return FundType::OpenEnd;
    } else if (code.length() == 6 && code.toInt() >= 110000 && code.toInt() < 120000) {
        return FundType::Bond;
    }
    
    return FundType::Unknown;
}