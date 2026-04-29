/**
 * @file DataSourceManager.cpp
 * @brief 数据源管理器实现
 */

#include "DataSourceManager.h"
#include "utils/Logger.h"

#include <QMutexLocker>
#include <QDateTime>

struct DataSourceManager::Impl {
    QHash<QString, QHash<QString, DataSourceInfo>> sources;  // type -> name -> info
    QHash<QString, QString> currentSources;                  // type -> current best source
    QHash<QString, QHash<QString, bool>> enabledSources;     // type -> name -> enabled
    
    QTimer* healthCheckTimer = nullptr;
    int failureThreshold = 3;
    int recoveryThreshold = 2;
    
    mutable QMutex mutex;
};

DataSourceManager* DataSourceManager::instance()
{
    static DataSourceManager instance;
    return &instance;
}

DataSourceManager::DataSourceManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->healthCheckTimer = new QTimer(this);
    connect(d->healthCheckTimer, &QTimer::timeout, this, &DataSourceManager::checkHealth);
    
    // 注册默认数据源
    registerDataSource("stock", "sina", 0);
    registerDataSource("stock", "tencent", 1);
    registerDataSource("stock", "eastmoney", 2);
    
    registerDataSource("forex", "sina", 0);
    registerDataSource("forex", "eastmoney", 1);
    registerDataSource("forex", "boc", 2);
    
    registerDataSource("crypto", "coingecko", 0);
    registerDataSource("crypto", "binance", 1);
    registerDataSource("crypto", "okx", 2);
    
    registerDataSource("fund", "eastmoney", 0);
    registerDataSource("fund", "danjuan", 1);
    registerDataSource("fund", "sina", 2);
    
    LOG_DEBUG("DataSourceManager initialized");
}

DataSourceManager::~DataSourceManager()
{
    stopHealthCheck();
}

void DataSourceManager::registerDataSource(const QString& type, const QString& name, int priority)
{
    QMutexLocker locker(&d->mutex);
    
    DataSourceInfo info;
    info.name = name;
    info.type = type;
    info.priority = priority;
    info.status = DataSourceStatus::Unknown;
    
    d->sources[type][name] = info;
    d->enabledSources[type][name] = true;
    
    // 如果是第一个注册的数据源，设为当前源
    if (!d->currentSources.contains(type)) {
        d->currentSources[type] = name;
    }
    
    LOG_INFO(QString("Registered data source: %1/%2, priority: %3").arg(type, name).arg(priority));
}

QString DataSourceManager::getBestSource(const QString& type) const
{
    QMutexLocker locker(&d->mutex);
    
    QString current = d->currentSources.value(type);
    if (current.isEmpty()) {
        LOG_WARNING(QString("No data source registered for type: %1").arg(type));
        return QString();
    }
    
    // 检查当前源是否可用
    if (d->enabledSources[type].value(current, false) &&
        d->sources[type][current].status != DataSourceStatus::Failed) {
        return current;
    }
    
    // 寻找最佳可用源
    QString bestSource;
    int bestPriority = INT_MAX;
    
    for (auto it = d->sources[type].begin(); it != d->sources[type].end(); ++it) {
        if (!d->enabledSources[type].value(it.key(), false)) continue;
        if (it.value().status == DataSourceStatus::Failed) continue;
        
        if (it.value().priority < bestPriority) {
            bestPriority = it.value().priority;
            bestSource = it.key();
        }
    }
    
    return bestSource;
}

void DataSourceManager::reportSuccess(const QString& type, const QString& name, int latencyMs)
{
    QMutexLocker locker(&d->mutex);
    
    if (!d->sources[type].contains(name)) return;
    
    DataSourceInfo& info = d->sources[type][name];
    info.failCount = 0;
    info.successCount++;
    info.lastSuccess = QDateTime::currentDateTime();
    
    if (latencyMs > 0) {
        info.latencyMs = (info.latencyMs + latencyMs) / 2;  // 平均延迟
    }
    
    locker.unlock();
    updateSourceStatus(type, name);
}

void DataSourceManager::reportFailure(const QString& type, const QString& name)
{
    QMutexLocker locker(&d->mutex);
    
    if (!d->sources[type].contains(name)) return;
    
    DataSourceInfo& info = d->sources[type][name];
    info.successCount = 0;
    info.failCount++;
    info.lastFailure = QDateTime::currentDateTime();
    
    LOG_WARNING(QString("Data source failure: %1/%2, fail count: %3").arg(type, name).arg(info.failCount));
    
    locker.unlock();
    updateSourceStatus(type, name);
    
    // 检查是否需要切换
    QString bestSource = getBestSource(type);
    if (bestSource.isEmpty()) {
        emit allSourcesFailed(type);
    }
}

DataSourceInfo DataSourceManager::getSourceInfo(const QString& type, const QString& name) const
{
    QMutexLocker locker(&d->mutex);
    return d->sources[type].value(name);
}

QHash<QString, DataSourceInfo> DataSourceManager::getAllSources(const QString& type) const
{
    QMutexLocker locker(&d->mutex);
    return d->sources.value(type);
}

void DataSourceManager::setSourceEnabled(const QString& type, const QString& name, bool enabled)
{
    QMutexLocker locker(&d->mutex);
    d->enabledSources[type][name] = enabled;
    
    LOG_INFO(QString("Data source %1/%2 %3").arg(type, name).arg(enabled ? "enabled" : "disabled"));
}

void DataSourceManager::setFailureThreshold(int threshold)
{
    QMutexLocker locker(&d->mutex);
    d->failureThreshold = threshold;
}

void DataSourceManager::setRecoveryThreshold(int threshold)
{
    QMutexLocker locker(&d->mutex);
    d->recoveryThreshold = threshold;
}

void DataSourceManager::startHealthCheck(int intervalMs)
{
    d->healthCheckTimer->start(intervalMs);
    LOG_INFO(QString("Health check started, interval: %1ms").arg(intervalMs));
}

void DataSourceManager::stopHealthCheck()
{
    d->healthCheckTimer->stop();
    LOG_INFO("Health check stopped");
}

void DataSourceManager::updateSourceStatus(const QString& type, const QString& name)
{
    QMutexLocker locker(&d->mutex);
    
    if (!d->sources[type].contains(name)) return;
    
    DataSourceInfo& info = d->sources[type][name];
    DataSourceStatus oldStatus = info.status;
    
    // 更新状态
    if (info.failCount >= d->failureThreshold) {
        info.status = DataSourceStatus::Failed;
    } else if (info.failCount > 0) {
        info.status = DataSourceStatus::Degraded;
    } else if (info.successCount >= d->recoveryThreshold) {
        info.status = DataSourceStatus::Healthy;
    }
    
    if (oldStatus != info.status) {
        locker.unlock();
        emit sourceStatusChanged(type, name, info.status);
        LOG_INFO(QString("Data source status changed: %1/%2 -> %3").arg(type, name)
                 .arg(static_cast<int>(info.status)));
    }
}

void DataSourceManager::checkHealth()
{
    QMutexLocker locker(&d->mutex);
    
    for (auto typeIt = d->sources.begin(); typeIt != d->sources.end(); ++typeIt) {
        for (auto nameIt = typeIt.value().begin(); nameIt != typeIt.value().end(); ++nameIt) {
            DataSourceInfo& info = nameIt.value();
            
            // 如果长时间没有成功，标记为失败
            if (info.lastSuccess.isValid()) {
                qint64 elapsed = info.lastSuccess.secsTo(QDateTime::currentDateTime());
                if (elapsed > 300 && info.status == DataSourceStatus::Healthy) {
                    info.status = DataSourceStatus::Degraded;
                    emit sourceStatusChanged(info.type, info.name, info.status);
                }
            }
        }
    }
}