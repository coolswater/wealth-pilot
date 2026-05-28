/**
 * @file AsyncTaskManager.cpp
 * @brief 异步任务管理器实现
 */

#include "AsyncTaskManager.h"
#include <QMutexLocker>

// ========== AsyncTaskManager 实现 ==========

AsyncTaskManager& AsyncTaskManager::instance()
{
    static AsyncTaskManager instance;
    return instance;
}

AsyncTaskManager::AsyncTaskManager()
    : m_threadPool(new QThreadPool(this))
    , m_totalTasks(0)
    , m_completedTasks(0)
    , m_failedTasks(0)
    , m_cancelledTasks(0)
{
}

AsyncTaskManager::~AsyncTaskManager()
{
    waitForAll(5000);  // 等待5秒
    m_threadPool->waitForDone(5000);
}

bool AsyncTaskManager::initialize(int maxThreads)
{
    QMutexLocker locker(&m_mutex);
    
    if (maxThreads <= 0) {
        maxThreads = QThread::idealThreadCount();
    }
    
    m_threadPool->setMaxThreadCount(maxThreads);
    return true;
}

bool AsyncTaskManager::cancelTask(const QString& taskId)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_tasks.contains(taskId)) {
        m_taskStates[taskId] = TaskState::Cancelled;
        m_cancelledTasks++;
        emit taskCancelled(taskId);
        return true;
    }
    return false;
}

TaskState AsyncTaskManager::taskState(const QString& taskId) const
{
    QMutexLocker locker(&m_mutex);
    return m_taskStates.value(taskId, TaskState::Pending);
}

int AsyncTaskManager::activeTaskCount() const
{
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (auto it = m_taskStates.begin(); it != m_taskStates.end(); ++it) {
        if (it.value() == TaskState::Running) {
            count++;
        }
    }
    return count;
}

int AsyncTaskManager::pendingTaskCount() const
{
    QMutexLocker locker(&m_mutex);
    int count = 0;
    for (auto it = m_taskStates.begin(); it != m_taskStates.end(); ++it) {
        if (it.value() == TaskState::Pending) {
            count++;
        }
    }
    return count;
}

QMap<QString, QVariant> AsyncTaskManager::statistics() const
{
    QMutexLocker locker(&m_mutex);
    
    QMap<QString, QVariant> stats;
    stats["totalTasks"] = m_totalTasks;
    stats["completedTasks"] = m_completedTasks;
    stats["failedTasks"] = m_failedTasks;
    stats["cancelledTasks"] = m_cancelledTasks;
    stats["activeTasks"] = activeTaskCount();
    stats["pendingTasks"] = pendingTaskCount();
    stats["maxThreads"] = m_threadPool->maxThreadCount();
    stats["activeThreads"] = m_threadPool->activeThreadCount();
    
    return stats;
}

void AsyncTaskManager::cleanupCompletedTasks()
{
    QMutexLocker locker(&m_mutex);
    
    QList<QString> toRemove;
    for (auto it = m_taskStates.begin(); it != m_taskStates.end(); ++it) {
        if (it.value() == TaskState::Completed ||
            it.value() == TaskState::Failed ||
            it.value() == TaskState::Cancelled ||
            it.value() == TaskState::Timeout) {
            toRemove.append(it.key());
        }
    }
    
    for (const QString& taskId : toRemove) {
        m_tasks.remove(taskId);
        m_taskStates.remove(taskId);
    }
}

void AsyncTaskManager::waitForAll(int timeoutMs)
{
    if (timeoutMs < 0) {
        m_threadPool->waitForDone();
    } else {
        m_threadPool->waitForDone(timeoutMs);
    }
}
