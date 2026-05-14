/**
 * @file DatabaseMigration.h
 * @brief 数据库迁移系统 - 版本化数据库结构管理
 *
 * @details 提供数据库版本迁移机制，支持数据库结构的版本化管理。
 * 参考 FinceptTerminal 的数据库迁移系统。
 *
 * @example
 * // 注册迁移
 * MigrationManager::instance().registerMigration(new InitialMigration());
 * MigrationManager::instance().registerMigration(new AddWatchlistMigration());
 *
 * // 执行迁移
 * auto result = MigrationManager::instance().migrate(db);
 */

#ifndef WEALTHPILOT_DATABASE_MIGRATION_H
#define WEALTHPILOT_DATABASE_MIGRATION_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMap>
#include <QDateTime>
#include <memory>
#include "../core/base/Result.h"

namespace WealthPilot {

/**
 * @brief 迁移记录
 */
struct MigrationRecord {
    int version = 0;           // 版本号
    QString name;              // 迁移名称
    QString description;       // 描述
    QDateTime appliedAt;       // 应用时间
    bool success = false;      // 是否成功
};

/**
 * @brief 数据库迁移接口
 */
class Migration {
public:
    virtual ~Migration() = default;
    
    /**
     * @brief 获取版本号
     */
    virtual int version() const = 0;
    
    /**
     * @brief 获取迁移名称
     */
    virtual QString name() const = 0;
    
    /**
     * @brief 获取描述
     */
    virtual QString description() const = 0;
    
    /**
     * @brief 执行迁移
     */
    virtual Result<void> up(QSqlDatabase& db) = 0;
    
    /**
     * @brief 回滚迁移
     */
    virtual Result<void> down(QSqlDatabase& db) = 0;
    
    /**
     * @brief 检查是否需要迁移
     */
    virtual bool shouldRun(int currentVersion) const {
        return version() > currentVersion;
    }
};

/**
 * @brief 迁移管理器
 */
class MigrationManager : public QObject {
    Q_OBJECT

public:
    static MigrationManager& instance() {
        static MigrationManager instance;
        return instance;
    }

    /**
     * @brief 注册迁移
     */
    void registerMigration(std::shared_ptr<Migration> migration) {
        if (migration) {
            m_migrations[migration->version()] = migration;
        }
    }

    /**
     * @brief 获取当前数据库版本
     */
    int getCurrentVersion(QSqlDatabase& db) {
        // 检查迁移表是否存在
        QSqlQuery query(db);
        if (!query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='migrations'")) {
            return 0;
        }
        
        if (!query.next()) {
            // 创建迁移表
            if (!query.exec(R"(
                CREATE TABLE migrations (
                    version INTEGER PRIMARY KEY,
                    name TEXT NOT NULL,
                    description TEXT,
                    applied_at TEXT,
                    success INTEGER
                )
            )")) {
                return 0;
            }
            return 0;
        }
        
        // 获取最新版本
        if (!query.exec("SELECT MAX(version) FROM migrations WHERE success = 1")) {
            return 0;
        }
        
        if (query.next()) {
            return query.value(0).toInt();
        }
        
        return 0;
    }

    /**
     * @brief 执行所有待迁移
     */
    Result<void> migrate(QSqlDatabase& db) {
        int currentVersion = getCurrentVersion(db);
        
        // 按版本号排序
        QList<int> versions = m_migrations.keys();
        std::sort(versions.begin(), versions.end());
        
        for (int version : versions) {
            auto& migration = m_migrations[version];
            
            if (migration->shouldRun(currentVersion)) {
                // 执行迁移
                auto result = migration->up(db);
                
                // 记录迁移
                MigrationRecord record;
                record.version = version;
                record.name = migration->name();
                record.description = migration->description();
                record.appliedAt = QDateTime::currentDateTime();
                record.success = result.isOk();
                
                QSqlQuery query(db);
                query.prepare(R"(
                    INSERT INTO migrations (version, name, description, applied_at, success)
                    VALUES (?, ?, ?, ?, ?)
                )");
                query.addBindValue(record.version);
                query.addBindValue(record.name);
                query.addBindValue(record.description);
                query.addBindValue(record.appliedAt.toString(Qt::ISODate));
                query.addBindValue(record.success ? 1 : 0);
                query.exec();
                
                if (result.isError()) {
                    return Result<void>::error(
                        QString("Migration %1 failed: %2")
                            .arg(version)
                            .arg(result.errorMessage()));
                }
                
                emit migrationCompleted(version, migration->name());
            }
        }
        
        return Result<void>::ok();
    }

    /**
     * @brief 回滚到指定版本
     */
    Result<void> rollback(QSqlDatabase& db, int targetVersion) {
        int currentVersion = getCurrentVersion(db);
        
        if (targetVersion >= currentVersion) {
            return Result<void>::error("INVALID_TARGET",
                QString("Target version %1 must be less than current %2")
                    .arg(targetVersion).arg(currentVersion));
        }
        
        // 按版本号降序排序
        QList<int> versions = m_migrations.keys();
        std::sort(versions.begin(), versions.end(), std::greater<int>());
        
        for (int version : versions) {
            if (version <= targetVersion) {
                break;
            }
            
            auto& migration = m_migrations[version];
            
            // 执行回滚
            auto result = migration->down(db);
            
            // 删除迁移记录
            QSqlQuery query(db);
            query.prepare("DELETE FROM migrations WHERE version = ?");
            query.addBindValue(version);
            query.exec();
            
            if (result.isError()) {
                return Result<void>::error(
                    QString("Rollback %1 failed: %2")
                        .arg(version)
                        .arg(result.errorMessage()));
            }
            
            emit rollbackCompleted(version, migration->name());
        }
        
        return Result<void>::ok();
    }

    /**
     * @brief 获取迁移历史
     */
    QList<MigrationRecord> getHistory(QSqlDatabase& db) {
        QList<MigrationRecord> records;
        
        QSqlQuery query(db);
        if (query.exec("SELECT version, name, description, applied_at, success FROM migrations ORDER BY version")) {
            while (query.next()) {
                MigrationRecord record;
                record.version = query.value(0).toInt();
                record.name = query.value(1).toString();
                record.description = query.value(2).toString();
                record.appliedAt = QDateTime::fromString(query.value(3).toString(), Qt::ISODate);
                record.success = query.value(4).toInt() == 1;
                records.append(record);
            }
        }
        
        return records;
    }

    /**
     * @brief 获取待执行的迁移
     */
    QList<std::shared_ptr<Migration>> getPendingMigrations(QSqlDatabase& db) {
        int currentVersion = getCurrentVersion(db);
        QList<std::shared_ptr<Migration>> pending;
        
        for (auto& migration : m_migrations) {
            if (migration->shouldRun(currentVersion)) {
                pending.append(migration);
            }
        }
        
        return pending;
    }

signals:
    void migrationCompleted(int version, const QString& name);
    void rollbackCompleted(int version, const QString& name);

private:
    MigrationManager() = default;
    MigrationManager(const MigrationManager&) = delete;
    MigrationManager& operator=(const MigrationManager&) = delete;

    QMap<int, std::shared_ptr<Migration>> m_migrations;
};

// ========== 具体迁移实现 ==========

/**
 * @brief 初始迁移 - 创建基础表
 */
class InitialMigration : public Migration {
public:
    int version() const override { return 1; }
    QString name() const override { return "Initial"; }
    QString description() const override { return "Create initial database tables"; }
    
    Result<void> up(QSqlDatabase& db) override {
        QSqlQuery query(db);
        
        // 用户表
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password_hash TEXT NOT NULL,
                email TEXT,
                created_at TEXT,
                updated_at TEXT
            )
        )")) {
            return Result<void>::error("CREATE_TABLE_FAILED", query.lastError().text());
        }
        
        // 自选股表
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS watchlist (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER,
                symbol TEXT NOT NULL,
                name TEXT,
                added_at TEXT,
                sort_order INTEGER,
                FOREIGN KEY (user_id) REFERENCES users(id)
            )
        )")) {
            return Result<void>::error("CREATE_TABLE_FAILED", query.lastError().text());
        }
        
        // 设置表
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS settings (
                key TEXT PRIMARY KEY,
                value TEXT,
                updated_at TEXT
            )
        )")) {
            return Result<void>::error("CREATE_TABLE_FAILED", query.lastError().text());
        }
        
        return Result<void>::ok();
    }
    
    Result<void> down(QSqlDatabase& db) override {
        QSqlQuery query(db);
        
        QStringList tables = {"users", "watchlist", "settings"};
        for (const auto& table : tables) {
            query.exec(QString("DROP TABLE IF EXISTS %1").arg(table));
        }
        
        return Result<void>::ok();
    }
};

/**
 * @brief 添加警告表迁移
 */
class AddAlertsMigration : public Migration {
public:
    int version() const override { return 2; }
    QString name() const override { return "AddAlerts"; }
    QString description() const override { return "Add alerts table for price alerts"; }
    
    Result<void> up(QSqlDatabase& db) override {
        QSqlQuery query(db);
        
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS alerts (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER,
                symbol TEXT NOT NULL,
                alert_type TEXT NOT NULL,
                target_price REAL,
                condition TEXT,
                enabled INTEGER DEFAULT 1,
                created_at TEXT,
                triggered_at TEXT,
                FOREIGN KEY (user_id) REFERENCES users(id)
            )
        )")) {
            return Result<void>::error("CREATE_TABLE_FAILED", query.lastError().text());
        }
        
        return Result<void>::ok();
    }
    
    Result<void> down(QSqlDatabase& db) override {
        QSqlQuery query(db);
        query.exec("DROP TABLE IF EXISTS alerts");
        return Result<void>::ok();
    }
};

/**
 * @brief 添加历史数据缓存表迁移
 */
class AddHistoryCacheMigration : public Migration {
public:
    int version() const override { return 3; }
    QString name() const override { return "AddHistoryCache"; }
    QString description() const override { return "Add history cache tables for market data"; }
    
    Result<void> up(QSqlDatabase& db) override {
        QSqlQuery query(db);
        
        // K线缓存表
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS bar_cache (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                symbol TEXT NOT NULL,
                period TEXT NOT NULL,
                time TEXT NOT NULL,
                open REAL,
                high REAL,
                low REAL,
                close REAL,
                volume REAL,
                amount REAL,
                UNIQUE(symbol, period, time)
            )
        )")) {
            return Result<void>::error("CREATE_TABLE_FAILED", query.lastError().text());
        }
        
        // 创建索引
        query.exec("CREATE INDEX IF NOT EXISTS idx_bar_cache_symbol_period ON bar_cache(symbol, period)");
        query.exec("CREATE INDEX IF NOT EXISTS idx_bar_cache_time ON bar_cache(time)");
        
        return Result<void>::ok();
    }
    
    Result<void> down(QSqlDatabase& db) override {
        QSqlQuery query(db);
        query.exec("DROP TABLE IF EXISTS bar_cache");
        return Result<void>::ok();
    }
};

} // namespace WealthPilot

#endif // WEALTHPILOT_DATABASE_MIGRATION_H