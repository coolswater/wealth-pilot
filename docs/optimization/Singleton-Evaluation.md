# Singleton 使用评估报告

## 现状分析

项目中使用 Singleton 的类：

| 类名                | 用途    | 是否适合单例  | 建议 |
|-------------------|-------|---------|----|
| CacheManager      | 缓存管理  | ✅ 合适    | 保持 |
| AIService         | AI 服务 | ✅ 合适    | 保持 |
| ConfigManager     | 配置管理  | ✅ 合适    | 保持 |
| EnvironmentConfig | 环境配置  | ✅ 合适    | 保持 |
| DatabaseManager   | 数据库管理 | ✅ 合适    | 保持 |
| NetworkManager    | 网络管理  | ✅ 合适    | 保持 |
| ChartConfig       | 图表配置  | ❌ 可能不合适 | 评估 |

## 评估标准

**适合单例的场景**：

- 全局唯一资源（数据库连接池、网络管理）
- 配置类（系统级配置）
- 服务类（需要全局访问）

**不适合单例的场景**：

- 可能需要多实例
- 测试时需要 mock
- 状态可变的业务对象

## 分析结论

### ChartConfig - 需要评估

当前实现：

```cpp
class ChartConfig : public QObject, public Singleton<ChartConfig>
```

问题：

- 图表配置可能需要多套（不同图表类型）
- 测试时难以 mock
- 用户可能需要自定义主题

建议：

- 改为普通类，通过 ThemeManager 管理
- 或者保持单例，但增加"配置快照"功能

### 其他 Singleton - 保持不变

其他 6 个类都是基础设施类，单例模式合适：

1. **CacheManager** - 全局缓存，必须唯一
2. **AIService** - AI 服务入口，全局统一
3. **ConfigManager** - 系统配置，全局唯一
4. **EnvironmentConfig** - 环境变量，全局唯一
5. **DatabaseManager** - 连接池，必须唯一
6. **NetworkManager** - 网络请求，统一管理

## 依赖注入替代方案

如果未来需要更灵活的架构，可以考虑：

### 方案：服务定位器模式

```cpp
// ServiceLocator.h
class ServiceLocator {
public:
    static void registerService(const QString& name, QObject* service);
    static QObject* getService(const QString& name);
    
    template<typename T>
    static T* get() {
        return qobject_cast<T*>(getService(T::serviceName()));
    }
};

// 使用
auto* cache = ServiceLocator::get<CacheManager>();
```

优点：

- 支持运行时替换
- 测试时可以注入 mock
- 不修改现有 Singleton

## 建议

**当前阶段**：保持现有 Singleton 结构

**理由**：

1. 7 个 Singleton 都有合理用途
2. 项目规模不大，单例足够
3. 重构风险大于收益

**未来改进**：

- 当项目规模扩大时，考虑引入 ServiceLocator
- 为 ChartConfig 增加"配置快照"功能
- 单元测试时使用 #define 跳过单例检查