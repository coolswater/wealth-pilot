# WealthPilot 架构文档

## 1. 项目概述

WealthPilot 是一个基于 Qt 6.10.2 和 C++17 开发的金融信息展示与分析软件，专为 PC 平台设计。

### 1.1 核心功能

- 股票、期货、外汇、基金和数字货币实时数据展示
- 自选股管理
- 市场全景视图
- 数据分析和图表展示
- AI 辅助投资建议

### 1.2 技术栈

- **开发语言**: C++17
- **GUI 框架**: Qt 6.10.2
- **构建系统**: CMake 3.16+
- **国际化**: Qt Linguist Tools
- **平台**: Windows / macOS

## 2. 架构设计

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                      Presentation Layer                       │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │ Views       │  │ Components  │  │ Widgets     │          │
│  │ (Pages)     │  │ (Reusable)  │  │ (Custom)    │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
├─────────────────────────────────────────────────────────────┤
│                      Application Layer                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │ Services    │  │ Controllers │  │ ViewModel   │          │
│  │ (Business)  │  │ (Logic)     │  │ (MVVM)      │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
├─────────────────────────────────────────────────────────────┤
│                      Data Layer                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │ DataHub     │  │ Storage     │  │ Sources     │          │
│  │ (Pub/Sub)   │  │ (Persist)   │  │ (External)  │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
├─────────────────────────────────────────────────────────────┤
│                      Core Layer                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │
│  │ DI          │  │ Config      │  │ Utils       │          │
│  │ (ServiceLoc)│  │ (Settings)  │  │ (Logger)    │          │
│  └─────────────┘  └─────────────┘  └─────────────┘          │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 核心模块

#### 2.2.1 DataHub (数据中枢)

- **职责**: 数据发布订阅中心，统一数据分发
- **特点**:
    - Topic-based 订阅机制
    - 数据缓存和新鲜度管理
    - 自动刷新和空闲检测
- **文件**: `src/core/datahub/DataHub.h/cpp`

#### 2.2.2 ServiceLocator (依赖注入)

- **职责**: 服务注册和解析容器
- **特点**:
    - 单例/瞬态生命周期
    - 线程安全
    - 性能优化缓存
- **文件**: `src/core/di/ServiceLocator.h/cpp`

#### 2.2.3 ErrorHandler (错误处理)

- **职责**: 统一错误处理和用户提示
- **特点**:
    - Result<T> 类型替代异常
    - 错误分级和分类
    - 用户友好提示
- **文件**: `src/core/base/ErrorHandler.h/cpp`

#### 2.2.4 PerformanceMonitor (性能监控)

- **职责**: 实时性能指标监控
- **特点**:
    - CPU/内存监控
    - 函数执行统计
    - 性能告警
- **文件**: `src/core/monitoring/PerformanceMonitor.h/cpp`

## 3. 数据流

### 3.1 数据获取流程

```
External API ──> DataSource ──> DataHub ──> Page/Component
                    │              │
                    │              ├── Subscribe(topic)
                    │              ├── Publish(topic, data)
                    │              └── Cache(topic)
                    │
                    └── StockDataSource
                    └── CryptoDataSource
                    └── ForexDataSource
                    └── FundDataSource
```

### 3.2 页面数据订阅

```cpp
// 页面订阅数据示例
void StockPage::initializePage() {
    // 订阅股票行情
    subscribe("market:quote:sh600000", [this](const QVariant& data) {
        updateQuote(data.value<StockQuote>());
    });

    // 订阅K线数据
    subscribe("market:kline:sh600000:day1", [this](const QVariant& data) {
        updateKLine(data.value<KLineData>());
    });
}
```

## 4. 页面架构

### 4.1 页面基类

- **BasePage**: 基础页面类，提供通用功能
- **DataHubPageBase**: 数据订阅页面基类，自动管理订阅

### 4.2 页面模板系统

使用 `PageTemplate` 创建标准化 UI 组件：

```cpp
// 创建页面头部
auto* header = PageTemplate::createPageHeader(this, "股票行情", true, true);

// 创建汇总卡片
auto* cards = PageTemplate::createSummaryCardRow(this, {
    {"总资产", "1,234,567.89"},
    {"今日盈亏", "+12,345.67"},
    {"持仓数量", "15"}
});

// 创建标准表格
auto* table = PageTemplate::createStandardTable(this, {"代码", "名称", "价格", "涨跌幅"});
```

## 5. 服务层

### 5.1 服务注册

```cpp
// 使用 ServiceRegistry 注册服务
ServiceRegistry::instance().declareService({
    "DataHub",
    "数据中枢",
    {},
    100,  // 最高优先级
    false,
    true  // 关键服务
});

// 注册所有服务
ServiceRegistry::instance().registerAll();
ServiceRegistry::instance().initializeAll();
```

### 5.2 服务解析

```cpp
// 获取服务实例
auto* dataHub = ServiceLocator::instance().resolve<DataHub>();
auto* configManager = ServiceLocator::instance().resolve<ConfigManager>();
```

## 6. 错误处理

### 6.1 Result 类型使用

```cpp
// 返回 Result 类型
Result<double> calculateProfit(double buyPrice, double sellPrice) {
    if (buyPrice <= 0 || sellPrice <= 0) {
        return Error{
            ErrorCategory::UserInput,
            ErrorLevel::Error,
            ErrorCodes::InvalidInput,
            "价格必须大于0"
        };
    }
    return sellPrice - buyPrice;
}

// 使用 Result
auto result = calculateProfit(10.0, 15.0);
if (result.isSuccess()) {
    qDebug() << "Profit:" << result.value();
} else {
    ErrorHandler::instance().handle(result.error());
}
```

### 6.2 错误分类

| 分类       | 说明    | 示例错误码                              |
|----------|-------|------------------------------------|
| Network  | 网络错误  | NET_TIMEOUT, NET_CONN_FAILED       |
| Database | 数据库错误 | DB_CONN_FAILED, DB_QUERY_FAILED    |
| Trading  | 交易错误  | TRD_ORDER_REJECTED, TRD_RISK_LIMIT |
| Data     | 数据错误  | DATA_NOT_FOUND, DATA_INVALID       |
| System   | 系统错误  | SYS_OOM, SYS_CRASH                 |

## 7. 性能优化

### 7.1 性能监控使用

```cpp
// 启动性能监控
PerformanceMonitor::instance().start(1000);  // 每秒采集

// 函数性能计时
void criticalFunction() {
    PERF_TIMER_FUNC();  // 自动计时
    // ... 函数逻辑
}

// 获取性能报告
QString report = PerformanceMonitor::instance().generateReport();
```

### 7.2 性能告警配置

```cpp
PerformanceAlertConfig config;
config.cpuThreshold = 80.0;
config.memoryThreshold = 80.0;
config.fpsThreshold = 30;
config.functionTimeThreshold = 100000000;  // 100ms

PerformanceMonitor::instance().setAlertConfig(config);
```

## 8. 测试

### 8.1 测试框架

项目使用 Qt Test 模块进行单元测试，测试文件位于 `tests/` 目录。

### 8.2 测试覆盖目标

| 模块             | 目标覆盖率 | 当前状态    |
|----------------|-------|---------|
| DataHub        | 80%   | 已创建测试框架 |
| OrderManager   | 80%   | 已创建测试框架 |
| TradingService | 80%   | 已创建测试框架 |

### 8.3 运行测试

```bash
# 构建测试
cmake --build . --target WealthPilotTests

# 运行测试
./WealthPilotTests
```

## 9. 构建和部署

### 9.1 构建命令

```bash
# 配置
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 构建
cmake --build build --target WealthPilot

# 安装
cmake --install build
```

### 9.2 依赖项

- Qt 6.10.2 (Core, Gui, Widgets, Network, Sql, Charts)
- CMake 3.16+
- C++17 编译器

## 10. 未来规划

### 10.1 短期目标

- 完善测试覆盖率
- 优化性能监控
- 完善国际化

### 10.2 中期目标

- 添加更多数据源
- 实现 AI 智能分析
- 支持更多交易接口

### 10.3 长期目标

- 跨平台支持 (Linux)
- 移动端适配
- 云端同步

---

*文档版本: 1.0.0*
*更新日期: 2026-05-20*