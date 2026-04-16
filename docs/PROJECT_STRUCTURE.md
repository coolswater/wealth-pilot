# WealthPilot 项目结构

**版本**: 2.0.0  
**更新日期**: 2026-04-14

---

## 目录结构

```
WealthPilot/
├── CMakeLists.txt           # 构建配置
├── resources/               # 资源文件
│   ├── icons/              # 图标
│   ├── styles/             # 样式表
│   └── fonts/              # 字体
├── external/               # 外部依赖
│   └── ctp/               # CTP API
├── docs/                   # 文档
│   ├── ARCHITECTURE_REVIEW.md
│   ├── CODE_STYLE_GUIDE.md
│   └── PROJECT_STRUCTURE.md
└── src/                    # 源代码
    ├── main.cpp           # 程序入口
    ├── core/              # 核心框架
    │   ├── Singleton.h           # 单例模板
    │   ├── ErrorCode.h           # 错误码定义
    │   ├── Result.h              # 结果类型
    │   ├── ServiceLocator.h      # 依赖注入
    │   ├── BasePage.h            # 页面基类
    │   ├── PageNavigator.h       # 导航器
    │   ├── ApplicationInitializer.h  # 应用初始化
    │   ├── CacheManager.h        # 缓存管理
    │   ├── DatabaseManager.h     # 数据库
    │   ├── EnvironmentConfig.h   # 环境配置
    │   ├── ThemeManager.h        # 主题管理
    │   └── ...
    ├── models/            # 数据模型
    │   ├── FuturesQuoteItem.h
    │   ├── FuturesQuoteModel.h
    │   └── StockQuoteItem.h
    ├── views/             # UI视图
    │   ├── mainWindow/    # 主窗口
    │   ├── dashboard/     # 首页
    │   ├── stock/         # 股票
    │   ├── futures/       # 期货
    │   ├── portfolio/     # 持仓
    │   ├── watchList/     # 自选
    │   ├── signalCenter/  # 信号中心
    │   ├── news/          # 资讯
    │   ├── settings/      # 设置
    │   ├── warning/       # 预警
    │   ├── aboutus/       # 关于
    │   └── widgets/       # 通用组件
    ├── services/          # 服务层
    │   ├── CTPService.h   # CTP服务
    │   └── AIService.h    # AI服务
    ├── plugins/           # 插件
    │   ├── IPlugin.h      # 插件接口
    │   ├── ICTPPlugin.h   # CTP插件接口
    │   ├── IAIPlugin.h    # AI插件接口
    │   ├── CTPPlugin.h    # CTP插件实现
    │   └── AIPlugin.h     # AI插件实现
    ├── network/           # 网络层
    │   ├── NetworkManager.h
    │   └── NetworkCache.h
    ├── ui/                # UI组件
    │   └── components/
    │       ├── ThemeEngine.h
    │       └── KLineChart.h
    └── utils/             # 工具类
        ├── Logger.h       # 日志
        ├── Result.h       # 结果类型
        └── TechnicalIndicators.h
```

---

## 命名空间结构

```cpp
namespace WealthPilot {
    namespace Core {}       // 核心框架
    namespace Models {}     // 数据模型
    namespace Views {}      // UI视图
    namespace Services {}   // 服务层
    namespace Plugins {}    // 插件
    namespace Network {}    // 网络层
    namespace Utils {}      // 工具类
}
```

---

## 模块职责

### Core - 核心框架

| 类 | 职责 |
|---|------|
| Singleton | 单例模板基类 |
| ServiceLocator | 依赖注入容器 |
| BasePage | 页面抽象基类 |
| PageNavigator | 页面导航管理 |
| ApplicationInitializer | 应用初始化流程 |
| CacheManager | 多级缓存管理 |
| DatabaseManager | 数据库操作封装 |
| EnvironmentConfig | 多环境配置 |
| ThemeManager | 主题管理 |

### Models - 数据模型

| 类 | 职责 |
|---|------|
| FuturesQuoteItem | 期货行情数据项 |
| FuturesQuoteModel | 期货行情表格模型 |
| StockQuoteItem | 股票行情数据项 |

### Views - UI视图

| 目录 | 职责 |
|------|------|
| mainWindow | 主窗口框架 |
| dashboard | 首页仪表盘 |
| stock | 股票行情页 |
| futures | 期货行情页、K线页 |
| portfolio | 持仓管理页 |
| widgets | 通用UI组件 |

### Services - 服务层

| 类 | 职责 |
|---|------|
| CTPService | CTP交易服务封装 |
| AIService | AI分析服务封装 |

### Plugins - 插件

| 类 | 职责 |
|---|------|
| IPlugin | 插件基础接口 |
| ICTPPlugin | CTP插件接口 |
| IAIPlugin | AI插件接口 |
| CTPPlugin | CTP插件实现 |
| AIPlugin | AI插件实现 |

### Network - 网络层

| 类 | 职责 |
|---|------|
| NetworkManager | HTTP请求管理 |
| NetworkCache | 网络缓存 |

### Utils - 工具类

| 类 | 职责 |
|---|------|
| Logger | 日志系统 |
| Result | 结果类型 |
| TechnicalIndicators | 技术指标计算 |

---

## 依赖关系

```
main.cpp
    └── MainWindow
            ├── ApplicationInitializer
            │       ├── CacheManager
            │       ├── DatabaseManager
            │       └── EnvironmentConfig
            ├── ServiceLocator
            │       ├── ICTPPlugin
            │       └── IAIPlugin
            ├── ThemeEngine
            └── Pages (BasePage)
                    ├── DashboardPage
                    ├── StockQuotesPage
                    └── FuturesQuotesPage
```

---

## 构建流程

1. **CMake 配置**
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..
   ```

2. **编译**
   ```bash
   cmake --build . --target WealthPilot -j 10
   ```

3. **运行**
   ```bash
   ./WealthPilot.exe
   ```

---

## 扩展指南

### 添加新页面

1. 在 `src/views/` 下创建目录
2. 继承 `BasePage` 实现页面
3. 在 `MainWindow::getPage()` 注册
4. 在 `SidebarWidget` 添加导航项

### 添加新服务

1. 在 `src/services/` 创建服务类
2. 定义接口（可选）
3. 在 `ApplicationInitializer` 注册到 `ServiceLocator`
4. 通过 `ServiceLocator::resolve<T>()` 获取

### 添加新插件

1. 在 `src/plugins/` 创建插件类
2. 继承 `IPlugin` 实现接口
3. 在 `PluginLoader` 注册
4. 通过 `PluginLoader::getPlugin<T>()` 获取
