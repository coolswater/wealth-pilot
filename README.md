# WealthPilot - 智能金融投资理财助手

<div align="center">

![Version](https://img.shields.io/badge/version-6.0.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey.svg)
![Qt](https://img.shields.io/badge/Qt-6.10.2-green.svg)
![C++](https://img.shields.io/badge/C++-17-orange.svg)
![License](https://img.shields.io/badge/license-MIT-blue.svg)

**专业的金融信息展示与分析软件，为投资者提供全面的市场洞察**

[功能特性](#功能特性) • [快速开始](#快速开始) • [项目结构](#项目结构) • [技术架构](#技术架构) • [文档](#文档)

</div>

---

## 📖 项目简介

WealthPilot 是一个基于 Qt 6.10.2 和 C++17 开发的金融信息展示与分析软件，专为 PC
平台设计。该软件提供股票、期货、外汇、基金和数字货币等金融产品的实时数据展示、自选股管理、市场全景等功能，旨在为用户提供全面的金融市场信息。

### 核心亮点

- 🎯 **多市场支持** - 股票、期货、外汇、基金、数字货币
- 📊 **实时行情** - 支持多种数据源（新浪、腾讯、东财）
- 🔌 **CTP集成** - 支持期货CTP接口
- 🤖 **AI分析** - 智能投资分析与建议
- 📈 **技术分析** - 缠论、K线形态识别
- 🎨 **现代UI** - 深色主题，流畅体验
- ⚡ **高性能** - DataHub统一调度，主题切换优化

---

## ✨ 功能特性

### 📊 行情展示

| 功能   | 描述             | 状态 |
|------|----------------|----|
| 股票行情 | A股实时行情、分时图、K线图 | ✅  |
| 期货行情 | 国内期货实时行情、K线图   | ✅  |
| 外汇行情 | 主要货币对实时行情      | ✅  |
| 基金净值 | 场内基金实时净值       | ✅  |
| 数字货币 | 主流数字货币行情       | ✅  |

### 📈 技术分析

| 功能   | 描述                | 状态 |
|------|-------------------|----|
| K线图表 | 专业K线图表组件          | ✅  |
| 技术指标 | MA、MACD、RSI、BOLL等 | ✅  |
| 缠论分析 | 缠论自动识别与分析         | ✅  |
| 形态识别 | K线形态自动识别          | ✅  |
| 画线工具 | 趋势线、水平线等          | ✅  |

### 🤖 智能分析

| 功能   | 描述     | 状态 |
|------|--------|----|
| AI助手 | 智能投资建议 | ✅  |
| 新闻分析 | 新闻情感分析 | ✅  |
| 风险评估 | 投资风险评估 | ✅  |
| 组合优化 | 投资组合建议 | ✅  |
| 信号中心 | 交易信号推送 | ✅  |

### 💼 投资管理

| 功能    | 描述        | 状态 |
|-------|-----------|----|
| 自选股管理 | 自选股分组、排序  | ✅  |
| 持仓管理  | 持仓盈亏计算    | ✅  |
| 预警系统  | 价格预警、指标预警 | ✅  |
| 交易记录  | 交易记录管理    | ✅  |
| 资产分析  | 资产配置分析    | ✅  |

---

## 🚀 快速开始

### 环境要求

- **操作系统**: Windows 10/11
- **编译器**: MinGW-w64 13.1.0 或 MSVC 2022
- **Qt版本**: Qt 6.10.2
- **CMake**: 3.16+
- **C++标准**: C++17

### 编译步骤

```bash
# 1. 克隆项目
git clone https://github.com/yourusername/wealth-pilot.git
cd wealth-pilot

# 2. 创建构建目录
mkdir build && cd build

# 3. 配置项目
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# 4. 编译项目
cmake --build . --target WealthPilot -j 10

# 5. 运行程序
./WealthPilot.exe
```

### 运行要求

- 确保Qt 6.10.2运行时库已安装
- 首次运行需要配置数据源

---

## 📁 项目结构

```
wealth-pilot/
├── src/                        # 源代码目录
│   ├── app/                    # 应用程序入口
│   ├── core/                   # 核心功能模块
│   │   ├── services/           # 服务层
│   │   │   ├── cache/          # 缓存管理
│   │   │   ├── di/             # 依赖注入
│   │   │   ├── navigation/     # 页面导航
│   │   │   ├── alert/          # 智能预警
│   │   │   └── lifecycle/      # 服务生命周期
│   │   ├── domain/             # 领域模型
│   │   │   ├── analysis/       # 分析模块
│   │   │   ├── backtest/       # 回测引擎
│   │   │   ├── portfolio/      # 组合优化
│   │   │   └── risk/           # 风险管理
│   │   └── trading/            # 交易管理
│   ├── data/                   # 数据层
│   │   ├── datahub/            # 数据中心（核心）
│   │   ├── market/             # 市场数据源
│   │   └── models/             # 数据模型
│   ├── infrastructure/         # 基础设施
│   │   ├── ai/                 # AI服务
│   │   ├── config/             # 配置管理
│   │   ├── database/           # 数据库
│   │   ├── network/            # 网络通信
│   │   └── plugins/            # 插件系统
│   ├── presentation/           # 表现层
│   │   ├── components/         # UI组件库（50+）
│   │   ├── views/              # 页面视图（28个）
│   │   ├── viewmodels/         # 视图模型
│   │   ├── styles/             # 主题样式
│   │   └── animation/          # 动画管理
│   └── shared/                 # 共享模块
│       ├── types/              # 类型定义
│       └── utils/              # 工具类
├── docs/                       # 项目文档
│   ├── architecture/           # 架构文档
│   ├── technical-debt/         # 技术债务
│   └── optimization/           # 优化方案
├── resources/                  # 资源文件
└── CMakeLists.txt              # 构建配置
```

### 核心模块说明

#### DataHub - 数据中心（v6.0 核心改进）

DataHub 是 WealthPilot 的核心数据中枢，负责：

- 发布/订阅模式的数据分发
- 数据生命周期管理（TTL、缓存）
- 统一的定时刷新调度
- 数据源注册和发现

```cpp
// 订阅数据
DataHub::instance()->subscribe("market:quote:sh600519", [](const QVariant& data) {
    StockQuote quote = data.value<StockQuote>();
    // 处理行情数据
});

// 注册数据生产者
stockDataSource->registerToDataHub(DataHub::instance(), 5000);
```

#### ThemeManager - 主题管理器（v2.0 性能优化）

- 样式表编译缓存
- 批量UI更新，减少重绘
- 异步监听器通知
- 切换性能提升 3-5 倍

---

## 🏗️ 技术架构

### 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                    Presentation Layer                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │ Dashboard│  │  Stock   │  │ Futures  │  │   News   │    │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘    │
│                         ↓ ThemeManager                       │
└─────────────────────────────────────────────────────────────┘
                            ↓ DataHub
┌─────────────────────────────────────────────────────────────┐
│                    Business Layer                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │ Analysis │  │  Trading │  │   AI     │  │  Alert   │    │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘    │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                      Data Layer                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │
│  │  Market  │  │ Storage  │  │ Network  │  │   CTP    │    │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### 设计模式

| 模式        | 应用场景                        |
|-----------|-----------------------------|
| MVVM      | 页面-视图模型分离                   |
| Singleton | 全局服务（DataHub, ThemeManager） |
| Observer  | DataHub 发布/订阅               |
| Factory   | 页面工厂、组件工厂                   |
| Strategy  | 数据源策略、分析策略                  |
| PIMPL     | 实现细节隐藏                      |

### 技术栈

| 类别  | 技术                    |
|-----|-----------------------|
| 语言  | C++17                 |
| 框架  | Qt 6.10.2             |
| 构建  | CMake + Ninja         |
| 数据库 | SQLite                |
| 网络  | QNetworkAccessManager |
| 图表  | Qt Charts             |
| 国际化 | Qt Linguist           |
| 日志  | 自定义Logger             |

---

## 📊 项目统计

| 指标          | 数量      |
|-------------|---------|
| 源文件 (.cpp)  | 180+    |
| 头文件 (.h)    | 200+    |
| 代码行数        | 118,878 |
| UI组件        | 50+     |
| 页面视图        | 28      |
| Singleton服务 | 25+     |
| TODO已清理     | 44%     |

---

## 📚 文档

### 架构文档

- [架构设计](docs/architecture/ArchitectureReview.md)
- [技术债务报告](docs/technical-debt/TechDebt-Cleanup-Report.md)
- [DataHub设计](src/data/datahub/DataHub.h)
- [主题管理器](src/presentation/styles/ThemeManager.h)

### 开发文档

- [开发者指南](docs/02-development/DEVELOPER_GUIDE.md)
- [编码规范](docs/02-development/CODING_STANDARDS.md)
- [样式指南](docs/04-style/STYLE_GUIDE.md)

### 功能文档

- [分析系统](docs/03-features/analysis-system-guide.md)
- [期货集成](docs/03-features/FUTURES_INTEGRATION_SUMMARY.md)
- [数据源集成](docs/05-integration/data-source-integration.md)

---

## 🎨 设计系统

### 主题支持

- 🌙 **深色主题** - 专业金融风格（默认）
- ☀️ **浅色主题** - 明亮浅色模式
- 👁️ **护眼主题** - 特殊护眼模式
- 🎯 **高对比度** - 无障碍支持

### 设计令牌

项目使用统一的设计令牌系统（Design Tokens），确保视觉一致性：

```cpp
// 颜色令牌
namespace Tokens::Colors {
    constexpr auto Primary = "#3B82F6";      // 主色
    constexpr auto Danger = "#EF4444";       // 危险色（涨）
    constexpr auto Success = "#10B981";      // 成功色（跌）
    constexpr auto Warning = "#F59E0B";      // 警告色
    constexpr auto BgBase = "#0A0E17";       // 基础背景
    constexpr auto TextPrimary = "#F3F4F6";  // 主文本
}
```

---

## 🔧 配置

### 数据源配置

支持多种数据源：

- **新浪财经** - 股票、基金行情
- **腾讯财经** - 股票行情
- **东方财富** - 股票、期货行情
- **CTP接口** - 期货实时行情

### CTP配置

```ini
[CTP]
MarketFront = tcp://218.202.237.33:10131
TradingFront = tcp://218.202.237.33:10130
BrokerID = 9999
UserID = your_user_id
Password = your_password
```

---

## 📝 更新日志

### v6.0.0 (2026-05-26)

**架构重构**

- ✅ DataHub 数据中心统一调度
- ✅ DashboardPage 迁移至 DataHub
- ✅ 移除独立 QTimer，统一刷新策略
- ✅ 服务生命周期管理

**性能优化**

- ✅ 主题切换性能提升 3-5 倍
- ✅ 批量 UI 更新，减少重绘
- ✅ 异步监听器通知
- ✅ K线图表 LOD 渲染

**功能拓展**

- ✅ AI选股器数据对接
- ✅ 智能预警推送（微信/邮件/钉钉）
- ✅ 策略回测可视化
- ✅ 回测报告面板

**代码质量**

- ✅ TODO 清理 44%（43→24）
- ✅ 添加架构审核报告
- ✅ 完善文件注释
- ✅ 修复编译错误

### v1.0.0 (2026-05-07)

- ✅ 完整的行情展示系统
- ✅ 技术分析工具
- ✅ AI智能分析
- ✅ 智能预警系统
- ✅ 多主题支持

---

## 🤝 贡献指南

欢迎贡献代码！请遵循以下步骤：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交 Pull Request

### 代码规范

- 遵循 [编码规范](docs/02-development/CODING_STANDARDS.md)
- 使用 [代码风格](docs/02-development/CODE_STYLE_GUIDE.md)
- 添加必要的注释和文档

---

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 👥 作者

**WealthPilot Team**

---

## 🙏 致谢

感谢以下开源项目：

- [Qt Framework](https://www.qt.io/)
- [Qt Charts](https://doc.qt.io/qt-6/qtcharts-index.html)
- [CTP API](http://www.sfit.com.cn/)

---

<div align="center">

**⭐ 如果这个项目对你有帮助，请给一个 Star！⭐**

Made with ❤️ by WealthPilot Team

</div>
