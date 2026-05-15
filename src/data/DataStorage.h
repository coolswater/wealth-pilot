/**
 * @file DataStorage.h
 * @brief 数据存储管理器 - 本地数据持久化
 *
 * @details 功能：
 * - K线数据存储
 * - 成交记录存储
 * - 配置数据存储
 * - 数据导入导出
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QHash>
#include <QMutex>
#include <memory>
#include "core/types/MarketTypes.h"

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::KLineData;
using WealthPilot::KLinePeriod;
using WealthPilot::StockQuote;
using WealthPilot::MarketSnapshot;

/**
 * @brief 数据存储管理器
 */
class DataStorage : public QObject
{
    Q_OBJECT

public:
    static DataStorage& instance();

    bool initialize(const QString &dbPath = QString());
    void shutdown();

    // ========== K线数据 ==========

    bool saveKLineData(const QString &symbol, KLinePeriod period, const QVector<KLineData> &data);
    QVector<KLineData> loadKLineData(const QString &symbol, KLinePeriod period,
                                      const QDateTime &from, const QDateTime &to);
    bool hasKLineData(const QString &symbol, KLinePeriod period);
    QDateTime getLastKLineTime(const QString &symbol, KLinePeriod period);
    void clearKLineData(const QString &symbol, KLinePeriod period);

    // ========== 成交记录 ==========

    bool saveTradeRecord(const QString &tradeId, const QString &orderId,
                         const QString &symbol, double price, int volume,
                         const QDateTime &time, const QString &direction);
    QVector<QVariantMap> loadTradeRecords(const QDateTime &from, const QDateTime &to);

    // ========== 配置数据 ==========

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void removeValue(const QString &key);

    // ========== 数据维护 ==========

    void optimize();
    qint64 getDatabaseSize() const;
    bool backup(const QString &backupPath);
    bool restore(const QString &backupPath);

signals:
    void dataSaved(const QString &symbol, int count);
    void errorOccurred(const QString &error);

private:
    DataStorage(QObject *parent = nullptr);
    ~DataStorage() override;
    Q_DISABLE_COPY(DataStorage)

    bool createTables();
    QString kLineTableName(const QString &symbol, KLinePeriod period) const;

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // DATASTORAGE_H
