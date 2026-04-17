# WealthPilot 架构文档

## 项目概述

WealthPilot 是一个专业级金融投资理财AI助手PC客户端，基于 Qt 6.10.2 + C++17 开发。

## 目录结构

```
D:\C++\wealth-pilot\
├── src/
│   ├── app/                    # 应用层
│   │   └── ApplicationInitializer.*  # 应用初始化
│   │
│   ├── core/                   # 核心层
│   │   ├── base/               # 基础类
│   │   │   ├── BasePage.*      # 页面基类
│   │   │   ├── Singleton.h     # 单例模板
│   │   │   └── ErrorCode.*     # 错误码定义
│   │   ├── types/              # 类型定义
│   │   │   └── MarketTypes.h   # 市场数据类型
│   │   ├── cache/              # 缓存系统
│   │   ├── config/             # 配置管理
│   │   ├── database/           # 数据库
│   │   ├── di/                 # 依赖注入
│   │   ├── navigation/         # 页面导航
│   │   └── task/               # 异步任务
│   │
│   ├── ctp/                    # CTP接口层
│   │   ├── api/                # CTP API封装
│   │   ├── config/             # CTP配置
│   │   └── service/            # CTP服务
│   │
│   ├── ai/                     # AI服务层
│   │   ├── service/            # AI服务
│   │   └── plugin/             # AI插件
│   │
│   ├── plugins/                # 插件框架
│   │   ├── IPlugin.h           # 插件接口
│   │   ├── ICTPPlugin.h        # CTP插件接口
│   │   └── IAIPlugin.h         # AI插件接口
│   │
│   ├── ui/                     # UI层
│   │   ├── components/         # UI组件
│   │   │   ├── ThemeColors.h   # 主题颜色
│   │   │   ├── BaseChartWidget.*  # 图表基类
│   │   │   ├── KLineChart.*    # K线图组件
│   │   │   ├── ChartToolBar.*  # 图表工具栏
│   │   │   ├── MarketDepthWidget.*  # 盘口组件
│   │   │   ├── TickTableView.* # 分笔成交表
│   │   │   └── ChartStatusBar.* # 图表状态栏
│   │   ├── animation/          # 动画系统
│   │   └── ThemeManager.*      # 主题管理
│   │
│   ├── views/                  # 视图层
│   │   ├── mainWindow/         # 主窗口
│   │   ├── dashboard/          # 仪表盘
│   │   ├── futures/            # 期货页面
│   │   ├── stock/              # 股票页面
│   │   ├── settings/           # 设置页面
│   │   └── widgets/            # 通用控件
│   │
│   ├── models/                 # 数据模型
│   ├── network/                # 网络层
│   ├── market/                 # 行情数据
│   └── utils/                  # 工具类
│
├── resources/                  # 资源文件
│   ├── style/                  # 样式表
│   ├── icons/                  # 图标
│   └── fonts/                  # 字体
│
└── external/                   # 外部依赖
    └── ctp/                    # CTP库
```

## 核心架构

### 1. 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                      Views (视图层)                          │
│  MainWindow, DashboardPage, FuturesKLinePage, etc.          │
├─────────────────────────────────────────────────────────────┤
│                     UI Components (UI组件层)                 │
│  KLineChart, ChartToolBar, MarketDepthWidget, etc.          │
├─────────────────────────────────────────────────────────────┤
│                      Core (核心层)                           │
│  Navigation, DI, Cache, Config, Database                    │
├─────────────────────────────────────────────────────────────┤
│                    Services (服务层)                         │
│  CTPService, AIService, NetworkManager                      │
├─────────────────────────────────────────────────────────────┤
│                     Plugins (插件层)                         │
│  ICTPPlugin, IAIPlugin                                       │
└─────────────────────────────────────────────────────────────┘
```

### 2. 依赖注入 (ServiceLocator)

```cpp
// 注册服务
ServiceLocator::instance().registerSingleton<ICTPPlugin, CTPPlugin>();

// 解析服务
ICTPPlugin* ctpPlugin = ServiceLocator::instance().resolve<ICTPPlugin>();
```

### 3. 页面导航 (PageNavigator)

```cpp
// 注册页面
PageFactoryRegistry::instance().registerPage("FuturesKLine", 
    []() -> BasePage* { return new FuturesKLinePage(); });

// 导航到页面
PageNavigator::instance().navigateTo("FuturesKLine", {
    {"instrumentId", "IF2501"},
    {"instrumentName", "沪深300指数期货"}
});
```

### 4. PIMPL 模式

所有组件使用 PIMPL (Pointer to Implementation) 模式：

```cpp
// 头文件
class MyWidget : public QWidget {
    struct Impl;
    std::unique_ptr<Impl> d;
};

// 实现文件
struct MyWidget::Impl {
    // 所有私有成员
    QLabel* label = nullptr;
    int value = 0;
};
```

## 组件设计

### K线图组件 (KLineChart)

```
KLineChart
├── 数据管理
│   ├── setData()        设置K线数据
│   ├── addData()        添加K线数据
│   └── updateLastData() 更新最后一条
├── 视图控制
│   ├── zoom()           缩放
│   ├── pan()            平移
│   └── resetView()      重置视图
├── 技术指标
│   ├── addIndicator()   添加指标
│   └── clearIndicators() 清空指标
└── 绘制方法
    ├── drawCandles()    绘制K线
    ├── drawVolume()     绘制成交量
    └── drawIndicators() 绘制指标
```

### 工具栏组件 (ChartToolBar)

```
ChartToolBar
├── 周期选择 (QComboBox)
│   └── 分时/1分/5分/15分/30分/60分/日线/周线/月线
├── 复权按钮 (QToolButton + Menu)
│   └── 不复权/前复权/后复权
├── 指标按钮 (QToolButton + Menu)
│   └── MA/MACD/RSI/KDJ/BOLL
├── 画线按钮 (QToolButton + Menu)
│   └── 趋势线/水平线/平行线/黄金分割
└── 图表类型按钮 (QToolButton + Menu)
    └── K线图/分时图
```

## 性能优化

### 1. 绘制优化

- **双缓冲绘制**：避免闪烁
- **数据压缩**：大量数据时自动压缩
- **延迟更新**：高频更新时合并刷新
- **跳过无效数据**：绘制时跳过无效K线

### 2. 内存优化

- **PIMPL 模式**：减少头文件依赖
- **智能指针**：自动管理内存
- **缓存管理**：LRU 缓存策略

### 3. 数据优化

- **分笔成交限制**：最大500条记录
- **指标增量计算**：避免全量重算
- **缓存历史数据**：减少重复请求

## 代码规范

### 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 类名 | PascalCase | `KLineChart` |
| 方法名 | camelCase | `setInstrument()` |
| 变量名 | camelCase | `instrumentId` |
| 成员变量 | m_ 前缀 | `m_showGrid` |
| 常量 | UPPER_CASE | `MAX_ROWS` |
| 枚举 | PascalCase | `KLinePeriod` |

### 注释规范

```cpp
/**
 * @brief 简短描述
 * @details 详细描述
 * @param name 参数说明
 * @return 返回值说明
 * @example 使用示例
 */
```

### 信号槽规范

```cpp
// 信号命名：过去式或名词
signals:
    void periodChanged(KLinePeriod period);
    void marketDataReceived(const MarketData& data);

// 槽函数命名：on + 信号名
private slots:
    void onPeriodChanged(KLinePeriod period);
    void onMarketDataReceived(const MarketData& data);
```

## 扩展指南

### 添加新页面

1. 继承 `BasePage`
2. 实现 `pageId()` 和 `initializePage()`
3. 注册到 `PageFactoryRegistry`

```cpp
class MyPage : public BasePage {
    Q_OBJECT
public:
    QString pageId() const override { return "MyPage"; }
    void initializePage() override { /* ... */ }
};

// 注册
PageFactoryRegistry::instance().registerPage("MyPage", 
    []() -> BasePage* { return new MyPage(); });
```

### 添加新指标

1. 在 `TechnicalIndicators` 中添加计算方法
2. 在 `ChartToolBar` 中添加菜单项
3. 在 `FuturesKLinePage::calculateIndicators()` 中调用

### 添加新组件

1. 继承 `BaseChartWidget` 或 `QWidget`
2. 使用 PIMPL 模式
3. 添加到 `src/ui/components/`

## 版本历史

- **v2.0.0** (2026-04-17)
  - 重构 K线页面，拆分组件
  - 添加统一的类型定义
  - 添加主题颜色配置
  - 优化性能和代码结构

- **v1.0.0** (2026-03-01)
  - 初始版本
