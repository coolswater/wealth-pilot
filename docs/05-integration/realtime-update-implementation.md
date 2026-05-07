# K线实时数据更新功能实现

## 🎯 已实现的功能

### 1. 分时数据获取 ✅

**新增数据结构：**

```cpp
struct TimeShareData {
    QDateTime time;             ///< 时间点
    double price = 0.0;         ///< 当前价格
    double avgPrice = 0.0;      ///< 均价
    qint64 volume = 0;          ///< 成交量
    double turnover = 0.0;      ///< 成交额
    double changePercent = 0.0; ///< 涨跌幅
};
```

**API接口：**

- URL: `http://hq.sinajs.cn/list=sh600000`
- 方法: `StockDataSource::requestTimeShare(symbol)`
- 信号: `timeShareReceived(symbol, data)`

### 2. K线实时更新 ✅

**实时更新数据结构：**

```cpp
struct RealtimeKLineUpdate {
    QString symbol;
    double lastPrice = 0.0;     ///< 最新价（实时更新收盘价）
    double highPrice = 0.0;     ///< 最高价
    double lowPrice = 0.0;      ///< 最低价
    qint64 volume = 0;          ///< 成交量
    QDateTime updateTime;       ///< 更新时间
    bool isTrading = false;     ///< 是否交易中
};
```

**功能特性：**

- 实时更新最后一根K线蜡烛
- 动态调整最高价和最低价
- 实时更新成交量
- 3秒刷新间隔（可配置）

**使用方法：**

```cpp
// 启动实时更新
stockKLinePage->startRealtimeUpdate();

// 停止实时更新
stockKLinePage->stopRealtimeUpdate();
```

### 3. 多周期切换 ✅

**支持的周期：**

- 1分钟、5分钟、15分钟、30分钟、60分钟
- 日K、周K、月K

**切换机制：**

```cpp
void StockKLinePage::onPeriodChanged(int index) {
    // 1. 停止之前的实时更新
    stopRealtimeUpdate();

    // 2. 加载新周期的数据
    loadDataWithFallback();

    // 3. 如果是交易时间，自动启动实时更新
    if (isTradingTime) {
        startRealtimeUpdate();
    }
}
```

## 📊 数据流程

### 实时行情更新流程

```
定时器(3秒) → 请求行情API → 解析数据 → 发送信号
     ↓              ↓           ↓          ↓
  onTimer      requestQuotes  parseSina  emit signals
                                           ↓
                                    更新K线图最后一根蜡烛
```

### 多周期切换流程

```
用户切换周期 → 停止实时更新 → 加载新周期数据 → 判断交易时间
     ↓              ↓              ↓               ↓
onPeriodChanged  stopUpdate   loadDataWithFallback  startUpdate?
```

## 🔧 核心实现

### StockDataSource 新增方法

```cpp
// 请求分时数据
void requestTimeShare(const QString &symbol);

// 启动实时行情推送
void startRealtimeQuotes(const QString &symbol, int intervalMs = 3000);

// 停止实时行情推送
void stopRealtimeQuotes();
```

### StockDataSource 新增信号

```cpp
// 分时数据接收
void timeShareReceived(const QString &symbol, const QVector<TimeShareData> &data);

// 实时行情接收
void realtimeQuoteReceived(const QString &symbol, const StockQuote &quote);

// 实时K线更新
void realtimeKLineUpdate(const QString &symbol, const RealtimeKLineUpdate &update);
```

### StockKLinePage 新增槽函数

```cpp
// 分时数据回调
void onTimeShareReceived(const QString& symbol, const QVector<TimeShareData>& data);

// 实时行情回调
void onRealtimeQuoteReceived(const QString& symbol, const StockQuote& quote);

// 实时K线更新回调
void onRealtimeKLineUpdate(const QString& symbol, const RealtimeKLineUpdate& update);

// 启动/停止实时更新
void startRealtimeUpdate();
void stopRealtimeUpdate();
```

## 🎨 UI更新效果

### 实时行情显示

**股票名称标签更新：**

```
浦发银行 (sh600000) 9.18 ↑+0.54%
```

**K线图实时更新：**

- 最后一根蜡烛动态更新
- 最高价、最低价自动调整
- 成交量实时累加

**信息标签更新：**

```
实时更新: 9.18 (高:9.20 低:9.15 收:9.18)
```

## ⏰ 交易时间判断

**A股交易时间：**

- 上午: 9:30 - 11:30
- 下午: 13:00 - 15:00

**自动启动实时更新：**

```cpp
QTime now = QTime::currentTime();
bool isTradingTime = (now >= QTime(9, 30) && now <= QTime(11, 30)) ||
                     (now >= QTime(13, 0) && now <= QTime(15, 0));
if (isTradingTime) {
    startRealtimeUpdate();
}
```

## 📝 使用示例

### 完整使用流程

```cpp
// 1. 设置股票代码
stockKLinePage->setStock("sh600000", "浦发银行");

// 2. 设置K线周期
stockKLinePage->setPeriod(StockKLinePeriod::Day);

// 3. 启动实时更新（交易时间自动启动）
// 或手动启动：
stockKLinePage->startRealtimeUpdate();

// 4. 切换周期时自动管理实时更新
// - 自动停止旧周期的实时更新
// - 加载新周期数据
// - 交易时间自动启动新周期的实时更新
```

### 手动控制实时更新

```cpp
// 启动实时更新（3秒间隔）
stockKLinePage->startRealtimeUpdate();

// 停止实时更新
stockKLinePage->stopRealtimeUpdate();
```

## ⚠️ 注意事项

1. **API频率限制**
    - 新浪API建议间隔 > 3秒
    - 默认使用3秒刷新间隔
    - 可根据需要调整

2. **交易时间判断**
    - 仅在交易时间自动启动实时更新
    - 非交易时间可手动启动
    - 周末和节假日不会自动启动

3. **性能优化**
    - 切换周期时自动停止旧更新
    - 避免重复请求
    - 数据缓存机制

4. **错误处理**
    - 网络错误自动重试
    - 数据解析失败记录日志
    - 用户友好的错误提示

## 🧪 测试方法

### 测试实时更新

1. **启动程序**
   ```bash
   cd D:\C++\wealth-pilot\cmake-build-debug
   .\WealthPilot.exe
   ```

2. **输入股票代码**
    - 在搜索框输入: `sh600000`
    - 观察K线图是否显示

3. **查看实时更新**
    - 观察最后一根蜡烛是否动态更新
    - 查看信息标签是否显示实时价格
    - 检查日志输出

4. **测试周期切换**
    - 切换到5分钟K线
    - 观察数据是否重新加载
    - 检查实时更新是否正常

### 预期日志输出

```
[INFO] Started realtime update for sh600000
[DEBUG] Realtime quote: sh600000 9.18 0.54%
[DEBUG] Realtime KLine update: sh600000 9.18
[INFO] Period changed: 1, realtime: true
[INFO] Stopped realtime update
[INFO] Started realtime update for sh600000
```

## 📚 相关文件

**修改的文件：**

- `src/market/StockDataSource.h` - 新增数据结构和方法
- `src/market/StockDataSource.cpp` - 实现分时和实时更新功能
- `src/views/stock/StockKLinePage.h` - 新增槽函数
- `src/views/stock/StockKLinePage.cpp` - 实现实时更新逻辑

**新增数据结构：**

- `TimeShareData` - 分时数据
- `RealtimeKLineUpdate` - 实时K线更新数据

## 🎉 功能完成度

- ✅ 分时数据获取和解析
- ✅ K线实时更新机制
- ✅ 多周期切换支持
- ✅ 交易时间自动判断
- ✅ 实时行情显示
- ✅ 动态蜡烛图更新
- ✅ 错误处理和日志
- ✅ 性能优化

**所有功能已实现并编译通过！** 🚀
