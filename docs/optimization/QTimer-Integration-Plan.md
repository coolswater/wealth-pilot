# QTimer 整合优化方案

## 现状分析

项目中 QTimer 使用情况：

- 总计：224 处引用
- 独立 new QTimer：20+ 处
- setInterval 调用：30+ 处
- QTimer::singleShot：25+ 处

## 问题

1. **资源浪费**：多个独立定时器同时运行
2. **刷新冲突**：不同数据源各自刷新，可能造成竞态
3. **难以管理**：定时器分散在各个类中，生命周期复杂
4. **调试困难**：无法统一追踪刷新行为

## 解决方案

### 方案：统一 DataHub 调度

DataHub 已有调度器（每秒检查），扩展其功能：

```cpp
// DataHub 调度策略
struct TopicPolicy {
    int ttlMs = 30000;           // 缓存有效期
    int minIntervalMs = 5000;    // 最小刷新间隔
    bool pushOnly = false;       // WebSocket推送模式
    int priority = 0;           // 优先级
};
```

### 改造步骤

#### 1. 移除独立 startAutoRefresh

**改造前**（StockDataSource）：

```cpp
void StockDataSource::startAutoRefresh(int intervalMs)
{
    m_refreshTimer->setInterval(intervalMs);
    m_refreshTimer->start();
}
```

**改造后**：

```cpp
void StockDataSource::registerToDataHub()
{
    // 通过 DataHub 订阅，不再独立刷新
    for (const auto& symbol : m_symbols) {
        dataHub().requestRefresh(
            QString("market:quote:%1").arg(symbol),
            TopicPolicy{.minIntervalMs = 5000}
        );
    }
}
```

#### 2. 页面级别改造

**改造前**（DashboardPage）：

```cpp
d->updateTimer = new QTimer(this);
d->updateTimer->setInterval(3000);
connect(d->updateTimer, &QTimer::timeout, this, &DashboardPage::updateRealTimeData);
d->updateTimer->start();
```

**改造后**：

```cpp
// 使用 DataHubPageBase 提供的订阅方法
void DashboardPage::initializePage()
{
    // 订阅指数数据，DataHub 自动管理刷新
    for (const auto& symbol : indexSymbols) {
        subscribeQuote(symbol, [this](const StockQuote& quote) {
            updateIndexDisplay(quote);
        });
    }
    // 无需手动启动定时器
}
```

#### 3. WebSocket 心跳保持

WebSocket 心跳是必须独立运行的，保持不变：

```cpp
// MarketDataWebSocket.cpp
d->heartbeatTimer->setInterval(30000);  // 30秒心跳
d->heartbeatTimer->start();
```

### 改造优先级

| 优先级 | 模块                   | 说明    |
|-----|----------------------|-------|
| P0  | StockDataSource      | 主要数据源 |
| P0  | DashboardPage        | 核心页面  |
| P1  | CryptoDataSource     | 数字货币  |
| P1  | ForexDataSource      | 外汇    |
| P1  | FundDataSource       | 基金    |
| P2  | ConditionOrderEngine | 条件单引擎 |
| P2  | RiskController       | 风控    |

### 预期效果

- 定时器数量减少 50%
- 刷新冲突减少
- 统一刷新策略配置
- 便于调试和监控

## 实施清单

- [ ] DataHub 增加 TopicPolicy 配置接口
- [ ] StockDataSource 移除 startAutoRefresh
- [ ] DashboardPage 使用 DataHub 订阅
- [ ] 其他 DataSource 逐步改造
- [ ] 移除未使用的 QTimer
