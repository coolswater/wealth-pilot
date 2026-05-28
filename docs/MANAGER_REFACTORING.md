# Manager 类职责重构方案

## 现状分析

当前项目中存在 28 个 Manager/Service 类，职责模糊，命名混乱。

## 职责定义

### Manager（管理者）

- **职责**：资源管理、生命周期控制
- **特点**：持有资源、管理状态、协调资源
- **示例**：`ConfigManager`（配置资源管理）、`CacheManager`（缓存资源管理）

### Service（服务）

- **职责**：业务逻辑、对外接口
- **特点**：无状态或少量状态、提供业务功能、可被替换
- **示例**：`TradingService`（交易业务逻辑）、`AIService`（AI业务逻辑）

### Controller（控制器）

- **职责**：协调逻辑、流程编排
- **特点**：协调多个Service/Manager、处理用户交互、业务流程控制
- **示例**：`DashboardController`（看板流程控制）、`TradingController`（交易流程控制）

### Repository（仓储）

- **职责**：数据访问、持久化
- **特点**：封装数据库/API访问、CRUD操作、数据查询
- **示例**：`QuoteRepository`（行情数据访问）、`UserRepository`（用户数据访问）

## 重构映射表

| 当前类名                     | 新命名                      | 新职责            | 新类别        |
|--------------------------|--------------------------|----------------|------------|
| DataSourceManager        | DataSourceRegistry       | 数据源注册与发现       | Manager    |
| FavoritesManager         | FavoritesRepository      | 自选股数据持久化       | Repository |
| QuoteDataManager         | QuoteRepository          | 行情数据访问         | Repository |
| AnalysisManager          | AnalysisService          | 分析业务逻辑         | Service    |
| SignalService            | SignalService            | 信号业务逻辑（不变）     | Service    |
| OrderManager             | OrderRepository          | 订单数据持久化        | Repository |
| PositionManager          | PositionRepository       | 持仓数据持久化        | Repository |
| TradingService           | TradingService           | 交易业务逻辑（不变）     | Service    |
| AIConversationManager    | AIConversationRepository | AI对话数据持久化      | Repository |
| AIService                | AIService                | AI业务逻辑（不变）     | Service    |
| SentimentAnalysisService | SentimentAnalysisService | 情绪分析业务逻辑（不变）   | Service    |
| SmartReportService       | SmartReportService       | 智能报告业务逻辑（不变）   | Service    |
| DataAPIManager           | DataAPIService           | 数据API业务逻辑      | Service    |
| ConfigManager            | ConfigRepository         | 配置数据访问         | Repository |
| CTPConfigManager         | CTPConfigRepository      | CTP配置数据访问      | Repository |
| CTPService               | CTPService               | CTP业务逻辑（不变）    | Service    |
| DatabaseManager          | DatabaseConnectionPool   | 数据库连接池管理       | Manager    |
| WebSocketManager         | WebSocketConnectionPool  | WebSocket连接池管理 | Manager    |
| NetworkManager           | NetworkService           | 网络业务逻辑         | Service    |
| PluginMarketManager      | PluginMarketService      | 插件市场业务逻辑       | Service    |
| AnimationManager         | AnimationController      | 动画流程控制         | Controller |
| DrawingToolManager       | DrawingToolController    | 绘图工具流程控制       | Controller |
| LayoutManager            | LayoutController         | 布局流程控制         | Controller |
| ShortcutManager          | ShortcutController       | 快捷键流程控制        | Controller |
| ThemeManager             | ThemeController          | 主题流程控制         | Controller |
| MultiAccountManager      | AccountRepository        | 账户数据持久化        | Repository |
| AlertNotificationService | AlertNotificationService | 告警通知业务逻辑（不变）   | Service    |
| CacheManager             | CacheManager             | 缓存资源管理（不变）     | Manager    |
| DataCacheManager         | DataCacheManager         | 数据缓存资源管理（不变）   | Manager    |
| UserFeedbackManager      | UserFeedbackService      | 用户反馈业务逻辑       | Service    |
| PageNavigatorManager     | PageNavigatorController  | 页面导航流程控制       | Controller |
| PerformanceManager       | PerformanceMonitor       | 性能监控管理         | Manager    |
| PermissionManager        | PermissionService        | 权限业务逻辑         | Service    |
| StrategyShareManager     | StrategyShareService     | 策略分享业务逻辑       | Service    |
| AsyncTaskManager         | AsyncTaskExecutor        | 异步任务执行管理       | Manager    |

## 重构步骤

### Phase 1: Repository 分离（低风险）

1. 创建 `Repository` 基类
2. 将数据访问逻辑从 Manager 提取到 Repository
3. 涉及类：FavoritesManager, QuoteDataManager, OrderManager, PositionManager

### Phase 2: Controller 创建（中等风险）

1. 创建 `Controller` 基类
2. 将协调逻辑从 Service 提取到 Controller
3. 涉及类：AnimationManager, DrawingToolManager, LayoutManager, ThemeManager

### Phase 3: Service 重命名（低风险）

1. 重命名纯业务逻辑类为 Service
2. 涉及类：DataAPIManager, PluginMarketManager, UserFeedbackManager

### Phase 4: Manager 清理（高风险）

1. 合并重复的 Manager（如 CacheManager 和 DataCacheManager）
2. 删除无用的 Manager
3. 涉及类：所有 Manager 类

## 命名约定

### 命名格式

- Manager: `XxxManager`（资源管理）
- Service: `XxxService`（业务逻辑）
- Controller: `XxxController`（流程控制）
- Repository: `XxxRepository`（数据访问）

### 文件组织

```
src/
├── core/
│   ├── services/         # Service 类
│   ├── repositories/     # Repository 类
│   ├── controllers/      # Controller 类
│   └── managers/         # Manager 类
├── data/
│   ├── repositories/     # 数据相关 Repository
│   └── services/         # 数据相关 Service
├── presentation/
│   ├── controllers/      # UI 相关 Controller
│   └── managers/         # UI 资源管理
└── infrastructure/
    ├── managers/         # 基础设施资源管理
    └── services/         # 基础设施业务逻辑
```

## 基类设计

### Repository 基类

```cpp
class IRepository {
public:
    virtual ~IRepository() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
};

template<typename T>
class Repository : public IRepository {
public:
    virtual T findById(const QString& id) = 0;
    virtual QVector<T> findAll() = 0;
    virtual bool save(const T& entity) = 0;
    virtual bool deleteById(const QString& id) = 0;
};
```

### Service 基类

```cpp
class IService {
public:
    virtual ~IService() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual QString serviceName() const = 0;
};
```

### Controller 基类

```cpp
class IController {
public:
    virtual ~IController() = default;
    virtual void initialize() = 0;
    virtual void shutdown() = 0;
    virtual void execute() = 0;
};
```

### Manager 基类

```cpp
class IManager {
public:
    virtual ~IManager() = default;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual QString managerName() const = 0;
};
```

## 风险评估

| 阶段      | 风险等级 | 预计工作量 | 影响范围  |
|---------|------|-------|-------|
| Phase 1 | 低    | 2天    | 数据访问层 |
| Phase 2 | 中    | 3天    | UI层   |
| Phase 3 | 低    | 1天    | 命名调整  |
| Phase 4 | 高    | 5天    | 全局    |

## 注意事项

1. **保持向后兼容**：重构期间保留旧类名，通过别名或继承桥接
2. **渐进式重构**：每次只重构一个类，编译验证后再继续
3. **测试覆盖**：每个重构步骤前添加单元测试
4. **文档更新**：同步更新架构文档和注释