# DataHub 页面迁移指南

本文档说明如何将现有页面迁移到使用 DataHub 数据中心。

## 迁移步骤

### 1. 修改头文件

**修改前**:

```cpp
#include "ui/components/BasePage.h"

class MyPage : public BasePage {
    QTimer* m_refreshTimer;  // ❌ 独立定时器
};
```

**修改后**:

```cpp
#include "ui/components/DataHubPageBase.h"

class MyPage : public DataHubPageBase {
    // ✅ 无需定时器，DataHub 统一调度
};
```

### 2. 修改初始化方法

**修改前**:

```cpp
void MyPage::initializePage() {
    // ❌ 创建独立定时器
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, [this]() {
        m_dataSource->fetchData();  // ❌ 直接调用服务
    });
    m_refreshTimer->start(5000);
}
```

**修改后**:

```cpp
void MyPage::initializePage() {
    // ✅ 设置 DataHub 订阅
    setupDataHubSubscriptions();
}

void MyPage::setupDataHubSubscriptions() {
    // 订阅行情数据
    for (const auto& symbol : m_symbols) {
        subscribeQuote(symbol, [this](const StockQuote& quote) {
            updateDisplay(quote);
        });
    }
    
    // 请求初始数据
    requestData(buildTopics(m_symbols), true);
}
```

### 3. 修改刷新方法

**修改前**:

```cpp
void MyPage::onRefresh() {
    m_dataSource->fetchData();  // ❌ 直接调用
}
```

**修改后**:

```cpp
void MyPage::onRefresh() {
    // ✅ 通过 DataHub 请求刷新
    requestData(m_topics, true);  // force = true
}
```

### 4. 删除清理代码

**修改前**:

```cpp
MyPage::~MyPage() {
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
    m_dataSource->unsubscribe(this);  // ❌ 手动取消订阅
}
```

**修改后**:

```cpp
// ✅ 无需手动清理，DataHub 自动管理
// 页面销毁时自动取消所有订阅
```

## 常用订阅方法

### 订阅股票行情

```cpp
// 方法1: 订阅特定股票
subscribeQuote("sh600000", [this](const StockQuote& quote) {
    updateQuote(quote);
});

// 方法2: 模式订阅（监听所有股票）
dataHub().subscribePattern(this, "market:quote:*",
    [this](const QString& topic, const QVariant& value) {
        // 处理更新
    });
```

### 订阅行情快照

```cpp
subscribeSnapshot("IF2501", [this](const MarketSnapshot& snapshot) {
    updateSnapshot(snapshot);
});
```

### 订阅 K 线数据

```cpp
subscribeKLine("sh600000", "day1", [this](const QVector<KLineData>& data) {
    updateKLineChart(data);
});
```

### 请求刷新数据

```cpp
// 刷新单个 Topic
requestData("market:quote:sh600000", true);

// 批量刷新
QStringList topics;
topics << "market:quote:sh600000" << "market:quote:sh600519";
requestData(topics, true);
```

### 获取缓存数据

```cpp
// 获取缓存的行情
auto quote = getCachedQuote("sh600000");
if (quote.has_value()) {
    // 使用数据
}
```

## Topic 命名规范

| Topic 模式                         | 说明   | 示例                           |
|----------------------------------|------|------------------------------|
| `market:quote:{symbol}`          | 股票行情 | `market:quote:sh600000`      |
| `market:futures:{symbol}`        | 期货行情 | `market:futures:IF2501`      |
| `market:kline:{symbol}:{period}` | K线数据 | `market:kline:sh600000:day1` |
| `market:timeshare:{symbol}`      | 分时数据 | `market:timeshare:sh600000`  |
| `market:snapshot:{symbol}`       | 行情快照 | `market:snapshot:IF2501`     |

## 迁移检查清单

- [ ] 修改基类为 `DataHubPageBase`
- [ ] 删除独立的 `QTimer`
- [ ] 删除直接调用数据服务的代码
- [ ] 实现 `setupDataHubSubscriptions()` 方法
- [ ] 使用 `subscribeQuote()` 等便捷方法订阅数据
- [ ] 使用 `requestData()` 请求刷新
- [ ] 删除手动取消订阅的代码
- [ ] 测试页面功能正常

## 已迁移页面

| 页面                | 状态    | 说明     |
|-------------------|-------|--------|
| StockQuotesPage   | ✅ 已完成 | 股票行情页面 |
| FuturesQuotesPage | ⏳ 待迁移 | 期货行情页面 |
| DashboardPage     | ⏳ 待迁移 | 仪表盘页面  |
| WatchListPage     | ⏳ 待迁移 | 自选股页面  |
| PortfolioPage     | ⏳ 待迁移 | 持仓页面   |
| NewsPage          | ⏳ 待迁移 | 新闻页面   |

## 注意事项

1. **生命周期管理**: 页面销毁时 DataHub 自动取消订阅，无需手动处理
2. **线程安全**: DataHub 的信号槽机制确保 UI 更新在主线程
3. **数据缓存**: DataHub 自动缓存数据，`peek()` 可获取缓存值
4. **刷新策略**: 由 DataHub 统一调度，避免重复请求
5. **错误处理**: Producer 获取失败时，订阅者不会收到更新

## 参考示例

完整示例请参考 `StockQuotesPage` 的实现：

- `src/views/stock/StockQuotesPage.h`
- `src/views/stock/StockQuotesPage.cpp`