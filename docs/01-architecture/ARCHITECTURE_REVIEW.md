# WealthPilot 架构审查报告

**审查日期**: 2026-04-14  
**审查人**: 架构师  
**项目版本**: 1.0.0

---

## 一、核心架构问题

### 1. Singleton 模式不一致 ⚠️ 高优先级

**问题描述**: Singleton::instance() 返回指针，但部分类自己实现 instance() 返回引用

**当前状态**:

```cpp
// Singleton 模板返回指针
Singleton<T>::instance() → T*

// 但这些类自己实现返回引用
ServiceLocator::instance() → ServiceLocator&
ApplicationInitializer::instance() → ApplicationInitializer&
ThemeEngine::instance() → ThemeEngine&
PluginLoader::instance() → PluginLoader&
```

**影响**: 调用方需要记住每个类的返回类型，容易出错

**建议**:

1. 统一使用 `Singleton<T>` 模板
2. 全部返回指针 `T*`
3. 提供 `ref()` 方法返回引用（可选）

---

### 2. 依赖注入混乱 ⚠️ 高优先级

**问题描述**: ServiceLocator 和 Singleton 混用，职责不清

**当前状态**:

- 有 ServiceLocator 作为 DI 容器
- 但大多数服务直接用 Singleton 模式
- MainWindow 中手动注册服务

**建议**:

```
核心服务 → ServiceLocator 注册
  - ICTPPlugin (CTP服务)
  - IAIPlugin (AI服务)
  - DatabaseManager
  - CacheManager

工具类 → Singleton 模式
  - Logger
  - ThemeEngine
  - ConfigManager
```

---

### 3. 命名空间使用不一致 ⚠️ 中优先级

**有命名空间**:

```cpp
CTP::CTPPlugin, CTP::MarketData
AI::AIPlugin
WealthPilot::Futures::registerPages()
ConfigKeys::*, Tokens::*
```

**无命名空间**:

```cpp
Logger, CacheManager, DatabaseManager
BasePage, PageNavigator
FuturesQuoteModel, StockQuoteItem
```

**建议**: 统一使用 `WealthPilot` 顶层命名空间

```cpp
namespace WealthPilot {
    namespace Core { /* ... */ }
    namespace Models { /* ... */ }
    namespace Views { /* ... */ }
    namespace Plugins { /* ... */ }
}
```

---

## 二、目录结构问题

### 4. 空目录和职责重叠 ⚠️ 中优先级

**空目录**:

```
src/controllers/     - 应删除或添加说明
src/views/cryptoCurrency/
src/views/forex/
src/views/fund/
src/views/profile/
src/views/user/
src/views/usStock/
```

**职责重叠**:

```
src/services/ vs src/plugins/ - 服务和插件界限不清
src/core/ vs src/utils/ - 工具类位置不一致
```

**建议目录结构**:

```
src/
├── core/           # 核心框架（Singleton, ServiceLocator, BasePage）
├── services/       # 业务服务（合并 plugins）
│   ├── ctp/        # CTP服务
│   └── ai/         # AI服务
├── models/         # 数据模型
├── views/          # UI视图
│   ├── widgets/    # 通用组件
│   ├── pages/      # 页面（合并各子目录）
│   └── mainWindow/
├── network/        # 网络层
└── utils/          # 纯工具类（无业务逻辑）
```

---

## 三、代码风格问题

### 5. 头文件注释风格不统一 ⚠️ 低优先级

**风格1 - 完整 Doxygen** (推荐):

```cpp
/**
 * @file Singleton.h
 * @brief 线程安全的单例模板基类
 * @author WealthPilot Team
 * @version 2.0.0
 */
```

**风格2 - 简单注释**:

```cpp
// TitleBarWidget.h
```

**风格3 - 无文件头**:

```cpp
#ifndef RESULT_H
```

**建议**: 统一使用 Doxygen 风格

---

### 6. PIMPL 模式使用不一致 ⚠️ 中优先级

**使用 PIMPL**:

- FuturesKLinePage, TradingPanel, RealtimeQuoteWidget
- KLineChart, CTPPlugin, AIPlugin

**未使用 PIMPL**:

- DashboardPage (直接在类中定义成员)
- Logger, CacheManager

**建议**: 所有公开头文件统一使用 PIMPL 模式

```cpp
// .h 文件
class MyService {
public:
    MyService();
    ~MyService();
private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

// .cpp 文件
struct MyService::Impl {
    // 所有成员变量
};
```

---

## 四、错误处理问题

### 7. 错误处理机制不统一 ⚠️ 高优先级

**问题**: 有 `Result<T>` 类型但使用不一致

**当前状态**:

- `src/utils/Result.h` 定义了完整的 Result<T>
- 大多数函数返回 bool 表示成功/失败
- 错误信息通过 LOG_ERROR 输出
- 没有统一的错误码体系

**建议**:

1. 定义统一错误码:

```cpp
enum class ErrorCode {
    // 网络
    NetworkTimeout = 1001,
    NetworkError = 1002,
    
    // 数据库
    DatabaseOpenFailed = 2001,
    DatabaseQueryFailed = 2002,
    
    // CTP
    CtpConnectFailed = 3001,
    CtpLoginFailed = 3002,
    
    // AI
    AiRequestFailed = 4001,
    AiParseError = 4002,
};
```

2. 所有可能失败的操作返回 `Result<T>`:

```cpp
// Before
bool connect(const QString& server);

// After
Result<void> connect(const QString& server);
```

---

## 五、性能和线程安全

### 8. 线程安全标记不清晰 ⚠️ 中优先级

**问题**: 无法从类声明看出是否线程安全

**建议**: 使用标记明确说明

```cpp
class CacheManager : public Singleton<CacheManager> {
    // THREAD_SAFE: 所有公共方法都是线程安全的
    // 使用 QMutexLocker 保护内部状态
};
```

---

## 六、优化实施计划

### Phase 1: 紧急修复 (1-2天)

1. ✅ 统一 Singleton 返回类型（已完成）
2. ✅ 修复编译错误（已完成）
3. 🔄 添加缺失的页面实现

### Phase 2: 架构重构 (3-5天)

1. 统一命名空间
2. 合并 services 和 plugins
3. 清理空目录
4. 统一 PIMPL 模式

### Phase 3: 代码质量 (持续)

1. 统一头文件注释
2. 统一错误处理
3. 添加线程安全标记
4. 完善单元测试

---

## 七、具体代码示例

### 推荐的服务类模板:

```cpp
/**
 * @file MyService.h
 * @brief 服务描述
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef WEALTHPILOT_CORE_MYSERVICE_H
#define WEALTHPILOT_CORE_MYSERVICE_H

#include "Singleton.h"
#include "Result.h"
#include <QObject>
#include <memory>

namespace WealthPilot::Core {

class MyService : public QObject, public Singleton<MyService>
{
    Q_OBJECT
    friend class Singleton<MyService>;

public:
    ~MyService() override;
    
    // 初始化
    Result<void> initialize();
    void shutdown();
    
    // 业务方法
    Result<Data> fetchData(const QString& id);
    
signals:
    void dataUpdated(const Data& data);
    void errorOccurred(const Error& error);

private:
    MyService();
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot::Core

#endif // WEALTHPILOT_CORE_MYSERVICE_H
```

---

## 八、总结

| 问题类型 | 数量 | 优先级分布  |
|------|----|--------|
| 核心架构 | 3  | 高2, 中1 |
| 目录结构 | 1  | 中1     |
| 代码风格 | 2  | 中1, 低1 |
| 错误处理 | 1  | 高1     |
| 线程安全 | 1  | 中1     |

**建议优先处理**:

1. Singleton 返回类型统一 ✅
2. 错误处理机制统一
3. 命名空间统一
4. 目录结构整理
