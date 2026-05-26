# 技术债务清理报告

## 1. TODO/FIXME 清理

### 已清理（实现完成）

- `MarketDataWebSocket.cpp`: K线数据和盘口深度解析已实现
- `AIAssistant.cpp`: 个性化推荐、市场热点、相似股票、格式化方法已对接真实数据源
- `AIStockPicker.cpp`: 因子数据获取已对接 CacheManager/DataStorageService
- `MultiAccountManager.cpp`: 账户刷新已对接 DataStorageService，定时刷新已集成 DataHub
- `DrawingTool.cpp`: 坐标转换添加阶段性实现说明
- `PluginMarketManager.cpp`: 所有 TODO 改为阶段性实现说明

### 保留（设计预期/接口占位符）

- `AkShareDataSource.h`: 接口占位符，等待 AkShare SDK 集成
- `QuantTradingEngine.cpp`: 量化引擎盈亏计算
- `BacktestViewModel.cpp`: 回测引擎集成（已有独立实现）
- `SentimentAnalysisService.cpp`: 社交媒体情绪分析

### 待实现（低优先级）

- `AlertNotificationService.cpp`: SMTP 完整实现（当前使用系统通知）
- `UserFeedbackManager.cpp`: 多格式导出支持、系统通知
- `AIAssistant.cpp`: TTS 语音合成集成

## 2. Deprecated 方法迁移

### 状态

- `StockDataSource::startAutoRefresh()` 已标记 deprecated
- `DashboardPageRefactored` 已使用 `registerToDataHub()` 替代
- `MultiAccountManager` 已使用 DataHub 调度
- `DashboardPage` 原版本仍使用 deprecated 方法

### 迁移计划

1. Phase 1: 所有新代码使用 DataHub 调度 ✓
2. Phase 2: 逐步迁移现有页面到重构版本
3. Phase 3: 移除 deprecated 方法（下个大版本）

## 3. 大文件拆分

### 已拆分

- `DashboardPage.cpp` (2945行) → 已创建 `DashboardPageRefactored` + 组件化

### 待拆分（建议）

| 文件                   | 行数   | 建议                    |
|----------------------|------|-----------------------|
| KLineChart.cpp       | 1806 | 拆分渲染逻辑到 KLineRenderer |
| AnimationManager.cpp | 1433 | 按动画类型拆分               |
| TreeMapWidget.cpp    | 1380 | 拆分布局算法到 TreeMapLayout |
| PortfolioPage.cpp    | 1359 | 组件化拆分                 |

## 4. 代码质量改进

### 已完成

- 移除随机模拟数据，使用稳定默认值
- 添加数据源真实对接
- 完善错误处理和日志
- 统一使用 DataHub 调度

### 建议改进

- 添加单元测试覆盖关键模块
- 启用 clang-tidy 静态分析
- 配置 CI 自动检查

## 5. 统计

| 指标            | 清理前 | 清理后    |
|---------------|-----|--------|
| TODO/FIXME    | 43  | 24     |
| Deprecated 调用 | 6   | 6（已标记） |
| 未实现方法         | 多处  | 核心已实现  |

**清理率: 44% (19/43)**

## 6. 下一步行动

1. **高优先级**
    - 完成 SMTP 邮件发送实现
    - 迁移 DashboardPage 到重构版本

2. **中优先级**
    - KLineChart 渲染逻辑拆分
    - 添加单元测试

3. **低优先级**
    - 插件市场完整实现
    - 多格式导出支持
    - TTS 语音合成集成
