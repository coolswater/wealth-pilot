/**
 * @file DataStorageService.cpp
 * @brief 数据存储服务实现
 */

#include "DataStorageService.h"
#include "utils/Logger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QStandardPaths>

// ============================================================================
// 静态成员
// ============================================================================

DataStorageService* DataStorageService::s_instance = nullptr;

// ============================================================================
// 实现
// ============================================================================

DataStorageService::DataStorageService()
    : QObject(nullptr)
{
}

DataStorageService::~DataStorageService()
{
}

DataStorageService* DataStorageService::instance()
{
    if (!s_instance) {
        s_instance = new DataStorageService();
    }
    return s_instance;
}

bool DataStorageService::initialize(const QString& dbPath)
{
    if (m_initialized) {
        return true;
    }
    
    // 设置数据库路径
    if (dbPath.isEmpty()) {
        m_dbPath = "D:/C++/wealth-pilot/datastorage/WealthPilot.db";
    } else {
        m_dbPath = dbPath;
    }
    
    // 确保目录存在
    QDir dir = QFileInfo(m_dbPath).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 配置数据库
    DatabaseConfig config;
    config.databaseName = m_dbPath;
    config.enableWAL = true;
    config.cacheSize = 8192;  // 8MB cache
    config.maxConnections = 5;
    
    // 初始化数据库管理器
    if (!DatabaseManager::instance()->initialize(config)) {
        LOG_ERROR("Failed to initialize database manager");
        return false;
    }
    
    // 创建表
    createTables();
    
    m_initialized = true;
    LOG_INFO(QString("DataStorageService initialized, db: %1").arg(m_dbPath));
    return true;
}

void DataStorageService::createTables()
{
    LOG_DEBUG("Creating data storage tables...");
    
    // 指数历史数据表
    DatabaseManager::instance()->executeQuery(R"(
        CREATE TABLE IF NOT EXISTS index_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            code TEXT NOT NULL,
            name TEXT,
            close_price REAL,
            change_percent REAL,
            volume REAL,
            amount REAL,
            date DATE NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            UNIQUE(code, date)
        )
    )");
    
    // 创建索引
    DatabaseManager::instance()->executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_index_history_code ON index_history(code)");
    DatabaseManager::instance()->executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_index_history_date ON index_history(date)");
    
    // 自选股表
    DatabaseManager::instance()->executeQuery(R"(
        CREATE TABLE IF NOT EXISTS watchlist (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT UNIQUE NOT NULL,
            name TEXT,
            sort_order INTEGER DEFAULT 0,
            group_name TEXT DEFAULT '默认',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    // 新闻表
    DatabaseManager::instance()->executeQuery(R"(
        CREATE TABLE IF NOT EXISTS news (
            id TEXT PRIMARY KEY,
            title TEXT NOT NULL,
            content TEXT,
            source TEXT,
            category TEXT,
            url TEXT,
            importance INTEGER DEFAULT 0,
            publish_time TIMESTAMP,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    DatabaseManager::instance()->executeQuery(
        "CREATE INDEX IF NOT EXISTS idx_news_publish_time ON news(publish_time)");
    
    // 行情缓存表
    DatabaseManager::instance()->executeQuery(R"(
        CREATE TABLE IF NOT EXISTS quote_cache (
            symbol TEXT PRIMARY KEY,
            name TEXT,
            last_price REAL,
            change_percent REAL,
            change_amount REAL,
            volume REAL,
            amount REAL,
            update_time TIMESTAMP,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    // 数据更新时间记录表
    DatabaseManager::instance()->executeQuery(R"(
        CREATE TABLE IF NOT EXISTS data_update_log (
            data_type TEXT PRIMARY KEY,
            last_update_time TIMESTAMP,
            record_count INTEGER DEFAULT 0
        )
    )");
    
    LOG_DEBUG("Data storage tables created");
}

// ============================================================================
// 指数数据
// ============================================================================

bool DataStorageService::saveIndexData(const QString& code, const QString& name,
                                        double closePrice, double changePercent,
                                        double volume, double amount, const QDate& date)
{
    QString sql = R"(
        INSERT OR REPLACE INTO index_history 
        (code, name, close_price, change_percent, volume, amount, date)
        VALUES (:code, :name, :close_price, :change_percent, :volume, :amount, :date)
    )";
    
    QMap<QString, QVariant> params;
    params[":code"] = code;
    params[":name"] = name;
    params[":close_price"] = closePrice;
    params[":change_percent"] = changePercent;
    params[":volume"] = volume;
    params[":amount"] = amount;
    params[":date"] = date.toString("yyyy-MM-dd");
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    if (!result.success) {
        LOG_ERROR(QString("Failed to save index data: %1").arg(result.error.text()));
        return false;
    }
    
    return true;
}

bool DataStorageService::saveIndexDataBatch(const QVector<IndexHistoryData>& data)
{
    if (data.isEmpty()) {
        return true;
    }
    
    QString sql = R"(
        INSERT OR REPLACE INTO index_history 
        (code, name, close_price, change_percent, volume, amount, date)
        VALUES (:code, :name, :close_price, :change_percent, :volume, :amount, :date)
    )";
    
    QVector<QMap<QString, QVariant>> batchData;
    for (const auto& item : data) {
        QMap<QString, QVariant> params;
        params[":code"] = item.code;
        params[":name"] = item.name;
        params[":close_price"] = item.closePrice;
        params[":change_percent"] = item.changePercent;
        params[":volume"] = item.volume;
        params[":amount"] = item.amount;
        params[":date"] = item.date.toString("yyyy-MM-dd");
        batchData.append(params);
    }
    
    QueryResult result = DatabaseManager::instance()->executeBatch(sql, batchData);
    if (!result.success) {
        LOG_ERROR(QString("Failed to save index data batch: %1").arg(result.error.text()));
        return false;
    }
    
    LOG_INFO(QString("Saved %1 index records").arg(data.size()));
    return true;
}

QVector<IndexHistoryData> DataStorageService::getLatestIndexData()
{
    QString sql = R"(
        SELECT code, name, close_price, change_percent, volume, amount, date
        FROM index_history
        WHERE date = (SELECT MAX(date) FROM index_history)
    )";
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql);
    QVector<IndexHistoryData> data;
    
    for (const auto& row : result.rows) {
        IndexHistoryData item;
        item.code = row["code"].toString();
        item.name = row["name"].toString();
        item.closePrice = row["close_price"].toDouble();
        item.changePercent = row["change_percent"].toDouble();
        item.volume = row["volume"].toDouble();
        item.amount = row["amount"].toDouble();
        item.date = QDate::fromString(row["date"].toString(), "yyyy-MM-dd");
        data.append(item);
    }
    
    return data;
}

QVector<IndexHistoryData> DataStorageService::getIndexDataByDate(const QDate& date)
{
    QString sql = R"(
        SELECT code, name, close_price, change_percent, volume, amount, date
        FROM index_history
        WHERE date = :date
    )";
    
    QMap<QString, QVariant> params;
    params[":date"] = date.toString("yyyy-MM-dd");
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    QVector<IndexHistoryData> data;
    
    for (const auto& row : result.rows) {
        IndexHistoryData item;
        item.code = row["code"].toString();
        item.name = row["name"].toString();
        item.closePrice = row["close_price"].toDouble();
        item.changePercent = row["change_percent"].toDouble();
        item.volume = row["volume"].toDouble();
        item.amount = row["amount"].toDouble();
        item.date = QDate::fromString(row["date"].toString(), "yyyy-MM-dd");
        data.append(item);
    }
    
    return data;
}

QVector<IndexHistoryData> DataStorageService::getIndexHistory(const QString& code, int days)
{
    QString sql = R"(
        SELECT code, name, close_price, change_percent, volume, amount, date
        FROM index_history
        WHERE code = :code
        ORDER BY date DESC
        LIMIT :limit
    )";
    
    QMap<QString, QVariant> params;
    params[":code"] = code;
    params[":limit"] = days;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    QVector<IndexHistoryData> data;
    
    for (const auto& row : result.rows) {
        IndexHistoryData item;
        item.code = row["code"].toString();
        item.name = row["name"].toString();
        item.closePrice = row["close_price"].toDouble();
        item.changePercent = row["change_percent"].toDouble();
        item.volume = row["volume"].toDouble();
        item.amount = row["amount"].toDouble();
        item.date = QDate::fromString(row["date"].toString(), "yyyy-MM-dd");
        data.append(item);
    }
    
    return data;
}

// ============================================================================
// 自选股数据
// ============================================================================

bool DataStorageService::addWatchlistItem(const QString& symbol, const QString& name,
                                           const QString& groupName)
{
    // 检查是否已存在
    if (isInWatchlist(symbol)) {
        LOG_DEBUG(QString("Symbol already in watchlist: %1").arg(symbol));
        return true;
    }
    
    // 获取最大排序值
    QString maxOrderSql = "SELECT COALESCE(MAX(sort_order), 0) as max_order FROM watchlist";
    QueryResult maxResult = DatabaseManager::instance()->executeQuery(maxOrderSql);
    int sortOrder = 0;
    if (!maxResult.rows.isEmpty()) {
        sortOrder = maxResult.rows[0]["max_order"].toInt() + 1;
    }
    
    QString sql = R"(
        INSERT INTO watchlist (symbol, name, sort_order, group_name)
        VALUES (:symbol, :name, :sort_order, :group_name)
    )";
    
    QMap<QString, QVariant> params;
    params[":symbol"] = symbol;
    params[":name"] = name;
    params[":sort_order"] = sortOrder;
    params[":group_name"] = groupName.isEmpty() ? "默认" : groupName;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    if (!result.success) {
        LOG_ERROR(QString("Failed to add watchlist item: %1").arg(result.error.text()));
        return false;
    }
    
    emit watchlistChanged();
    LOG_INFO(QString("Added to watchlist: %1").arg(symbol));
    return true;
}

bool DataStorageService::removeWatchlistItem(const QString& symbol)
{
    QString sql = "DELETE FROM watchlist WHERE symbol = :symbol";
    
    QMap<QString, QVariant> params;
    params[":symbol"] = symbol;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    if (!result.success) {
        LOG_ERROR(QString("Failed to remove watchlist item: %1").arg(result.error.text()));
        return false;
    }
    
    emit watchlistChanged();
    LOG_INFO(QString("Removed from watchlist: %1").arg(symbol));
    return true;
}

bool DataStorageService::updateWatchlistItem(const WatchlistItem& item)
{
    QString sql = R"(
        UPDATE watchlist 
        SET name = :name, sort_order = :sort_order, group_name = :group_name,
            updated_at = CURRENT_TIMESTAMP
        WHERE symbol = :symbol
    )";
    
    QMap<QString, QVariant> params;
    params[":symbol"] = item.symbol;
    params[":name"] = item.name;
    params[":sort_order"] = item.sort_order;
    params[":group_name"] = item.group_name;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    if (!result.success) {
        LOG_ERROR(QString("Failed to update watchlist item: %1").arg(result.error.text()));
        return false;
    }
    
    emit watchlistChanged();
    return true;
}

QVector<WatchlistItem> DataStorageService::getAllWatchlistItems()
{
    QString sql = R"(
        SELECT symbol, name, sort_order, group_name, created_at, updated_at
        FROM watchlist
        ORDER BY sort_order ASC
    )";
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql);
    QVector<WatchlistItem> items;
    
    for (const auto& row : result.rows) {
        WatchlistItem item;
        item.symbol = row["symbol"].toString();
        item.name = row["name"].toString();
        item.sort_order = row["sort_order"].toInt();
        item.group_name = row["group_name"].toString();
        item.created_at = row["created_at"].toDateTime();
        item.updated_at = row["updated_at"].toDateTime();
        items.append(item);
    }
    
    return items;
}

bool DataStorageService::isInWatchlist(const QString& symbol)
{
    QString sql = "SELECT COUNT(*) as count FROM watchlist WHERE symbol = :symbol";
    
    QMap<QString, QVariant> params;
    params[":symbol"] = symbol;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    if (!result.rows.isEmpty()) {
        return result.rows[0]["count"].toInt() > 0;
    }
    return false;
}

QStringList DataStorageService::getWatchlistSymbols()
{
    QString sql = "SELECT symbol FROM watchlist ORDER BY sort_order ASC";
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql);
    QStringList symbols;
    
    for (const auto& row : result.rows) {
        symbols.append(row["symbol"].toString());
    }
    
    return symbols;
}

bool DataStorageService::updateWatchlistOrder(const QString& symbol, int order)
{
    QString sql = "UPDATE watchlist SET sort_order = :order WHERE symbol = :symbol";
    
    QMap<QString, QVariant> params;
    params[":symbol"] = symbol;
    params[":order"] = order;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    return result.success;
}

// ============================================================================
// 新闻数据
// ============================================================================

bool DataStorageService::saveNews(const NewsItem& news)
{
    QString sql = R"(
        INSERT OR REPLACE INTO news 
        (id, title, content, source, category, url, importance, publish_time)
        VALUES (:id, :title, :content, :source, :category, :url, :importance, :publish_time)
    )";
    
    QMap<QString, QVariant> params;
    params[":id"] = news.id;
    params[":title"] = news.title;
    params[":content"] = news.content;
    params[":source"] = news.source;
    params[":category"] = news.categories.isEmpty() ? "" : news.categories.first();
    params[":url"] = news.url;
    params[":importance"] = 0;  // NewsItem 没有 importance 字段
    params[":publish_time"] = news.publishTime;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    return result.success;
}

bool DataStorageService::saveNewsBatch(const QVector<NewsItem>& newsList)
{
    if (newsList.isEmpty()) {
        return true;
    }
    
    QString sql = R"(
        INSERT OR REPLACE INTO news 
        (id, title, content, source, category, url, importance, publish_time)
        VALUES (:id, :title, :content, :source, :category, :url, :importance, :publish_time)
    )";
    
    QVector<QMap<QString, QVariant>> batchData;
    for (const auto& news : newsList) {
        QMap<QString, QVariant> params;
        params[":id"] = news.id;
        params[":title"] = news.title;
        params[":content"] = news.content;
        params[":source"] = news.source;
        params[":category"] = news.categories.isEmpty() ? "" : news.categories.first();
        params[":url"] = news.url;
        params[":importance"] = 0;
        params[":publish_time"] = news.publishTime;
        batchData.append(params);
    }
    
    QueryResult result = DatabaseManager::instance()->executeBatch(sql, batchData);
    if (result.success) {
        LOG_INFO(QString("Saved %1 news items").arg(newsList.size()));
    }
    return result.success;
}

QVector<NewsItem> DataStorageService::getLatestNews(int count)
{
    QString sql = R"(
        SELECT id, title, content, source, category, url, importance, publish_time, created_at
        FROM news
        ORDER BY publish_time DESC
        LIMIT :limit
    )";
    
    QMap<QString, QVariant> params;
    params[":limit"] = count;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    QVector<NewsItem> items;
    
    for (const auto& row : result.rows) {
        NewsItem item;
        item.id = row["id"].toString();
        item.title = row["title"].toString();
        item.content = row["content"].toString();
        item.source = row["source"].toString();
        QString category = row["category"].toString();
        if (!category.isEmpty()) {
            item.categories.append(category);
        }
        item.url = row["url"].toString();
        item.publishTime = row["publish_time"].toDateTime();
        items.append(item);
    }
    
    return items;
}

bool DataStorageService::newsExists(const QString& id)
{
    QString sql = "SELECT COUNT(*) as count FROM news WHERE id = :id";
    
    QMap<QString, QVariant> params;
    params[":id"] = id;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    if (!result.rows.isEmpty()) {
        return result.rows[0]["count"].toInt() > 0;
    }
    return false;
}

void DataStorageService::cleanOldNews(int daysToKeep)
{
    QString sql = R"(
        DELETE FROM news 
        WHERE created_at < datetime('now', :days || ' days')
    )";
    
    QMap<QString, QVariant> params;
    params[":days"] = QString("-%1").arg(daysToKeep);
    
    DatabaseManager::instance()->executeQuery(sql, params);
    LOG_DEBUG(QString("Cleaned news older than %1 days").arg(daysToKeep));
}

// ============================================================================
// 行情缓存
// ============================================================================

bool DataStorageService::saveQuoteCache(const QString& symbol, const CachedQuoteData& data)
{
    QString sql = R"(
        INSERT OR REPLACE INTO quote_cache 
        (symbol, name, last_price, change_percent, change_amount, volume, amount, update_time)
        VALUES (:symbol, :name, :last_price, :change_percent, :change_amount, :volume, :amount, :update_time)
    )";
    
    QMap<QString, QVariant> params;
    params[":symbol"] = symbol;
    params[":name"] = data.name;
    params[":last_price"] = data.lastPrice;
    params[":change_percent"] = data.changePercent;
    params[":change_amount"] = data.changeAmount;
    params[":volume"] = data.volume;
    params[":amount"] = data.amount;
    params[":update_time"] = data.update_time;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    return result.success;
}

bool DataStorageService::saveQuoteCacheBatch(const QVector<CachedQuoteData>& dataList)
{
    if (dataList.isEmpty()) {
        return true;
    }
    
    QString sql = R"(
        INSERT OR REPLACE INTO quote_cache 
        (symbol, name, last_price, change_percent, change_amount, volume, amount, update_time)
        VALUES (:symbol, :name, :last_price, :change_percent, :change_amount, :volume, :amount, :update_time)
    )";
    
    QVector<QMap<QString, QVariant>> batchData;
    for (const auto& data : dataList) {
        QMap<QString, QVariant> params;
        params[":symbol"] = data.symbol;
        params[":name"] = data.name;
        params[":last_price"] = data.lastPrice;
        params[":change_percent"] = data.changePercent;
        params[":change_amount"] = data.changeAmount;
        params[":volume"] = data.volume;
        params[":amount"] = data.amount;
        params[":update_time"] = data.update_time;
        batchData.append(params);
    }
    
    QueryResult result = DatabaseManager::instance()->executeBatch(sql, batchData);
    return result.success;
}

CachedQuoteData DataStorageService::getQuoteCache(const QString& symbol)
{
    QString sql = R"(
        SELECT symbol, name, last_price, change_percent, change_amount, volume, amount, update_time
        FROM quote_cache
        WHERE symbol = :symbol
    )";
    
    QMap<QString, QVariant> params;
    params[":symbol"] = symbol;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    CachedQuoteData data;
    
    if (!result.rows.isEmpty()) {
        const auto& row = result.rows[0];
        data.symbol = row["symbol"].toString();
        data.name = row["name"].toString();
        data.lastPrice = row["last_price"].toDouble();
        data.changePercent = row["change_percent"].toDouble();
        data.changeAmount = row["change_amount"].toDouble();
        data.volume = row["volume"].toDouble();
        data.amount = row["amount"].toDouble();
        data.update_time = row["update_time"].toDateTime();
    }
    
    return data;
}

QVector<CachedQuoteData> DataStorageService::getAllQuoteCache()
{
    QString sql = R"(
        SELECT symbol, name, last_price, change_percent, change_amount, volume, amount, update_time
        FROM quote_cache
    )";
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql);
    QVector<CachedQuoteData> dataList;
    
    for (const auto& row : result.rows) {
        CachedQuoteData data;
        data.symbol = row["symbol"].toString();
        data.name = row["name"].toString();
        data.lastPrice = row["last_price"].toDouble();
        data.changePercent = row["change_percent"].toDouble();
        data.changeAmount = row["change_amount"].toDouble();
        data.volume = row["volume"].toDouble();
        data.amount = row["amount"].toDouble();
        data.update_time = row["update_time"].toDateTime();
        dataList.append(data);
    }
    
    return dataList;
}

void DataStorageService::clearQuoteCache()
{
    DatabaseManager::instance()->executeQuery("DELETE FROM quote_cache");
    LOG_DEBUG("Quote cache cleared");
}

// ============================================================================
// 通用方法
// ============================================================================

bool DataStorageService::hasLocalData()
{
    // 检查是否有指数数据
    QueryResult result = DatabaseManager::instance()->executeQuery(
        "SELECT COUNT(*) as count FROM index_history");
    if (!result.rows.isEmpty() && result.rows[0]["count"].toInt() > 0) {
        return true;
    }
    
    // 检查是否有行情缓存
    result = DatabaseManager::instance()->executeQuery(
        "SELECT COUNT(*) as count FROM quote_cache");
    if (!result.rows.isEmpty() && result.rows[0]["count"].toInt() > 0) {
        return true;
    }
    
    return false;
}

QDateTime DataStorageService::getLastUpdateTime(const QString& dataType)
{
    QString sql = "SELECT last_update_time FROM data_update_log WHERE data_type = :type";
    
    QMap<QString, QVariant> params;
    params[":type"] = dataType;
    
    QueryResult result = DatabaseManager::instance()->executeQuery(sql, params);
    if (!result.rows.isEmpty()) {
        return result.rows[0]["last_update_time"].toDateTime();
    }
    return QDateTime();
}

void DataStorageService::setLastUpdateTime(const QString& dataType, const QDateTime& time)
{
    QString sql = R"(
        INSERT OR REPLACE INTO data_update_log (data_type, last_update_time)
        VALUES (:type, :time)
    )";
    
    QMap<QString, QVariant> params;
    params[":type"] = dataType;
    params[":time"] = time;
    
    DatabaseManager::instance()->executeQuery(sql, params);
}
