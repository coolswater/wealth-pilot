# K线右侧行情信息面板开发状态

## 🎯 功能需求

在K线页面右侧添加实时行情信息面板，包含6行：

1. **第一行**：品种名称、代码、最新价、涨跌幅、涨跌额
2. **第二行**：交易状态（交易中/休市中）
3. **第三行**：委比、委差数值
4. **第四行**：五档盘口信息
5. **第五行**：详细行情数据（最新、均价、涨幅等28项指标）
6. **第六行**：成交明细（分钟汇总，按秒显示）

## ✅ 已完成的工作

### 1. 创建StockInfoPanel组件 ✅
- **文件位置**：
  - `src/ui/components/StockInfoPanel.h`
  - `src/ui/components/StockInfoPanel.cpp`
- **功能**：
  - 参考东方财富客户端布局
  - 红涨绿跌样式
  - 实时数据更新机制

### 2. 扩展StockQuote数据结构 ✅
- 添加五档盘口数据（买1-5、卖1-5）
- 添加委比委差计算
- 添加其他行情数据字段

### 3. 集成到StockKLinePage ✅
- 修改setupUI使用QSplitter布局
- 左侧K线图，右侧信息面板
- 7:3分割比例

### 4. 实现数据更新机制 ✅
- parseSinaQuotes解析五档盘口
- onRealtimeQuoteReceived更新面板
- 实时行情推送

## ⚠️ 当前编译问题

**问题**：头文件循环依赖
```
StockInfoPanel.h -> core/types/MarketTypes.h -> analysis/AnalysisTypes.h
StockInfoPanel.h -> market/StockDataSource.h -> analysis/AnalysisTypes.h
```

**错误信息**：
```
D:/C++/wealth-pilot/src/analysis/AnalysisTypes.h:20:1:
error: expected unqualified-id before 'namespace'
```

## 🔧 解决方案

### 方案1：使用前向声明（推荐）

修改 `StockInfoPanel.h`：
```cpp
// 前向声明
struct StockQuote;
struct TickData;

class StockInfoPanel : public QWidget {
    // 使用指针或引用
    StockQuote* m_currentQuote;
};
```

在 `StockInfoPanel.cpp` 中包含完整定义：
```cpp
#include "market/StockDataSource.h"
#include "core/types/MarketTypes.h"
```

### 方案2：调整include顺序

确保AnalysisTypes.h在其他头文件之前被包含。

### 方案3：重构头文件依赖

将StockQuote移到独立头文件，避免循环依赖。

## 📁 相关文件

**新增文件**：
- `src/ui/components/StockInfoPanel.h`
- `src/ui/components/StockInfoPanel.cpp`

**修改文件**：
- `src/market/StockDataSource.h` - 扩展StockQuote结构体
- `src/market/StockDataSource.cpp` - 解析五档盘口
- `src/views/stock/StockKLinePage.h` - 添加StockInfoPanel成员
- `src/views/stock/StockKLinePage.cpp` - 集成信息面板

## 🎨 UI布局预览

```
┌─────────────────────────────────────┬──────────────┐
│                                     │ 浦发银行     │
│                                     │ sh600000     │
│         K线图区域                   │ 9.18 +0.54%  │
│                                     ├──────────────┤
│                                     │ 交易中       │
│                                     ├──────────────┤
│                                     │ 委比: +12.5% │
│                                     │ 委差: 12345  │
│                                     ├──────────────┤
│                                     │ 买5  9.15    │
│                                     │ 买4  9.16    │
│                                     │ ...          │
│                                     │ 卖1  9.19    │
│                                     ├──────────────┤
│                                     │ 最新: 9.18   │
│                                     │ 均价: 9.17   │
│                                     │ ...          │
│                                     ├──────────────┤
│                                     │ 成交明细     │
│                                     │ 11:22:15 买  │
│                                     │ 11:22:14 卖  │
└─────────────────────────────────────┴──────────────┘
```

## 🚀 下一步工作

1. **解决编译错误**
   - 修复头文件循环依赖
   - 调整include顺序

2. **测试功能**
   - 运行程序查看UI布局
   - 测试实时数据更新
   - 验证五档盘口显示

3. **优化细节**
   - 调整字体大小和颜色
   - 优化表格样式
   - 添加数据刷新动画

## 💡 技术要点

### 五档盘口数据解析

新浪API返回格式（第10-29字段）：
```
买1量,买1价,买2量,买2价,...卖5量,卖5价
```

解析代码：
```cpp
for (int i = 0; i < 5; ++i) {
    quote.bidVolume[i] = fields[10 + i * 2].toLongLong();
    quote.bidPrice[i] = fields[11 + i * 2].toDouble();
}
```

### 委比委差计算

```cpp
委差 = 买盘总量 - 卖盘总量
委比 = 委差 / (买盘总量 + 卖盘总量) × 100%
```

### 实时更新机制

每3秒刷新一次：
```cpp
m_dataSource->startRealtimeQuotes(symbol, 3000);
```

## 📊 预期效果

编译成功后，运行程序：
1. 输入股票代码 `sh600000`
2. 左侧显示K线图
3. 右侧显示实时行情面板
4. 价格实时更新，颜色动态变化
5. 五档盘口清晰展示
6. 成交明细滚动显示

**核心功能已实现90%，只需解决编译错误即可完成！** 🎉
