/**
 * @file DatabaseManager.cpp
 * @brief Database Manager Implementation
 */

#include "DatabaseManager.h"
#include "shared/utils/Logger.h"
#include <QSqlRecord>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QUuid>

// ========== ConnectionPool Implementation ==========

ConnectionPool::ConnectionPool(const DatabaseConfig& config)
    : m_config(config)
    , m_connectionCounter(0)
{
    LOG_DEBUG("ConnectionPool created");
}

ConnectionPool::~ConnectionPool()
{
    cleanup();
    LOG_DEBUG("ConnectionPool destroyed");
}

void ConnectionPool::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    // Create minimum connections
    for (int i = 0; i < m_config.minConnections; ++i) {
        QSqlDatabase db = createConnection();
        if (db.isValid()) {
            m_availableConnections.enqueue(db);
        }
    }
}

void ConnectionPool::cleanup()
{
    QMutexLocker locker(&m_mutex);
    
    // Close all connections
    while (!m_availableConnections.isEmpty()) {
        QSqlDatabase db = m_availableConnections.dequeue();
        QString connectionName = db.connectionName();
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
    }
    
    for (auto& db : m_usedConnections) {
        QString connectionName = db.connectionName();
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
    }
    m_usedConnections.clear();
    
    LOG_DEBUG("ConnectionPool cleaned up");
}

QSqlDatabase ConnectionPool::getConnection()
{
    QMutexLocker locker(&m_mutex);
    
    // Wait for available connection
    while (m_availableConnections.isEmpty() && m_usedConnections.size() >= m_config.maxConnections) {
        m_condition.wait(&m_mutex, m_config.connectionTimeout);
        if (m_availableConnections.isEmpty() && m_usedConnections.size() >= m_config.maxConnections) {
            LOG_ERROR("Connection pool exhausted");
            return QSqlDatabase();
        }
    }
    
    // Get or create connection
    QSqlDatabase db;
    if (!m_availableConnections.isEmpty()) {
        db = m_availableConnections.dequeue();
    } else {
        db = createConnection();
    }
    
    if (db.isValid()) {
        m_usedConnections[db.connectionName()] = db;
    }
    
    return db;
}

void ConnectionPool::returnConnection(QSqlDatabase connection)
{
    if (!connection.isValid()) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QString connectionName = connection.connectionName();
    m_usedConnections.remove(connectionName);
    
    if (m_availableConnections.size() < m_config.maxConnections) {
        m_availableConnections.enqueue(connection);
    } else {
        connection.close();
        QSqlDatabase::removeDatabase(connectionName);
    }
    
    m_condition.wakeOne();
}

QSqlDatabase ConnectionPool::createConnection()
{
    QString connectionName = QString("db_%1").arg(++m_connectionCounter);
    
    QSqlDatabase db;
    if (m_config.databaseName.endsWith(".db") || m_config.databaseName.contains(".")) {
        // SQLite database
        QString dbPath;
        
        // 检查是否为绝对路径（Windows: C:/ 或 D:/ 等，或 Unix: /）
        if (m_config.databaseName.contains(":/") || m_config.databaseName.startsWith("/")) {
            // 绝对路径，直接使用
            dbPath = m_config.databaseName;
        } else {
            // 相对路径，添加应用目录前缀
            dbPath = QCoreApplication::applicationDirPath() + "/data/" + m_config.databaseName;
        }
        
        // 确保目录存在
        QDir dir = QFileInfo(dbPath).absoluteDir();
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbPath);
    } else {
        // Other database (MySQL, PostgreSQL, etc.)
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setHostName(m_config.hostName);
        db.setPort(m_config.port);
        db.setUserName(m_config.userName);
        db.setPassword(m_config.password);
        db.setDatabaseName(m_config.databaseName);
    }
    
    if (!db.open()) {
        LOG_ERROR(QString("Failed to open database: %1").arg(db.lastError().text()));
        return QSqlDatabase();
    }
    
    // Apply optimizations
    if (m_config.enableWAL) {
        QSqlQuery query(db);
        query.exec("PRAGMA journal_mode=WAL");
    }
    
    QSqlQuery query(db);
    query.exec(QString("PRAGMA cache_size=%1").arg(m_config.cacheSize));
    query.exec(QString("PRAGMA page_size=%1").arg(m_config.pageSize));
    
    if (m_config.enableForeignKeys) {
        query.exec("PRAGMA foreign_keys=ON");
    }
    
    return db;
}

int ConnectionPool::availableConnections() const
{
    QMutexLocker locker(&m_mutex);
    return m_availableConnections.size();
}

int ConnectionPool::totalConnections() const
{
    QMutexLocker locker(&m_mutex);
    return m_availableConnections.size() + m_usedConnections.size();
}

// ========== AsyncQueryThread Implementation ==========

AsyncQueryThread::AsyncQueryThread(ConnectionPool* pool, QObject* parent)
    : QThread(parent)
    , m_pool(pool)
    , m_running(true)
{
    LOG_DEBUG("AsyncQueryThread created");
}

void AsyncQueryThread::executeQuery(const QString& queryId, const QString& sql, const QMap<QString, QVariant>& params)
{
    QMutexLocker locker(&m_mutex);
    
    QueryTask task;
    task.queryId = queryId;
    task.sql = sql;
    task.params = params;
    task.isBatch = false;
    
    m_taskQueue.enqueue(task);
    m_condition.wakeOne();
}

void AsyncQueryThread::executeBatch(const QString& queryId, const QString& sql, const QVector<QMap<QString, QVariant>>& batchData)
{
    QMutexLocker locker(&m_mutex);
    
    QueryTask task;
    task.queryId = queryId;
    task.sql = sql;
    task.batchData = batchData;
    task.isBatch = true;
    
    m_taskQueue.enqueue(task);
    m_condition.wakeOne();
}

void AsyncQueryThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    m_condition.wakeAll();
}

void AsyncQueryThread::run()
{
    while (m_running) {
        QueryTask task;
        
        {
            QMutexLocker locker(&m_mutex);
            while (m_taskQueue.isEmpty() && m_running) {
                m_condition.wait(&m_mutex, 1000);
            }
            
            if (!m_running && m_taskQueue.isEmpty()) {
                break;
            }
            
            if (!m_taskQueue.isEmpty()) {
                task = m_taskQueue.dequeue();
            }
        }
        
        if (task.queryId.isEmpty()) {
            continue;
        }
        
        // Execute query
        QSqlDatabase db = m_pool->getConnection();
        QueryResult result;
        
        QElapsedTimer timer;
        timer.start();
        
        if (task.isBatch) {
            // Batch operation
            QSqlQuery query(db);
            query.prepare(task.sql);
            
            for (const auto& params : task.batchData) {
                for (auto it = params.begin(); it != params.end(); ++it) {
                    query.bindValue(it.key(), it.value());
                }
                if (query.exec()) {
                    ++result.affectedRows;
                }
            }
            result.success = true;
        } else {
            // Single query
            QSqlQuery query(db);
            query.prepare(task.sql);
            
            for (auto it = task.params.begin(); it != task.params.end(); ++it) {
                query.bindValue(it.key(), it.value());
            }
            
            result.success = query.exec();
            result.error = query.lastError();
            
            if (result.success) {
                while (query.next()) {
                    QMap<QString, QVariant> row;
                    QSqlRecord record = query.record();
                    for (int i = 0; i < record.count(); ++i) {
                        row[record.fieldName(i)] = record.value(i);
                    }
                    result.rows.append(row);
                }
                result.affectedRows = query.numRowsAffected();
            }
        }
        
        result.executionTime = timer.elapsed() * 1000; // Convert to microseconds
        
        m_pool->returnConnection(db);
        
        emit queryCompleted(task.queryId, result);
    }
}

// ========== DatabaseManager Implementation ==========

DatabaseManager::DatabaseManager()
    : m_totalQueries(0)
    , m_totalTime(0)
    , m_failedQueries(0)
{
    LOG_DEBUG("DatabaseManager created");
}

DatabaseManager::~DatabaseManager()
{
    // 如果没有调用 shutdown，在这里清理
    if (m_asyncThread || m_connectionPool) {
        // 停止异步线程
        if (m_asyncThread) {
            m_asyncThread->stop();
            m_asyncThread->quit();
            m_asyncThread->wait();
            m_asyncThread.reset();
        }
        
        // 清理连接池
        if (m_connectionPool) {
            m_connectionPool->cleanup();
            m_connectionPool.reset();
        }
    }
    
    LOG_DEBUG("DatabaseManager destroyed");
}

bool DatabaseManager::initialize(const DatabaseConfig& config)
{
    QMutexLocker locker(&m_mutex);
    
    // Create connection pool
    m_connectionPool = std::make_unique<ConnectionPool>(config);
    m_connectionPool->initialize();
    
    // Create async query thread
    m_asyncThread = std::make_unique<AsyncQueryThread>(m_connectionPool.get());
    connect(m_asyncThread.get(), &AsyncQueryThread::queryCompleted,
            this, &DatabaseManager::asyncQueryCompleted);
    m_asyncThread->start();
    
    // Create tables
    createTables();

    return true;
}

QueryResult DatabaseManager::executeQuery(const QString& sql, const QMap<QString, QVariant>& params)
{
    QElapsedTimer timer;
    timer.start();
    
    QSqlDatabase db = m_connectionPool->getConnection();
    QueryResult result;
    
    QSqlQuery query(db);
    query.prepare(sql);
    
    for (auto it = params.begin(); it != params.end(); ++it) {
        query.bindValue(it.key(), it.value());
    }
    
    result.success = query.exec();
    result.error = query.lastError();
    
    if (result.success) {
        while (query.next()) {
            QMap<QString, QVariant> row;
            QSqlRecord record = query.record();
            for (int i = 0; i < record.count(); ++i) {
                row[record.fieldName(i)] = record.value(i);
            }
            result.rows.append(row);
        }
        result.affectedRows = query.numRowsAffected();
    } else {
        LOG_ERROR(QString("Query failed: %1 - %2").arg(sql, result.error.text()));
        ++m_failedQueries;
    }
    
    m_connectionPool->returnConnection(db);
    
    result.executionTime = timer.elapsed() * 1000;
    ++m_totalQueries;
    m_totalTime += result.executionTime;
    
    return result;
}

QString DatabaseManager::executeQueryAsync(const QString& sql, const QMap<QString, QVariant>& params)
{
    QString queryId = QUuid::createUuid().toString();
    m_asyncThread->executeQuery(queryId, sql, params);
    return queryId;
}

QueryResult DatabaseManager::executeBatch(const QString& sql, const QVector<QMap<QString, QVariant>>& batchData)
{
    QElapsedTimer timer;
    timer.start();
    
    QSqlDatabase db = m_connectionPool->getConnection();
    QueryResult result;
    
    db.transaction();
    
    QSqlQuery query(db);
    query.prepare(sql);
    
    int successCount = 0;
    for (const auto& params : batchData) {
        for (auto it = params.begin(); it != params.end(); ++it) {
            query.bindValue(it.key(), it.value());
        }
        if (query.exec()) {
            ++successCount;
        }
    }
    
    if (db.commit()) {
        result.success = true;
        result.affectedRows = successCount;
    } else {
        db.rollback();
        result.success = false;
        result.error = db.lastError();
        LOG_ERROR(QString("Batch failed: %1").arg(result.error.text()));
    }
    
    m_connectionPool->returnConnection(db);
    
    result.executionTime = timer.elapsed() * 1000;
    ++m_totalQueries;
    m_totalTime += result.executionTime;
    
    return result;
}

bool DatabaseManager::executeTransaction(const std::function<bool()>& operations)
{
    QSqlDatabase db = m_connectionPool->getConnection();
    
    if (!db.transaction()) {
        LOG_ERROR("Failed to start transaction");
        m_connectionPool->returnConnection(db);
        return false;
    }
    
    bool success = operations();
    
    if (success) {
        success = db.commit();
        if (!success) {
            LOG_ERROR(QString("Commit failed: %1").arg(db.lastError().text()));
        }
    } else {
        db.rollback();
        LOG_DEBUG("Transaction rolled back");
    }
    
    m_connectionPool->returnConnection(db);
    return success;
}

QueryResult DatabaseManager::executePreparedStatement(const QString& statementId, const QMap<QString, QVariant>& params)
{
    auto it = m_preparedStatements.find(statementId);
    if (it == m_preparedStatements.end()) {
        QueryResult result;
        result.success = false;
        LOG_ERROR(QString("Prepared statement not found: %1").arg(statementId));
        return result;
    }
    
    return executeQuery(it.value(), params);
}

void DatabaseManager::registerPreparedStatement(const QString& statementId, const QString& sql)
{
    m_preparedStatements[statementId] = sql;
    LOG_DEBUG(QString("Prepared statement registered: %1").arg(statementId));
}

QVector<QMap<QString, QVariant>> DatabaseManager::getTableInfo(const QString& tableName)
{
    QueryResult result = executeQuery(QString("PRAGMA table_info(%1)").arg(tableName));
    return result.rows;
}

void DatabaseManager::optimize()
{
    LOG_DEBUG("Optimizing database...");
    
    QSqlDatabase db = m_connectionPool->getConnection();
    
    QSqlQuery query(db);
    query.exec("PRAGMA optimize");
    query.exec("VACUUM");
    query.exec("ANALYZE");
    
    m_connectionPool->returnConnection(db);
    
    LOG_INFO("Database optimized");
}

QMap<QString, QVariant> DatabaseManager::statistics() const
{
    QMap<QString, QVariant> stats;
    stats["totalQueries"] = m_totalQueries;
    stats["totalTime"] = m_totalTime;
    stats["failedQueries"] = m_failedQueries;
    stats["avgQueryTime"] = m_totalQueries > 0 ? m_totalTime / m_totalQueries : 0;
    stats["availableConnections"] = m_connectionPool ? m_connectionPool->availableConnections() : 0;
    stats["totalConnections"] = m_connectionPool ? m_connectionPool->totalConnections() : 0;
    return stats;
}

bool DatabaseManager::backup(const QString& backupPath)
{
    // Close all connections temporarily
    m_connectionPool->cleanup();
    
    // Get database file path
    QString dbPath = QCoreApplication::applicationDirPath() + "/data/wealthpilot.db";
    
    // Copy database file
    bool success = QFile::copy(dbPath, backupPath);
    
    // Reinitialize connections
    m_connectionPool->initialize();
    
    if (success) {
        LOG_INFO(QString("Database backed up to: %1").arg(backupPath));
    } else {
        LOG_ERROR("Database backup failed");
    }
    
    return success;
}

bool DatabaseManager::restore(const QString& backupPath)
{
    if (!QFile::exists(backupPath)) {
        LOG_ERROR("Backup file not found");
        return false;
    }
    
    // Close all connections
    m_connectionPool->cleanup();
    
    // Get database file path
    QString dbPath = QCoreApplication::applicationDirPath() + "/data/wealthpilot.db";
    
    // Delete original file
    QFile::remove(dbPath);
    
    // Copy backup file
    if (QFile::copy(backupPath, dbPath)) {
        // Reinitialize connection pool
        m_connectionPool->initialize();
        LOG_INFO("Database restore completed");
        return true;
    }
    
    LOG_ERROR("Database restore failed");
    return false;
}

void DatabaseManager::createTables()
{
    LOG_DEBUG("Creating database tables...");
    
    // User config table
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS user_config (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            key TEXT UNIQUE NOT NULL,
            value TEXT,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    // Favorites table
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS favorites (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT UNIQUE NOT NULL,
            exchange TEXT,
            name TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    // Trade history table
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS trade_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT NOT NULL,
            exchange TEXT,
            direction TEXT,
            price REAL,
            quantity REAL,
            amount REAL,
            commission REAL,
            trade_time TIMESTAMP,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    // Position table
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS positions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT UNIQUE NOT NULL,
            exchange TEXT,
            direction TEXT,
            quantity REAL,
            avg_price REAL,
            current_price REAL,
            profit_loss REAL,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    // Cache table
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS cache (
            key TEXT PRIMARY KEY,
            value TEXT,
            expire_time TIMESTAMP,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    LOG_DEBUG("Database tables created");
}

void DatabaseManager::applyOptimizations()
{
    QSqlDatabase db = m_connectionPool->getConnection();
    
    QSqlQuery query(db);
    query.exec("PRAGMA synchronous=NORMAL");
    query.exec("PRAGMA temp_store=MEMORY");
    query.exec("PRAGMA mmap_size=268435456");  // 256MB
    
    m_connectionPool->returnConnection(db);
    
    LOG_DEBUG("Database optimizations applied");
}

void DatabaseManager::shutdown()
{
    QMutexLocker locker(&m_mutex);
    
    LOG_INFO("Shutting down DatabaseManager...");
    
    // 停止异步线程
    if (m_asyncThread) {
        m_asyncThread->stop();
        m_asyncThread->quit();
        m_asyncThread->wait();
        m_asyncThread.reset();
    }
    
    // 清理连接池
    if (m_connectionPool) {
        m_connectionPool->cleanup();
        m_connectionPool.reset();
    }
    
    LOG_INFO("DatabaseManager shutdown complete");
}
