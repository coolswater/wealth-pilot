# WealthPilot MVVM 架构优化方案

## 一、架构设计目标

### 1. 混合架构策略

```
┌─────────────────────────────────────────────────────────────────┐
│                      应用层                                      │
├─────────────────────────────────────────────────────────────────┤
│  新功能: QML View ←→ ViewModel ←→ Service ←→ Model             │
│  旧功能: Widget View ←→ Controller ←→ Service ←→ Model         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                      服务层 (保持不变)                            │
│  ServiceLocator, TradingService, StockDataSource...            │
└─────────────────────────────────────────────────────────────────┘
```

### 2. 核心组件

| 组件            | 职责             | 技术实现                     |
|---------------|----------------|--------------------------|
| ViewModel     | 数据绑定、状态管理、命令处理 | QObject + Q_PROPERTY     |
| View (QML)    | UI 渲染、用户交互     | Qt Quick QML             |
| View (Widget) | 传统 UI 渲染       | QWidget (保持)             |
| Controller    | Widget 页面的业务逻辑 | 新增层                      |
| Model         | 数据结构           | QAbstractTableModel (保持) |
| Service       | 业务逻辑           | 保持不变                     |

---

## 二、ViewModel 基础框架

### 1. ViewModel 基类设计

```cpp
// ViewModelBase.h - 所有 ViewModel 的基类
class ViewModelBase : public QObject {
    Q_OBJECT
    
    // 公共属性
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    
public:
    // 命令执行
    template<typename Func>
    void executeCommand(Func func, const QString& description = QString());
    
    // 属性绑定
    void bindToService(QObject* service, const char* signal, 
                       const char* property, const char* handler);
    
signals:
    void isLoadingChanged();
    void errorMessageChanged();
    void statusMessageChanged();
    void commandExecuted(const QString& description, bool success);
};
```

### 2. 命令模式实现

```cpp
// Command.h - 命令封装
class Command : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool canExecute READ canExecute NOTIFY canExecuteChanged)
    Q_PROPERTY(bool isExecuting READ isExecuting NOTIFY isExecutingChanged)
    
public:
    void execute();
    void cancel();
    
signals:
    void executed();
    void cancelled();
    void canExecuteChanged();
    void isExecutingChanged();
};
```

---

## 三、迁移优先级

### Phase 1：核心交易模块（优先）

| 页面/组件           | 当前技术    | 迁移方案            | 优先级 |
|-----------------|---------|-----------------|-----|
| TradingPanel    | Widget  | ViewModel + QML | P0  |
| OrderDialog     | Widget  | ViewModel + QML | P0  |
| PositionManager | Widget  | ViewModel       | P1  |
| RiskController  | Service | 保持              | -   |

### Phase 2：行情展示模块

| 页面/组件             | 当前技术   | 迁移方案       | 优先级 |
|-------------------|--------|------------|-----|
| StockQuotesPage   | Widget | Controller | P1  |
| FuturesQuotesPage | Widget | Controller | P1  |
| MarketDepthWidget | Widget | ViewModel  | P2  |

### Phase 3：分析模块

| 页面/组件            | 当前技术   | 迁移方案            | 优先级 |
|------------------|--------|-----------------|-----|
| BacktestPage     | Widget | ViewModel + QML | P2  |
| SignalCenterPage | Widget | ViewModel       | P3  |

---

## 四、目录结构规划

```
src/
├── viewmodels/           # 新增：ViewModel 层
│   ├── ViewModelBase.h/cpp
│   ├── TradingViewModel.h/cpp
│   ├── OrderViewModel.h/cpp
│   ├── PositionViewModel.h/cpp
│   ├── MarketDepthViewModel.h/cpp
│   └── BacktestViewModel.h/cpp
│
├── controllers/          # 新增：Controller 层（Widget 页面）
│   ├── ControllerBase.h/cpp
│   ├── StockQuotesController.h/cpp
│   ├── FuturesQuotesController.h/cpp
│   └── WatchlistController.h/cpp
│
├── views/
│   ├── qml/              # 新增：QML 视图
│   │   ├── TradingPanel.qml
│   │   ├── OrderDialog.qml
│   │   ├── MarketDepth.qml
│   │   └── BacktestPanel.qml
│   │
│   ├── widgets/          # 保持：Widget 视图
│   │   ├── StockQuotesPage.cpp (使用 Controller)
│   │   └── FuturesQuotesPage.cpp (使用 Controller)
│   │
│   └── components/       # 保持：通用组件
│
├── models/               # 保持：数据模型
├── services/             # 保持：业务服务
└── core/                 # 保持：基础设施
```

---

## 五、数据绑定机制

### 1. QML 与 ViewModel 绑定

```qml
// TradingPanel.qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import WealthPilot.ViewModels 1.0

Item {
    id: root
    
    // ViewModel 实例
    TradingViewModel {
        id: viewModel
    }
    
    // 数据绑定
    Column {
        // 价格显示 - 自动绑定
        Text {
            text: viewModel.currentPrice.toFixed(2)
            color: viewModel.priceChange >= 0 ? "#F85149" : "#3FB950"
        }
        
        // 持仓信息 - 自动绑定
        Row {
            Text { text: "多头: " + viewModel.longPosition }
            Text { text: "空头: " + viewModel.shortPosition }
        }
        
        // 交易按钮 - 命令绑定
        Button {
            text: "买入开仓"
            enabled: viewModel.buyOpenCommand.canExecute
            onClicked: viewModel.buyOpenCommand.execute()
        }
    }
}
```

### 2. ViewModel 属性定义

```cpp
// TradingViewModel.h
class TradingViewModel : public ViewModelBase {
    Q_OBJECT
    
    // 数据属性 - 自动通知 QML 更新
    Q_PROPERTY(double currentPrice READ currentPrice NOTIFY currentPriceChanged)
    Q_PROPERTY(double priceChange READ priceChange NOTIFY priceChangeChanged)
    Q_PROPERTY(int longPosition READ longPosition NOTIFY positionChanged)
    Q_PROPERTY(int shortPosition READ shortPosition NOTIFY positionChanged)
    Q_PROPERTY(double availableFund READ availableFund NOTIFY fundChanged)
    
    // 命令属性 - QML 可直接调用
    Q_PROPERTY(Command* buyOpenCommand READ buyOpenCommand CONSTANT)
    Q_PROPERTY(Command* sellOpenCommand READ sellOpenCommand CONSTANT)
    Q_PROPERTY(Command* buyCloseCommand READ buyCloseCommand CONSTANT)
    Q_PROPERTY(Command* sellCloseCommand READ sellCloseCommand CONSTANT)
    
    // 状态属性
    Q_PROPERTY(QString instrumentId READ instrumentId NOTIFY instrumentChanged)
    Q_PROPERTY(QString instrumentName READ instrumentName NOTIFY instrumentChanged)
    
public:
    // 命令执行
    void executeBuyOpen();
    void executeSellOpen();
    void executeBuyClose();
    void executeSellClose();
    
signals:
    void currentPriceChanged();
    void priceChangeChanged();
    void positionChanged();
    void fundChanged();
    void instrumentChanged();
    void orderSubmitted(const QString& orderId);
};
```

---

## 六、Widget 页面 Controller 模式

### 1. Controller 基类

```cpp
// ControllerBase.h
class ControllerBase : public QObject {
    Q_OBJECT
    
public:
    explicit ControllerBase(QObject* parent = nullptr);
    
    // 绑定 View
    template<typename T>
    void bindView(T* view);
    
    // 服务注入
    template<typename T>
    T* getService();
    
protected:
    // 状态管理
    void setState(const QString& key, const QVariant& value);
    QVariant getState(const QString& key) const;
    
signals:
    void stateChanged(const QString& key, const QVariant& value);
    void errorOccurred(const QString& error);
};
```

### 2. StockQuotesController 示例

```cpp
// StockQuotesController.h
class StockQuotesController : public ControllerBase {
    Q_OBJECT
    
public:
    // 数据操作
    void refreshData();
    void searchData(const QString& keyword);
    void filterByMarket(const QString& market);
    
    // 导出操作
    void exportToCSV();
    void exportToExcel();
    
signals:
    void dataRefreshed(int count);
    void dataFiltered(int visibleCount);
    void exportCompleted(const QString& filePath);
    void exportFailed(const QString& error);
    
private:
    StockDataSource* m_dataSource;
    StockQuoteModel* m_model;
};

// StockQuotesPage.cpp - 使用 Controller
class StockQuotesPage : public BasePage {
public:
    void setupUI() {
        // UI 构建...
        m_controller = new StockQuotesController(this);
        
        // 连接 Controller 信号
        connect(m_refreshBtn, &QPushButton::clicked,
                m_controller, &StockQuotesController::refreshData);
        
        connect(m_controller, &StockQuotesController::dataRefreshed,
                this, [this](int count) {
                    m_statusLabel->setText(QString("已加载 %1 条").arg(count));
                });
    }
};
```

---

## 七、实现步骤

### Step 1：创建基础框架

1. 创建 `ViewModelBase` 基类
2. 创建 `Command` 命令类
3. 创建 `ControllerBase` 基类
4. 配置 QML 类型注册

### Step 2：迁移交易模块

1. 创建 `TradingViewModel`
2. 创建 `OrderViewModel`
3. 创建 QML 视图文件
4. 集成到 MainWindow

### Step 3：迁移行情模块

1. 创建 `StockQuotesController`
2. 修改 `StockQuotesPage` 使用 Controller
3. 创建 `FuturesQuotesController`
4. 修改 `FuturesQuotesPage` 使用 Controller

### Step 4：迁移分析模块

1. 创建 `BacktestViewModel`
2. 创建 QML 回测面板
3. 创建 `SignalCenterViewModel`

---

## 八、测试策略

### 1. ViewModel 单元测试

```cpp
// TradingViewModelTest.cpp
class TradingViewModelTest : public QObject {
    Q_OBJECT
    
private slots:
    void testBuyOpenCommand() {
        TradingViewModel viewModel;
        viewModel.setInstrument("IF2506", "沪深300", 4000.0);
        
        QSignalSpy spy(&viewModel, &TradingViewModel::orderSubmitted);
        viewModel.buyOpenCommand()->execute();
        
        QCOMPARE(spy.count(), 1);
    }
    
    void testPriceUpdate() {
        TradingViewModel viewModel;
        QSignalSpy spy(&viewModel, &TradingViewModel::currentPriceChanged);
        
        viewModel.updatePrice(4010.0);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(viewModel.currentPrice(), 4010.0);
    }
};
```

### 2. Controller 单元测试

```cpp
// StockQuotesControllerTest.cpp
void testRefreshData() {
    StockQuotesController controller;
    QSignalSpy spy(&controller, &StockQuotesController::dataRefreshed);
    
    controller.refreshData();
    
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.count(), 1);
}
```

---

## 九、预期收益

| 收益项      | 说明                           |
|----------|------------------------------|
| **可测试性** | ViewModel/Controller 可独立单元测试 |
| **可维护性** | 业务逻辑与 UI 分离，便于修改             |
| **可扩展性** | 新功能使用 QML，开发效率更高             |
| **代码复用** | ViewModel 可被多个 View 共用       |
| **状态管理** | 统一的状态管理机制                    |
| **渐进迁移** | 旧代码保持稳定，新功能逐步迁移              |

---

## 十、风险与对策

| 风险       | 对策                      |
|----------|-------------------------|
| QML 性能问题 | 复杂表格保持 Widget，简单界面用 QML |
| 学习成本     | 提供示例代码和培训文档             |
| 迁移工作量    | 分阶段迁移，优先核心模块            |
| 兼容性问题    | 保持混合架构，不强制统一            |