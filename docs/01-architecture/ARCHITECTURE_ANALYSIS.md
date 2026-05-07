# WealthPilot 项目架构分析报告

## 分析时间

2026-04-21 10:00

## 项目概览

### 代码统计

| 模块      | 文件数     | 代码行数       | 占比       |
|---------|---------|------------|----------|
| views   | 76      | 15,711     | 43%      |
| ui      | 27      | 5,952      | 16%      |
| core    | 26      | 5,133      | 14%      |
| trading | 15      | 4,014      | 11%      |
| ctp     | 10      | 2,429      | 7%       |
| utils   | 13      | 2,081      | 6%       |
| ai      | 4       | 1,346      | 4%       |
| 其他      | 14      | 2,360      | 6%       |
| **总计**  | **185** | **36,526** | **100%** |

## 发现的问题

### 1. 冗余目录 ❌

| 目录                | 问题    | 建议           |
|-------------------|-------|--------------|
| cmake-build-debug | 旧构建目录 | 删除           |
| .idea             | IDE配置 | 加入.gitignore |
| .qtcreator        | IDE配置 | 加入.gitignore |

### 2. 样式系统重复 ⚠️

| 文件             | 功能   | 问题            |
|----------------|------|---------------|
| Tokens.h       | 设计令牌 | ✅ 核心          |
| ChartStyles.h  | 图表样式 | 有独立Colors命名空间 |
| PageStyles.h   | 页面样式 | 功能重叠          |
| ThemeColors.h  | 主题颜色 | 功能重叠          |
| ThemeEngine.h  | 主题引擎 | 功能重叠          |
| ThemeManager.h | 主题管理 | 功能重叠          |

**建议：** 统一到 Tokens.h + ThemeManager.h

### 3. 空文件/骨架文件 ⚠️

| 文件                     | 行数 | 问题   |
|------------------------|----|------|
| NewsPage.h             | 18 | 骨架文件 |
| StockQuotesPage.h      | 18 | 骨架文件 |
| WarningPage.h          | 18 | 骨架文件 |
| FuturesQuoteDelegate.h | 17 | 骨架文件 |

### 4. 模块结构问题 ⚠️

#### views 模块过大（76文件）

```
views/
├── aboutus/      (1 file)
├── account/      (2 files)
├── dashboard/    (2 files)
├── futures/      (6 files)
├── mainWindow/   (2 files)
├── news/         (2 files) ← 骨架
├── portfolio/    (2 files)
├── settings/     (6 files)
├── signalCenter/ (2 files)
├── stock/        (4 files)
├── trading/      (8 files)
├── warning/      (2 files) ← 骨架
├── watchList/    (2 files)
└── widgets/      (35 files) ← 应移到 ui/components
```

#### widgets 应该在 ui/components 下

当前 `views/widgets/` 包含 35 个通用组件，应该移到 `ui/components/`

### 5. 命名不一致 ⚠️

- `AboutUSPage` vs `AccountPage` (大小写不一致)
- `FuturesKLinePage` vs `StockKLinePage` (命名一致 ✅)
- `ThemeEngine` vs `ThemeManager` (功能重叠)

## 优化建议

### 第一阶段：清理冗余

1. 删除旧构建目录 `cmake-build-debug`
2. 更新 `.gitignore`

### 第二阶段：统一样式系统

1. 保留 `Tokens.h` 作为唯一设计令牌来源
2. 保留 `ThemeManager.h` 作为主题管理器
3. 合并 `ThemeColors.h` 和 `PageStyles.h` 到 `Tokens.h`
4. 重构 `ChartStyles.h` 使用 `Tokens`

### 第三阶段：重构模块结构

1. 将 `views/widgets/` 移到 `ui/components/`
2. 合并骨架页面或实现完整功能
3. 统一命名规范

### 第四阶段：文档完善

1. 更新 README.md
2. 创建用户使用手册
3. 创建开发者指南

## 当前架构评估

### 优点 ✅

- 模块划分清晰（core/ui/views分离）
- 使用 PIMPL 模式隐藏实现
- 设计令牌系统（Tokens.h）
- 国际化支持（i18n）

### 需改进 ⚠️

- 样式系统重复
- views 模块过大
- 部分骨架文件未完成
- 文档不完善
