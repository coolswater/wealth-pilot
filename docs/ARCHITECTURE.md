# WealthPilot 架构文档

## 概述

WealthPilot 是一个基于 Qt 6.10.2 和 C++17 开发的金融信息展示与分析软件，专为 PC 平台设计。

## 技术栈

- **开发语言**: C++17
- **GUI 框架**: Qt 6.10.2
- **构建系统**: CMake 3.16+
- **国际化**: Qt Linguist Tools
- **操作系统**: Windows (主要), macOS (次要)

## 项目结构

```
wealth-pilot/
├── src/                    # 源代码目录
│   ├── ai/                 # AI服务模块
│   ├── analysis/           # 技术分析模块
│   ├── app/                # 应用初始化管理
│   ├── controllers/        # 控制器层
│   ├── core/               # 核心模块
│   │   ├── cache/          # 缓存管理
│   │   ├── config/         # 配置管理
│   │   ├── data/           # 数据管理
│   │   ├── database/       # 数据库管理
│   │   ├── datahub/        # 数据中心（发布订阅）
│   │   ├── di/             # 依赖注入
│   │   ├── navigation/     # 页面导航
│   │   ├── plugin/         # 插件系统
│   │   └── task/           # 异步任务管理
│   ├── ctp/                # CTP交易接口
│   ├── data/               # 数据层
│   ├── domain/             # 业务领域模型
│   ├── market/             # 市场数据
│   ├── models/             # 数据模型
│   ├── network/            # 网络通信
│   ├── plugins/            # 插件实现
│   ├── trading/            # 交易服务
│   ├── ui/                 # UI组件和样式
│   ├── utils/              # 工具类
│   ├── viewmodels/         # ViewModel层
│   └── views/              # 视图层
├── tests/                  # 测试代码
├── config/                 # 配置文件
├── logs/                   # 日志目录
└── external/               # 外部依赖
```

## 核心架构

### 1. 分层架构

```
┌─────────────────────────────────────────────────┐
│                    Views                         │  视图层 (UI)
├─────────────────────────────────────────────────┤
│                  ViewModels                      │  视图模型层
├─────────────────────────────────────────────────┤
│                 Controllers                      │  控制器层
├─────────────────────────────────────────────────┤
│                   Services                       │  服务层
├─────────────────────────────────────────────────┤
│                    Core                          │  核心层
├─────────────────────────────────────────────────┤
│                    Data                          │  数据层
└─────────────────────────────────────────────────┘
```

### 2. 核心模块

#### 2.1 依赖注入 (ServiceLocator)

```cpp
// 注册服务
ServiceLocator::instance().registerSingleton<IMyService, MyService>();

// 解析服务
auto service = ServiceLocator::instance().resolve<IMyService>();
```

#### 2.2 数据中心 (DataHub)

发布订阅模式的数据中心，支持多订阅者：

```cpp
// 订阅数据
DataHub::instance().subscribe(this, "market.AAPL", [](const QVariant& data) {
    // 处理数据
});

// 发布数据
DataHub::instance().publish("market.AAPL", quoteData);
```

#### 2.3 缓存管理 (CacheManager)

两级缓存系统（L1内存 + L2磁盘）：

```cpp
// 设置缓存
CacheManager::instance()->set("key", data, 60000); // 60秒过期

// 获取缓存
auto data = CacheManager::instance()->get<QByteArray>("key");
```

#### 2.4 异步任务管理 (AsyncTaskManager)

```cpp
// 执行异步任务
AsyncTaskManager::instance().run([]() {
    // 后台任务
    return result;
}).then([](const Result& result) {
    // 主线程回调
});
```

### 3. 插件系统

#### 3.1 插件接口

```cpp
class IPlugin : public QObject {
    Q_OBJECT
public:
    virtual PluginMetaData metaData() const = 0;
    virtual PluginState state() const = 0;
    virtual bool load() = 0;
    virtual bool initialize(const QJsonObject& config) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void unload() = 0;
};
```

#### 3.2 内置插件

- **CTPPlugin**: CTP交易接口插件
- **AIPlugin**: AI智能投顾插件

### 4. 初始化流程

```
ApplicationInitializer::initialize()
    │
    ├── initializeCore()
    │   ├── Logger
    │   ├── EnvironmentConfig
    │   ├── CacheManager
    │   └── DataHub
    │
    ├── initializeServices()
    │   ├── DataService
    │   ├── TradingService
    │   └── AIService
    │
    ├── initializePlugins()
    │   ├── CTPPlugin
    │   └── AIPlugin
    │
    └── initializeUI()
        └── ThemeManager
```

## 配置管理

### 配置文件

| 文件                        | 说明      |
|---------------------------|---------|
| `config/dataSources.json` | 数据源配置   |
| `config/ctp_config.json`  | CTP交易配置 |
| `config/ai_config.json`   | AI服务配置  |

### 配置加载

```cpp
// 加载数据源配置
DataSourceConfig::instance().loadFromFile("config/dataSources.json");

// 加载CTP配置
PluginConfigLoader::instance().loadCTPConfig("config/ctp_config.json");

// 加载AI配置
PluginConfigLoader::instance().loadAIConfig("config/ai_config.json");
```

## 数据流

```
用户操作 → View → ViewModel → Controller → Service → DataHub → DataSource
                                                    ↓
                                              CacheManager
                                                    ↓
                                              DatabaseManager
```

## 主题系统

支持深色/浅色主题切换：

```cpp
// 切换主题
ThemeManager::instance()->setTheme(ThemeType::Dark);

// 监听主题变化
connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, &MyClass::onThemeChanged);
```

## 日志系统

```cpp
LOG_DEBUG("Debug message");
LOG_INFO("Info message");
LOG_WARNING("Warning message");
LOG_ERROR("Error message");
```

日志文件位置: `logs/wealthpilot-YYYY-MM-DD.log`

## 测试

### 运行测试

```bash
# 编译测试
cmake --build . --target TestServiceLocator TestCacheManager

# 运行测试
./TestServiceLocator.exe
./TestCacheManager.exe
```

### 测试覆盖

- TestServiceLocator: 服务定位器测试
- TestCacheManager: 缓存管理器测试
- TestPluginLoader: 插件加载器测试
- TestApplicationInitializer: 应用初始化测试
- PerformanceTest: 性能测试
- AnalysisTest: 分析模块测试

## 扩展开发

### 添加新页面

1. 在 `src/views/` 创建页面类
2. 在 `src/viewmodels/` 创建对应的 ViewModel
3. 在 `MainWindow::createPages()` 注册页面

### 添加新数据源

1. 继承 `IDataSource` 接口
2. 在 `config/dataSources.json` 添加配置
3. 在 `DataSourceManager` 注册数据源

### 添加新插件

1. 继承 `IPlugin` 接口
2. 实现必要的虚函数
3. 在 `ApplicationInitializer::initializePlugins()` 注册插件

## 性能优化建议

1. 使用 `CacheManager` 缓存频繁访问的数据
2. 使用 `DataHub` 的批量订阅减少信号连接
3. 使用 `AsyncTaskManager` 处理耗时操作
4. 避免在主线程执行网络请求

## 已知问题

1. CTP/AI 插件需要配置账号信息才能启用
2. 部分页面功能仍在开发中
3. 测试覆盖率需要提高

## 版本历史

- v1.0.0: 初始版本，基础功能完成
- v2.0.0: 架构重构，引入依赖注入和数据中心

---

*文档最后更新: 2026-05-15*