# MVVM 架构应用指南

## 一、架构概述

WealthPilot 项目采用混合架构模式：

- **MVVM 模式** - 用于 QML 视图（新功能）
- **MVP 模式** - 用于 Widget 页面（现有功能）

```
┌─────────────────────────────────────────────────────────────────┐
│                        应用架构                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   QML 视图 (新功能)              Widget 页面 (现有功能)          │
│   ┌─────────────────┐           ┌─────────────────┐            │
│   │  TradingPanel   │           │  StockQuotesPage│            │
│   │  OrderDialog    │           │  FuturesQuotes  │            │
│   │  BacktestPanel  │           │  ...            │            │
│   └────────┬────────┘           └────────┬────────┘            │
│            │                             │                      │
│            ▼                             ▼                      │
│   ┌─────────────────┐           ┌─────────────────┐            │
│   │   ViewModel     │           │   Controller    │            │
│   │ (MVVM 模式)     │           │ (MVP 模式)      │            │
│   └────────┬────────┘           └────────┬────────┘            │
│            │                             │                      │
│            └─────────────┬───────────────┘                      │
│                          ▼                                      │
│                 ┌─────────────────┐                             │
│                 │    Service      │                             │
│                 │  (业务逻辑)     │                             │
│                 └─────────────────┘                             │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## 二、MVVM 模式详解

### 2.1 核心组件

#### ViewModelBase - ViewModel 基类

```cpp
#include "viewmodels/ViewModelBase.h"

class MyViewModel : public WealthPilot::ViewModelBase
{
    Q_OBJECT

    // 属性定义
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    // 命令定义
    Command* submitCommand() { return m_submitCommand; }

private:
    Command* m_submitCommand;
};
```

#### Command - 命令模式

```cpp
// 创建命令
m_submitCommand = new Command([this]() {
    // 执行逻辑
    doSubmit();
}, this);

// 设置条件
m_submitCommand->setCanExecuteCondition([this]() {
    return !m_title.isEmpty() && !m_isLoading;
});

// 触发重新评估
m_submitCommand->raiseCanExecuteChanged();
```

### 2.2 在 QML 中使用

```qml
import QtQuick 2.15
import QtQuick.Controls 2.15
import WealthPilot.ViewModels 1.0

Item {
    // 创建 ViewModel
    MyViewModel {
        id: viewModel

        onTitleChanged: {
            console.log("Title:", title)
        }
    }

    Column {
        // 数据绑定
        TextField {
            text: viewModel.title
            onTextChanged: viewModel.title = text
        }

        // 命令绑定
        Button {
            text: "提交"
            enabled: viewModel.submitCommand.canExecute
            onClicked: viewModel.submitCommand.execute()
        }

        // 状态显示
        BusyIndicator {
            running: viewModel.isLoading
            visible: viewModel.isLoading
        }
    }
}
```

### 2.3 已实现的 ViewModel

| ViewModel         | 用途    | QML 视图            |
|-------------------|-------|-------------------|
| TradingViewModel  | 交易面板  | TradingPanel.qml  |
| OrderViewModel    | 下单对话框 | OrderDialog.qml   |
| BacktestViewModel | 回测面板  | BacktestPanel.qml |

## 三、MVP 模式详解

### 3.1 核心组件

#### ControllerBase - Controller 基类

```cpp
#include "controllers/ControllerBase.h"

class MyController : public WealthPilot::ControllerBase
{
    Q_OBJECT

public:
    void initialize() override;
    void refreshData();
    void searchData(const QString& keyword);

signals:
    void dataRefreshed();
    void dataFiltered(int count);
};
```

### 3.2 在 Widget 中使用

```cpp
#include "controllers/MyController.h"

class MyPage : public BasePage
{
    Q_OBJECT

public:
    MyPage(QWidget* parent = nullptr)
    {
        // 创建 Controller
        m_controller = new MyController(this);
        m_controller->initialize();

        // 连接信号
        connect(m_controller, &MyController::dataRefreshed,
                this, &MyPage::onDataRefreshed);

        // 设置模型
        m_tableView->setModel(m_controller->model());
    }

private slots:
    void onDataRefreshed() {
        // 更新 UI
        m_countLabel->setText(QString("共 %1 条").arg(
            m_controller->model()->rowCount()));
    }

private:
    MyController* m_controller;
};
```

## 四、服务注入

### 4.1 ServiceLocator

```cpp
#include "core/di/ServiceLocator.h"

// 注册服务
ServiceLocator::instance()->registerService<TradingService>(
    new TradingService());

// 获取服务
TradingService* service = ServiceLocator::instance()->getService<TradingService>();

// 在 Controller/ViewModel 中获取
m_tradingService = getService<TradingService>();
```

### 4.2 已注册的服务

| 服务              | 用途     |
|-----------------|--------|
| TradingService  | 交易业务逻辑 |
| StockDataSource | 股票数据源  |
| CTPService      | 期货数据源  |
| DatabaseManager | 数据库管理  |

## 五、最佳实践

### 5.1 ViewModel 设计原则

1. **单一职责** - 每个 ViewModel 只负责一个视图
2. **属性通知** - 所有属性变化都要发射信号
3. **命令模式** - 用户操作通过 Command 执行
4. **服务注入** - 通过 ServiceLocator 获取服务
5. **错误处理** - 通过 errorOccurred 信号报告错误

### 5.2 Controller 设计原则

1. **数据管理** - 负责数据的获取、过滤、排序
2. **状态管理** - 管理页面状态
3. **信号槽连接** - 与视图通过信号槽通信
4. **服务注入** - 通过 ServiceLocator 获取服务

### 5.3 命名规范

```cpp
// ViewModel
class TradingViewModel;      // 交易 ViewModel
class OrderViewModel;        // 下单 ViewModel

// Controller
class StockQuotesController; // 股票行情 Controller
class FuturesQuotesController; // 期货行情 Controller

// QML
TradingPanel.qml;            // 交易面板
OrderDialog.qml;             // 下单对话框

// Widget
StockQuotesPage;             // 股票行情页面
FuturesQuotesPage;           // 期货行情页面
```

## 六、迁移指南

### 6.1 从 Widget 迁移到 QML

1. 创建对应的 ViewModel
2. 创建 QML 视图
3. 在 ViewModel 中实现业务逻辑
4. 通过数据绑定连接 ViewModel 和 QML
5. 注册 ViewModel 到 QML 引擎

### 6.2 示例：迁移股票行情页面

**步骤 1：创建 ViewModel**

```cpp
class StockQuotesViewModel : public ViewModelBase
{
    Q_OBJECT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText)
    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)

public:
    Command* refreshCommand();
    Command* exportCommand();

private:
    StockQuoteModel* m_model;
    QString m_searchText;
};
```

**步骤 2：创建 QML 视图**

```qml
StockQuotesPanel.qml:
    StockQuotesViewModel {
        id: viewModel
    }

    TableView {
        model: viewModel.model
    }

    Button {
        text: "刷新"
        onClicked: viewModel.refreshCommand.execute()
    }
```

**步骤 3：注册 ViewModel**

```cpp
qmlRegisterType<StockQuotesViewModel>(
    "WealthPilot.ViewModels", 1, 0, "StockQuotesViewModel");
```

## 七、测试

### 7.1 ViewModel 测试

```cpp
void TestTradingViewModel::testSubmitOrder()
{
    TradingViewModel viewModel;

    // 设置属性
    viewModel.setInstrumentId("sh600000");
    viewModel.setPrice(10.0);
    viewModel.setQuantity(100);

    // 执行命令
    QVERIFY(viewModel.submitCommand()->canExecute());
    viewModel.submitCommand()->execute();

    // 验证结果
    QCOMPARE(viewModel.lastOrderId(), "order_001");
}
```

### 7.2 Controller 测试

```cpp
void TestStockQuotesController::testRefreshData()
{
    StockQuotesController controller;
    controller.initialize();

    QSignalSpy spy(&controller, &StockQuotesController::dataRefreshed);

    controller.refreshData();

    QVERIFY(spy.wait(1000));
    QCOMPARE(controller.model()->rowCount(), 100);
}
```

## 八、文件结构

```
src/
├── viewmodels/           # MVVM ViewModel
│   ├── ViewModelBase.h/cpp
│   ├── TradingViewModel.h/cpp
│   ├── OrderViewModel.h/cpp
│   ├── BacktestViewModel.h/cpp
│   └── ViewModelRegistration.h
│
├── controllers/          # MVP Controller
│   ├── ControllerBase.h/cpp
│   ├── StockQuotesController.h/cpp
│   └── FuturesQuotesController.h/cpp
│
├── views/
│   ├── qml/              # QML 视图
│   │   ├── TradingPanel.qml
│   │   ├── OrderDialog.qml
│   │   └── BacktestPanel.qml
│   │
│   └── stock/            # Widget 页面
│       ├── StockQuotesPage.h/cpp
│       └── StockKLinePage.h/cpp
│
└── core/
    └── di/
        └── ServiceLocator.h/cpp  # 服务定位器
```

---

**版本**: 1.0.0
**日期**: 2026-05-12
**作者**: WealthPilot Team