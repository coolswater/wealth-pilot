# 样式问题修复报告

## 已修复的问题

### 1. 主题切换映射错误 ✅

**问题**：SettingsPage 的主题下拉框顺序与 ThemeType 枚举不匹配

**原因**：
- 下拉框顺序：浅色(0), 深色(1), 自动(2)
- ThemeType 枚举：Dark(0), Light(1), HighContrast(2), EyeCare(3)

**修复**：
- 修改下拉框使用 ThemeType 枚举值作为 userData
- 使用 `currentData().toInt()` 获取正确的主题类型
- 更新 loadSettings/saveSettings 方法

### 2. 主题刷新不完整 ✅

**问题**：切换主题后部分组件样式未更新

**修复**：
- 在 ThemeManager::applyTheme() 中添加强制刷新所有顶级窗口
- 调用 `widget->setStyleSheet()` 和 `widget->repaint()`

## 剩余问题

### 内联样式过多

以下页面仍大量使用内联样式，可能导致主题切换后样式不一致：

| 页面 | 内联样式数量 |
|------|-------------|
| PortfolioPage.cpp | 38 |
| DashboardPage.cpp | 37 |
| BacktestPage.cpp | 20 |
| SignalCenterPage.cpp | 18 |
| NewsPage.cpp | 18 |
| FundPage.cpp | 16 |
| AlertCenterPage.cpp | 14 |
| ForexPage.cpp | 11 |

### 建议的解决方案

**方案1：迁移到 QSS（推荐）**
- 将内联样式迁移到 QSS 文件
- 使用对象名选择器
- 优点：统一管理，主题切换自动生效

**方案2：使用主题监听器**
- 在每个页面注册主题变化监听器
- 主题切换时重新应用样式
- 优点：改动较小

**方案3：使用 Tokens 动态生成**
- 所有内联样式使用 Tokens 或 ThemeManager 获取颜色
- 优点：灵活，但需要手动刷新

## 测试建议

1. **主题切换测试**：
   - 在设置页面切换主题
   - 检查所有页面是否正确刷新
   - 特别注意 DashboardPage 和 PortfolioPage

2. **样式一致性测试**：
   - 检查标题、按钮、输入框样式
   - 检查涨跌颜色
   - 检查图表颜色

## Git 提交记录

```
01c425e fix: 修复主题切换问题
```

---

**更新时间**: 2026-05-08
