# K线页面数据源集成说明

## 📊 数据源架构

K线页面已经完整集成了真实数据源，支持以下数据获取方式：

### 1. 数据源类型

**StockDataSource** 支持三种数据源：
- **新浪财经** (默认) - 免费，无需API Key
- **腾讯财经** - 免费
- **东方财富** - 免费

### 2. 数据加载流程

```
用户请求 → 缓存检查 → 数据库检查 → 网络获取
    ↓           ↓           ↓           ↓
  显示数据   命中则显示   命中则显示   请求API
             后台更新     后台更新     保存到缓存/DB
```

### 3. 支持的数据类型

#### 实时行情 (StockQuote)
- 股票代码、名称
- 最新价、开盘价、最高价、最低价
- 昨收价、收盘价
- 成交量、成交额
- 涨跌幅、涨跌额
- 更新时间

#### K线数据 (KLineData)
- 时间戳
- 开盘价、最高价、最低价、收盘价
- 成交量
- 支持多种周期：
  - 日K、周K、月K
  - 5分钟、15分钟、30分钟、60分钟

## 🔧 使用方法

### 在代码中使用

```cpp
// 1. 创建数据源
auto* dataSource = new StockDataSource(StockDataSource::Source::Sina, this);

// 2. 连接信号
connect(dataSource, &StockDataSource::kLineReceived,
        this, &MyClass::onKLineReceived);
connect(dataSource, &StockDataSource::quotesReceived,
        this, &MyClass::onQuotesReceived);

// 3. 请求数据
// 请求K线数据
dataSource->requestKLine("sh600000", KLinePeriod::Day1, 500);

// 请求实时行情
dataSource->requestQuotes({"sh600000", "sz000001"});

// 4. 启动自动刷新（可选）
dataSource->startAutoRefresh(5000); // 5秒刷新一次
```

### 在StockKLinePage中使用

```cpp
// 设置股票代码
stockKLinePage->setStock("sh600000", "浦发银行");

// 设置K线周期
stockKLinePage->setPeriod(StockKLinePeriod::Day);

// 切换图表类型
stockKLinePage->setChartType(ChartType::KLine);
```

## 📡 API说明

### 新浪财经API

#### 实时行情
```
URL: http://hq.sinajs.cn/list=sh600000,sz000001
返回格式: var hq_str_sh600000="浦发银行,10.50,10.45,10.52,10.60,10.40,10.51,10.52,12345678,130000000,..."
```

#### K线数据
```
URL: http://money.finance.sina.com.cn/quotes_service/api/json_v2.php/CN_MarketData.getKLineData?symbol=sh600000&scale=daily&datalen=500
返回格式: 日期,开盘,最高,最低,收盘,成交量
```

### 股票代码格式

- 上海证券交易所: `sh` + 6位代码 (如 sh600000)
- 深圳证券交易所: `sz` + 6位代码 (如 sz000001)

## 💾 缓存机制

### 内存缓存
- 使用 `CacheManager` 进行内存缓存
- K线数据缓存时间: 5分钟
- 行情数据缓存时间: 实时更新

### 数据库存储
- 自动保存K线历史数据
- 支持离线查看
- 增量更新机制

## 🔄 自动刷新

```cpp
// 启动自动刷新（5秒间隔）
dataSource->startAutoRefresh(5000);

// 停止自动刷新
dataSource->stopAutoRefresh();
```

## ⚠️ 注意事项

1. **网络权限**: 确保应用有网络访问权限
2. **API限制**: 新浪API有频率限制，建议间隔>3秒
3. **数据延迟**: 免费API可能有15分钟延迟
4. **错误处理**: 已实现完整的错误处理和重试机制

## 🧪 测试方法

### 测试K线数据获取

```cpp
// 在StockKLinePage中测试
void testKLine() {
    setStock("sh600000", "浦发银行");
    // 等待数据加载完成，查看日志输出
}
```

### 测试实时行情

```cpp
void testQuotes() {
    if (!m_dataSource) {
        m_dataSource = new StockDataSource(StockDataSource::Source::Sina, this);
        connect(m_dataSource, &StockDataSource::quotesReceived,
                [](const QVector<StockQuote>& quotes) {
                    for (const auto& q : quotes) {
                        qDebug() << q.symbol << q.name << q.lastPrice;
                    }
                });
    }
    m_dataSource->requestQuotes({"sh600000", "sz000001"});
}
```

## 📝 日志输出

数据源会输出详细的日志信息：

```
[INFO] StockDataSource created, source: 0
[INFO] Requesting KLine: sh600000, period: 0
[DEBUG] Parsed 500 KLines for sh600000
[INFO] KLine data received: 500 items for sh600000
```

## 🎯 下一步

1. **测试运行**: 运行程序，查看K线数据是否正常加载
2. **性能优化**: 根据实际使用情况调整缓存策略
3. **错误处理**: 添加用户友好的错误提示
4. **数据验证**: 验证数据的准确性和完整性

## 📚 相关文件

- `src/market/StockDataSource.h` - 数据源头文件
- `src/market/StockDataSource.cpp` - 数据源实现
- `src/views/stock/StockKLinePage.h` - K线页面头文件
- `src/views/stock/StockKLinePage.cpp` - K线页面实现
- `src/core/data/CacheManager.h` - 缓存管理器
- `src/core/data/DatabaseManager.h` - 数据库管理器
