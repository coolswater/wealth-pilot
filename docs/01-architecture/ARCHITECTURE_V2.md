# WealthPilot 架构文档

## 版本信息
- **版本**: 2.0.0
- **更新日期**: 2026-05-11
- **技术栈**: Qt 6.10.2 + C++17

---

## 1. 系统架构概览

```
┌─────────────────────────────────────────────────────────────────────┐
│                         WealthPilot 架构                              │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    表现层 (Presentation)                     │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐   │   │
│  │  │  Widgets  │ │   QML     │ │   Views    │ │  Dialogs  │   │   │
│  │  │  (C++)    │ │  (GPU)    │ │  (Pages)   │ │           │   │   │
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘   │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                              │                                      │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    业务层 (Business)                         │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐   │   │
│  │  │  Trading  │ │ Analysis   │ │   Alert   │ │    AI     │   │   │
│  │  │  Service  │ │  Engine    │ │  System   │ │  Service  │   │   │
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘   │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                              │                                      │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    数据层 (Data)                             │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐   │   │
│  │  │  Network  │ │  Database  │ │   Cache   │ │  Storage  │   │   │
│  │  │  Layer    │ │  Manager   │ │  Manager  │ │  Service  │   │   │
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘   │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                              │                                      │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    基础设施层 (Infrastructure)                │   │
│  │  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌───────────┐   │   │
│  │  │  Plugin   │ │  Service   │ │   Error   │ │   Config  │   │   │
│  │  │  System   │ │  Locator   │ │  Handler  │ │  Manager  │   │   │
│  │  └───────────┘ └───────────┘ └───────────┘ └───────────┘   │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 2. 核心模块

### 2.1 基础设施层

| 模块 | 文件 | 功能 |
|------|------|------|
| **ServiceLocator** | `src/core/di/ServiceLocator.h` | 依赖注入容器，管理服务生命周期 |
| **PluginLoader** | `src/core/plugin/PluginSystem.h` | 插件加载、卸载、管理 |
| **ErrorHandler** | `src/core/base/ErrorHandler.h` | 统一错误处理、日志、用户提示 |
| **ConfigManager** | `src/core/config/ConfigManager.h` | 配置管理、环境变量 |
| **CacheManager** | `src/core/cache/CacheManager.h` | 三级缓存系统 |

### 2.2 数据层

| 模块 | 文件 | 功能 |
|------|------|------|
| **DatabaseManager** | `src/core/database/DatabaseManager.h` | SQLite 数据库管理 |
| **DataStorageService** | `src/data/DataStorageService.h` | 数据存储服务 |
| **StockDataSource** | `src/market/StockDataSource.h` | 股票数据源（Sina API） |
| **MarketDataHub** | `src/market/MarketDataHub.h` | 市场数据中心 |

### 2.3 业务层

| 模块 | 文件 | 功能 |
|------|------|------|
| **TradingService** | `src/trading/TradingService.h` | 交易服务 |
| **OrderManager** | `src/trading/OrderManager.h` | 订单管理 |
| **PositionManager** | `src/trading/PositionManager.h` | 持仓管理 |
| **RiskController** | `src/trading/RiskController.h` | 风控系统 |
| **SmartAlertSystem** | `src/core/alert/SmartAlertSystem.h` | 智能预警 |
| **ChanLunAnalyzer** | `src/analysis/chanlun/ChanLunAnalyzer.h` | 缠论分析 |

### 2.4 表现层

| 模块 | 文件 | 功能 |
|------|------|------|
| **MainWindow** | `src/views/mainWindow/MainWindow.h` | 主窗口 |
| **ThemeManager** | `src/ui/ThemeManager.h` | 主题管理 |
| **KLineChart** | `src/ui/components/KLineChart.h` | K线图组件 |
| **QmlKLineWidget** | `src/ui/components/QmlKLineWidget.h` | QML K线容器 |

---

## 3. 混合架构设计

### 3.1 Widgets + QML 混合渲染

```
┌─────────────────────────────────────────────────────────────┐
│                    混合架构设计                               │
├─────────────────────────────────────────────────────────────┤
│  QML 层（动态可视化）                                         │
│  - K线图、分时图、数据仪表盘                                  │
│  - GPU 加速、流畅动画、声明式 UI                              │
│  - 文件: qml/charts/KLineChart.qml                           │
├─────────────────────────────────────────────────────────────┤
│  Widgets 层（复杂业务窗口）                                   │
│  - 主窗口、对话框、复杂表单                                   │
│  - QmlKLineWidget 容器组件                                   │
├─────────────────────────────────────────────────────────────┤
│  C++ 核心层（业务逻辑）                                       │
│  - 数据模型、网络通信、业务逻辑                               │
│  - QmlDataBridge 数据桥接                                    │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 数据桥接

```cpp
// QmlDataBridge 提供数据模型给 QML
class QmlDataBridge {
    KLineQmlModel* klineModel();      // K线数据模型
    TimeShareQmlModel* timeShareModel(); // 分时数据模型
    RealtimeQuoteQml* realtimeQuote();   // 实时行情
};
```

---

## 4. 插件系统

### 4.1 插件接口

```cpp
class IPlugin {
    virtual bool load() = 0;
    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void unload() = 0;
};
```

### 4.2 内置插件

| 插件 | 文件 | 功能 |
|------|------|------|
| **CTPPlugin** | `src/plugins/CTPPlugin.h` | CTP 交易接口 |
| **AIPlugin** | `src/plugins/AIPlugin.h` | AI 分析服务 |

---

## 5. 错误处理

### 5.1 错误码定义

```cpp
enum class ErrorCode : int {
    Success = 0,           // 成功
    // 1xxx: 通用错误
    // 2xxx: 网络错误
    // 3xxx: 数据库错误
    // 4xxx: CTP错误
    // 5xxx: AI错误
    // 6xxx: 配置错误
    // 7xxx: 缓存错误
    // 8xxx: 插件错误
};
```

### 5.2 错误处理流程

```
错误发生 → ErrorHandler::handleError()
         → 记录日志
         → 发送信号
         → 用户提示（可选）
         → 恢复建议
```

---

## 6. 国际化支持

### 6.1 翻译文件

- `resources/i18n/wealth-pilot_zh_CN.ts` - 简体中文
- `resources/i18n/wealth-pilot_en_US.ts` - 英文

### 6.2 使用方式

```cpp
// 加载翻译
QTranslator translator;
translator.load(":/i18n/wealth-pilot_zh_CN");
app.installTranslator(&translator);

// 使用翻译
tr("Stock");  // → "股票"
```

---

## 7. 性能优化

### 7.1 缓存策略

```
L1 Cache (内存) → L2 Cache (磁盘) → Network/API
     <1ms            <10ms            >100ms
```

### 7.2 异步处理

- AsyncTaskManager 管理后台任务
- 信号槽机制避免阻塞 UI
- 批量操作减少 API 调用

---

## 8. 测试框架

### 8.1 单元测试

| 测试文件 | 测试目标 |
|----------|----------|
| TestServiceLocator.cpp | 服务定位器 |
| TestCacheManager.cpp | 缓存管理器 |
| TestPluginLoader.cpp | 插件加载器 |
| TestApplicationInitializer.cpp | 应用初始化 |

### 8.2 运行测试

```bash
cmake --build . --target WealthPilotTests
ctest --output-on-failure
```

---

## 9. 目录结构

```
wealth-pilot/
├── src/
│   ├── app/           # 应用入口
│   ├── core/          # 核心模块
│   │   ├── base/      # 基础类
│   │   ├── config/    # 配置
│   │   ├── cache/     # 缓存
│   │   ├── database/  # 数据库
│   │   ├── di/        # 依赖注入
│   │   ├── plugin/    # 插件系统
│   │   └── types/     # 类型定义
│   ├── market/        # 市场数据
│   ├── trading/       # 交易系统
│   ├── analysis/      # 技术分析
│   ├── ai/            # AI 服务
│   ├── ui/            # UI 组件
│   │   ├── components/
│   │   ├── animation/
│   │   ├── utils/
│   │   └── qml/       # QML 桥接
│   ├── views/         # 页面视图
│   ├── plugins/       # 插件实现
│   └── utils/         # 工具类
├── qml/               # QML 文件
│   ├── charts/
│   ├── components/
│   └── models/
├── resources/         # 资源文件
│   ├── fonts/
│   ├── icons/
│   ├── styles/
│   └── i18n/
├── tests/             # 测试文件
└── docs/              # 文档
```

---

**文档版本**: 2.0.0  
**最后更新**: 2026-05-11
