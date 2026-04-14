# 期货页面集成 - 使用指南

## 概述

本文档说明如何将 `FuturesQuotesPage`（行情列表页）和 `FuturesKLinePage`（K线详情页）集成使用，实现点击行情列表跳转到K线详情页，并接入实时行情数据。

---

## 功能特性

### 1. 页面跳转
- ✅ 双击行情列表项跳转到K线详情页
- ✅ 自动传递合约代码和参数
- ✅ 支持返回导航

### 2. 实时行情
- ✅ K线详情页自动订阅选中合约的实时行情
- ✅ 实时更新K线图表
- ✅ 实时更新五档盘口
- ✅ 实时更新技术指标

### 3. 数据共享
- ✅ CTP连接共享
- ✅ 行情数据缓存
- ✅ 合约信息共享

---

## 架构设计

```
┌─────────────────────────────────────────────────────────┐
│              FuturesPageIntegration                      │
│                  (集成管理器)                            │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────────┐      ┌─────────────────┐         │
│  │ FuturesQuotesPage│      │ FuturesKLinePage │         │
│  │  (行情列表页)    │      │  (K线详情页)     │         │
│  ├─────────────────┤      ├─────────────────┤         │
│  │ - 行情列表      │      │ - K线图表       │         │
│  │ - 合约搜索      │      │ - 技术指标      │
│  │ - 实时更新      │      │ - 五档盘口      │         │
│  │ - 双击跳转 ────┼─────>│ - 交易下单      │         │
│  └─────────────────┘      └─────────────────┘         │
│           │                        │                   │
│           └────────────────────────┘                   │
│                    │                                    │
│                    ▼                                    │
│         ┌──────────────────┐                           │
│         │  PageNavigator   │                           │
│         │  (导航管理器)    │                           │
│         └──────────────────┘                           │
│                    │                                    │
│                    ▼                                    │
│         ┌──────────────────┐                           │
│         │   CTPPlugin      │                           │
│         │  (CTP服务插件)   │                           │
│         └──────────────────┘                           │
└─────────────────────────────────────────────────────────┘
```

---

## 使用方法

### 方法一：使用集成管理器（推荐）

```cpp
#include "FuturesPageIntegration.h"
#include <QStackedWidget>

// 1. 创建页面容器
QStackedWidget* stackedWidget = new QStackedWidget(this);

// 2. 初始化集成管理器
FuturesPageIntegration::instance().initialize(stackedWidget);

// 3. 显示页面
stackedWidget->show();

// 4. 导航到K线详情页
FuturesPageIntegration::instance().showKLinePage("IF2504");

// 5. 返回行情列表页
FuturesPageIntegration::instance().goBack();
```

### 方法二：手动集成

```cpp
#include "FuturesQuotesPage.h"
#include "FuturesKLinePage.h"
#include "PageNavigator.h"
#include <QStackedWidget>

// 1. 创建页面
QStackedWidget* stackedWidget = new QStackedWidget(this);
FuturesQuotesPage* quotesPage = new FuturesQuotesPage(stackedWidget);
FuturesKLinePage* klinePage = new FuturesKLinePage(stackedWidget);

// 2. 添加到容器
stackedWidget->addWidget(quotesPage);
stackedWidget->addWidget(klinePage);

// 3. 连接导航信号
connect(quotesPage, &FuturesQuotesPage::navigateToKLinePage,
        this, [klinePage, stackedWidget](const QString& instrumentId, 
                                         const QVariantMap& params) {
    // 设置合约
    klinePage->setInstrument(instrumentId);
    
    // 激活页面
    klinePage->onPageActivated(params);
    
    // 切换显示
    stackedWidget->setCurrentWidget(klinePage);
});

// 4. 显示行情列表页
stackedWidget->setCurrentWidget(quotesPage);
```

---

## 信号流程

### 1. 双击行情列表项

```
用户双击行情列表项
    ↓
FuturesQuotesPage::onRowDoubleClicked()
    ↓
emit navigateToKLinePage(instrumentId, params)
    ↓
FuturesPageIntegration::onNavigateToKLinePage()
    ↓
PageNavigator::navigateTo("FuturesKLinePage", params)
    ↓
FuturesKLinePage::onPageActivated(params)
    ↓
FuturesKLinePage::setInstrument(instrumentId)
    ↓
CTPPlugin::subscribeMarketData({instrumentId})
    ↓
实时行情数据推送
```

### 2. 实时行情更新

```
CTP行情数据到达
    ↓
CTPPlugin::onMarketData()
    ↓
emit marketDataUpdated(data)
    ↓
FuturesKLinePage::onQuoteUpdated(data)
    ↓
更新K线图表
更新五档盘口
更新技术指标
更新交易面板
```

---

## 关键代码说明

### 1. 行情列表页跳转逻辑

```cpp
// FuturesQuotesPage.cpp

void FuturesQuotesPage::onRowDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    // 获取点击的合约信息
    QModelIndex sourceIndex = d->m_proxyModel->mapToSource(index);
    auto item = d->m_model->itemAt(sourceIndex.row());
    if (!item) return;

    // 构建导航参数
    QVariantMap params;
    params[NavParam::INSTRUMENT_ID] = item->contractName;
    params[NavParam::INSTRUMENT_NAME] = item->contractName;
    params[NavParam::SOURCE_PAGE] = pageId();
    
    // 发送导航信号
    emit navigateToKLinePage(item->contractName, params);
}
```

### 2. K线详情页接收参数

```cpp
// FuturesKLinePage.cpp

void FuturesKLinePage::onPageActivated(const QVariantMap& params)
{
    // 处理导航参数
    if (params.contains(NavParam::INSTRUMENT_ID)) {
        QString instrumentId = params[NavParam::INSTRUMENT_ID].toString();
        setInstrument(instrumentId);
    }
    
    if (params.contains(NavParam::PERIOD)) {
        int periodValue = params[NavParam::PERIOD].toInt();
        setPeriod(static_cast<KLinePeriod>(periodValue));
    }
}

void FuturesKLinePage::setInstrument(const QString& instrumentId)
{
    d->instrumentId = instrumentId;
    
    // 更新UI
    if (d->quoteWidget) {
        d->quoteWidget->setInstrument(instrumentId);
    }
    if (d->tradingPanel) {
        d->tradingPanel->setInstrument(instrumentId);
    }
    
    // 加载K线数据
    loadKLineData();
    
    // 订阅实时行情
    if (d->ctpPlugin) {
        d->ctpPlugin->subscribeMarketData({instrumentId});
    }
}
```

### 3. 实时行情更新

```cpp
// FuturesKLinePage.cpp

void FuturesKLinePage::onQuoteUpdated(const MarketData& quote)
{
    d->currentQuote = quote;
    
    // 更新行情显示
    updateQuoteDisplay(quote);
    
    // 更新交易面板价格
    if (d->tradingPanel) {
        d->tradingPanel->setPrice(quote.lastPrice);
    }
    
    // 更新K线（如果是当前周期）
    if (!d->klineData.isEmpty()) {
        KLineData lastKline = d->klineData.last();
        
        // 更新最高价、最低价、收盘价
        lastKline.high = qMax(lastKline.high, quote.lastPrice);
        lastKline.low = qMin(lastKline.low, quote.lastPrice);
        lastKline.close = quote.lastPrice;
        lastKline.volume = quote.volume;
        
        d->klineChart->updateLastData(lastKline);
    }
}
```

---

## 配置选项

### 1. 导航参数

```cpp
namespace NavParam {
    // 合约相关
    constexpr const char* INSTRUMENT_ID = "instrumentId";      // 合约代码
    constexpr const char* INSTRUMENT_NAME = "instrumentName";  // 合约名称
    constexpr const char* EXCHANGE_ID = "exchangeId";          // 交易所代码
    
    // K线相关
    constexpr const char* PERIOD = "period";                   // K线周期
    constexpr const char* SHOW_INDICATORS = "showIndicators";  // 显示指标
    
    // 交易相关
    constexpr const char* DIRECTION = "direction";             // 买卖方向
    constexpr const char* PRICE = "price";                     // 价格
    constexpr const char* VOLUME = "volume";                   // 数量
    
    // 来源页面
    constexpr const char* SOURCE_PAGE = "sourcePage";          // 来源页面ID
}
```

### 2. 页面注册

```cpp
// 注册页面到导航器
PageNavigator::instance().registerPage("FuturesQuotesPage", []() -> BasePage* {
    return new FuturesQuotesPage();
});

PageNavigator::instance().registerPage("FuturesKLinePage", []() -> BasePage* {
    return new FuturesKLinePage();
});
```

---

## 性能优化

### 1. 数据缓存
- K线数据缓存5分钟
- 行情数据实时更新
- 技术指标计算缓存

### 2. 订阅管理
- 自动订阅选中合约
- 离开页面自动取消订阅
- 批量订阅优化

### 3. 渲染优化
- 双缓冲绘制
- 仅更新变化部分
- 数据压缩显示

---

## 错误处理

### 1. CTP连接失败

```cpp
// 自动重连机制
if (!d->m_isCtpConnected.load()) {
    LOG_WARNING("CTP not connected, attempting reconnection...");
    if (d->m_CTPService) {
        d->m_CTPService->setupConnections();
    }
}
```

### 2. 数据异常

```cpp
// 数据有效性检查
if (!index.isValid()) return;

auto item = d->m_model->itemAt(sourceIndex.row());
if (!item) return;
```

### 3. 页面未初始化

```cpp
if (!d->stackedWidget || !d->klinePage) {
    LOG_WARNING("Cannot show kline page: not initialized");
    return;
}
```

---

## 扩展功能

### 1. 添加更多导航参数

```cpp
// 传递更多参数
QVariantMap params;
params[NavParam::INSTRUMENT_ID] = instrumentId;
params[NavParam::PERIOD] = static_cast<int>(KLinePeriod::Minute15);
params[NavParam::SHOW_INDICATORS] = QStringList{"MA5", "MA10", "MACD"};

emit navigateToKLinePage(instrumentId, params);
```

### 2. 添加页面切换动画

```cpp
// 使用QPropertyAnimation实现平滑过渡
QPropertyAnimation* animation = new QPropertyAnimation(stackedWidget, "geometry");
animation->setDuration(300);
animation->start();
```

### 3. 添加历史记录

```cpp
// 导航器自动记录历史
PageNavigator::instance().navigateTo("FuturesKLinePage", params);

// 返回上一页
if (PageNavigator::instance().canGoBack()) {
    PageNavigator::instance().goBack();
}
```

---

## 测试方法

### 1. 单元测试

```cpp
void TestFuturesPageIntegration::testNavigation()
{
    QStackedWidget stackedWidget;
    FuturesPageIntegration::instance().initialize(&stackedWidget);
    
    // 测试跳转
    FuturesPageIntegration::instance().showKLinePage("IF2504");
    QCOMPARE(stackedWidget.currentWidget(), 
             FuturesPageIntegration::instance().klinePage());
    
    // 测试返回
    FuturesPageIntegration::instance().goBack();
    QCOMPARE(stackedWidget.currentWidget(), 
             FuturesPageIntegration::instance().quotesPage());
}
```

### 2. 集成测试

```cpp
void TestFuturesPageIntegration::testRealtimeQuote()
{
    // 模拟行情数据
    MarketData quote;
    quote.instrumentId = "IF2504";
    quote.lastPrice = 3850.0;
    
    // 发送行情更新
    emit ctpPlugin->marketDataUpdated(quote);
    
    // 验证K线页面更新
    QCOMPARE(klinePage->currentQuote().lastPrice, 3850.0);
}
```

---

## 常见问题

### Q1: 双击没有跳转？
**A**: 检查信号连接是否正确：
```cpp
connect(d->m_tableView, &QTableView::doubleClicked, 
        this, &FuturesQuotesPage::onRowDoubleClicked);
```

### Q2: K线页面没有行情数据？
**A**: 检查CTP订阅是否成功：
```cpp
if (d->ctpPlugin) {
    bool success = d->ctpPlugin->subscribeMarketData({instrumentId});
    LOG_DEBUG(QString("Subscribe result: %1").arg(success));
}
```

### Q3: 如何自定义跳转行为？
**A**: 重写 `onRowDoubleClicked` 方法：
```cpp
void FuturesQuotesPage::onRowDoubleClicked(const QModelIndex &index)
{
    // 自定义逻辑
    ...
    
    // 调用基类方法
    BaseClass::onRowDoubleClicked(index);
}
```

---

## 更新日志

### v2.0.0 (2026-04-14)
- ✅ 实现页面跳转功能
- ✅ 实现实时行情订阅
- ✅ 创建页面导航管理器
- ✅ 创建集成管理器
- ✅ 完善文档和示例

---

**作者**: WealthPilot Team  
**版本**: 2.0.0  
**更新日期**: 2026-04-14
