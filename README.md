# WealthPilot 领航资产管理AI助手

<p align="center">
  <img src="resources/images/logo.png" alt="WealthPilot Logo" width="200"/>
</p>

<p align="center">
  <strong>专业级金融信息展示与分析软件</strong>
</p>

<p align="center">
  <a href="#功能特性">功能特性</a> •
  <a href="#技术栈">技术栈</a> •
  <a href="#快速开始">快速开始</a> •
  <a href="#项目结构">项目结构</a> •
  <a href="#开发指南">开发指南</a>
</p>

---

## 功能特性

### 📊 行情展示
- **股票行情**：A股、港股、美股实时行情
- **期货行情**：国内期货、国际期货实时行情
- **数字货币**：主流数字货币行情
- **外汇行情**：主要货币对实时汇率

### 📈 技术分析
- **K线图表**：专业级K线图表，支持多周期切换
- **技术指标**：MA、MACD、KDJ、RSI、BOLL等
- **画线工具**：趋势线、水平线、斐波那契等

### 💼 投资组合
- **持仓管理**：实时持仓盈亏计算
- **资产配置**：饼图展示资产分布
- **风险分析**：投资组合风险评估

### 🤖 AI助手
- **智能问答**：基于AI的金融问答
- **市场分析**：AI驱动的市场分析
- **投资建议**：个性化投资建议

### ⚙️ 系统功能
- **多主题支持**：深色、浅色、护眼主题
- **国际化**：中英文切换
- **数据缓存**：本地数据缓存加速

## 技术栈

| 类别 | 技术 |
|------|------|
| 开发语言 | C++ 17 |
| GUI框架 | Qt 6.10.2 |
| 构建系统 | CMake 3.16+ |
| 图表库 | Qt Charts |
| 数据库 | SQLite |
| 国际化 | Qt Linguist |
| 行情接口 | CTP (期货) |

## 快速开始

### 环境要求

- Windows 10/11 64位
- Qt 6.10.2 (MinGW 64-bit)
- CMake 3.16+
- Git

### 构建步骤

```bash
# 1. 克隆项目
git clone https://github.com/your-repo/wealth-pilot.git
cd wealth-pilot

# 2. 创建构建目录
mkdir build && cd build

# 3. 配置项目
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# 4. 编译
cmake --build . --target WealthPilot

# 5. 运行
./WealthPilot.exe
```

### Qt Creator 构建

1. 打开 Qt Creator
2. 文件 → 打开文件或项目 → 选择 `CMakeLists.txt`
3. 配置项目（选择 Qt 6.10.2 MinGW 64-bit）
4. 点击"构建" → "构建项目"
5. 点击"运行"

## 项目结构

```
wealth-pilot/
├── CMakeLists.txt              # 根 CMake 配置
├── README.md                   # 项目说明
├── docs/                       # 项目文档
│   ├── ARCHITECTURE_ANALYSIS.md
│   ├── STYLE_OPTIMIZATION_FINAL.md
│   └── ...
├── external/                   # 第三方依赖
│   └── ctp/                    # CTP 行情接口
├── resources/                  # 静态资源
│   ├── fonts/                  # 字体文件
│   ├── i18n/                   # 国际化翻译
│   ├── icons/                  # 图标资源
│   ├── images/                 # 图片资源
│   └── style/                  # QSS 样式表
│       ├── theme_dark.qss      # 深色主题
│       ├── theme_light.qss     # 浅色主题
│       └── theme_eyecare.qss   # 护眼主题
├── scripts/                    # 构建脚本
├── src/                        # 源代码
│   ├── app/                    # 应用入口
│   ├── core/                   # 核心模块
│   │   ├── base/               # 基础类
│   │   ├── cache/              # 缓存管理
│   │   ├── config/             # 配置管理
│   │   ├── database/           # 数据库
│   │   ├── di/                 # 依赖注入
│   │   ├── navigation/         # 导航管理
│   │   ├── task/               # 异步任务
│   │   └── types/              # 类型定义
│   ├── ui/                     # UI组件
│   │   ├── animation/          # 动画管理
│   │   └── components/         # 通用组件
│   ├── views/                  # 页面视图
│   │   ├── dashboard/          # 仪表盘
│   │   ├── futures/            # 期货模块
│   │   ├── portfolio/          # 投资组合
│   │   ├── settings/           # 系统设置
│   │   ├── stock/              # 股票模块
│   │   ├── trading/            # 交易模块
│   │   ├── widgets/            # 通用控件
│   │   └── ...
│   ├── ai/                     # AI 模块
│   ├── ctp/                    # CTP 接口
│   ├── trading/                # 交易逻辑
│   ├── market/                 # 市场数据
│   ├── models/                 # 数据模型
│   ├── network/                # 网络模块
│   ├── plugins/                # 插件接口
│   └── utils/                  # 工具类
└── tests/                      # 单元测试
```

## 开发指南

### 设计系统

项目使用统一的设计令牌系统（Design Tokens），所有颜色、间距、字体等设计变量定义在 `src/core/config/Tokens.h` 中。

#### 颜色规范

```cpp
// 金融标准：红涨绿跌
Tokens::Colors::Danger   // #EF4444 - 涨（红）
Tokens::Colors::Success  // #10B981 - 跌（绿）

// 主题色
Tokens::Colors::Primary  // #3B82F6 - 主蓝色

// 背景色（深色主题）
Tokens::Colors::BgBase      // #1A1F2E - 主背景
Tokens::Colors::BgSurface   // #0F1419 - 表面背景
Tokens::Colors::BgElevated  // #242937 - 卡片背景
```

#### 间距规范

```cpp
Tokens::Spacing::XS   // 4px
Tokens::Spacing::SM   // 8px
Tokens::Spacing::MD   // 16px
Tokens::Spacing::LG   // 24px
Tokens::Spacing::XL   // 32px
```

### 代码规范

- **命名规范**：驼峰命名法，类名首字母大写
- **注释规范**：使用 Doxygen 格式注释
- **代码风格**：遵循 C++ Core Guidelines

### 添加新页面

1. 在 `src/views/` 下创建页面目录
2. 继承 `BasePage` 类
3. 在 `PageFactoryRegistry` 中注册页面
4. 更新 `CMakeLists.txt`

## 许可证

本项目仅供学习和研究使用。

## 贡献

欢迎提交 Issue 和 Pull Request。

---

<p align="center">
  Made with ❤️ by WealthPilot Team
</p>
