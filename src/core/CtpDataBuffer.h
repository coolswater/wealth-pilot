#ifndef CTPDATABUFFER_H
#define CTPDATABUFFER_H

/////////////////////////////////////////////////////////////////////////
///@file CtpDataBuffer.h
///@brief 批量缓冲队列管理 - 提升高频行情处理性能
/////////////////////////////////////////////////////////////////////////

#include <QtCore/QObject>
#include <QtCore/QQueue>
#include <QtCore/QMutex>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <functional>  // 添加头文件
#include <vector>

namespace CTP {

/**
 * @brief 批量缓冲模板类
 * @tparam T 数据类型
 * @details 使用双缓冲机制，将高频数据收集后批量发射，降低信号发射频率
 *
 * 注意：由于模板类不支持 Q_OBJECT 宏，我们使用 std::function 回调替代 Qt 信号
 */
template<typename T>
class BatchBuffer : public QObject {
    // 移除了 Q_OBJECT 宏，因为 MOC 不支持模板类

public:
    explicit BatchBuffer(QObject *parent = nullptr) : QObject(parent) {};
    explicit BatchBuffer(int batchSize = 100, int flushIntervalMs = 16,
                         QObject *parent = nullptr)
        : QObject(parent)
        , m_batchSize(batchSize)
        , m_flushIntervalMs(flushIntervalMs)
        , m_callback(nullptr) {  // 初始化回调为空

        // 使用Qt::PreciseTimer保证精度
        m_timer.setTimerType(Qt::PreciseTimer);
        m_timer.setInterval(flushIntervalMs);
        // 使用函数指针语法连接，无需 MOC 支持
        connect(&m_timer, &QTimer::timeout, this, &BatchBuffer::flush);
        m_timer.start();
    }

    /**
     * @brief 添加数据到缓冲
     * @details 线程安全，支持多线程生产
     */
    void push(T&& data) {
        QMutexLocker locker(&m_mutex);
        m_buffer.push_back(std::move(data));

        // 达到批量大小立即刷新（C++17 guaranteed copy elision）
        if (static_cast<int>(m_buffer.size()) >= m_batchSize) {
            // 使用 Qt::QueuedConnection 确保线程安全
            QMetaObject::invokeMethod(this, &BatchBuffer::flush,
                                      Qt::QueuedConnection);
        }
    }

    void push(const T& data) {
        QMutexLocker locker(&m_mutex);
        m_buffer.push_back(data);
        if (static_cast<int>(m_buffer.size()) >= m_batchSize) {
            QMetaObject::invokeMethod(this, &BatchBuffer::flush,
                                      Qt::QueuedConnection);
        }
    }

    /**
     * @brief 设置数据就绪时的回调函数
     * @param callback 回调函数，参数为批量数据的 const 引用
     *
     * 使用方式：
     * @code
     * buffer->setCallback([](const std::vector<MyData>& data) {
     *     // 处理批量数据
     * });
     * @endcode
     */
    void setCallback(std::function<void(const std::vector<T>&)> callback) {
        QMutexLocker locker(&m_callbackMutex);
        m_callback = callback;
    }

    /**
     * @brief 清除回调函数
     */
    void clearCallback() {
        QMutexLocker locker(&m_callbackMutex);
        m_callback = nullptr;
    }

    /**
     * @brief 强制刷新缓冲
     */
    void flush() {
        std::vector<T> temp;
        {
            QMutexLocker locker(&m_mutex);
            if (m_buffer.empty()) return;
            temp.swap(m_buffer);  // C++17 O(1)交换
        }

        // 触发回调（替代原来的 emit batchReady）
        std::function<void(const std::vector<T>&)> callback;
        {
            QMutexLocker locker(&m_callbackMutex);
            callback = m_callback;
        }
        if (callback) {
            callback(temp);
        }
    }

    /**
     * @brief 清空缓冲
     */
    void clear() {
        QMutexLocker locker(&m_mutex);
        m_buffer.clear();
        m_buffer.shrink_to_fit();
    }

    // 为了兼容性，保留旧的信号函数声明（但实际通过回调实现）
    // 注意：这些不再是 Qt 信号，只是普通的回调包装
    void batchReady(const std::vector<T>& data) {
        // 此方法保留用于兼容旧代码的直接调用
        std::function<void(const std::vector<T>&)> callback;
        {
            QMutexLocker locker(&m_callbackMutex);
            callback = m_callback;
        }
        if (callback) {
            callback(data);
        }
    }

private:
    std::vector<T> m_buffer;              // C++17 vector优化
    QMutex m_mutex;                      // 数据缓冲区互斥锁
    QMutex m_callbackMutex;              // 回调函数互斥锁（线程安全）
    QTimer m_timer;                      // 定时刷新
    int m_batchSize;
    int m_flushIntervalMs;
    std::function<void(const std::vector<T>&)> m_callback;  // 回调函数替代信号
};

} // namespace CTP

#endif // CTPDATABUFFER_H
