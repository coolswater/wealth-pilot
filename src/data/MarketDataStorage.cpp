/**
 * @file MarketDataStorage.cpp
 * @brief 市场数据存储管理器实现
 */

#include "MarketDataStorage.h"
#include "core/database/DatabaseManager.h"
#include "core/cache/DataCacheManager.h"
#include "data/market/StockDataSource.h"
#include "data/market/ForexDataSource.h"
#include "data/market/CryptoDataSource.h"
#include "data/market/FundDataSource.h"
#include "shared/utils/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QMutexLocker>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>

struct MarketDataStorage::Impl {
    QSqlDatabase db;
    QString dbPath;
    bool initialized = false;
    
    QHash<MarketDataType, int> ttlSettings;
    QHash<QString, DataFreshness> freshnessCache;
    
    mutable QMutex mutex;
};

MarketDataStorage* MarketDataStorage::instance()
{
    static MarketDataStorage instance;
    return &instance;
}

MarketDataStorage::MarketDataStorage(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    // 默认TTL设置
    d->ttlSettings[MarketDataType::Stock] = 60;      // 股票60秒
    d->ttlSettings[MarketDataType::Futures] = 30;    // 期货30秒
    d->ttlSettings[MarketDataType::Crypto] = 60;     // 数字货币60秒
    d->ttlSettings[MarketDataType::Fund] = 3600;     // 基金1小时
    d->ttlSettings[MarketDataType::Forex] = 60;      // 外汇60秒
}

MarketDataStorage::~MarketDataStorage()
{
    shutdown();
}

bool MarketDataStorage::initialize(const QString& dbPath)
{
    QMutexLocker locker(&d->mutex);
    
    if (d->initialized) {
        return true;
    }
    
    // 使用DatabaseManager获取连接
    d->db = QSqlDatabase::database();
    if (!d->db.isOpen()) {
        LOG_ERROR("Failed to get database connection");
        return false;
    }
    
    if (!createTables()) {
        LOG_ERROR("Failed to create tables");
        return false;
    }
    
    d->initialized = true;
    LOG_INFO("MarketDataStorage initialized");
    return true;
}

void MarketDataStorage::shutdown()
{
    QMutexLocker locker(&d->mutex);
    d->initialized = false;
    d->freshnessCache.clear();
    LOG_INFO("MarketDataStorage shutdown");
}

bool MarketDataStorage::createTables()
{
    QSqlQuery query(d->db);
    
    // 股票行情表
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS stock_quotes (
            code TEXT PRIMARY KEY,
            name TEXT,
            last_price REAL,
            open_price REAL,
            high_price REAL,
            low_price REAL,
            pre_close REAL,
            volume INTEGER,
            turnover REAL,
            change_percent REAL,
            change_amount REAL,
            update_time DATETIME,
            data_json TEXT
        )
    )")) {
        LOG_ERROR(QString("Failed to create stock_quotes table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // 期货行情表
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS futures_quotes (
            instrument_id TEXT PRIMARY KEY,
            name TEXT,
            last_price REAL,
            open_price REAL,
            high_price REAL,
            low_price REAL,
            pre_settlement REAL,
            volume INTEGER,
            turnover REAL,
            change_percent REAL,
            change_amount REAL,
            update_time DATETIME,
            data_json TEXT
        )
    )")) {
        LOG_ERROR(QString("Failed to create futures_quotes table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // 数字货币行情表
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS crypto_quotes (
            symbol TEXT PRIMARY KEY,
            name TEXT,
            price REAL,
            price_cny REAL,
            change_24h REAL,
            volume_24h REAL,
            market_cap REAL,
            high_24h REAL,
            low_24h REAL,
            rank INTEGER,
            update_time DATETIME,
            data_json TEXT
        )
    )")) {
        LOG_ERROR(QString("Failed to create crypto_quotes table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // 基金净值表
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS fund_quotes (
            code TEXT PRIMARY KEY,
            name TEXT,
            type INTEGER,
            nav REAL,
            acc_nav REAL,
            last_price REAL,
            change_percent REAL,
            change_amount REAL,
            manager TEXT,
            company TEXT,
            scale REAL,
            update_time DATETIME,
            data_json TEXT
        )
    )")) {
        LOG_ERROR(QString("Failed to create fund_quotes table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // 外汇汇率表
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS forex_quotes (
            pair TEXT PRIMARY KEY,
            base_currency TEXT,
            quote_currency TEXT,
            rate REAL,
            bid REAL,
            ask REAL,
            change REAL,
            change_percent REAL,
            high_24h REAL,
            low_24h REAL,
            update_time DATETIME,
            data_json TEXT
        )
    )")) {
        LOG_ERROR(QString("Failed to create forex_quotes table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // 创建索引
    query.exec("CREATE INDEX IF NOT EXISTS idx_stock_update ON stock_quotes(update_time)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_futures_update ON futures_quotes(update_time)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_crypto_update ON crypto_quotes(update_time)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_fund_update ON fund_quotes(update_time)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_forex_update ON forex_quotes(update_time)");
    
    return true;
}

QString MarketDataStorage::tableName(MarketDataType type) const
{
    switch (type) {
    case MarketDataType::Stock: return "stock_quotes";
    case MarketDataType::Futures: return "futures_quotes";
    case MarketDataType::Crypto: return "crypto_quotes";
    case MarketDataType::Fund: return "fund_quotes";
    case MarketDataType::Forex: return "forex_quotes";
    }
    return QString();
}

// ========== 股票数据实现 ==========

bool MarketDataStorage::saveStockQuote(const QString& code, const StockQuote& quote)
{
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare(R"(
        INSERT OR REPLACE INTO stock_quotes 
        (code, name, last_price, open_price, high_price, low_price, pre_close,
         volume, turnover, change_percent, change_amount, update_time, data_json)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(code);
    query.addBindValue(quote.name);
    query.addBindValue(quote.lastPrice);
    query.addBindValue(quote.openPrice);
    query.addBindValue(quote.highPrice);
    query.addBindValue(quote.lowPrice);
    query.addBindValue(quote.preClose);
    query.addBindValue(quote.volume);
    query.addBindValue(quote.turnover);
    query.addBindValue(quote.changePercent);
    query.addBindValue(quote.changeAmount);
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(QString()); // JSON备用
    
    if (!query.exec()) {
        LOG_ERROR(QString("Failed to save stock quote: %1").arg(query.lastError().text()));
        return false;
    }
    
    // 更新新鲜度缓存
    DataFreshness freshness;
    freshness.lastUpdate = QDateTime::currentDateTime();
    freshness.ttlSeconds = d->ttlSettings[MarketDataType::Stock];
    d->freshnessCache[QString("stock_%1").arg(code)] = freshness;
    
    // 更新内存缓存
    DataCacheManager::instance()->set(QString("stock_%1").arg(code), quote, 60000);
    
    return true;
}

bool MarketDataStorage::saveStockQuotes(const QVector<QPair<QString, StockQuote>>& quotes)
{
    d->db.transaction();
    
    for (const auto& pair : quotes) {
        if (!saveStockQuote(pair.first, pair.second)) {
            d->db.rollback();
            return false;
        }
    }
    
    d->db.commit();
    return true;
}

bool MarketDataStorage::getStockQuote(const QString& code, StockQuote& outQuote)
{
    // 1. 先检查内存缓存
    if (DataCacheManager::instance()->get(QString("stock_%1").arg(code), outQuote)) {
        return true;
    }
    
    // 2. 检查数据库
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare("SELECT * FROM stock_quotes WHERE code = ?");
    query.addBindValue(code);
    
    if (query.exec() && query.next()) {
        outQuote.symbol = query.value("code").toString();
        outQuote.name = query.value("name").toString();
        outQuote.lastPrice = query.value("last_price").toDouble();
        outQuote.openPrice = query.value("open_price").toDouble();
        outQuote.highPrice = query.value("high_price").toDouble();
        outQuote.lowPrice = query.value("low_price").toDouble();
        outQuote.preClose = query.value("pre_close").toDouble();
        outQuote.volume = query.value("volume").toLongLong();
        outQuote.turnover = query.value("turnover").toDouble();
        outQuote.changePercent = query.value("change_percent").toDouble();
        outQuote.changeAmount = query.value("change_amount").toDouble();
        
        // 写入内存缓存
        locker.unlock();
        DataCacheManager::instance()->set(QString("stock_%1").arg(code), outQuote, 60000);
        
        return true;
    }
    
    return false;
}

QVector<StockQuote> MarketDataStorage::getStockQuotes(const QStringList& codes)
{
    QVector<StockQuote> result;
    for (const QString& code : codes) {
        StockQuote quote;
        if (getStockQuote(code, quote)) {
            result.append(quote);
        }
    }
    return result;
}

QVector<StockQuote> MarketDataStorage::getAllStockQuotes()
{
    QVector<StockQuote> result;
    
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.exec("SELECT * FROM stock_quotes");
    
    while (query.next()) {
        StockQuote quote;
        quote.symbol = query.value("code").toString();
        quote.name = query.value("name").toString();
        quote.lastPrice = query.value("last_price").toDouble();
        quote.openPrice = query.value("open_price").toDouble();
        quote.highPrice = query.value("high_price").toDouble();
        quote.lowPrice = query.value("low_price").toDouble();
        quote.preClose = query.value("pre_close").toDouble();
        quote.volume = query.value("volume").toLongLong();
        quote.turnover = query.value("turnover").toDouble();
        quote.changePercent = query.value("change_percent").toDouble();
        quote.changeAmount = query.value("change_amount").toDouble();
        result.append(quote);
    }
    
    return result;
}

// ========== 期货数据实现 ==========

bool MarketDataStorage::saveFuturesQuote(const QString& instrumentId, const QVariantMap& quote)
{
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare(R"(
        INSERT OR REPLACE INTO futures_quotes 
        (instrument_id, name, last_price, open_price, high_price, low_price, pre_settlement,
         volume, turnover, change_percent, change_amount, update_time, data_json)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(instrumentId);
    query.addBindValue(quote.value("name").toString());
    query.addBindValue(quote.value("lastPrice").toDouble());
    query.addBindValue(quote.value("openPrice").toDouble());
    query.addBindValue(quote.value("highPrice").toDouble());
    query.addBindValue(quote.value("lowPrice").toDouble());
    query.addBindValue(quote.value("preSettlement").toDouble());
    query.addBindValue(quote.value("volume").toLongLong());
    query.addBindValue(quote.value("turnover").toDouble());
    query.addBindValue(quote.value("changePercent").toDouble());
    query.addBindValue(quote.value("changeAmount").toDouble());
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(QString());
    
    if (!query.exec()) {
        LOG_ERROR(QString("Failed to save futures quote: %1").arg(query.lastError().text()));
        return false;
    }
    
    // 更新缓存
    DataCacheManager::instance()->set(QString("futures_%1").arg(instrumentId), quote, 30000);
    
    return true;
}

bool MarketDataStorage::getFuturesQuote(const QString& instrumentId, QVariantMap& outQuote)
{
    // 先检查缓存
    if (DataCacheManager::instance()->get(QString("futures_%1").arg(instrumentId), outQuote)) {
        return true;
    }
    
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare("SELECT * FROM futures_quotes WHERE instrument_id = ?");
    query.addBindValue(instrumentId);
    
    if (query.exec() && query.next()) {
        outQuote["instrumentId"] = query.value("instrument_id");
        outQuote["name"] = query.value("name");
        outQuote["lastPrice"] = query.value("last_price");
        outQuote["openPrice"] = query.value("open_price");
        outQuote["highPrice"] = query.value("high_price");
        outQuote["lowPrice"] = query.value("low_price");
        outQuote["preSettlement"] = query.value("pre_settlement");
        outQuote["volume"] = query.value("volume");
        outQuote["turnover"] = query.value("turnover");
        outQuote["changePercent"] = query.value("change_percent");
        outQuote["changeAmount"] = query.value("change_amount");
        
        locker.unlock();
        DataCacheManager::instance()->set(QString("futures_%1").arg(instrumentId), outQuote, 30000);
        
        return true;
    }
    
    return false;
}

QVector<QVariantMap> MarketDataStorage::getAllFuturesQuotes()
{
    QVector<QVariantMap> result;
    
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.exec("SELECT * FROM futures_quotes");
    
    while (query.next()) {
        QVariantMap quote;
        quote["instrumentId"] = query.value("instrument_id");
        quote["name"] = query.value("name");
        quote["lastPrice"] = query.value("last_price");
        quote["openPrice"] = query.value("open_price");
        quote["highPrice"] = query.value("high_price");
        quote["lowPrice"] = query.value("low_price");
        quote["preSettlement"] = query.value("pre_settlement");
        quote["volume"] = query.value("volume");
        quote["turnover"] = query.value("turnover");
        quote["changePercent"] = query.value("change_percent");
        quote["changeAmount"] = query.value("change_amount");
        result.append(quote);
    }
    
    return result;
}

// ========== 数字货币数据实现 ==========

bool MarketDataStorage::saveCryptoQuote(const QString& symbol, const CryptoQuote& quote)
{
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare(R"(
        INSERT OR REPLACE INTO crypto_quotes 
        (symbol, name, price, price_cny, change_24h, volume_24h, market_cap,
         high_24h, low_24h, rank, update_time, data_json)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(symbol);
    query.addBindValue(quote.name);
    query.addBindValue(quote.price);
    query.addBindValue(quote.priceCny);
    query.addBindValue(quote.change24h);
    query.addBindValue(quote.volume24h);
    query.addBindValue(quote.marketCap);
    query.addBindValue(quote.high24h);
    query.addBindValue(quote.low24h);
    query.addBindValue(quote.rank);
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(QString());
    
    if (!query.exec()) {
        LOG_ERROR(QString("Failed to save crypto quote: %1").arg(query.lastError().text()));
        return false;
    }
    
    DataCacheManager::instance()->set(QString("crypto_%1").arg(symbol), quote, 60000);
    
    return true;
}

bool MarketDataStorage::saveCryptoQuotes(const QVector<CryptoQuote>& quotes)
{
    d->db.transaction();
    
    for (const CryptoQuote& quote : quotes) {
        if (!saveCryptoQuote(quote.symbol, quote)) {
            d->db.rollback();
            return false;
        }
    }
    
    d->db.commit();
    return true;
}

bool MarketDataStorage::getCryptoQuote(const QString& symbol, CryptoQuote& outQuote)
{
    if (DataCacheManager::instance()->get(QString("crypto_%1").arg(symbol), outQuote)) {
        return true;
    }
    
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare("SELECT * FROM crypto_quotes WHERE symbol = ?");
    query.addBindValue(symbol);
    
    if (query.exec() && query.next()) {
        outQuote.symbol = query.value("symbol").toString();
        outQuote.name = query.value("name").toString();
        outQuote.price = query.value("price").toDouble();
        outQuote.priceCny = query.value("price_cny").toDouble();
        outQuote.change24h = query.value("change_24h").toDouble();
        outQuote.volume24h = query.value("volume_24h").toDouble();
        outQuote.marketCap = query.value("market_cap").toDouble();
        outQuote.high24h = query.value("high_24h").toDouble();
        outQuote.low24h = query.value("low_24h").toDouble();
        outQuote.rank = query.value("rank").toInt();
        
        locker.unlock();
        DataCacheManager::instance()->set(QString("crypto_%1").arg(symbol), outQuote, 60000);
        
        return true;
    }
    
    return false;
}

QVector<CryptoQuote> MarketDataStorage::getAllCryptoQuotes()
{
    QVector<CryptoQuote> result;
    
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.exec("SELECT * FROM crypto_quotes ORDER BY rank");
    
    while (query.next()) {
        CryptoQuote quote;
        quote.symbol = query.value("symbol").toString();
        quote.name = query.value("name").toString();
        quote.price = query.value("price").toDouble();
        quote.priceCny = query.value("price_cny").toDouble();
        quote.change24h = query.value("change_24h").toDouble();
        quote.volume24h = query.value("volume_24h").toDouble();
        quote.marketCap = query.value("market_cap").toDouble();
        quote.high24h = query.value("high_24h").toDouble();
        quote.low24h = query.value("low_24h").toDouble();
        quote.rank = query.value("rank").toInt();
        result.append(quote);
    }
    
    return result;
}

// ========== 基金数据实现 ==========

bool MarketDataStorage::saveFundQuote(const QString& code, const FundQuote& quote)
{
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare(R"(
        INSERT OR REPLACE INTO fund_quotes 
        (code, name, type, nav, acc_nav, last_price, change_percent, change_amount,
         manager, company, scale, update_time, data_json)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(code);
    query.addBindValue(quote.name);
    query.addBindValue(static_cast<int>(quote.type));
    query.addBindValue(quote.nav);
    query.addBindValue(quote.accNav);
    query.addBindValue(quote.lastPrice);
    query.addBindValue(quote.changePercent);
    query.addBindValue(quote.changeAmount);
    query.addBindValue(quote.manager);
    query.addBindValue(quote.company);
    query.addBindValue(quote.scale);
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(QString());
    
    if (!query.exec()) {
        LOG_ERROR(QString("Failed to save fund quote: %1").arg(query.lastError().text()));
        return false;
    }
    
    DataCacheManager::instance()->set(QString("fund_%1").arg(code), quote, 3600000);
    
    return true;
}

bool MarketDataStorage::saveFundQuotes(const QVector<FundQuote>& quotes)
{
    d->db.transaction();
    
    for (const FundQuote& quote : quotes) {
        if (!saveFundQuote(quote.code, quote)) {
            d->db.rollback();
            return false;
        }
    }
    
    d->db.commit();
    return true;
}

bool MarketDataStorage::getFundQuote(const QString& code, FundQuote& outQuote)
{
    if (DataCacheManager::instance()->get(QString("fund_%1").arg(code), outQuote)) {
        return true;
    }
    
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare("SELECT * FROM fund_quotes WHERE code = ?");
    query.addBindValue(code);
    
    if (query.exec() && query.next()) {
        outQuote.code = query.value("code").toString();
        outQuote.name = query.value("name").toString();
        outQuote.type = static_cast<FundType>(query.value("type").toInt());
        outQuote.nav = query.value("nav").toDouble();
        outQuote.accNav = query.value("acc_nav").toDouble();
        outQuote.lastPrice = query.value("last_price").toDouble();
        outQuote.changePercent = query.value("change_percent").toDouble();
        outQuote.changeAmount = query.value("change_amount").toDouble();
        outQuote.manager = query.value("manager").toString();
        outQuote.company = query.value("company").toString();
        outQuote.scale = query.value("scale").toDouble();
        
        locker.unlock();
        DataCacheManager::instance()->set(QString("fund_%1").arg(code), outQuote, 3600000);
        
        return true;
    }
    
    return false;
}

QVector<FundQuote> MarketDataStorage::getAllFundQuotes()
{
    QVector<FundQuote> result;
    
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.exec("SELECT * FROM fund_quotes ORDER BY change_percent DESC");
    
    while (query.next()) {
        FundQuote quote;
        quote.code = query.value("code").toString();
        quote.name = query.value("name").toString();
        quote.type = static_cast<FundType>(query.value("type").toInt());
        quote.nav = query.value("nav").toDouble();
        quote.accNav = query.value("acc_nav").toDouble();
        quote.lastPrice = query.value("last_price").toDouble();
        quote.changePercent = query.value("change_percent").toDouble();
        quote.changeAmount = query.value("change_amount").toDouble();
        quote.manager = query.value("manager").toString();
        quote.company = query.value("company").toString();
        quote.scale = query.value("scale").toDouble();
        result.append(quote);
    }
    
    return result;
}

// ========== 外汇数据实现 ==========

bool MarketDataStorage::saveForexQuote(const QString& pair, const ForexQuote& quote)
{
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare(R"(
        INSERT OR REPLACE INTO forex_quotes 
        (pair, base_currency, quote_currency, rate, bid, ask, change, change_percent,
         high_24h, low_24h, update_time, data_json)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    
    query.addBindValue(pair);
    query.addBindValue(quote.baseCurrency);
    query.addBindValue(quote.quoteCurrency);
    query.addBindValue(quote.rate);
    query.addBindValue(quote.bid);
    query.addBindValue(quote.ask);
    query.addBindValue(quote.change);
    query.addBindValue(quote.changePercent);
    query.addBindValue(quote.high24h);
    query.addBindValue(quote.low24h);
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(QString());
    
    if (!query.exec()) {
        LOG_ERROR(QString("Failed to save forex quote: %1").arg(query.lastError().text()));
        return false;
    }
    
    DataCacheManager::instance()->set(QString("forex_%1").arg(pair), quote, 60000);
    
    return true;
}

bool MarketDataStorage::saveForexQuotes(const QVector<ForexQuote>& quotes)
{
    d->db.transaction();
    
    for (const ForexQuote& quote : quotes) {
        if (!saveForexQuote(quote.pair, quote)) {
            d->db.rollback();
            return false;
        }
    }
    
    d->db.commit();
    return true;
}

bool MarketDataStorage::getForexQuote(const QString& pair, ForexQuote& outQuote)
{
    if (DataCacheManager::instance()->get(QString("forex_%1").arg(pair), outQuote)) {
        return true;
    }
    
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.prepare("SELECT * FROM forex_quotes WHERE pair = ?");
    query.addBindValue(pair);
    
    if (query.exec() && query.next()) {
        outQuote.pair = query.value("pair").toString();
        outQuote.baseCurrency = query.value("base_currency").toString();
        outQuote.quoteCurrency = query.value("quote_currency").toString();
        outQuote.rate = query.value("rate").toDouble();
        outQuote.bid = query.value("bid").toDouble();
        outQuote.ask = query.value("ask").toDouble();
        outQuote.change = query.value("change").toDouble();
        outQuote.changePercent = query.value("change_percent").toDouble();
        outQuote.high24h = query.value("high_24h").toDouble();
        outQuote.low24h = query.value("low_24h").toDouble();
        
        locker.unlock();
        DataCacheManager::instance()->set(QString("forex_%1").arg(pair), outQuote, 60000);
        
        return true;
    }
    
    return false;
}

QVector<ForexQuote> MarketDataStorage::getAllForexQuotes()
{
    QVector<ForexQuote> result;
    
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.exec("SELECT * FROM forex_quotes");
    
    while (query.next()) {
        ForexQuote quote;
        quote.pair = query.value("pair").toString();
        quote.baseCurrency = query.value("base_currency").toString();
        quote.quoteCurrency = query.value("quote_currency").toString();
        quote.rate = query.value("rate").toDouble();
        quote.bid = query.value("bid").toDouble();
        quote.ask = query.value("ask").toDouble();
        quote.change = query.value("change").toDouble();
        quote.changePercent = query.value("change_percent").toDouble();
        quote.high24h = query.value("high_24h").toDouble();
        quote.low24h = query.value("low_24h").toDouble();
        result.append(quote);
    }
    
    return result;
}

// ========== 数据新鲜度管理 ==========

bool MarketDataStorage::isDataFresh(MarketDataType type, const QString& code)
{
    QString key = QString("%1_%2").arg(tableName(type)).arg(code);
    
    if (d->freshnessCache.contains(key)) {
        return !d->freshnessCache[key].isExpired();
    }
    
    return false;
}

DataFreshness MarketDataStorage::getDataFreshness(MarketDataType type, const QString& code)
{
    QString key = QString("%1_%2").arg(tableName(type)).arg(code);
    return d->freshnessCache.value(key);
}

void MarketDataStorage::setDataTTL(MarketDataType type, int ttlSeconds)
{
    d->ttlSettings[type] = ttlSeconds;
}

// ========== 数据维护 ==========

void MarketDataStorage::cleanExpiredData()
{
    QMutexLocker locker(&d->mutex);
    
    QDateTime threshold = QDateTime::currentDateTime().addSecs(-3600); // 清理1小时前的数据
    
    QStringList tables = {"stock_quotes", "futures_quotes", "crypto_quotes", "fund_quotes", "forex_quotes"};
    
    for (const QString& table : tables) {
        QSqlQuery query(d->db);
        query.prepare(QString("DELETE FROM %1 WHERE update_time < ?").arg(table));
        query.addBindValue(threshold);
        query.exec();
    }
    
    LOG_INFO("Cleaned expired market data");
}

void MarketDataStorage::clearAllData()
{
    QMutexLocker locker(&d->mutex);
    
    QStringList tables = {"stock_quotes", "futures_quotes", "crypto_quotes", "fund_quotes", "forex_quotes"};
    
    for (const QString& table : tables) {
        QSqlQuery query(d->db);
        query.exec(QString("DELETE FROM %1").arg(table));
    }
    
    d->freshnessCache.clear();
    DataCacheManager::instance()->clear();
    
    LOG_INFO("Cleared all market data");
}

qint64 MarketDataStorage::getDatabaseSize() const
{
    if (d->dbPath.isEmpty()) return 0;
    
    QFileInfo info(d->dbPath);
    return info.size();
}

void MarketDataStorage::optimize()
{
    QMutexLocker locker(&d->mutex);
    
    QSqlQuery query(d->db);
    query.exec("VACUUM");
    query.exec("ANALYZE");
    
    LOG_INFO("Database optimized");
}
