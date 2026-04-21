# WealthPilot 硬编码颜色扫描报告

## 扫描时间
2026-04-21 09:16

## 扫描结果

### 总体统计
- **硬编码颜色总数：** 217 处
- **涉及文件数：** 约 30 个

### 按文件分布（Top 20）

| 排名 | 文件 | 数量 | 说明 |
|------|------|------|------|
| 1 | core/config/Tokens.h | 31 | ✅ 设计令牌定义（正常） |
| 2 | ui/components/ChartStyles.h | 28 | ⚠️ 图表样式（待统一） |
| 3 | ui/components/ThemeEngine.cpp | 24 | ⚠️ 主题引擎（待统一） |
| 4 | ui/ThemeManager.cpp | 24 | ⚠️ 主题管理器（待统一） |
| 5 | views/portfolio/PortfolioPage.cpp | 18 | ❌ 页面硬编码 |
| 6 | ui/components/ChartConfig.cpp | 16 | ⚠️ 图表配置 |
| 7 | views/futures/FuturesKLinePage.cpp | 15 | ❌ 页面硬编码 |
| 8 | views/widgets/AssetPieChart.cpp | 12 | ❌ 组件硬编码 |
| 9 | views/stock/StockKLinePage.cpp | 11 | ❌ 页面硬编码 |
| 10 | views/widgets/OrderDialog.h | 6 | ❌ 对话框硬编码 |
| 11 | ui/components/KLineChart.cpp | 6 | ⚠️ K线图组件 |
| 12 | ui/components/TickTableView.cpp | 5 | ⚠️ 分笔表格 |
| 13 | ui/components/ChartConfig.h | 4 | ⚠️ 图表配置 |
| 14 | ui/components/MarketDepthWidget.cpp | 3 | ⚠️ 盘口组件 |
| 15 | views/futures/FuturesQuotesPage.cpp | 3 | ❌ 页面硬编码 |
| 16 | views/trading/TradingPanel.cpp | 2 | ❌ 页面硬编码 |
| 17 | views/warning/WarningPage.cpp | 2 | ❌ 页面硬编码 |
| 18 | ui/components/BaseChartWidget.h | 2 | ⚠️ 图表基类 |
| 19 | views/stock/StockQuotesPage.cpp | 2 | ❌ 页面硬编码 |
| 20 | ui/components/PageStyles.h | 1 | ✅ 已统一 |

## 分类分析

### ✅ 正常（设计定义文件）
- `Tokens.h` - 设计令牌定义，颜色定义是正常的

### ⚠️ 待统一（样式/主题文件）
- `ChartStyles.h` - 图表样式，应引用 Tokens
- `ThemeEngine.cpp` - 主题引擎，应引用 Tokens
- `ThemeManager.cpp` - 主题管理器，应引用 Tokens
- `ChartConfig.h/cpp` - 图表配置，应引用 Tokens
- `KLineChart.cpp` - K线图组件
- `TickTableView.cpp` - 分笔表格
- `MarketDepthWidget.cpp` - 盘口组件
- `BaseChartWidget.h` - 图表基类

### ❌ 需要修复（页面硬编码）
- `PortfolioPage.cpp` - 18 处
- `FuturesKLinePage.cpp` - 15 处
- `AssetPieChart.cpp` - 12 处
- `StockKLinePage.cpp` - 11 处
- `OrderDialog.h` - 6 处
- `FuturesQuotesPage.cpp` - 3 处
- `TradingPanel.cpp` - 2 处
- `WarningPage.cpp` - 2 处
- `StockQuotesPage.cpp` - 2 处

## 优先级建议

### 高优先级（核心样式文件）
1. `ThemeEngine.cpp` - 主题引擎，影响全局
2. `ThemeManager.cpp` - 主题管理器
3. `ChartStyles.h` - 图表样式

### 中优先级（图表组件）
4. `ChartConfig.h/cpp`
5. `KLineChart.cpp`
6. `BaseChartWidget.h`

### 低优先级（页面文件）
7. `PortfolioPage.cpp`
8. `FuturesKLinePage.cpp`
9. `StockKLinePage.cpp`
10. 其他页面

## 下一步行动

### 第一阶段：统一核心样式文件
- [ ] 更新 `ThemeEngine.cpp` 使用 Tokens
- [ ] 更新 `ThemeManager.cpp` 使用 Tokens
- [ ] 更新 `ChartStyles.h` 引用 Tokens

### 第二阶段：统一图表组件
- [ ] 更新 `ChartConfig.h/cpp`
- [ ] 更新 `KLineChart.cpp`
- [ ] 更新 `BaseChartWidget.h`

### 第三阶段：修复页面硬编码
- [ ] 扫描并修复所有页面文件
- [ ] 移除所有硬编码颜色值

## 预期效果
完成全部优化后，硬编码颜色数量应降至：
- Tokens.h: ~31 处（设计定义）
- 其他文件: 0 处
