# 线程与 QWaitCondition 安全分析报告

**生成时间**: 2026-05-27  
**项目**: WealthPilot  
**目的**: 排查关闭时崩溃问题，确保线程安全退出

---

## 一、继承 QThread 的类

### 1. AsyncQueryThread (DatabaseManager.h:96)

```cpp
class AsyncQueryThread : public QThread
```

**用途**: 数据库异步查询线程  
**QWaitCondition**: `m_condition` (行127)  
**状态**: ✅ **已正确处理**

**析构逻辑** (DatabaseManager.cpp:798-824):

```cpp
// 1. 标记 pool 无效
m_asyncThread->invalidatePool();

// 2. 停止线程
m_asyncThread->stop();  // 设置 m_running = false, wakeAll()

// 3. 释放锁让线程退出
locker.unlock();

// 4. quit() + wait(5秒超时)
m_asyncThread->quit();
if (!m_asyncThread->wait(5000)) {
    LOG_WARNING("AsyncQueryThread did not stop gracefully, terminating...");
    m_asyncThread->terminate();
    m_asyncThread->wait();
}

// 5. 重置智能指针
m_asyncThread.reset();
```

**问题**: ⚠️ `terminate()` 是危险操作，可能导致资源泄漏

---

### 2. MarketWorker (CtpMarketSpi.h:108)

```cpp
class MarketWorker : public QThread {
    void run() override {
        thread_local bool initialized = false;
        if (!initialized) {
            initialized = true;
            exec();  // 进入Qt事件循环
        }
    }
};
```

**用途**: CTP 行情工作线程  
**QWaitCondition**: 无  
**状态**: ⚠️ **潜在问题**

**问题分析**:

1. `MarketWorker` 使用 `QScopedPointer<MarketWorker> worker` 管理 (CtpMarketSpi.cpp:35)
2. **析构函数** `CtpMarketSpi::~CtpMarketSpi() = default` (行58)
3. **缺失**: 没有在析构前调用 `worker->quit()` + `worker->wait()`
4. **风险**: QScopedPointer 析构时会直接 delete 线程对象，但线程可能还在运行

---

## 二、QWaitCondition 成员变量

### 1. ConnectionPool::m_condition (DatabaseManager.h:88)

**用途**: 连接池等待条件  
**状态**: ✅ **已正确处理**

**cleanup() 逻辑** (DatabaseManager.cpp:43-103):

```cpp
// 1. 设置关闭标志
m_shuttingDown = true;
m_condition.wakeAll();  // 唤醒所有等待的线程

// 2. 等待连接返回（最多5秒）
while (!m_usedConnections.isEmpty() && waitTimer.elapsed() < 5000) {
    QThread::msleep(100);
}

// 3. 强制关闭剩余连接
```

---

### 2. AsyncQueryThread::m_condition (DatabaseManager.h:127)

**用途**: 任务队列等待条件  
**状态**: ✅ **已正确处理**

**stop() 逻辑** (DatabaseManager.cpp:284-288):

```cpp
void AsyncQueryThread::stop() {
    m_running = false;
    m_condition.wakeAll();  // 唤醒等待中的线程
}
```

---

### 3. AsyncTaskManager (AsyncTaskManager.h:24)

**用途**: 异步任务管理器  
**QWaitCondition**: 包含头文件但未使用成员变量  
**状态**: ✅ 无问题

---

## 三、关闭流程分析

### main.cpp 关闭顺序 (行65-85)

```cpp
void cleanupServices() {
    // 1. ApplicationInitializer 按逆序关闭所有模块（含插件）
    ApplicationInitializer::instance().shutdown();

    // 2. 显式关闭 DataHub
    WealthPilot::DataHub::DataHub::instance().shutdown();

    // 3. 等待线程池完成（最多30秒）
    QThreadPool::globalInstance()->waitForDone(30000);

    // 4. ServiceLifecycle 按逆序关闭注册服务
    WealthPilot::ServiceLifecycle::instance()->shutdownAll();
}
```

**触发时机**: `QApplication::aboutToQuit` 信号 (main.cpp:211)

---

### ApplicationInitializer::shutdown() (ApplicationInitializer.cpp:140-160)

```cpp
// 1. 调用所有模块的 shutdownFunc
for (const auto& module : m_modules[InitPhase::Core]) {
    if (module.shutdownFunc) {
        module.shutdownFunc();
    }
}

// 2. 等待全局线程池完成
QThreadPool::globalInstance()->waitForDone(30000);

// 3. 清理服务定位器
ServiceLocator::instance().clear();
```

---

## 四、发现的问题

### 🔴 问题 1: MarketWorker 未正确关闭

**文件**: `src/infrastructure/ctp/api/CtpMarketSpi.cpp:58`

```cpp
CtpMarketSpi::~CtpMarketSpi() = default;  // 直接析构，未停止线程
```

**风险**:

- `QScopedPointer<MarketWorker> worker` 析构时直接 delete
- 如果 `MarketWorker` 还在运行 `exec()`，会导致崩溃

**修复方案**:

```cpp
CtpMarketSpi::~CtpMarketSpi() {
    // 停止 MarketWorker 线程
    if (d->worker && d->worker->isRunning()) {
        d->worker->quit();
        if (!d->worker->wait(5000)) {
            LOG_WARNING("MarketWorker did not stop gracefully");
            d->worker->terminate();
            d->worker->wait();
        }
    }
    // QScopedPointer 会自动 delete
}
```

---

### 🔴 问题 2: CTPService::disconnect() 未停止 SPI

**文件**: `src/infrastructure/ctp/service/CTPService.cpp:42-44`

```cpp
CTPService::~CTPService() {
    disconnect();  // 只断开信号连接，未停止线程
}
```

**修复方案**:

```cpp
CTPService::~CTPService() {
    // 停止行情 SPI
    if (d->marketSpi) {
        d->marketSpi->release();  // 释放 CTP API
        delete d->marketSpi;
        d->marketSpi = nullptr;
    }
    
    // 停止交易 SPI
    if (d->tradingSpi) {
        d->tradingSpi->release();
        delete d->tradingSpi;
        d->tradingSpi = nullptr;
    }
}
```

---

### ⚠️ 问题 3: terminate() 使用

**文件**: `DatabaseManager.cpp:818`

```cpp
m_asyncThread->terminate();  // 强制终止线程
```

**风险**:

- `terminate()` 不会执行栈展开，资源不会释放
- 可能导致死锁或内存泄漏

**建议**:

- 保留 terminate() 作为最后手段
- 但要确保被 terminate 的线程没有持有锁

---

### ⚠️ 问题 4: 缺少线程进入/退出日志

**现状**: 无法追踪线程生命周期

**建议**: 在关键线程添加详细日志

```cpp
void AsyncQueryThread::run() {
    LOG_INFO("AsyncQueryThread started, tid=" + QString::number((qint64)QThread::currentThreadId()));
    
    while (m_running) {
        // ...
    }
    
    LOG_INFO("AsyncQueryThread exiting");
}
```

---

## 五、优化建议

### 建议 1: 添加线程生命周期日志

**范围**: AsyncQueryThread, MarketWorker, ConnectionPool

**实现**:

```cpp
// 线程进入
LOG_INFO(QString("[Thread] %1 started, tid=%2")
    .arg(metaObject()->className())
    .arg((qint64)QThread::currentThreadId()));

// 线程退出
LOG_INFO(QString("[Thread] %1 exiting")
    .arg(metaObject()->className()));

// QWaitCondition 析构前后
LOG_INFO("QWaitCondition before destruction");
// ... destruct
LOG_INFO("QWaitCondition after destruction");
```

---

### 建议 2: 使用 RAII 线程守卫

```cpp
class ThreadGuard {
    QThread* m_thread;
    int m_timeoutMs;
public:
    ThreadGuard(QThread* thread, int timeoutMs = 5000)
        : m_thread(thread), m_timeoutMs(timeoutMs) {}
    
    ~ThreadGuard() {
        if (m_thread && m_thread->isRunning()) {
            m_thread->quit();
            if (!m_thread->wait(m_timeoutMs)) {
                LOG_WARNING("ThreadGuard: force terminating thread");
                m_thread->terminate();
                m_thread->wait();
            }
        }
    }
};
```

---

### 建议 3: 统一线程关闭流程

**标准模式**:

```cpp
void shutdownThread(QThread* thread, const QString& name, int timeoutMs = 5000) {
    if (!thread || !thread->isRunning()) return;
    
    LOG_INFO(QString("Stopping thread: %1").arg(name));
    
    thread->quit();
    
    if (!thread->wait(timeoutMs)) {
        LOG_WARNING(QString("Thread %1 did not stop in %2ms, terminating")
            .arg(name).arg(timeoutMs));
        thread->terminate();
        thread->wait();
    }
    
    LOG_INFO(QString("Thread %1 stopped").arg(name));
}
```

---

### 建议 4: 检查所有 delete thread

**搜索结果**: 未发现直接 `delete thread` 的代码 ✅

---

## 六、修复优先级

| 优先级   | 问题                   | 影响    | 修复难度 |
|-------|----------------------|-------|------|
| 🔴 P0 | MarketWorker 未正确关闭   | 关闭崩溃  | 低    |
| 🔴 P0 | CTPService 析构未停止 SPI | 资源泄漏  | 低    |
| ⚠️ P1 | 添加线程生命周期日志           | 难以调试  | 低    |
| ⚠️ P1 | 统一线程关闭流程             | 代码一致性 | 中    |
| ℹ️ P2 | 移除 terminate()       | 理论风险  | 中    |

---

## 七、验证清单

- [ ] 修复 CtpMarketSpi 析构函数
- [ ] 修复 CTPService 析构函数
- [ ] 添加 AsyncQueryThread 日志
- [ ] 添加 MarketWorker 日志
- [ ] 添加 ConnectionPool 日志
- [ ] 编译验证
- [ ] 运行测试：正常关闭
- [ ] 运行测试：快速关闭（关闭时正在查询）
- [ ] 运行测试：CTP 连接状态下关闭

---

## 八、总结

**主要问题**:

1. `MarketWorker` 和 `CTPService` 析构时未停止线程
2. 缺少线程生命周期日志，难以定位问题

**修复方案**:

1. 在析构函数中添加 `quit()` + `wait()` 逻辑
2. 添加详细日志追踪线程进入/退出
3. 考虑使用 RAII 守卫统一管理

**预期效果**:

- 消除关闭时的崩溃
- 提供详细的线程生命周期日志
- 便于未来排查类似问题
