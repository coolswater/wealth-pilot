# WealthPilot Views页面待重构清单

## 📋 检查时间
**2026-05-07 15:06 GMT+8**

## 🎯 待重构文件列表

### 1. PortfolioPage.cpp - 高优先级 ⚠️

**位置**：`src/views/portfolio/PortfolioPage.cpp`
**硬编码数量**：12处

**详细列表**：

| 行号 | 硬编码颜色 | 用途 | 应替换为 |
|------|-----------|------|---------|
| 12 | #1E1F24 | 注释：主背景 | Colors::BgBase |
| 13 | #2C2D33 | 注释：卡片背景 | Colors::BgElevated |
| 14 | #FF3B30, #34C759 | 注释：涨跌颜色 | Colors::Danger, Colors::Success |
| 526 | #3A3B41 | 进度条背景 | Colors::BgHover |
| 531 | #34C759, #F97316, #FF3B30 | 渐变色 | Colors::Success, Warning, Danger |
| 550 | #3A3B41 | 分割线颜色 | Colors::Border |
| 589 | #3A3B41 | 下拉框背景 | Colors::BgHover |
| 600 | #3A3B41 | 选中背景 | Colors::BgHover |
| 744 | #3A3B41 | 边框颜色 | Colors::Border |
| 769 | #3A3B41 | 网格线颜色 | Colors::Border |
| 770 | #3A3B41 | 选中背景 | Colors::BgHover |
| 777 | #25262B | 背景色 | Colors::BgElevated |

**重构优先级**：🔴 高

---

### 2. StockKLinePage.cpp - 高优先级 ⚠️

**位置**：`src/views/stock/StockKLinePage.cpp`
**硬编码数量**：6处

**详细列表**：

| 行号 | 硬编码颜色 | 用途 | 应替换为 |
|------|-----------|------|---------|
| 73 | #0F1419 | 背景填充 | Colors::BgSurface |
| 76 | #4A5568 | 画笔颜色 | Colors::TextSecondary |
| 100 | #1E293B | 网格线 | Colors::Border |
| 108 | #6B7280 | 虚线颜色 | Colors::TextTertiary |
| 112 | #3B82F6 | 蓝色线条 | Colors::Primary |
| 997 | #00AA00, #AA0000 | 涨跌颜色 | Colors::Success, Colors::Danger |

**重构优先级**：🔴 高

---

### 3. DashboardPage.cpp - 低优先级 ℹ️

**位置**：`src/views/dashboard/DashboardPage.cpp`
**硬编码数量**：3处（仅注释）

**详细列表**：

| 行号 | 硬编码颜色 | 用途 | 应替换为 |
|------|-----------|------|---------|
| 11 | #0d1117 | 注释：主背景 | Colors::BgBase |
| 12 | #161b22 | 注释：卡片背景 | Colors::BgElevated |
| 13 | #ff4d4f, #00b578 | 注释：涨跌颜色 | Colors::Danger, Colors::Success |

**重构优先级**：🟢 低（仅注释，不影响代码）

---

### 4. NewsPage.cpp - 中优先级 ⚡

**位置**：`src/views/news/NewsPage.cpp`
**硬编码数量**：2处

**详细列表**：

| 行号 | 硬编码颜色 | 用途 | 应替换为 |
|------|-----------|------|---------|
| 376 | #3d3d5c | 滚动条颜色 | Colors::Border |
| 576 | #2563EB | 激活颜色 | Colors::PrimaryHover |

**重构优先级**：🟡 中

---

### 5. FuturesQuotesPage.cpp - 中优先级 ⚡

**位置**：`src/views/futures/FuturesQuotesPage.cpp`
**硬编码数量**：1处

**详细列表**：

| 行号 | 硬编码颜色 | 用途 | 应替换为 |
|------|-----------|------|---------|
| 305 | #4CAF50 | 连接状态颜色 | Colors::Success |

**重构优先级**：🟡 中

---

### 6. SignalCenterPage.cpp - 中优先级 ⚡

**位置**：`src/views/signalCenter/SignalCenterPage.cpp`
**硬编码数量**：1处

**详细列表**：

| 行号 | 硬编码颜色 | 用途 | 应替换为 |
|------|-----------|------|---------|
| 405 | #3d3d5c | 滚动条颜色 | Colors::Border |

**重构优先级**：🟡 中

---

## 📊 统计汇总

| 文件 | 硬编码数量 | 优先级 | 预计工作量 |
|------|-----------|--------|-----------|
| PortfolioPage.cpp | 12处 | 🔴 高 | 15分钟 |
| StockKLinePage.cpp | 6处 | 🔴 高 | 10分钟 |
| DashboardPage.cpp | 3处 | 🟢 低 | 2分钟 |
| NewsPage.cpp | 2处 | 🟡 中 | 5分钟 |
| FuturesQuotesPage.cpp | 1处 | 🟡 中 | 2分钟 |
| SignalCenterPage.cpp | 1处 | 🟡 中 | 2分钟 |
| **总计** | **25处** | - | **约36分钟** |

---

## 🎯 重构计划

### 第一阶段：高优先级（约25分钟）
1. ✅ PortfolioPage.cpp（12处）
2. ✅ StockKLinePage.cpp（6处）

### 第二阶段：中优先级（约9分钟）
3. ✅ NewsPage.cpp（2处）
4. ✅ FuturesQuotesPage.cpp（1处）
5. ✅ SignalCenterPage.cpp（1处）

### 第三阶段：低优先级（约2分钟）
6. ✅ DashboardPage.cpp（3处注释）

---

## 🔧 重构方法

### 统一替换规则

```cpp
// 背景色
#0F1419, #1E1F24, #161b22 → Colors::BgBase 或 Colors::BgSurface
#2C2D33, #25262B → Colors::BgElevated
#3A3B41, #1E293B → Colors::Border 或 Colors::BgHover

// 文本色
#4A5568, #6B7280 → Colors::TextSecondary 或 Colors::TextTertiary

// 功能色
#FF3B30, #AA0000, #ff4d4f → Colors::Danger
#34C759, #00AA00, #00b578 → Colors::Success
#F97316 → Colors::Warning
#3B82F6, #2563EB → Colors::Primary 或 Colors::PrimaryHover
#4CAF50 → Colors::Success
```

---

## 📝 注意事项

1. **注释中的硬编码**：DashboardPage.cpp中的硬编码仅存在于注释中，不影响实际代码，优先级最低
2. **渐变色**：PortfolioPage.cpp中的渐变色需要特别注意，确保颜色顺序正确
3. **滚动条颜色**：NewsPage.cpp和SignalCenterPage.cpp中的滚动条颜色可以统一处理
4. **测试验证**：重构后需要编译测试，确保样式显示正确

---

**Views页面待重构清单 v1.0**
