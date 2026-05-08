# 硬编码颜色排查报告

## 排查时间
2026-05-08

## 排查范围
- 所有 `src/` 目录下的 `.cpp` 和 `.h` 文件
- 查找模式：`#[0-9A-Fa-f]{6}`, `rgb()`, `rgba()`, `QColor("#...")`

## 排查结果

### ✅ 已统一管理的颜色

#### 1. Tokens.h - 设计令牌系统
**文件**: `src/core/config/Tokens.h`
**状态**: ✅ 已更新为 WealthPilot 设计规范

所有颜色常量已统一管理：
- 主色调：Primary, PrimaryHover, PrimaryDark
- 功能色：Success, Danger, Warning, Info
- 背景色：BgBase, BgSurface, BgElevated, BgHover
- 文字色：TextPrimary, TextSecondary, TextTertiary, TextDisabled
- 边框色：Border, BorderLight, BorderFocus
- 图表色：Chart系列颜色

#### 2. ChartStyles.h - 图表样式
**文件**: `src/ui/components/ChartStyles.h`
**状态**: ✅ 已更新为 WealthPilot 设计规范

所有图表相关颜色和样式已统一管理。

#### 3. ThemeManager - 主题管理器
**文件**: `src/ui/ThemeManager.h`, `src/ui/ThemeManager.cpp`
**状态**: ✅ 支持动态主题切换

提供 `currentTheme()` 方法获取当前主题颜色。

### ✅ 已修复的硬编码颜色

| 文件 | 修复内容 |
|------|----------|
| `SidebarWidget.cpp` | 使用 ThemeManager 获取颜色 |
| `AIAssistantPanelWidget.cpp` | 使用 ThemeManager 获取颜色 |
| `SignalMarker.h` | 更新为 Tokens 颜色 |
| `SignalService.cpp` | 更新为 Tokens 颜色 |
| `OrderDialog.h` | 使用 Tokens::Colors |
| `BaseChartWidget.h` | 更新网格颜色 |
| `ChartConfig.h` | 更新示例颜色 |
| `ChartStatusBar.h` | 更新连接状态颜色 |
| `PluginSystem.cpp` | 更新默认颜色 |

### ✅ 已使用主题颜色的页面

以下页面已正确使用 `Tokens::Colors` 或 `ThemeManager::currentTheme()`：

| 页面 | setStyleSheet 调用数 | 状态 |
|------|---------------------|------|
| DashboardPage.cpp | 54 | ✅ 使用主题颜色 |
| PortfolioPage.cpp | 59 | ✅ 使用主题颜色 |
| SignalCenterPage.cpp | 20 | ✅ 使用主题颜色 |
| BacktestPage.cpp | 20 | ✅ 使用主题颜色 |
| NewsPage.cpp | 20 | ✅ 使用主题颜色 |
| FundPage.cpp | 19 | ✅ 使用主题颜色 |
| AlertCenterPage.cpp | 18 | ✅ 使用主题颜色 |
| ForexPage.cpp | 11 | ✅ 使用主题颜色 |
| AboutUSPage.cpp | 8 | ✅ 使用主题颜色 |
| StockKLinePage.cpp | 7 | ✅ 使用主题颜色 |
| WarningPage.cpp | 7 | ✅ 使用主题颜色 |
| FuturesKLinePage.cpp | 7 | ✅ 使用主题颜色 |
| MainWindow.cpp | 4 | ✅ 使用主题颜色 |
| SettingsPage.cpp | 3 | ✅ 使用主题颜色 |
| AccountPage.cpp | 2 | ✅ 使用主题颜色 |

### 📋 动态生成的 rgba 颜色

以下文件使用动态生成的 rgba 颜色，**这是正确的做法**：

- `AIAssistantPanelWidget.cpp` - 从主题颜色动态生成 rgba
- `SidebarWidget.cpp` - 从主题颜色动态生成 rgba
- `PortfolioPage.cpp` - 从主题颜色动态生成 rgba

### 🎨 技术指标颜色（保持不变）

以下技术指标颜色是行业标准，保持不变：
- MA5: #FFD700 (金色)
- MA10: #00CED1 (青色)
- MA20: #FF6B6B (珊瑚红)
- MA30: #9B59B6 (紫色)
- MA60: #3498DB (蓝色)
- MACD: #FFD700
- RSI: #9B59B6
- KDJ_K/D/J: 金色/青色/珊瑚红
- BOLL: #FFD700

## 统一管理方案

### 方案1: 使用 Tokens::Colors（推荐用于静态颜色）

```cpp
#include "core/config/Tokens.h"

// 使用
QString color = Tokens::Colors::Primary;
QColor bgColor(Tokens::Colors::BgBase);
```

### 方案2: 使用 ThemeManager（推荐用于动态主题）

```cpp
#include "ui/ThemeManager.h"

// 获取当前主题颜色
ThemeColors theme = ThemeManager::instance()->currentTheme();
QString bgColor = theme.bgPrimary;

// 注册主题监听器
ThemeManager::instance()->registerThemeChangeListener(this, [this]() {
    updateTheme();
});
```

### 方案3: 使用 QSS 文件（推荐用于全局样式）

```cpp
// 在 ThemeManager 中加载 QSS
ThemeManager::instance()->applyTheme(themeType);
```

## 结论

✅ **所有硬编码颜色已统一管理**

- 静态颜色：由 `Tokens.h` 管理
- 动态颜色：由 `ThemeManager` 管理
- 全局样式：由 `theme_dark.qss` 等 QSS 文件管理

## Git 提交记录

```
ebc3fed fix: 修复剩余硬编码颜色问题
fff261e fix: 修复所有硬编码颜色问题
cffc747 fix: 修复左侧导航栏和AI面板主题样式
de0ace1 style: 按照WealthPilot设计规范调整深色主题配色
```

---

**更新时间**: 2026-05-08
**状态**: ✅ 完成
