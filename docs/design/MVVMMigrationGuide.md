# MVVM 架构迁移指南

## 一、迁移概述

本指南帮助将现有 Widget 页面迁移到 MVVM 架构。

### 迁移策略

| 页面类型  | 迁移方案            | 说明                      |
|-------|-----------------|-------------------------|
| 简单展示页 | Controller 模式   | 保持 Widget，添加 Controller |
| 复杂交互页 | ViewModel + QML | 新建 QML 视图               |
| 对话框   | ViewModel + QML | 使用 QML Dialog           |

---

## 二、迁移步骤

### Step 1：分析现有页面

```cpp
// 分析 TradingPanel.cpp
class TradingPanel : public QWidget {
    // UI 组件 - View 层职责
    QLabel* m_instrumentLabel;
    QPushButton* m_buyOpenBtn;
    
    // 业务逻辑 - 应移至 ViewModel/Controller
    void onBuyOpenClicked();
    void updateAccountInfo();
    
    // 数据状态 - 应移至 ViewModel
    double m_lastPrice;
    int m_longPosition;
};
```

### Step 2：创建 ViewModel/Controller

#### 方案 A：创建 ViewModel（用于 QML）

```cpp
// 1. 创建 TradingViewModel.h
class TradingViewModel : public ViewModelBase {
    Q_PROPERTY(double currentPrice READ currentPrice NOTIFY priceChanged)
    Q_PROPERTY(Command* buyOpenCommand READ buyOpenCommand CONSTANT)
    // ...
};

// 2. 创建 TradingViewModel.cpp
void TradingViewModel::executeBuyOpen() {
    // 从 TradingPanel.cpp 移入业务逻辑
}
```

#### 方案 B：创建 Controller（用于 Widget）

```cpp
// 1. 创建 StockQuotesController.h
class StockQuotesController : public ControllerBase {
    Q_INVOKABLE void refreshData();
    Q_INVOKABLE void searchData(const QString& keyword);
    // ...
};

// 2. 创建 StockQuotesController.cpp
void StockQuotesController::refreshData() {
    // 从 StockQuotesPage.cpp 移入业务逻辑
}
```

### Step 3：修改 View

#### QML View

```qml
// TradingPanel.qml
TradingViewModel { id: viewModel }

Button {
    text: "买入开仓"
    enabled: viewModel.buyOpenCommand.canExecute
    onClicked: viewModel.buyOpenCommand.execute()
}
```

#### Widget View（使用 Controller）

```cpp
// StockQuotesPage.cpp - 修改后
void StockQuotesPage::setupUI() {
    // UI 构建（保持不变）
    
    // 使用 Controller
    m_controller = new StockQuotesController(this);
    
    // 连接 Controller
    connect(m_refreshBtn, &QPushButton::clicked,
            m_controller, &StockQuotesController::refreshData);
    connect(m_controller, &StockQuotesController::dataRefreshed,
            this, [this](int count) {
                m_statusLabel->setText(QString("已加载 %1 条").arg(count));
            });
    
    // 设置模型
    m_tableView->setModel(m_controller->proxyModel());
}
```

### Step 4：注册类型

```cpp
// main.cpp 或 ApplicationInitializer.cpp
#include "viewmodels/ViewModelRegistration.h"

void initializeQml() {
    QQmlApplicationEngine engine;
    WealthPilot::initializeQmlEnvironment(engine);
}
```

---

## 三、迁移示例

### 示例 1：TradingPanel → TradingViewModel + QML

#### 原代码（TradingPanel.cpp）

```cpp
void TradingPanel::onBuyOpenClicked() {
    // 业务逻辑混在 View 中
    if (!m_instrumentId.isEmpty()) {
        OrderRequest request;
        request.instrumentId = m_instrumentId;
        request.direction = PositionDirection::Long;
        request.volume = m_orderVolume;
        request.price = m_orderPrice;
        
        // 风控检查
        auto result = RiskController::instance().checkOrder(request);
        if (!result.passed) {
            QMessageBox::warning(this, "风控警告", result.message);
            return;
        }
        
        // 提交订单
        QString orderId = TradingService::instance().submitOrder(request);
        updateOrderTable();
    }
}
```

#### 迁移后（TradingViewModel.cpp）

```cpp
void TradingViewModel::executeBuyOpen() {
    // 业务逻辑在 ViewModel 中
    setStatus(QString("买入开仓 %1 手").arg(m_orderVolume));
    
    // 风控检查
    if (m_riskController) {
        OrderRequest request;
        request.instrumentId = m_instrumentId;
        request.direction = PositionDirection::Long;
        request.volume = m_orderVolume;
        request.price = m_orderPrice;
        
        auto result = m_riskController->checkOrder(request);
        if (!result.passed) {
            setError(result.message);
            emit riskWarning(result.message);
            return;
        }
    }
    
    // 提交订单
    QString orderId = m_tradingService->submitOrder(...);
    emit orderSubmitted(orderId, "买入开仓");
}
```

#### QML 视图（TradingPanel.qml）

```qml
TradingViewModel { id: viewModel }

Button {
    text: "买入开仓"
    enabled: viewModel.buyOpenCommand.canExecute
    onClicked: viewModel.buyOpenCommand.execute()
}

// 错误处理
Connections {
    target: viewModel
    function onRiskWarning(message) {
        warningDialog.show(message)
    }
}
```

---

### 示例 2：StockQuotesPage → StockQuotesController

#### 原代码（StockQuotesPage.cpp）

```cpp
void StockQuotesPage::onRefreshData() {
    // 业务逻辑在 View 中
    m_statusLabel->setText("加载中...");
    
    StockDataSource* source = ServiceLocator::instance().resolve<StockDataSource>();
    source->refreshData();
    
    // 处理回调
    connect(source, &StockDataSource::dataReceived, this, [this](auto quotes) {
        m_model->setData(quotes);
        m_statusLabel->setText(QString("已加载 %1 条").arg(quotes.size()));
    });
}
```

#### 迁移后

```cpp
// StockQuotesController.cpp
void StockQuotesController::refreshData() {
    beginOperation("刷新数据");
    emit dataLoading(true);
    
    if (m_dataSource) {
        m_dataSource->refreshData();
    }
}

void StockQuotesController::onDataReceived(const QVector<StockQuote>& quotes) {
    m_allQuotes = quotes;
    m_model->setData(quotes);
    
    emit dataRefreshed(quotes.size());
    emit dataLoading(false);
    endOperation("刷新数据", true);
}

// StockQuotesPage.cpp（简化）
void StockQuotesPage::setupUI() {
    m_controller = new StockQuotesController(this);
    
    connect(m_refreshBtn, &QPushButton::clicked,
            m_controller, &StockQuotesController::refreshData);
    connect(m_controller, &StockQuotesController::dataRefreshed,
            this, [this](int count) {
                m_statusLabel->setText(QString("已加载 %1 条").arg(count));
            });
    
    m_tableView->setModel(m_controller->proxyModel());
}
```

---

## 四、测试迁移

### ViewModel 单元测试

```cpp
// TradingViewModelTest.cpp
class TradingViewModelTest : public QObject {
    Q_OBJECT
    
private slots:
    void testBuyOpenCommand() {
        TradingViewModel viewModel;
        viewModel.setInstrument("IF2506", "沪深300");
        viewModel.setOrderPrice(4000.0);
        viewModel.setOrderVolume(1);
        
        QSignalSpy spy(&viewModel, &TradingViewModel::orderSubmitted);
        
        QVERIFY(viewModel.buyOpenCommand()->canExecute());
        viewModel.buyOpenCommand()->execute();
        
        QCOMPARE(spy.count(), 1);
    }
    
    void testPriceUpdate() {
        TradingViewModel viewModel;
        QSignalSpy spy(&viewModel, &TradingViewModel::priceChanged);
        
        viewModel.onPriceUpdated("IF2506", 4010.0);
        
        QCOMPARE(viewModel.currentPrice(), 4010.0);
        QCOMPARE(spy.count(), 1);
    }
};
```

### Controller 单元测试

```cpp
// StockQuotesControllerTest.cpp
void testRefreshData() {
    StockQuotesController controller;
    controller.initialize();
    
    QSignalSpy spy(&controller, &StockQuotesController::dataRefreshed);
    
    controller.refreshData();
    
    QVERIFY(spy.wait(5000));
    QCOMPARE(controller.totalCount(), 100);
}
```

---

## 五、迁移清单

### TradingPanel

| 组件   | 原位置                           | 新位置                             | 状态  |
|------|-------------------------------|---------------------------------|-----|
| 价格显示 | TradingPanel                  | TradingViewModel.currentPrice   | 待迁移 |
| 持仓显示 | TradingPanel                  | TradingViewModel.position       | 待迁移 |
| 买入按钮 | TradingPanel.onBuyOpenClicked | TradingViewModel.executeBuyOpen | 待迁移 |
| 风控检查 | TradingPanel                  | TradingViewModel                | 待迁移 |

### StockQuotesPage

| 组件   | 原位置             | 新位置                               | 状态  |
|------|-----------------|-----------------------------------|-----|
| 数据刷新 | StockQuotesPage | StockQuotesController.refreshData | 待迁移 |
| 搜索筛选 | StockQuotesPage | StockQuotesController.searchData  | 待迁移 |
| 导出功能 | StockQuotesPage | StockQuotesController.exportToCSV | 待迁移 |

---

## 六、常见问题

### Q1：如何处理服务依赖？

```cpp
// ViewModel 中获取服务
void TradingViewModel::initialize() {
    m_tradingService = getService<TradingService>();
    m_riskController = getService<RiskController>();
}
```

### Q2：如何处理异步操作？

```cpp
// 使用 executeCommand
void StockQuotesController::refreshData() {
    executeAsync(
        [this]() { return m_dataSource->fetchData(); },
        [this](auto result) { emit dataRefreshed(result.size()); },
        [this](auto error) { setError(error); }
    );
}
```

### Q3：如何保持向后兼容？

```cpp
// 同时支持 Widget 和 QML
class TradingPanelWidget : public QWidget {
    // 传统 Widget 实现
};

class TradingPanelQml : public QQuickWidget {
    // QML 实现
    // source = "qrc:/qml/TradingPanel.qml"
};

// 根据配置选择
QWidget* createTradingPanel(bool useQml) {
    if (useQml) {
        return new TradingPanelQml();
    }
    return new TradingPanelWidget();
}
```

---

## 七、迁移进度跟踪

| 模块 | 页面                | 方案              | 进度      |
|----|-------------------|-----------------|---------|
| 交易 | TradingPanel      | ViewModel + QML | 📝 设计完成 |
| 交易 | OrderDialog       | ViewModel + QML | 📝 设计完成 |
| 交易 | PositionManager   | Controller      | 📝 待开始  |
| 行情 | StockQuotesPage   | Controller      | 📝 设计完成 |
| 行情 | FuturesQuotesPage | Controller      | 📝 待开始  |
| 分析 | BacktestPage      | ViewModel + QML | 📝 待开始  |
| 分析 | SignalCenterPage  | ViewModel       | 📝 待开始  |