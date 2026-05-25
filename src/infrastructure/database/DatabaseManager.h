/**
 * @file DatabaseManager.h
 * @brief Database Manager - High-performance SQLite database management
 * @author WealthPilot Team
 * @version 2.0.0
 * 
 * @details Features:
 * - Connection pool management
 * - Batch operation optimization
 * - Transaction support
 * - Async queries
 * - Performance monitoring
 * 
 * @thread_safe All public methods are thread-safe
 */
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include "shared/base/Singleton.h"
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QMap>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QThread>
#include <memory>

/**
 * @brief Database configuration
 */
struct DatabaseConfig {
    QString databaseName = "wealthpilot.db";  // Database name
    QString hostName;                          // Host name
    int port = 0;                              // Port
    QString userName;                          // Username
    QString password;                          // Password
    int maxConnections = 10;                   // Max connections
    int minConnections = 2;                    // Min connections
    int connectionTimeout = 30000;             // Connection timeout (ms)
    bool enableWAL = true;                     // Enable WAL mode
    int cacheSize = 4096;                      // Cache size (KB)
    int pageSize = 4096;                       // Page size (bytes)
    bool enableForeignKeys = true;             // Enable foreign keys
};

/**
 * @brief Query result
 */
struct QueryResult {
    bool success = false;                      // Success flag
    QSqlError error;                           // Error info
    QVector<QMap<QString, QVariant>> rows;     // Query results
    int affectedRows = 0;                      // Affected rows
    qint64 executionTime = 0;                  // Execution time (microseconds)
};

/**
 * @brief Database connection pool
 */
class ConnectionPool : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionPool(const DatabaseConfig& config);
    ~ConnectionPool();

    QSqlDatabase getConnection();
    void returnConnection(QSqlDatabase connection);
    void initialize();
    void cleanup();

    int availableConnections() const;
    int totalConnections() const;

private:
    QSqlDatabase createConnection();
    
    DatabaseConfig m_config;
    QQueue<QSqlDatabase> m_availableConnections;
    QMap<QString, QSqlDatabase> m_usedConnections;
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    int m_connectionCounter = 0;
};

/**
 * @brief Async query thread
 */
class AsyncQueryThread : public QThread
{
    Q_OBJECT

public:
    explicit AsyncQueryThread(ConnectionPool* pool, QObject* parent = nullptr);
    
    void executeQuery(const QString& queryId, const QString& sql, const QMap<QString, QVariant>& params = QMap<QString, QVariant>());
    void executeBatch(const QString& queryId, const QString& sql, const QVector<QMap<QString, QVariant>>& batchData);
    void stop();  ///< 停止线程

signals:
    void queryCompleted(const QString& queryId, const QueryResult& result);

protected:
    void run() override;

private:
    struct QueryTask {
        QString queryId;
        QString sql;
        QMap<QString, QVariant> params;
        bool isBatch = false;
        QVector<QMap<QString, QVariant>> batchData;
    };
    
    ConnectionPool* m_pool;
    QQueue<QueryTask> m_taskQueue;
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_running = false;
};

/**
 * @brief Database Manager
 */
class DatabaseManager : public QObject, public Singleton<DatabaseManager>
{
    Q_OBJECT
    friend class Singleton<DatabaseManager>;

public:
    /**
     * @brief Initialize database manager
     */
    bool initialize(const DatabaseConfig& config = DatabaseConfig());

    /**
     * @brief Execute query (synchronous)
     */
    QueryResult executeQuery(const QString& sql, const QMap<QString, QVariant>& params = QMap<QString, QVariant>());

    /**
     * @brief Execute query (asynchronous)
     */
    QString executeQueryAsync(const QString& sql, const QMap<QString, QVariant>& params = QMap<QString, QVariant>());

    /**
     * @brief Execute batch operation
     */
    QueryResult executeBatch(const QString& sql, const QVector<QMap<QString, QVariant>>& batchData);

    /**
     * @brief Execute transaction
     */
    bool executeTransaction(const std::function<bool()>& operations);

    /**
     * @brief Execute prepared statement
     */
    QueryResult executePreparedStatement(const QString& statementId, const QMap<QString, QVariant>& params);

    /**
     * @brief Register prepared statement
     */
    void registerPreparedStatement(const QString& statementId, const QString& sql);

    /**
     * @brief Get table info
     */
    QVector<QMap<QString, QVariant>> getTableInfo(const QString& tableName);

    /**
     * @brief Optimize database
     */
    void optimize();

    /**
     * @brief Shutdown database manager
     */
    void shutdown();

    /**
     * @brief Get database statistics
     */
    QMap<QString, QVariant> statistics() const;

    /**
     * @brief Backup database
     */
    bool backup(const QString& backupPath);

    /**
     * @brief Restore database
     */
    bool restore(const QString& backupPath);

signals:
    /**
     * @brief Async query completed signal
     */
    void asyncQueryCompleted(const QString& queryId, const QueryResult& result);

    /**
     * @brief Error signal
     */
    void errorOccurred(const QString& error);

private:
    DatabaseManager();
    ~DatabaseManager();

    // Create table structure
    void createTables();
    
    // Apply optimizations
    void applyOptimizations();

    std::unique_ptr<ConnectionPool> m_connectionPool;
    std::unique_ptr<AsyncQueryThread> m_asyncThread;
    QMap<QString, QString> m_preparedStatements;
    mutable QMutex m_mutex;
    
    // Statistics
    qint64 m_totalQueries = 0;
    qint64 m_totalTime = 0;
    qint64 m_failedQueries = 0;
};

#endif // DATABASEMANAGER_H
