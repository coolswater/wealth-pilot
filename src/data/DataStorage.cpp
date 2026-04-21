/**
 * @file DataStorage.cpp
 * @brief 数据存储管理器实现
 */

#include "DataStorage.h"
#include "utils/Logger.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QMutexLocker>
#include <QFile>
#include <QDir>
#include <QDateTime>

struct DataStorage::Impl {
    QSqlDatabase db;
    QString dbPath;
    mutable QMutex mutex;
    bool initialized = false;
};

DataStorage& DataStorage::instance()
{
    static DataStorage instance;
    return instance;
}

DataStorage::DataStorage(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    LOG_DEBUG("DataStorage created");
}

DataStorage::~DataStorage()
{
    shutdown();
    LOG_DEBUG("DataStorage destroyed");
}

bool DataStorage::initialize(const QString &dbPath)
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        return true;
    }

    // 设置数据库路径
    d->dbPath = dbPath.isEmpty() 
        ? QDir::currentPath() + "/data/wealthpilot.db"
        : dbPath;

    // 确保目录存在
    QDir dir = QFileInfo(d->dbPath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 打开数据库
    d->db = QSqlDatabase::addDatabase("QSQLITE", "wealthpilot");
    d->db.setDatabaseName(d->dbPath);

    if (!d->db.open()) {
        LOG_ERROR(QString("Failed to open database: %1").arg(d->db.lastError().text()));
        return false;
    }

    // 创建表
    if (!createTables()) {
        LOG_ERROR("Failed to create tables");
        return false;
    }

    d->initialized = true;
    LOG_INFO(QString("DataStorage initialized: %1").arg(d->dbPath));
    return true;
}

void DataStorage::shutdown()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    if (d->db.isOpen()) {
        d->db.close();
    }

    d->initialized = false;
    LOG_INFO("DataStorage shutdown");
}

bool DataStorage::createTables()
{
    QSqlQuery query(d->db);

    // 配置表
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS config (
            key TEXT PRIMARY KEY,
            value TEXT,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        LOG_ERROR(QString("Failed to create config table: %1").arg(query.lastError().text()));
        return false;
    }

    // 成交记录表
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS trades (
            trade_id TEXT PRIMARY KEY,
            order_id TEXT,
            symbol TEXT,
            price REAL,
            volume INTEGER,
            time DATETIME,
            direction TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )")) {
        LOG_ERROR(QString("Failed to create trades table: %1").arg(query.lastError().text()));
        return false;
    }

    // 创建索引
    query.exec("CREATE INDEX IF NOT EXISTS idx_trades_symbol ON trades(symbol)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_trades_time ON trades(time)");

    return true;
}

QString DataStorage::kLineTableName(const QString &symbol, KLinePeriod period) const
{
    return QString("kline_%1_%2").arg(symbol.toLower().replace(".", "_"))
                                   .arg(static_cast<int>(period));
}

bool DataStorage::saveKLineData(const QString &symbol, KLinePeriod period, const QVector<KLineData> &data)
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized || data.isEmpty()) {
        return false;
    }

    QString tableName = kLineTableName(symbol, period);
    QSqlQuery query(d->db);

    // 创建K线表
    QString createSql = QString(R"(
        CREATE TABLE IF NOT EXISTS %1 (
            time DATETIME PRIMARY KEY,
            open REAL,
            high REAL,
            low REAL,
            close REAL,
            volume INTEGER,
            turnover REAL,
            open_interest REAL
        )
    )").arg(tableName);

    if (!query.exec(createSql)) {
        LOG_ERROR(QString("Failed to create kline table: %1").arg(query.lastError().text()));
        return false;
    }

    // 批量插入
    d->db.transaction();

    QString insertSql = QString("INSERT OR REPLACE INTO %1 (time, open, high, low, close, volume, turnover, open_interest) "
                                "VALUES (?, ?, ?, ?, ?, ?, ?, ?)").arg(tableName);
    query.prepare(insertSql);

    int count = 0;
    for (const auto &kline : data) {
        query.addBindValue(kline.time);
        query.addBindValue(kline.open);
        query.addBindValue(kline.high);
        query.addBindValue(kline.low);
        query.addBindValue(kline.close);
        query.addBindValue(kline.volume);
        query.addBindValue(kline.turnover);
        query.addBindValue(kline.openInterest);

        if (query.exec()) {
            count++;
        }
    }

    d->db.commit();

    LOG_DEBUG(QString("Saved %1 KLines for %2").arg(count).arg(symbol));
    emit dataSaved(symbol, count);
    return true;
}

QVector<KLineData> DataStorage::loadKLineData(const QString &symbol, KLinePeriod period,
                                               const QDateTime &from, const QDateTime &to)
{
    QMutexLocker locker(&d->mutex);

    QVector<KLineData> result;

    if (!d->initialized) {
        return result;
    }

    QString tableName = kLineTableName(symbol, period);
    QSqlQuery query(d->db);

    QString sql = QString("SELECT time, open, high, low, close, volume, turnover, open_interest "
                          "FROM %1 WHERE time >= ? AND time <= ? ORDER BY time ASC").arg(tableName);
    query.prepare(sql);
    query.addBindValue(from);
    query.addBindValue(to);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to load kline data: %1").arg(query.lastError().text()));
        return result;
    }

    while (query.next()) {
        KLineData kline;
        kline.time = query.value(0).toDateTime();
        kline.open = query.value(1).toDouble();
        kline.high = query.value(2).toDouble();
        kline.low = query.value(3).toDouble();
        kline.close = query.value(4).toDouble();
        kline.volume = query.value(5).toLongLong();
        kline.turnover = query.value(6).toDouble();
        kline.openInterest = query.value(7).toDouble();
        result.append(kline);
    }

    return result;
}

bool DataStorage::hasKLineData(const QString &symbol, KLinePeriod period)
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return false;
    }

    QString tableName = kLineTableName(symbol, period);
    QSqlQuery query(d->db);

    return query.exec(QString("SELECT name FROM sqlite_master WHERE type='table' AND name='%1'").arg(tableName))
           && query.next();
}

QDateTime DataStorage::getLastKLineTime(const QString &symbol, KLinePeriod period)
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return QDateTime();
    }

    QString tableName = kLineTableName(symbol, period);
    QSqlQuery query(d->db);

    if (!query.exec(QString("SELECT MAX(time) FROM %1").arg(tableName))) {
        return QDateTime();
    }

    if (query.next()) {
        return query.value(0).toDateTime();
    }

    return QDateTime();
}

void DataStorage::clearKLineData(const QString &symbol, KLinePeriod period)
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    QString tableName = kLineTableName(symbol, period);
    QSqlQuery query(d->db);
    query.exec(QString("DROP TABLE IF EXISTS %1").arg(tableName));

    LOG_INFO(QString("Cleared KLine data for %1").arg(symbol));
}

bool DataStorage::saveTradeRecord(const QString &tradeId, const QString &orderId,
                                   const QString &symbol, double price, int volume,
                                   const QDateTime &time, const QString &direction)
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return false;
    }

    QSqlQuery query(d->db);
    query.prepare("INSERT OR REPLACE INTO trades (trade_id, order_id, symbol, price, volume, time, direction) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(tradeId);
    query.addBindValue(orderId);
    query.addBindValue(symbol);
    query.addBindValue(price);
    query.addBindValue(volume);
    query.addBindValue(time);
    query.addBindValue(direction);

    return query.exec();
}

QVector<QVariantMap> DataStorage::loadTradeRecords(const QDateTime &from, const QDateTime &to)
{
    QMutexLocker locker(&d->mutex);

    QVector<QVariantMap> result;

    if (!d->initialized) {
        return result;
    }

    QSqlQuery query(d->db);
    query.prepare("SELECT * FROM trades WHERE time >= ? AND time <= ? ORDER BY time DESC");
    query.addBindValue(from);
    query.addBindValue(to);

    if (!query.exec()) {
        return result;
    }

    while (query.next()) {
        QVariantMap record;
        QSqlRecord rec = query.record();
        for (int i = 0; i < rec.count(); ++i) {
            record[rec.fieldName(i)] = rec.value(i);
        }
        result.append(record);
    }

    return result;
}

void DataStorage::setValue(const QString &key, const QVariant &value)
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    QSqlQuery query(d->db);
    query.prepare("INSERT OR REPLACE INTO config (key, value, updated_at) VALUES (?, ?, ?)");
    query.addBindValue(key);
    query.addBindValue(value.toString());
    query.addBindValue(QDateTime::currentDateTime());
    query.exec();
}

QVariant DataStorage::value(const QString &key, const QVariant &defaultValue) const
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return defaultValue;
    }

    QSqlQuery query(d->db);
    query.prepare("SELECT value FROM config WHERE key = ?");
    query.addBindValue(key);

    if (query.exec() && query.next()) {
        return query.value(0);
    }

    return defaultValue;
}

void DataStorage::removeValue(const QString &key)
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    QSqlQuery query(d->db);
    query.prepare("DELETE FROM config WHERE key = ?");
    query.addBindValue(key);
    query.exec();
}

void DataStorage::optimize()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    QSqlQuery query(d->db);
    query.exec("VACUUM");
    LOG_INFO("Database optimized");
}

qint64 DataStorage::getDatabaseSize() const
{
    QMutexLocker locker(&d->mutex);

    QFile file(d->dbPath);
    return file.size();
}

bool DataStorage::backup(const QString &backupPath)
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return false;
    }

    // 关闭数据库
    d->db.close();

    // 复制文件
    bool success = QFile::copy(d->dbPath, backupPath);

    // 重新打开
    d->db.open();

    if (success) {
        LOG_INFO(QString("Database backed up to: %1").arg(backupPath));
    }

    return success;
}

bool DataStorage::restore(const QString &backupPath)
{
    QMutexLocker locker(&d->mutex);

    if (!QFile::exists(backupPath)) {
        return false;
    }

    // 关闭数据库
    if (d->db.isOpen()) {
        d->db.close();
    }

    // 删除旧文件
    QFile::remove(d->dbPath);

    // 复制备份文件
    bool success = QFile::copy(backupPath, d->dbPath);

    // 重新打开
    if (success) {
        d->db.open();
        LOG_INFO(QString("Database restored from: %1").arg(backupPath));
    }

    return success;
}
