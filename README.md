# WealthPilot - 智能金融投资理财助手

<div align="center">

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
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
├── src/                    # 源代码目录
│   ├── ai/                 # AI分析模块
│   ├── analysis/           # 技术分析模块
│   ├── app/                # 应用程序入口
│   ├── core/               # 核心功能模块
│   ├── ctp/                # CTP接口模块
│   ├── data/               # 数据存储模块
│   ├── domain/             # 领域模型
│   ├── market/             # 市场数据模块
│   ├── models/             # 数据模型
│   ├── network/            # 网络通信模块
│   ├── plugins/            # 插件系统
│   ├── trading/            # 交易管理模块
│   ├── ui/                 # UI组件
│   ├── utils/              # 工具类
│   └── views/              # 页面视图
├── external/               # 外部依赖
├── docs/                   # 项目文档
├── resources/              # 资源文件
├── translations/           # 国际化文件
└── CMakeLists.txt          # 构建配置
```

### 模块说明

#### 核心模块 (src/core)

- **config** - 配置管理（设计令牌、主题管理）
- **navigation** - 页面导航系统
- **di** - 依赖注入容器
- **cache** - 缓存管理
- **task** - 异步任务管理
- **alert** - 智能预警系统
- **analysis** - 新闻情感分析

#### UI模块 (src/ui)

- **components** - UI组件库（50+组件）
- **animation** - 动画管理
- **utils** - UI工具类
- **ThemeManager** - 主题管理器

#### 视图模块 (src/views)

- **dashboard** - 仪表盘页面
- **stock** - 股票相关页面
- **futures** - 期货相关页面
- **news** - 新闻资讯页面
- **portfolio** - 持仓管理页面
- **settings** - 设置页面
- **signalCenter** - 信号中心页面

---

## 🏗️ 技术架构

### 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                      Presentation Layer                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │ Dashboard│  │  Stock   │  │ Futures  │  │   News   │ │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘ │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                      Business Layer                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │ Analysis │  │  Trading │  │   AI     │  │  Alert   │ │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘ │
└─────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────┐
│                       Data Layer                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │  Market  │  │ Storage  │  │ Network  │  │   CTP    │ │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘ │
└─────────────────────────────────────────────────────────┘
```

### 设计模式

- **MVVM** - 模型-视图-视图模型
- **DI** - 依赖注入
- **Observer** - 观察者模式
- **Factory** - 工厂模式
- **Singleton** - 单例模式
- **Strategy** - 策略模式

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

| 指标         | 数量     |
|------------|--------|
| 源文件 (.cpp) | 136    |
| 头文件 (.h)   | 160    |
| 代码行数       | 79,659 |
| UI组件       | 50+    |
| 页面视图       | 15+    |
| 文档页数       | 50+    |

---

## 📚 文档

### 开发文档

- [架构设计](docs/01-architecture/ARCHITECTURE.md)
- [开发者指南](docs/02-development/DEVELOPER_GUIDE.md)
- [编码规范](docs/02-development/CODING_STANDARDS.md)
- [样式指南](docs/04-style/STYLE_GUIDE.md)

### 功能文档

- [分析系统](docs/03-features/analysis-system-guide.md)
- [期货集成](docs/03-features/FUTURES_INTEGRATION_SUMMARY.md)
- [数据源集成](docs/05-integration/data-source-integration.md)

### 用户文档

- [用户手册](docs/07-user/USER_MANUAL.md)
- [API文档](docs/07-user/API_DOCUMENTATION.md)
- [快速测试](docs/07-user/quick-test-guide.md)

完整文档请查看 [docs/README.md](docs/README.md)

---

## 🎨 设计系统

### 主题支持

- 🌙 **深色主题** - 护眼深色模式
- ☀️ **浅色主题** - 明亮浅色模式
- 👁️ **护眼主题** - 特殊护眼模式

### 设计令牌

项目使用统一的设计令牌系统（Design Tokens），确保视觉一致性：

```cpp
// 颜色令牌
namespace Tokens::Colors {
    constexpr auto Primary = "#3B82F6";      // 主色
    constexpr auto Danger = "#EF4444";       // 危险色
    constexpr auto Success = "#10B981";      // 成功色
    constexpr auto Warning = "#F59E0B";      // 警告色
    constexpr auto BgBase = "#1F2937";       // 基础背景
    constexpr auto TextPrimary = "#FFFFFF";  // 主文本
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
MarketFront = tcp://180.168.146.187:10131
TradingFront = tcp://180.168.146.187:10130
BrokerID = 9999
UserID = your_user_id
Password = your_password
```

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

## 📝 更新日志

### v1.0.0 (2026-05-07)

**新增功能**

- ✅ 完整的行情展示系统
- ✅ 技术分析工具
- ✅ AI智能分析
- ✅ 智能预警系统
- ✅ 多主题支持

**样式重构**

- ✅ 消除75+处硬编码颜色
- ✅ 统一设计令牌系统
- ✅ 创建样式辅助工具

**性能优化**

- ✅ 数据缓存机制
- ✅ 异步任务管理
- ✅ UI渲染优化

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
