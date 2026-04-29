/**
 * @file StockDataSource.cpp
 * @brief 股票数据源实现
 */

#include "StockDataSource.h"
#include "utils/Logger.h"

#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

StockDataSource::StockDataSource(Source source, QObject *parent)
    : QObject(parent)
    , m_source(source)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_refreshTimer(new QTimer(this))
{
    connect(m_refreshTimer, &QTimer::timeout, this, &StockDataSource::onRefreshTimer);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &StockDataSource::onNetworkReply);
    LOG_DEBUG(QString("StockDataSource created, source: %1").arg(static_cast<int>(source)));
}

StockDataSource::~StockDataSource()
{
    stopAutoRefresh();
}

void StockDataSource::requestQuotes(const QStringList &symbols)
{
    if (symbols.isEmpty()) {
        return;
    }

    m_subscribedSymbols = symbols;
    QString url = buildQuotesUrl(symbols);

    QNetworkRequest request{QUrl(url)};
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    request.setRawHeader("Referer", "http://finance.sina.com.cn");
    QNetworkReply *reply = m_networkManager->get(request);
    m_pendingRequests[reply] = url;
    m_requestTypes[reply] = {RequestType::Quotes, QString()};

    LOG_INFO(QString("Requesting quotes: %1 symbols, URL: %2").arg(symbols.size()).arg(url));
}

void StockDataSource::requestKLine(const QString &symbol, KLinePeriod period, int count)
{
    QString url = buildKLineUrl(symbol, period, count);

    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(QUrl(url)));
    m_pendingRequests[reply] = url;
    m_requestTypes[reply] = {RequestType::KLine, symbol};

    LOG_DEBUG(QString("Requesting KLine: %1, period: %2").arg(symbol).arg(static_cast<int>(period)));
}

void StockDataSource::requestStockList(const QString &market)
{
    // 新浪股票列表API
    QString url = "http://vip.stock.finance.sina.com.cn/quotes_service/api/json_v2.php/Market_Center.getHQNodeData";

    QUrlQuery query;
    query.addQueryItem("num", "5000");
    query.addQueryItem("node", market.isEmpty() ? "sh_a" : market);

    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(QUrl(url + "?" + query.toString())));
    m_pendingRequests[reply] = url;
    m_requestTypes[reply] = {RequestType::StockList, market};

    LOG_DEBUG(QString("Requesting stock list: %1").arg(market.isEmpty() ? "sh_a" : market));
}

StockQuote StockDataSource::getCachedQuote(const QString &symbol) const
{
    return m_quoteCache.value(symbol);
}

void StockDataSource::startAutoRefresh(int intervalMs)
{
    m_refreshTimer->start(intervalMs);
    LOG_INFO(QString("Auto refresh started, interval: %1ms").arg(intervalMs));
}

void StockDataSource::stopAutoRefresh()
{
    m_refreshTimer->stop();
    LOG_INFO("Auto refresh stopped");
}

void StockDataSource::onNetworkReply(QNetworkReply *reply)
{
    reply->deleteLater();

    auto it = m_requestTypes.find(reply);
    if (it == m_requestTypes.end()) {
        return;
    }

    auto [type, param] = it.value();
    m_requestTypes.erase(it);
    m_pendingRequests.remove(reply);

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        LOG_ERROR(QString("Network error: %1").arg(reply->errorString()));
        return;
    }

    QByteArray data = reply->readAll();
    LOG_DEBUG(QString("Network reply received: %1 bytes, type: %2")
        .arg(data.size()).arg(static_cast<int>(type)));

    switch (type) {
    case RequestType::Quotes:
        switch (m_source) {
        case Source::Sina:
            parseSinaQuotes(data);
            break;
        case Source::Tencent:
            parseTencentQuotes(data);
            break;
        case Source::EastMoney:
            parseEastMoneyQuotes(data);
            break;
        }
        break;

    case RequestType::KLine:
        parseSinaKLine(data, param);
        break;

    case RequestType::StockList:
        // 解析股票列表
        parseSinaStockList(data);
        break;
    }
}

void StockDataSource::parseSinaStockList(const QByteArray &data)
{
    // 解析股票列表数据
    QString response = QString::fromLocal8Bit(data);
    
    // 解析格式：每行一个股票信息
    QStringList lines = response.split('\n', Qt::SkipEmptyParts);
    QStringList symbols;
    
    for (const QString& line : lines) {
        // 格式：代码,名称,行业,市值等
        QStringList fields = line.split(',');
        if (fields.size() >= 1) {
            QString symbol = fields[0].trimmed();
            
            // 验证股票代码格式
            if (symbol.length() == 6 && symbol.toInt() > 0) {
                symbols.append(symbol);
            }
        }
    }
    
    if (!symbols.isEmpty()) {
        emit stockListReceived(symbols);
        LOG_INFO(QString("Parsed %1 stocks from list").arg(symbols.size()));
    }
}

void StockDataSource::onRefreshTimer()
{
    if (!m_subscribedSymbols.isEmpty()) {
        requestQuotes(m_subscribedSymbols);
    }
}

void StockDataSource::parseSinaQuotes(const QByteArray &data)
{
    // 新浪行情格式: var hq_str_sh600000="浦发银行,12.34,12.30,12.50,12.20,12.45,12.46,12345678,..."
    QString response = QString::fromLocal8Bit(data);
    QVector<StockQuote> quotes;

    // 更简单的正则表达式
    QRegularExpression re("var hq_str_(\\w+)=\"([^\"]*)\"");
    QRegularExpressionMatchIterator it = re.globalMatch(response);

    int matchCount = 0;
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        matchCount++;
        QString symbol = match.captured(1);
        QString content = match.captured(2);

        if (content.isEmpty()) {
            LOG_DEBUG(QString("Empty content for symbol: %1").arg(symbol));
            continue;
        }

        QStringList fields = content.split(',');
        if (fields.size() < 10) {
            LOG_DEBUG(QString("Not enough fields for %1: %2 fields").arg(symbol).arg(fields.size()));
            continue;
        }

        StockQuote quote;
        quote.symbol = symbol;
        quote.name = fields[0];
        quote.openPrice = fields[1].toDouble();
        quote.preClose = fields[2].toDouble();
        quote.lastPrice = fields[3].toDouble();
        quote.highPrice = fields[4].toDouble();
        quote.lowPrice = fields[5].toDouble();
        quote.volume = fields[8].toLongLong();
        quote.turnover = fields[9].toDouble();

        if (quote.preClose > 0) {
            quote.changeAmount = quote.lastPrice - quote.preClose;
            quote.changePercent = quote.changeAmount / quote.preClose * 100.0;
        }

        quote.updateTime = QDateTime::currentDateTime();

        m_quoteCache[symbol] = quote;
        quotes.append(quote);
    }

    LOG_DEBUG(QString("Regex matched %1 times, parsed %2 quotes").arg(matchCount).arg(quotes.size()));

    if (!quotes.isEmpty()) {
        emit quotesReceived(quotes);
        LOG_INFO(QString("Parsed %1 quotes from Sina").arg(quotes.size()));
    } else {
        LOG_WARNING(QString("No quotes parsed, response length: %1").arg(response.length()));
    }
}

void StockDataSource::parseTencentQuotes(const QByteArray &data)
{
    // 腾讯行情格式: v_sh600000="51~浦发银行~600000~12.34~..."
    QString response = QString::fromLocal8Bit(data);
    QVector<StockQuote> quotes;
    
    // 解析腾讯行情格式
    QRegularExpression re("v_(\\w+)=\"([^\"]*)\"");
    QRegularExpressionMatchIterator it = re.globalMatch(response);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString symbol = match.captured(1);
        QString content = match.captured(2);
        
        if (content.isEmpty()) continue;
        
        // 腾讯格式：用~分隔
        QStringList fields = content.split('~');
        if (fields.size() >= 10) {
            StockQuote quote;
            quote.symbol = fields[2];  // 股票代码
            quote.name = fields[1];    // 股票名称
            quote.lastPrice = fields[3].toDouble();  // 当前价
            quote.preClose = fields[4].toDouble();   // 昨收
            quote.openPrice = fields[5].toDouble();  // 今开
            quote.volume = fields[6].toLongLong();   // 成交量
            quote.turnover = fields[37].toDouble();  // 成交额
            quote.highPrice = fields[33].toDouble(); // 最高
            quote.lowPrice = fields[34].toDouble();  // 最低
            
            if (quote.preClose > 0) {
                quote.changeAmount = quote.lastPrice - quote.preClose;
                quote.changePercent = quote.changeAmount / quote.preClose * 100.0;
            }
            
            quote.updateTime = QDateTime::currentDateTime();
            m_quoteCache[symbol] = quote;
            quotes.append(quote);
        }
    }
    
    if (!quotes.isEmpty()) {
        emit quotesReceived(quotes);
        LOG_DEBUG(QString("Tencent: parsed %1 quotes").arg(quotes.size()));
    }
}

void StockDataSource::parseEastMoneyQuotes(const QByteArray &data)
{
    // 东方财富返回JSON格式
    QString response = QString::fromLocal8Bit(data);
    QVector<StockQuote> quotes;
    
    // 解析JSON格式
    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        LOG_WARNING("Invalid EastMoney JSON response");
        return;
    }
    
    QJsonObject root = doc.object();
    QJsonArray dataArray = root["data"].toArray();
    
    for (const auto& item : dataArray) {
        QJsonObject obj = item.toObject();
        StockQuote quote;
        
        quote.symbol = obj["code"].toString();
        quote.name = obj["name"].toString();
        quote.lastPrice = obj["price"].toDouble();
        quote.preClose = obj["pc"].toDouble();
        quote.openPrice = obj["open"].toDouble();
        quote.highPrice = obj["high"].toDouble();
        quote.lowPrice = obj["low"].toDouble();
        quote.volume = obj["volume"].toVariant().toLongLong();
        quote.turnover = obj["amount"].toDouble();
        
        if (quote.preClose > 0) {
            quote.changeAmount = quote.lastPrice - quote.preClose;
            quote.changePercent = quote.changeAmount / quote.preClose * 100.0;
        }
        
        quote.updateTime = QDateTime::currentDateTime();
        m_quoteCache[quote.symbol] = quote;
        quotes.append(quote);
    }
    
    if (!quotes.isEmpty()) {
        emit quotesReceived(quotes);
        LOG_DEBUG(QString("EastMoney: parsed %1 quotes").arg(quotes.size()));
    }
}

void StockDataSource::parseSinaKLine(const QByteArray &data, const QString &symbol)
{
    // 新浪K线格式: 数据以换行分隔，每行: 日期,开盘,最高,最低,收盘,成交量
    QString response = QString::fromLocal8Bit(data);
    QVector<KLineData> klines;

    QStringList lines = response.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        QStringList fields = line.split(',');
        if (fields.size() < 6) {
            continue;
        }

        KLineData kline;
        kline.time = QDateTime::fromString(fields[0], "yyyy-MM-dd");
        if (!kline.time.isValid()) {
            kline.time = QDateTime::fromString(fields[0], "yyyy-MM-dd hh:mm:ss");
        }
        kline.open = fields[1].toDouble();
        kline.high = fields[2].toDouble();
        kline.low = fields[3].toDouble();
        kline.close = fields[4].toDouble();
        kline.volume = fields[5].toLongLong();

        if (kline.isValid()) {
            klines.append(kline);
        }
    }

    if (!klines.isEmpty()) {
        emit kLineReceived(symbol, klines);
        LOG_DEBUG(QString("Parsed %1 KLines for %2").arg(klines.size()).arg(symbol));
    }
}

QString StockDataSource::buildQuotesUrl(const QStringList &symbols) const
{
    switch (m_source) {
    case Source::Sina: {
        QString list = symbols.join(',');
        return QString("http://hq.sinajs.cn/list=%1").arg(list);
    }
    case Source::Tencent: {
        QString list = symbols.join(',');
        return QString("http://qt.gtimg.cn/q=%1").arg(list);
    }
    case Source::EastMoney: {
        // 东方财富需要不同的API格式
        QString codes = symbols.join(',');
        return QString("http://push2.eastmoney.com/api/qt/clist/get?secids=%1&fields=f12,f14,f2,f3,f4,f5,f6").arg(codes);
    }
    }
    return QString();
}

QString StockDataSource::buildKLineUrl(const QString &symbol, KLinePeriod period, int count) const
{
    // 新浪K线API
    QString periodStr;
    switch (period) {
    case KLinePeriod::Day1: periodStr = "daily"; break;
    case KLinePeriod::Week1: periodStr = "weekly"; break;
    case KLinePeriod::Month1: periodStr = "monthly"; break;
    case KLinePeriod::Minute5: periodStr = "5"; break;
    case KLinePeriod::Minute15: periodStr = "15"; break;
    case KLinePeriod::Minute30: periodStr = "30"; break;
    case KLinePeriod::Hour1: periodStr = "60"; break;
    default: periodStr = "daily"; break;
    }

    QString sinaSymbol = toSinaSymbol(symbol);
    return QString("http://money.finance.sina.com.cn/quotes_service/api/json_v2.php/CN_MarketData.getKLineData"
                   "?symbol=%1&scale=%2&datalen=%3")
        .arg(sinaSymbol, periodStr)
        .arg(count);
}

QString StockDataSource::normalizeSymbol(const QString &symbol)
{
    // 移除前缀 sh/sz
    if (symbol.startsWith("sh") || symbol.startsWith("sz")) {
        return symbol.mid(2);
    }
    return symbol;
}

QString StockDataSource::toSinaSymbol(const QString &symbol)
{
    // 如果已经有前缀，直接返回
    if (symbol.startsWith("sh") || symbol.startsWith("sz") || 
        symbol.startsWith("SH") || symbol.startsWith("SZ")) {
        return symbol.toLower();
    }
    
    // 根据代码第一位判断市场
    // 6开头是上海，其他是深圳
    if (symbol.startsWith("6")) {
        return "sh" + symbol;
    } else {
        return "sz" + symbol;
    }
}
