/**
 * @file BatchUpdateManager.cpp
 * @brief 批量更新管理器实现
 */

#include "BatchUpdateManager.h"
#include "shared/utils/Logger.h"
#include <QDateTime>
#include <QWidget>
#include <QApplication>

namespace WealthPilot {

// ========== BatchUpdateManager ==========

BatchUpdateManager* BatchUpdateManager::instance()
{
    static BatchUpdateManager instance;
    return &instance;
}

BatchUpdateManager::BatchUpdateManager()
    : QObject(nullptr)
    , m_batchTimer(new QTimer(this))
{
    m_batchTimer->setSingleShot(true);
    m_batchTimer->setInterval(m_mergeWindowMs);
    connect(m_batchTimer, &QTimer::timeout, this, &BatchUpdateManager::processBatch);
    
    LOG_DEBUG("[BatchUpdateManager] Initialized");
}

BatchUpdateManager::~BatchUpdateManager()
{
    if (m_batchTimer->isActive()) {
        m_batchTimer->stop();
        processBatch();
    }
    
    LOG_DEBUG(QString("[BatchUpdateManager] Destroyed, stats: total=%1, merged=%2, rate=%3%")
              .arg(m_totalUpdates)
              .arg(m_totalMerged)
              .arg(m_totalUpdates > 0 ? m_totalMerged * 100.0 / m_totalUpdates : 0, 0, 'f', 1));
}

void BatchUpdateManager::requestUpdate(const QString& id, std::function<void()> update, int priority)
{
    bool isNew = !m_pendingUpdates.contains(id);
    
    m_pendingUpdates[id] = BatchUpdateItem{
        id,
        update,
        priority,
        QDateTime::currentMSecsSinceEpoch()
    };
    
    if (!isNew) {
        m_totalMerged++;
        emit updateMerged(id);
    }
    
    m_totalUpdates++;
    
    // 启动定时器（如果未启动）
    if (!m_batchTimer->isActive() && !m_inBatchUpdate) {
        m_batchTimer->start();
    }
    
    LOG_DEBUG(QString("[BatchUpdateManager] Requested update: %1 (pending=%2)")
              .arg(id)
              .arg(m_pendingUpdates.size()));
}

void BatchUpdateManager::cancelUpdate(const QString& id)
{
    m_pendingUpdates.remove(id);
    
    // 如果没有待处理的更新，停止定时器
    if (m_pendingUpdates.isEmpty() && m_batchTimer->isActive()) {
        m_batchTimer->stop();
    }
}

void BatchUpdateManager::flush()
{
    if (m_batchTimer->isActive()) {
        m_batchTimer->stop();
    }
    processBatch();
}

void BatchUpdateManager::clear()
{
    m_pendingUpdates.clear();
    if (m_batchTimer->isActive()) {
        m_batchTimer->stop();
    }
}

void BatchUpdateManager::setMergeWindow(int milliseconds)
{
    m_mergeWindowMs = milliseconds;
    m_batchTimer->setInterval(milliseconds);
}

void BatchUpdateManager::setUpdatesEnabledOptimization(bool enabled)
{
    m_updatesEnabledOptimization = enabled;
}

int BatchUpdateManager::pendingCount() const
{
    return m_pendingUpdates.size();
}

void BatchUpdateManager::beginBatchUpdate()
{
    m_batchDepth++;
    m_inBatchUpdate = true;
    
    if (m_updatesEnabledOptimization && m_batchDepth == 1) {
        // 禁用全局更新
        if (qApp) {
            qApp->setUpdatesEnabled(false);
        }
    }
    
    emit batchUpdateStarted();
}

void BatchUpdateManager::endBatchUpdate()
{
    if (m_batchDepth > 0) {
        m_batchDepth--;
    }
    
    if (m_batchDepth == 0) {
        m_inBatchUpdate = false;
        
        // 执行待处理的更新
        processBatch();
        
        // 重新启用全局更新
        if (m_updatesEnabledOptimization && qApp) {
            qApp->setUpdatesEnabled(true);
        }
        
        emit batchUpdateCompleted(m_pendingUpdates.size());
    }
}

void BatchUpdateManager::processBatch()
{
    if (m_pendingUpdates.isEmpty()) {
        return;
    }
    
    if (m_inBatchUpdate) {
        // 在批量更新块中，延迟执行
        return;
    }
    
    executeUpdates();
}

void BatchUpdateManager::executeUpdates()
{
    if (m_pendingUpdates.isEmpty()) {
        return;
    }
    
    LOG_DEBUG(QString("[BatchUpdateManager] Executing batch update, count=%1")
              .arg(m_pendingUpdates.size()));
    
    // 按优先级排序
    QVector<BatchUpdateItem> items;
    items.reserve(m_pendingUpdates.size());
    for (const auto& item : m_pendingUpdates) {
        items.append(item);
    }
    
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.priority > b.priority;
    });
    
    // 执行更新
    int executedCount = 0;
    for (const auto& item : items) {
        if (item.update) {
            item.update();
            executedCount++;
        }
    }
    
    m_pendingUpdates.clear();
    
    LOG_DEBUG(QString("[BatchUpdateManager] Batch update complete, executed=%1")
              .arg(executedCount));
    
    emit batchUpdateCompleted(executedCount);
}

// ========== ThemeBatchUpdater ==========

QHash<QString, std::pair<std::function<void()>, int>> ThemeBatchUpdater::s_themeUpdates;

void ThemeBatchUpdater::registerThemeUpdate(const QString& id, std::function<void()> update, int priority)
{
    s_themeUpdates[id] = {update, priority};
    
    LOG_DEBUG(QString("[ThemeBatchUpdater] Registered theme update: %1 (priority=%2)")
              .arg(id)
              .arg(priority));
}

void ThemeBatchUpdater::applyThemeBatch()
{
    LOG_INFO(QString("[ThemeBatchUpdater] Applying theme batch, count=%1")
             .arg(s_themeUpdates.size()));
    
    // 开始批量更新
    BatchUpdateScope scope;
    
    // 按优先级排序
    QVector<QPair<QString, int>> sorted;
    for (auto it = s_themeUpdates.begin(); it != s_themeUpdates.end(); ++it) {
        sorted.append({it.key(), it.value().second});
    }
    
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    
    // 执行更新
    for (const auto& item : sorted) {
        auto it = s_themeUpdates.find(item.first);
        if (it != s_themeUpdates.end() && it.value().first) {
            it.value().first();
        }
    }
}

int ThemeBatchUpdater::estimateUpdateTime()
{
    // 基于历史数据估算（每个更新约5ms）
    return s_themeUpdates.size() * 5;
}

} // namespace WealthPilot
