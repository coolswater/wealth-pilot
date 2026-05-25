/**
 * @file AsyncTaskManager.h
 * @brief 异步任务管理器 - 高性能异步处理框架
 *
 * @details 功能：
 * - 任务队列
 * - 线程池
 * - 任务优先级
 * - 任务依赖
 * - 取消和超时
 * - 性能监控
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef ASYNCTASKMANAGER_H
#define ASYNCTASKMANAGER_H

#include <QObject>
#include <QRunnable>
#include <QThreadPool>
#include <QMap>
#include <QMutex>
#include <QWaitCondition>
#include <QFuture>
#include <QPromise>
#include <QElapsedTimer>
#include <QTimer>
#include <memory>
#include <functional>

/**
 * @brief 任务状态
 */
enum class TaskState {
    Pending,        // 等待中
    Running,        // 运行中
    Completed,      // 已完成
    Failed,         // 失败
    Cancelled,      // 已取消
    Timeout        // 超时
};

/**
 * @brief 任务优先级
 */
enum class TaskPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Urgent = 3
};

/**
 * @brief 任务结果
 */
template<typename T>
struct TaskResult {
    bool success;
    T value;
    QString error;
    qint64 executionTime;  // 微秒
    TaskState state;
};

/**
 * @brief 异步任务
 * @note 模板类不能使用 Q_OBJECT 宏，使用回调函数代替信号
 */
template<typename T>
class AsyncTask : public QRunnable
{
public:
    using TaskFunction = std::function<T()>;
    using CallbackFunction = std::function<void(const TaskResult<T>&)>;

    AsyncTask(const QString& taskId, TaskFunction func, TaskPriority priority = TaskPriority::Normal)
        : m_taskId(taskId)
        , m_function(func)
        , m_priority(priority)
        , m_state(TaskState::Pending)
    {
        setAutoDelete(false);
    }

    void run() override
    {
        QElapsedTimer timer;
        timer.start();
        
        TaskResult<T> result;
        result.state = TaskState::Running;
        
        try {
            result.value = m_function();
            result.success = true;
            result.state = TaskState::Completed;
        } catch (const std::exception& e) {
            result.success = false;
            result.error = QString::fromStdString(e.what());
            result.state = TaskState::Failed;
        } catch (...) {
            result.success = false;
            result.error = "Unknown error";
            result.state = TaskState::Failed;
        }
        
        result.executionTime = timer.nsecsElapsed() / 1000;
        
        m_state = result.state;
        
        if (m_callback) {
            m_callback(result);
        }
    }

    void setCallback(CallbackFunction callback) { m_callback = callback; }
    QString taskId() const { return m_taskId; }
    TaskPriority priority() const { return m_priority; }
    TaskState state() const { return m_state; }

    bool isFinished() const { 
        return m_state == TaskState::Completed || 
               m_state == TaskState::Failed || 
               m_state == TaskState::Cancelled ||
               m_state == TaskState::Timeout;
    }

private:
    QString m_taskId;
    TaskFunction m_function;
    CallbackFunction m_callback;
    TaskPriority m_priority;
    TaskState m_state;
};

/**
 * @brief 异步任务管理器
 */
class AsyncTaskManager : public QObject
{
    Q_OBJECT

public:
    static AsyncTaskManager& instance();

    /**
     * @brief 初始化
     */
    bool initialize(int maxThreads = QThread::idealThreadCount());

    /**
     * @brief 提交任务
     */
    template<typename T>
    QString submitTask(const QString& taskId,
                      typename AsyncTask<T>::TaskFunction func,
                      TaskPriority priority = TaskPriority::Normal,
                      typename AsyncTask<T>::CallbackFunction callback = nullptr);

    /**
     * @brief 提交带超时的任务
     */
    template<typename T>
    QString submitTaskWithTimeout(const QString& taskId,
                                 typename AsyncTask<T>::TaskFunction func,
                                 int timeoutMs,
                                 TaskPriority priority = TaskPriority::Normal,
                                 typename AsyncTask<T>::CallbackFunction callback = nullptr);

    /**
     * @brief 取消任务
     */
    bool cancelTask(const QString& taskId);

    /**
     * @brief 等待任务完成
     */
    template<typename T>
    TaskResult<T> waitForTask(const QString& taskId, int timeoutMs = -1);

    /**
     * @brief 等待所有任务完成
     */
    void waitForAll(int timeoutMs = -1);

    /**
     * @brief 获取任务状态
     */
    TaskState taskState(const QString& taskId) const;

    /**
     * @brief 获取活动任务数
     */
    int activeTaskCount() const;

    /**
     * @brief 获取待处理任务数
     */
    int pendingTaskCount() const;

    /**
     * @brief 获取统计信息
     */
    QMap<QString, QVariant> statistics() const;

    /**
     * @brief 清理已完成任务
     */
    void cleanupCompletedTasks();

signals:
    /**
     * @brief 任务完成信号
     */
    void taskCompleted(const QString& taskId);

    /**
     * @brief 任务失败信号
     */
    void taskFailed(const QString& taskId, const QString& error);

    /**
     * @brief 任务取消信号
     */
    void taskCancelled(const QString& taskId);

private:
    AsyncTaskManager();
    ~AsyncTaskManager();
    AsyncTaskManager(const AsyncTaskManager&) = delete;
    AsyncTaskManager& operator=(const AsyncTaskManager&) = delete;

    QThreadPool* m_threadPool;
    QMap<QString, QRunnable*> m_tasks;
    QMap<QString, TaskState> m_taskStates;
    mutable QMutex m_mutex;
    
    // 统计信息
    qint64 m_totalTasks;
    qint64 m_completedTasks;
    qint64 m_failedTasks;
    qint64 m_cancelledTasks;
};

// ========== 模板实现 ==========

template<typename T>
QString AsyncTaskManager::submitTask(const QString& taskId,
                                    typename AsyncTask<T>::TaskFunction func,
                                    TaskPriority priority,
                                    typename AsyncTask<T>::CallbackFunction callback)
{
    QMutexLocker locker(&m_mutex);
    
    // 包装回调函数，加入统计和状态更新
    auto wrappedCallback = [this, taskId, callback](const TaskResult<T>& result) {
        QMutexLocker locker(&m_mutex);
        if (result.success) {
            m_completedTasks++;
            m_taskStates[taskId] = TaskState::Completed;
            locker.unlock();
            emit taskCompleted(taskId);
        } else {
            m_failedTasks++;
            m_taskStates[taskId] = TaskState::Failed;
            locker.unlock();
            emit taskFailed(taskId, result.error);
        }
        if (callback) {
            callback(result);
        }
    };
    
    AsyncTask<T>* task = new AsyncTask<T>(taskId, func, priority);
    task->setCallback(wrappedCallback);
    
    m_tasks[taskId] = task;
    m_taskStates[taskId] = TaskState::Pending;
    m_totalTasks++;
    
    // 根据优先级设置
    int qtPriority = static_cast<int>(priority);
    m_threadPool->start(task, qtPriority);
    
    return taskId;
}

template<typename T>
QString AsyncTaskManager::submitTaskWithTimeout(const QString& taskId,
                                               typename AsyncTask<T>::TaskFunction func,
                                               int timeoutMs,
                                               TaskPriority priority,
                                               typename AsyncTask<T>::CallbackFunction callback)
{
    QString actualTaskId = submitTask<T>(taskId, func, priority, callback);
    
    // 设置超时定时器
    QTimer::singleShot(timeoutMs, this, [this, actualTaskId]() {
        if (taskState(actualTaskId) == TaskState::Running ||
            taskState(actualTaskId) == TaskState::Pending) {
            cancelTask(actualTaskId);
            QMutexLocker locker(&m_mutex);
            m_taskStates[actualTaskId] = TaskState::Timeout;
        }
    });
    
    return actualTaskId;
}

template<typename T>
TaskResult<T> AsyncTaskManager::waitForTask(const QString& taskId, int timeoutMs)
{
    Q_UNUSED(taskId);
    Q_UNUSED(timeoutMs);
    // 实现等待逻辑
    TaskResult<T> result;
    result.success = false;
    result.state = TaskState::Failed;
    return result;
}

#endif // ASYNCTASKMANAGER_H
