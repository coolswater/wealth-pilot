# WealthPilot 项目架构文档

## 一、项目概述

**WealthPilot** 是一款基于 Qt 6.10.2 和 C++17 开发的专业级金融信息展示与分析软件，专为 PC 平台设计。

### 1.1 技术栈

| 技术    | 版本     | 说明        |
|-------|--------|-----------|
| Qt    | 6.10.2 | GUI框架     |
| C++   | 17     | 编程语言      |
| CMake | 3.16+  | 构建系统      |
| 编译器   | MinGW  | Windows平台 |

### 1.2 核心功能

- 股票行情展示与分析
- 期货行情展示与分析
- 基金净值查询
- 外汇汇率查询
- 数字货币行情
- K线图表分析
- 技术指标计算
- 交易下单（CTP接口）
- 风险控制
- AI智能助手

---

## 二、架构设计

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                        应用层 (App)                          │
│  ApplicationInitializer - 应用初始化器                       │
├─────────────────────────────────────────────────────────────┤
│                        视图层 (Views)                        │
│  MainWindow │ Dashboard │ StockKLine │ FuturesKLine │ ...   │
├─────────────────────────────────────────────────────────────┤
│                        UI组件层 (UI)                         │
│  KLineChart │ PieChart │ StatusBar │ SideBar │ ...          │
├─────────────────────────────────────────────────────────────┤
│                        业务层 (Business)                     │
│  TradingService │ MarketService │ AIService │ ...           │
├─────────────────────────────────────────────────────────────┤
│                        核心层 (Core)                         │
│  ServiceLocator │ NavigationManager │ ConfigManager │ ...   │
├─────────────────────────────────────────────────────────────┤
│                        数据层 (Data)                         │
│  StockDataSource │ DatabaseManager │ CacheManager │ ...     │
├─────────────────────────────────────────────────────────────┤
│                        基础层 (Base)                         │
│  BasePage │ Singleton │ MarketTypes │ ...                   │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 目录结构

```
D:\C++\wealth-pilot\
├── src/
│   ├── app/                    # 应用初始化
│   │   ├── ApplicationInitializer.h
│   │   └── ApplicationInitializer.cpp
│   │
│   ├── core/                   # 核心模块
│   │   ├── base/               # 基础类
│   │   │   ├── BasePage.h      # 页面基类
│   │   │   └── Singleton.h     # 单例模板
│   │   ├── config/             # 配置管理
│   │   │   ├── ConfigManager.h
│   │   │   └── Tokens.h        # 颜色常量
│   │   ├── database/           # 数据库
│   │   ├── cache/              # 缓存
│   │   ├── di/                 # 依赖注入
│   │   │   └── ServiceLocator.h
│   │   ├── navigation/         # 导航管理
│   │   ├── task/               # 异步任务
│   │   └── types/              # 类型定义
│   │       └── MarketTypes.h
│   │
│   ├── views/                  # 页面视图
│   │   ├── mainWindow/         # 主窗口
│   │   ├── dashboard/          # 仪表盘
│   │   ├── stock/              # 股票页面
│   │   ├── futures/            # 期货页面
│   │   ├── fund/               # 基金页面 ✨新增
│   │   ├── forex/              # 外汇页面 ✨新增
│   │   ├── crypto/             # 数字货币页面 ✨新增
│   │   ├── watchList/          # 自选列表
│   │   ├── trading/            # 交易页面
│   │   ├── account/            # 账户页面
│   │   ├── settings/           # 设置页面
│   │   └── news/               # 新闻页面
│   │
│   ├── ui/                     # UI组件
│   │   ├── components/         # 通用组件
│   │   │   ├── KLineChart.h    # K线图
│   │   │   ├── StatusBar.h     # 状态栏
│   │   │   └── ...
│   │   └── animation/          # 动画效果
│   │
│   ├── trading/                # 交易模块
│   │   ├── TradingService.h    # 交易服务
│   │   ├── OrderManager.h      # 订单管理
│   │   ├── PositionManager.h   # 持仓管理
│   │   ├── RiskController.h    # 风控系统
│   │   └── ConditionOrderEngine.h
│   │
│   ├── market/                 # 行情数据
│   │   └── StockDataSource.h
│   │
│   ├── ctp/                    # CTP接口
│   │   ├── api/                # API封装
│   │   ├── service/            # CTP服务
│   │   └── config/             # CTP配置
│   │
│   ├── ai/                     # AI模块
│   │   ├── service/            # AI服务
│   │   └── plugin/             # AI插件
│   │
│   ├── network/                # 网络模块
│   │
│   ├── data/                   # 数据存储
│   │
│   └── utils/                  # 工具类
│       └── Logger.h
│
├── CMakeLists.txt              # CMake配置
└── README.md                   # 项目说明
```

---

## 三、核心模块详解

### 3.1 ServiceLocator（服务定位器）

**功能**：高性能依赖注入容器，管理所有服务的生命周期。

**特性**：

- 单例模式
- 瞬态模式
- 工厂模式
- 线程安全
- 性能优化缓存

**使用示例**：

```cpp
// 注册服务
ServiceLocator::instance().registerSingleton<ITradingService, TradingService>();

// 解析服务
auto* tradingService = ServiceLocator::instance().resolve<ITradingService>();
```

### 3.2 NavigationManager（导航管理器）

**功能**：页面路由和导航控制。

**特性**：

- 页面注册与管理
- 页面跳转控制
- 导航历史记录
- 页面参数传递

**使用示例**：

```cpp
// 注册页面
NavigationManager::instance().registerPage("StockKLine", stockKLinePage);

// 导航到页面
NavigationManager::instance().navigateTo("StockKLine", {{"symbol", "600519"}});
```

### 3.3 BasePage（页面基类）

**功能**：所有页面的基类，提供统一的接口。

```cpp
class BasePage : public QWidget
{
public:
    virtual QString pageId() const = 0;      // 页面ID
    virtual QString pageName() const = 0;    // 页面名称
    virtual void initialize() {}              // 初始化
    virtual void refresh() {}                 // 刷新数据
};
```

---

## 四、功能模块

### 4.1 行情模块

| 模块   | 文件               | 功能            |
|------|------------------|---------------|
| 股票行情 | StockKLinePage   | K线图、技术指标、五档盘口 |
| 期货行情 | FuturesKLinePage | 期货K线、持仓分析     |
| 基金行情 | FundPage ✨       | 基金净值、持仓明细     |
| 外汇行情 | ForexPage ✨      | 汇率查询、汇率换算     |
| 数字货币 | CryptoPage ✨     | 加密货币行情、市值排名   |

### 4.2 交易模块

| 模块   | 文件                   | 功能        |
|------|----------------------|-----------|
| 交易服务 | TradingService       | 统一交易入口    |
| 订单管理 | OrderManager         | 订单提交、撤销   |
| 持仓管理 | PositionManager      | 持仓查询、盈亏计算 |
| 风控系统 | RiskController       | 风险检查、预警   |
| 条件单  | ConditionOrderEngine | 条件单触发     |

### 4.3 数据模块

| 模块    | 文件              | 功能          |
|-------|-----------------|-------------|
| 股票数据源 | StockDataSource | 新浪/腾讯/东财API |
| 数据库   | DatabaseManager | SQLite数据存储  |
| 缓存    | CacheManager    | 内存缓存管理      |

---

## 五、新增模块说明

### 5.1 基金模块 (FundPage)

**文件位置**：`src/views/fund/`

**功能**：

- 基金列表展示（ETF、LOF、开放式、货币、债券）
- 基金详情查看
- 持仓明细展示
- K线图（场内基金）
- 搜索筛选
- 加自选

**数据结构**：

```cpp
struct FundQuote {
    QString code;           // 基金代码
    QString name;           // 基金名称
    FundType type;          // 基金类型
    double nav;             // 单位净值
    double accNav;          // 累计净值
    double lastPrice;       // 最新价（场内）
    double changePercent;   // 涨跌幅
    QString manager;        // 基金经理
    QString company;        // 基金公司
    double scale;           // 基金规模
};
```

### 5.2 外汇模块 (ForexPage)

**文件位置**：`src/views/forex/`

**功能**：

- 主要货币对行情
- 汇率走势图
- 汇率换算工具
- 历史汇率查询

**数据结构**：

```cpp
struct ForexQuote {
    QString pair;           // 货币对（USD/CNY）
    double rate;            // 当前汇率
    double bid;             // 买入价
    double ask;             // 卖出价
    double changePercent;   // 涨跌幅
    double high24h;         // 24小时最高
    double low24h;          // 24小时最低
};
```

### 5.3 数字货币模块 (CryptoPage)

**文件位置**：`src/views/crypto/`

**功能**：

- 主流加密货币行情
- 市值排名
- 24小时涨跌幅
- K线图表

**数据结构**：

```cpp
struct CryptoQuote {
    QString symbol;         // 代币符号（BTC）
    QString name;           // 名称
    double price;           // 当前价格
    double change24h;       // 24小时涨跌幅
    double volume24h;       // 24小时成交量
    double marketCap;       // 市值
    int rank;               // 排名
};
```

---

## 六、颜色主题

### 6.1 深色主题配色

| 用途     | 颜色代码    | 说明   |
|--------|---------|------|
| 主背景    | #0A0A0A | 纯黑背景 |
| 图表背景   | #0D0D0D | 略浅背景 |
| 边框/分隔线 | #2A2A2A | 分隔线  |
| 网格线    | #1A1A1A | 低对比度 |
| 阳线（涨）  | #00D4AA | 青绿色  |
| 阴线（跌）  | #FF3366 | 玫红色  |
| 主文字    | #FFFFFF | 白色   |
| 常规文字   | #AAAAAA | 浅灰   |
| 标签文字   | #666666 | 深灰   |
| 按钮背景   | #2A2A2A | 按钮色  |

---

## 七、编译说明

### 7.1 环境要求

- Qt 6.10.2
- CMake 3.16+
- MinGW 或 MSVC

### 7.2 编译命令

```bash
# 配置
cmake -B build -G "MinGW Makefiles"

# 编译
cmake --build build --config Release

# 运行
./build/WealthPilot.exe
```

---

## 八、开发规范

### 8.1 代码规范

- 使用 C++17 标准
- 使用智能指针管理内存
- 使用 PIMPL 模式隐藏实现
- 使用 Qt 信号槽机制
- 代码注释使用中文

### 8.2 命名规范

- 类名：大驼峰（PascalCase）
- 函数名：小驼峰（camelCase）
- 变量名：小驼峰（camelCase）
- 常量：全大写下划线（UPPER_SNAKE_CASE）
- 成员变量：m_ 前缀

### 8.3 文件规范

- 头文件使用 .h 扩展名
- 实现文件使用 .cpp 扩展名
- 每个类一个文件
- 文件名与类名一致

---

## 九、新增模块说明（2024-04更新）

### 9.1 基金模块 (FundPage)

**文件位置**：`src/views/fund/`

**功能**：

- 基金列表展示（ETF、LOF、开放式、货币、债券）
- 基金详情查看
- 持仓明细展示
- K线图（场内基金）
- 搜索筛选
- 加自选

### 9.2 外汇模块 (ForexPage)

**文件位置**：`src/views/forex/`

**功能**：

- 主要货币对行情
- 汇率走势图
- 汇率换算工具
- 历史汇率查询

### 9.3 数字货币模块 (CryptoPage)

**文件位置**：`src/views/crypto/`

**功能**：

- 主流加密货币行情
- 市值排名
- 24小时涨跌幅
- K线图表

### 9.4 策略回测模块 (BacktestPage) ✨新增

**文件位置**：`src/views/backtest/`

**功能**：

- 策略编写与编辑
- 历史数据回测
- 回测结果展示（收益曲线、最大回撤、夏普比率）
- 策略参数优化
- 回测报告导出

**数据结构**：

```cpp
struct BacktestResult {
    double totalReturn;     // 总收益率
    double annualReturn;    // 年化收益率
    double maxDrawdown;     // 最大回撤
    double sharpeRatio;     // 夏普比率
    double winRate;         // 胜率
    double profitFactor;    // 盈亏比
    int totalTrades;        // 总交易次数
};
```

### 9.5 预警中心模块 (AlertCenterPage) ✨新增

**文件位置**：`src/views/alert/`

**功能**：

- 价格预警设置（涨跌幅、价格突破）
- 预警触发记录
- 消息推送设置
- 预警历史查询

**预警类型**：

- 价格高于/低于
- 涨幅高于/跌幅高于
- 成交量高于
- 换手率高于

### 9.6 数据导出工具 (DataExporter) ✨新增

**文件位置**：`src/utils/DataExporter.h/cpp`

**功能**：

- 导出K线数据
- 导出交易记录
- 导出回测报告（PDF）
- 导出预警记录

**导出格式**：

```cpp
enum class ExportFormat {
    CSV,        // CSV格式
    Excel,      // Excel格式
    PDF,        // PDF格式
    JSON        // JSON格式
};
```

---

## 十、版本历史

| 版本    | 日期      | 说明             |
|-------|---------|----------------|
| 1.0.0 | 2024-01 | 初始版本           |
| 1.1.0 | 2024-03 | 新增基金、外汇、数字货币模块 |

---

**作者**：WealthPilot Team  
**许可**：MIT License
