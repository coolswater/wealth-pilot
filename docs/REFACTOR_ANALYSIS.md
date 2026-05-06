# WealthPilot 项目全面重构分析报告

生成时间：2026-04-30

## 一、项目概况

| 指标 | 数值 |
|------|------|
| 总文件数 | 231个（.h/.cpp） |
| 总代码量 | 1914 KB |
| 最大文件 | DashboardPage.cpp (1997行) |
| 最大目录 | views/ (606KB, 54文件) |

---

## 二、发现的问题

### 2.1 大文件问题（需拆分）

| 文件 | 行数 | 问题 |
|------|------|------|
| DashboardPage.cpp | 1997 | 职责过多，包含多个内部类 |
| StockKLinePage.cpp | 1899 | K线逻辑与UI耦合 |
| KLineChart.cpp | 1486 | 绘制逻辑复杂 |
| TreeMapWidget.cpp | 1194 | 可优化绘制性能 |
| AnimationManager.cpp | 1194 | 动画管理可简化 |

### 2.2 重复定义

| 类名 | 位置 | 建议 |
|------|------|------|
| WatchlistModel | DashboardPage.h, WatchListPage.h | 合并到 models/ |

### 2.3 目录结构问题

- **views/** 过大（606KB），包含页面、集成、注册代码
- **core/** 职责过多（配置、缓存、导航、数据库）
- **缺少统一的领域层**

---

## 三、重构方案

### 阶段1：清理冗余（立即执行）

1. 合并重复的 WatchlistModel
2. 清理未使用的代码
3. 统一 BasePage 引用

### 阶段2：拆分大文件

1. DashboardPage.cpp → 拆分为多个组件
2. KLineChart.cpp → 绘制引擎 + 数据管理
3. AnimationManager.cpp → 使用 Qt 动画框架

### 阶段3：性能优化

1. 高性能数据模型（已创建 QuoteModelBase）
2. 异步数据加载（已创建 AsyncDataService）
3. 帧率控制（已创建 FrameRateController）

---

## 四、预期效果

| 指标 | 当前 | 目标 |
|------|------|------|
| 最大文件行数 | 1997 | 500 |
| 帧率 | 30fps | 60fps |
| 启动时间 | 3s | 1.5s |
