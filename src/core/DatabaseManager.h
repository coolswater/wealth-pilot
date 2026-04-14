/**
 * @file DatabaseManager.h
 * @brief 数据库管理器 - 高性能SQLite数据库管理
 *
 * @details 功能：
 * - 连接池管理
 * - 批量操作优化
 * - 事务支持
 * - 异步查询
 * - 性能监控
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include "Singleton.h"
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
 * @brief 数据库配置
 */
struct DatabaseConfig {
    QString databaseName;       // 数据库名称
    QString hostName;          // 主机名
    int port;                  // 端口
    QString userName;          // 用户名
    QString password;          // 密码
    int maxConnections;        // 最大连接数
    int minConnections;        // 最小连接数
    int connectionTimeout;     // 连接超时（毫秒）
    bool enableWAL;            // 启用WAL模式
    int cacheSize;             // 缓存大小（KB）
    int pageSize;              // 页面大小（字节）
    bool enableForeignKeys;    // 启用外键约束
};

/**
 * @brief 查询结果
 */
struct QueryResult {
    bool success;                              // 是否成功
    QSqlError error;                          // 错误信息
    QVector<QMap<QString, QVariant>> rows;    // 查询结果
    int affectedRows;                         // 影响的行数
    qint64 executionTime;                     // 执行时间（微秒）
};

/**
 * @brief 数据库连接池
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
    int m_connectionCounter;
};

/**
 * @brief 异步查询线程
 */
class AsyncQueryThread : public QThread
{
    Q_OBJECT

public:
    explicit AsyncQueryThread(ConnectionPool* pool, QObject* parent = nullptr);
    
    void executeQuery(const QString& queryId, const QString& sql, const QMap<QString, QVariant>& params = QMap<QString, QVariant>());
    void executeBatch(const QString& queryId, const QString& sql, const QVector<QMap<QString, QVariant>>& batchData);

signals:
    void queryCompleted(const QString& queryId, const QueryResult& result);

protected:
    void run() override;

private:
    struct QueryTask {
        QString queryId;
        QString sql;
        QMap<QString, QVariant> params;
        bool isBatch;
        QVector<QMap<QString, QVariant>> batchData;
    };
    
    ConnectionPool* m_pool;
    QQueue<QueryTask> m_taskQueue;
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_running;
};

/**
 * @brief 数据库管理器
 */
class DatabaseManager : public QObject, public Singleton<DatabaseManager>
{
    Q_OBJECT
    friend class Singleton<DatabaseManager>;

public:
    /**
     * @brief 初始化数据库管理器
     */
    bool initialize(const DatabaseConfig& config = DatabaseConfig());

    /**
     * @brief 执行查询（同步）
     */
    QueryResult executeQuery(const QString& sql, const QMap<QString, QVariant>& params = QMap<QString, QVariant>());

    /**
     * @brief 执行查询（异步）
     */
    QString executeQueryAsync(const QString& sql, const QMap<QString, QVariant>& params = QMap<QString, QVariant>());

    /**
     * @brief 执行批量操作
     */
    QueryResult executeBatch(const QString& sql, const QVector<QMap<QString, QVariant>>& batchData);

    /**
     * @brief 执行事务
     */
    bool executeTransaction(const std::function<bool()>& operations);

    /**
     * @brief 执行预处理语句
     */
    QueryResult executePreparedStatement(const QString& statementId, const QMap<QString, QVariant>& params);

    /**
     * @brief 注册预处理语句
     */
    void registerPreparedStatement(const QString& statementId, const QString& sql);

    /**
     * @brief 获取表信息
     */
    QVector<QMap<QString, QVariant>> getTableInfo(const QString& tableName);

    /**
     * @brief 优化数据库
     */
    void optimize();

    /**
     * @brief 获取数据库统计信息
     */
    QMap<QString, QVariant> statistics() const;

    /**
     * @brief 备份数据库
     */
    bool backup(const QString& backupPath);

    /**
     * @brief 恢复数据库
     */
    bool restore(const QString& backupPath);

signals:
    /**
     * @brief 异步查询完成信号
     */
    void asyncQueryCompleted(const QString& queryId, const QueryResult& result);

    /**
     * @brief 错误信号
     */
    void errorOccurred(const QString& error);

private:
    DatabaseManager();
    ~DatabaseManager();

    // 创建表结构
    void createTables();
    
    // 优化设置
    void applyOptimizations();

    std::unique_ptr<ConnectionPool> m_connectionPool;
    std::unique_ptr<AsyncQueryThread> m_asyncThread;
    QMap<QString, QString> m_preparedStatements;
    mutable QMutex m_mutex;
    
    // 统计信息
    qint64 m_totalQueries;
    qint64 m_totalTime;
    qint64 m_failedQueries;
};

#endif // DATABASEMANAGER_H
