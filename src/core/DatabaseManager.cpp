/**
 * @file DatabaseManager.cpp
 * @brief 数据库管理器实现 - 高性能SQLite数据库管理
 *
 * @details 实现功能：
 * - 连接池管理：减少连接创建开销
 * - 批量操作优化：提高数据插入效率
 * - 事务支持：保证数据一致性
 * - 异步查询：不阻塞UI线程
 * - 性能监控：跟踪查询性能
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "DatabaseManager.h"
#include "../utils/Logger.h"
#include <QSqlRecord>
#include <QFile>
#include <QDir>
#include <QElapsedTimer>
#include <QCoreApplication>
#include <QUuid>

// ========== ConnectionPool 实现 ==========

/**
 * @brief 构造函数
 * @param config 数据库配置
 */
ConnectionPool::ConnectionPool(const DatabaseConfig& config)
    : m_config(config)
    , m_connectionCounter(0)
{
    LOG_DEBUG("ConnectionPool created");
}

/**
 * @brief 析构函数 - 清理所有连接
 */
ConnectionPool::~ConnectionPool()
{
    cleanup();
    LOG_DEBUG("ConnectionPool destroyed");
}

/**
 * @brief 初始化连接池
 * @details 创建最小数量的连接，预热连接池
 */
void ConnectionPool::initialize()
{
    QMutexLocker locker(&m_mutex);
    
    LOG_INFO(QString("Initializing connection pool: min=%1, max=%2")
        .arg(m_config.minConnections).arg(m_config.maxConnections));
    
    // 创建最小数量的连接
    for (int i = 0; i < m_config.minConnections; ++i) {
        QSqlDatabase conn = createConnection();
        if (conn.isOpen()) {
            m_availableConnections.enqueue(conn);
        }
    }
    
    LOG_INFO(QString("Connection pool initialized: %1 connections created")
        .arg(m_availableConnections.size()));
}

/**
 * @brief 清理连接池
 * @details 关闭并删除所有连接
 */
void ConnectionPool::cleanup()
{
    QMutexLocker locker(&m_mutex);
    
    // 关闭可用连接
    while (!m_availableConnections.isEmpty()) {
        QSqlDatabase conn = m_availableConnections.dequeue();
        QString connName = conn.connectionName();
        conn.close();
        QSqlDatabase::removeDatabase(connName);
    }
    
    // 关闭使用中的连接
    for (auto it = m_usedConnections.begin(); it != m_usedConnections.end(); ++it) {
        it.value().close();
        QSqlDatabase::removeDatabase(it.key());
    }
    m_usedConnections.clear();
    
    LOG_INFO("Connection pool cleaned up");
}

/**
 * @brief 获取连接
 * @return 数据库连接
 * @details 如果没有可用连接且未达到最大数量，创建新连接
 */
QSqlDatabase ConnectionPool::getConnection()
{
    QMutexLocker locker(&m_mutex);
    
    // 如果有可用连接，直接返回
    if (!m_availableConnections.isEmpty()) {
        QSqlDatabase conn = m_availableConnections.dequeue();
        m_usedConnections[conn.connectionName()] = conn;
        return conn;
    }
    
    // 如果未达到最大连接数，创建新连接
    if (m_usedConnections.size() < m_config.maxConnections) {
        QSqlDatabase conn = createConnection();
        if (conn.isOpen()) {
            m_usedConnections[conn.connectionName()] = conn;
            return conn;
        }
    }
    
    // 等待可用连接
    locker.unlock();
    m_condition.wait(&m_mutex, m_config.connectionTimeout);
    locker.relock();
    
    // 再次尝试获取
    if (!m_availableConnections.isEmpty()) {
        QSqlDatabase conn = m_availableConnections.dequeue();
        m_usedConnections[conn.connectionName()] = conn;
        return conn;
    }
    
    // 返回无效连接
    LOG_ERROR("Failed to get database connection: timeout");
    return QSqlDatabase();
}

/**
 * @brief 归还连接
 * @param connection 要归还的连接
 */
void ConnectionPool::returnConnection(QSqlDatabase connection)
{
    QMutexLocker locker(&m_mutex);
    
    QString connName = connection.connectionName();
    
    // 从使用中移除
    m_usedConnections.remove(connName);
    
    // 添加到可用队列
    if (connection.isOpen()) {
        m_availableConnections.enqueue(connection);
    } else {
        // 连接已关闭，删除
        QSqlDatabase::removeDatabase(connName);
    }
    
    // 通知等待的线程
    m_condition.wakeOne();
}

/**
 * @brief 创建新连接
 * @return 新创建的数据库连接
 */
QSqlDatabase ConnectionPool::createConnection()
{
    // 生成唯一连接名
    QString connName = QString("conn_%1").arg(++m_connectionCounter);
    
    // 添加SQLite数据库
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
    
    // 设置数据库路径
    QString dbPath = QCoreApplication::applicationDirPath() + "/" + m_config.databaseName;
    db.setDatabaseName(dbPath);
    
    // 打开数据库
    if (!db.open()) {
        LOG_ERROR(QString("Failed to open database: %1, error: %2")
            .arg(dbPath).arg(db.lastError().text()));
        return db;
    }
    
    // 应用优化设置
    if (m_config.enableWAL) {
        db.exec("PRAGMA journal_mode=WAL");
    }
    
    db.exec(QString("PRAGMA cache_size=%1").arg(m_config.cacheSize));
    db.exec(QString("PRAGMA page_size=%1").arg(m_config.pageSize));
    
    if (m_config.enableForeignKeys) {
        db.exec("PRAGMA foreign_keys=ON");
    }
    
    // 性能优化设置
    db.exec("PRAGMA synchronous=NORMAL");
    db.exec("PRAGMA temp_store=MEMORY");
    db.exec("PRAGMA locking_mode=NORMAL");
    
    LOG_DEBUG(QString("Connection created: %1").arg(connName));
    
    return db;
}

/**
 * @brief 获取可用连接数
 */
int ConnectionPool::availableConnections() const
{
    QMutexLocker locker(&m_mutex);
    return m_availableConnections.size();
}

/**
 * @brief 获取总连接数
 */
int ConnectionPool::totalConnections() const
{
    QMutexLocker locker(&m_mutex);
    return m_availableConnections.size() + m_usedConnections.size();
}

// ========== AsyncQueryThread 实现 ==========

/**
 * @brief 构造函数
 * @param pool 连接池指针
 * @param parent 父对象
 */
AsyncQueryThread::AsyncQueryThread(ConnectionPool* pool, QObject* parent)
    : QThread(parent)
    , m_pool(pool)
    , m_running(true)
{
    LOG_DEBUG("AsyncQueryThread created");
}

/**
 * @brief 执行异步查询
 * @param queryId 查询ID
 * @param sql SQL语句
 * @param params 参数
 */
void AsyncQueryThread::executeQuery(const QString& queryId, 
                                   const QString& sql, 
                                   const QMap<QString, QVariant>& params)
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

/**
 * @brief 执行批量操作
 * @param queryId 查询ID
 * @param sql SQL语句
 * @param batchData 批量数据
 */
void AsyncQueryThread::executeBatch(const QString& queryId,
                                   const QString& sql,
                                   const QVector<QMap<QString, QVariant>>& batchData)
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

/**
 * @brief 线程主循环
 */
void AsyncQueryThread::run()
{
    LOG_DEBUG("AsyncQueryThread started");
    
    while (m_running) {
        QMutexLocker locker(&m_mutex);
        
        // 等待任务
        while (m_taskQueue.isEmpty() && m_running) {
            m_condition.wait(&m_mutex);
        }
        
        if (!m_running) {
            break;
        }
        
        // 获取任务
        QueryTask task = m_taskQueue.dequeue();
        locker.unlock();
        
        // 执行任务
        QElapsedTimer timer;
        timer.start();
        
        QSqlDatabase db = m_pool->getConnection();
        QueryResult result;
        result.success = false;
        
        if (db.isOpen()) {
            QSqlQuery query(db);
            
            // 准备查询
            if (query.prepare(task.sql)) {
                // 绑定参数
                for (auto it = task.params.begin(); it != task.params.end(); ++it) {
                    query.bindValue(":" + it.key(), it.value());
                }
                
                // 执行
                if (task.isBatch) {
                    // 批量操作
                    db.transaction();
                    for (const auto& data : task.batchData) {
                        for (auto it = data.begin(); it != data.end(); ++it) {
                            query.bindValue(":" + it.key(), it.value());
                        }
                        if (!query.exec()) {
                            result.error = query.lastError();
                            break;
                        }
                    }
                    if (result.error.isValid()) {
                        db.rollback();
                    } else {
                        db.commit();
                        result.success = true;
                        result.affectedRows = task.batchData.size();
                    }
                } else {
                    // 单次查询
                    if (query.exec()) {
                        result.success = true;
                        
                        // 获取结果
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
                        result.error = query.lastError();
                    }
                }
            } else {
                result.error = query.lastError();
            }
            
            m_pool->returnConnection(db);
        }
        
        result.executionTime = timer.nsecsElapsed() / 1000;
        
        // 发送结果
        emit queryCompleted(task.queryId, result);
    }
    
    LOG_DEBUG("AsyncQueryThread stopped");
}

// ========== DatabaseManager 实现 ==========

/**
 * @brief 构造函数
 */
DatabaseManager::DatabaseManager()
    : m_totalQueries(0)
    , m_totalTime(0)
    , m_failedQueries(0)
{
    LOG_DEBUG("DatabaseManager created");
}

/**
 * @brief 析构函数
 */
DatabaseManager::~DatabaseManager()
{
    if (m_asyncThread) {
        m_asyncThread->quit();
        m_asyncThread->wait();
    }
    
    if (m_connectionPool) {
        m_connectionPool->cleanup();
    }
    
    LOG_DEBUG("DatabaseManager destroyed");
}

/**
 * @brief 初始化数据库管理器
 * @param config 数据库配置
 * @return 是否成功
 */
bool DatabaseManager::initialize(const DatabaseConfig& config)
{
    QElapsedTimer timer;
    timer.start();
    
    LOG_INFO("Initializing DatabaseManager...");
    
    // 设置默认配置
    DatabaseConfig actualConfig = config;
    if (actualConfig.databaseName.isEmpty()) {
        actualConfig.databaseName = "wealthpilot.db";
    }
    if (actualConfig.maxConnections <= 0) {
        actualConfig.maxConnections = 10;
    }
    if (actualConfig.minConnections <= 0) {
        actualConfig.minConnections = 2;
    }
    if (actualConfig.connectionTimeout <= 0) {
        actualConfig.connectionTimeout = 5000;
    }
    if (actualConfig.cacheSize <= 0) {
        actualConfig.cacheSize = 10240; // 10MB
    }
    if (actualConfig.pageSize <= 0) {
        actualConfig.pageSize = 4096; // 4KB
    }
    
    // 创建连接池
    m_connectionPool = std::make_unique<ConnectionPool>(actualConfig);
    m_connectionPool->initialize();
    
    // 创建异步查询线程
    m_asyncThread = std::make_unique<AsyncQueryThread>(m_connectionPool.get());
    m_asyncThread->start();
    
    // 创建表结构
    createTables();
    
    // 应用优化设置
    applyOptimizations();
    
    LOG_INFO(QString("DatabaseManager initialized in %1ms")
        .arg(timer.elapsed()));
    
    return true;
}

/**
 * @brief 执行查询（同步）
 * @param sql SQL语句
 * @param params 参数
 * @return 查询结果
 */
QueryResult DatabaseManager::executeQuery(const QString& sql, 
                                         const QMap<QString, QVariant>& params)
{
    QElapsedTimer timer;
    timer.start();
    
    QueryResult result;
    result.success = false;
    
    QSqlDatabase db = m_connectionPool->getConnection();
    
    if (!db.isOpen()) {
        result.error = QSqlError("Connection not available", "", QSqlError::ConnectionError);
        m_failedQueries++;
        return result;
    }
    
    QSqlQuery query(db);
    
    // 准备查询
    if (!query.prepare(sql)) {
        result.error = query.lastError();
        m_failedQueries++;
        m_connectionPool->returnConnection(db);
        return result;
    }
    
    // 绑定参数
    for (auto it = params.begin(); it != params.end(); ++it) {
        query.bindValue(":" + it.key(), it.value());
    }
    
    // 执行查询
    if (!query.exec()) {
        result.error = query.lastError();
        m_failedQueries++;
        m_connectionPool->returnConnection(db);
        return result;
    }
    
    // 获取结果
    while (query.next()) {
        QMap<QString, QVariant> row;
        QSqlRecord record = query.record();
        for (int i = 0; i < record.count(); ++i) {
            row[record.fieldName(i)] = record.value(i);
        }
        result.rows.append(row);
    }
    
    result.success = true;
    result.affectedRows = query.numRowsAffected();
    result.executionTime = timer.nsecsElapsed() / 1000;
    
    m_totalQueries++;
    m_totalTime += result.executionTime;
    
    m_connectionPool->returnConnection(db);
    
    return result;
}

/**
 * @brief 执行查询（异步）
 * @param sql SQL语句
 * @param params 参数
 * @return 查询ID
 */
QString DatabaseManager::executeQueryAsync(const QString& sql, 
                                          const QMap<QString, QVariant>& params)
{
    QString queryId = QUuid::createUuid().toString();
    m_asyncThread->executeQuery(queryId, sql, params);
    return queryId;
}

/**
 * @brief 执行批量操作
 * @param sql SQL语句
 * @param batchData 批量数据
 * @return 查询结果
 */
QueryResult DatabaseManager::executeBatch(const QString& sql, 
                                         const QVector<QMap<QString, QVariant>>& batchData)
{
    QElapsedTimer timer;
    timer.start();
    
    QueryResult result;
    result.success = false;
    
    if (batchData.isEmpty()) {
        return result;
    }
    
    QSqlDatabase db = m_connectionPool->getConnection();
    
    if (!db.isOpen()) {
        result.error = QSqlError("Connection not available", "", QSqlError::ConnectionError);
        m_failedQueries++;
        return result;
    }
    
    // 开始事务
    if (!db.transaction()) {
        result.error = db.lastError();
        m_failedQueries++;
        m_connectionPool->returnConnection(db);
        return result;
    }
    
    QSqlQuery query(db);
    
    // 准备查询
    if (!query.prepare(sql)) {
        result.error = query.lastError();
        db.rollback();
        m_failedQueries++;
        m_connectionPool->returnConnection(db);
        return result;
    }
    
    // 批量执行
    int successCount = 0;
    for (const auto& data : batchData) {
        // 绑定参数
        for (auto it = data.begin(); it != data.end(); ++it) {
            query.bindValue(":" + it.key(), it.value());
        }
        
        if (query.exec()) {
            successCount++;
        } else {
            LOG_WARNING(QString("Batch insert failed: %1").arg(query.lastError().text()));
        }
        
        query.finish();
    }
    
    // 提交事务
    if (!db.commit()) {
        result.error = db.lastError();
        db.rollback();
        m_failedQueries++;
        m_connectionPool->returnConnection(db);
        return result;
    }
    
    result.success = true;
    result.affectedRows = successCount;
    result.executionTime = timer.nsecsElapsed() / 1000;
    
    m_totalQueries++;
    m_totalTime += result.executionTime;
    
    m_connectionPool->returnConnection(db);
    
    LOG_DEBUG(QString("Batch operation completed: %1/%2 rows")
        .arg(successCount).arg(batchData.size()));
    
    return result;
}

/**
 * @brief 执行事务
 * @param operations 操作函数
 * @return 是否成功
 */
bool DatabaseManager::executeTransaction(const std::function<bool()>& operations)
{
    QSqlDatabase db = m_connectionPool->getConnection();
    
    if (!db.isOpen()) {
        return false;
    }
    
    // 开始事务
    if (!db.transaction()) {
        m_connectionPool->returnConnection(db);
        return false;
    }
    
    // 执行操作
    bool success = false;
    try {
        success = operations();
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Transaction failed: %1").arg(e.what()));
        success = false;
    }
    
    // 提交或回滚
    if (success) {
        success = db.commit();
    } else {
        db.rollback();
    }
    
    m_connectionPool->returnConnection(db);
    
    return success;
}

/**
 * @brief 执行预处理语句
 * @param statementId 语句ID
 * @param params 参数
 * @return 查询结果
 */
QueryResult DatabaseManager::executePreparedStatement(const QString& statementId,
                                                     const QMap<QString, QVariant>& params)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_preparedStatements.contains(statementId)) {
        QueryResult result;
        result.success = false;
        result.error = QSqlError("Statement not found", "", QSqlError::StatementError);
        return result;
    }
    
    QString sql = m_preparedStatements[statementId];
    locker.unlock();
    
    return executeQuery(sql, params);
}

/**
 * @brief 注册预处理语句
 * @param statementId 语句ID
 * @param sql SQL语句
 */
void DatabaseManager::registerPreparedStatement(const QString& statementId, const QString& sql)
{
    QMutexLocker locker(&m_mutex);
    m_preparedStatements[statementId] = sql;
    
    LOG_DEBUG(QString("Prepared statement registered: %1").arg(statementId));
}

/**
 * @brief 获取表信息
 * @param tableName 表名
 * @return 表信息
 */
QVector<QMap<QString, QVariant>> DatabaseManager::getTableInfo(const QString& tableName)
{
    QString sql = QString("PRAGMA table_info(%1)").arg(tableName);
    QueryResult result = executeQuery(sql);
    return result.rows;
}

/**
 * @brief 优化数据库
 */
void DatabaseManager::optimize()
{
    LOG_INFO("Optimizing database...");
    
    QSqlDatabase db = m_connectionPool->getConnection();
    
    if (db.isOpen()) {
        // 分析数据库
        db.exec("ANALYZE");
        
        // 清理碎片
        db.exec("VACUUM");
        
        // 重建索引
        db.exec("REINDEX");
        
        LOG_INFO("Database optimized");
    }
    
    m_connectionPool->returnConnection(db);
}

/**
 * @brief 获取数据库统计信息
 */
QMap<QString, QVariant> DatabaseManager::statistics() const
{
    QMap<QString, QVariant> stats;
    
    stats["totalQueries"] = m_totalQueries;
    stats["totalTime"] = m_totalTime;
    stats["failedQueries"] = m_failedQueries;
    stats["avgQueryTime"] = m_totalQueries > 0 ? m_totalTime / m_totalQueries : 0;
    
    if (m_connectionPool) {
        stats["availableConnections"] = m_connectionPool->availableConnections();
        stats["totalConnections"] = m_connectionPool->totalConnections();
    }
    
    return stats;
}

/**
 * @brief 备份数据库
 * @param backupPath 备份路径
 * @return 是否成功
 */
bool DatabaseManager::backup(const QString& backupPath)
{
    LOG_INFO(QString("Backing up database to: %1").arg(backupPath));
    
    // 确保目录存在
    QDir dir = QFileInfo(backupPath).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 获取数据库文件路径
    QSqlDatabase db = m_connectionPool->getConnection();
    QString dbPath = db.databaseName();
    m_connectionPool->returnConnection(db);
    
    // 复制文件
    if (QFile::copy(dbPath, backupPath)) {
        LOG_INFO("Database backup completed");
        return true;
    }
    
    LOG_ERROR("Database backup failed");
    return false;
}

/**
 * @brief 恢复数据库
 * @param backupPath 备份路径
 * @return 是否成功
 */
bool DatabaseManager::restore(const QString& backupPath)
{
    LOG_INFO(QString("Restoring database from: %1").arg(backupPath));
    
    if (!QFile::exists(backupPath)) {
        LOG_ERROR("Backup file not found");
        return false;
    }
    
    // 关闭所有连接
    m_connectionPool->cleanup();
    
    // 获取数据库文件路径
    QSqlDatabase db = m_connectionPool->getConnection();
    QString dbPath = db.databaseName();
    m_connectionPool->returnConnection(db);
    
    // 删除原文件
    QFile::remove(dbPath);
    
    // 复制备份文件
    if (QFile::copy(backupPath, dbPath)) {
        // 重新初始化连接池
        m_connectionPool->initialize();
        LOG_INFO("Database restore completed");
        return true;
    }
    
    LOG_ERROR("Database restore failed");
    return false;
}

/**
 * @brief 创建表结构
 */
void DatabaseManager::createTables()
{
    LOG_DEBUG("Creating database tables...");
    
    // 用户配置表
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS user_config (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            key TEXT UNIQUE NOT NULL,
            value TEXT,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    // 自选股表
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS favorites (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT UNIQUE NOT NULL,
            exchange TEXT,
            name TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    // 交易记录表
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS trades (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT NOT NULL,
            direction TEXT NOT NULL,
            price REAL NOT NULL,
            volume INTEGER NOT NULL,
            timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )");
    
    // K线数据缓存表
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS kline_cache (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            symbol TEXT NOT NULL,
            period TEXT NOT NULL,
            open REAL,
            high REAL,
            low REAL,
            close REAL,
            volume INTEGER,
            timestamp TIMESTAMP,
            UNIQUE(symbol, period, timestamp)
        )
    )");
    
    // 创建索引
    executeQuery("CREATE INDEX IF NOT EXISTS idx_favorites_symbol ON favorites(symbol)");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_trades_symbol ON trades(symbol)");
    executeQuery("CREATE INDEX IF NOT EXISTS idx_kline_symbol_period ON kline_cache(symbol, period)");
    
    LOG_DEBUG("Database tables created");
}

/**
 * @brief 应用优化设置
 */
void DatabaseManager::applyOptimizations()
{
    LOG_DEBUG("Applying database optimizations...");
    
    QSqlDatabase db = m_connectionPool->getConnection();
    
    if (db.isOpen()) {
        // 性能优化PRAGMA
        db.exec("PRAGMA journal_mode = WAL");
        db.exec("PRAGMA synchronous = NORMAL");
        db.exec("PRAGMA cache_size = -10240"); // 10MB
        db.exec("PRAGMA temp_store = MEMORY");
        db.exec("PRAGMA mmap_size = 268435456"); // 256MB
        db.exec("PRAGMA page_size = 4096");
        
        LOG_DEBUG("Database optimizations applied");
    }
    
    m_connectionPool->returnConnection(db);
}
