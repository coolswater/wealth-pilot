# WealthPilot 编码规范

## 版本：2.0.0

## 更新日期：2026-04-17

---

## 一、命名规范

### 1.1 通用规则

| 类型   | 规范             | 示例                                         |
|------|----------------|--------------------------------------------|
| 类名   | PascalCase     | `KLineChart`, `MarketDepthWidget`          |
| 方法名  | camelCase      | `setInstrument()`, `calculateIndicators()` |
| 函数名  | camelCase      | `formatPrice()`, `stringToPeriod()`        |
| 变量名  | camelCase      | `instrumentId`, `lastPrice`                |
| 成员变量 | m_ 前缀          | `m_showGrid`, `m_currentPeriod`            |
| 私有成员 | d-> 前缀 (PIMPL) | `d->instrumentId`                          |
| 常量   | UPPER_CASE     | `MAX_ROWS`, `DEFAULT_PERIOD`               |
| 枚举   | PascalCase     | `KLinePeriod`, `AdjustmentType`            |
| 枚举值  | PascalCase     | `KLinePeriod::Minute15`                    |
| 命名空间 | camelCase      | `MarketUtils`                              |
| 宏    | UPPER_CASE     | `PERF_START`, `REGISTER_COMPONENT`         |

### 1.2 文件命名

```
ClassName.h          // 头文件
ClassName.cpp        // 实现文件
```

### 1.3 前缀约定

| 前缀    | 含义       | 示例                      |
|-------|----------|-------------------------|
| `I`   | 接口类      | `IPlugin`, `ICTPPlugin` |
| `m_`  | 成员变量     | `m_enabled`, `m_data`   |
| `d->` | PIMPL 成员 | `d->instrumentId`       |
| `k`   | 常量       | `kMaxRows`              |

---

## 二、注释规范

### 2.1 文件头注释

```cpp
/**
 * @file FileName.h
 * @brief 简短描述
 *
 * @details 详细描述：
 * - 功能1
 * - 功能2
 * - 功能3
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
```

### 2.2 类注释

```cpp
/**
 * @brief 类的简短描述
 *
 * @details 类的详细描述：
 * - 功能说明
 * - 使用场景
 *
 * @example
 * @code
 * MyClass* obj = new MyClass();
 * obj->doSomething();
 * @endcode
 */
class MyClass : public BaseClass
{
    Q_OBJECT
    // ...
};
```

### 2.3 方法注释

```cpp
/**
 * @brief 方法的简短描述
 * @details 方法的详细描述
 * @param paramName 参数说明
 * @return 返回值说明
 * @throws ExceptionType 异常说明
 * @note 注意事项
 * @see 相关方法
 */
ReturnType methodName(ParamType paramName);
```

### 2.4 成员变量注释

```cpp
struct Impl {
    QString instrumentId;       ///< 合约代码
    double lastPrice = 0.0;     ///< 最新价格
    int maxRows = 500;          ///< 最大行数
};
```

### 2.5 代码块注释

```cpp
// ========== 公共接口 ==========

// ========== 私有方法 ==========

// ---------- 数据处理 ----------
```

---

## 三、代码结构

### 3.1 头文件结构

```cpp
#ifndef CLASSNAME_H
#define CLASSNAME_H

// 1. Qt 标准库
#include <QWidget>
#include <QString>

// 2. 项目公共类型
#include "core/types/MarketTypes.h"

// 3. 前向声明
class QComboBox;
class QToolButton;

// 4. 类定义
class ClassName : public BaseClass
{
    Q_OBJECT

public:
    // 构造与析构
    explicit ClassName(QWidget* parent = nullptr);
    ~ClassName() override;

    // 公共接口
    void publicMethod();

signals:
    // 信号
    void someSignal();

public slots:
    // 公共槽函数

protected:
    // 保护方法

private slots:
    // 私有槽函数

private:
    // 私有方法
    void privateMethod();

    // PIMPL
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // CLASSNAME_H
```

### 3.2 实现文件结构

```cpp
/**
 * @file ClassName.cpp
 * @brief 类的实现
 */

#include "ClassName.h"

// 1. Qt 头文件
#include <QVBoxLayout>

// 2. 项目头文件
#include "utils/Logger.h"

// 3. PIMPL 实现
struct ClassName::Impl {
    // 成员变量
};

// 4. 构造与析构
ClassName::ClassName(QWidget* parent)
    : BaseClass(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

ClassName::~ClassName() = default;

// 5. 公共方法
void ClassName::publicMethod()
{
    // 实现
}

// 6. 私有方法
void ClassName::privateMethod()
{
    // 实现
}
```

---

## 四、PIMPL 模式

### 4.1 定义

```cpp
// 头文件
class MyClass : public QWidget
{
    Q_OBJECT
public:
    explicit MyClass(QWidget* parent = nullptr);
    ~MyClass() override;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};
```

### 4.2 实现

```cpp
// 实现文件
struct MyClass::Impl {
    // UI 组件
    QLabel* label = nullptr;
    QPushButton* button = nullptr;

    // 数据
    QString data;
    int value = 0;

    // 状态
    bool enabled = false;
};

MyClass::MyClass(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    // 使用 d-> 访问成员
    d->label = new QLabel(this);
}

MyClass::~MyClass() = default;  // 必须在 cpp 文件中定义
```

---

## 五、信号与槽

### 5.1 信号命名

```cpp
signals:
    // 事件信号：过去式
    void periodChanged(KLinePeriod period);
    void dataUpdated(const MarketData& data);
    void connectionLost();

    // 状态信号：名词
    void progressChanged(int percent);
    void selectionChanged(const QString& id);
```

### 5.2 槽函数命名

```cpp
private slots:
    // on + 信号名
    void onPeriodChanged(KLinePeriod period);
    void onDataUpdated(const MarketData& data);

    // 按钮点击
    void onRefreshClicked();
    void onSettingsClicked();
```

### 5.3 连接方式

```cpp
// 推荐：新式连接（编译时检查）
connect(sender, &Sender::signalName,
        receiver, &Receiver::slotName);

// Lambda 表达式
connect(button, &QPushButton::clicked, this, [this]() {
    refresh();
});

// 带参数的 Lambda
connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) {
    onSelectionChanged(index);
});
```

---

## 六、内存管理

### 6.1 智能指针

```cpp
// 独占所有权
std::unique_ptr<Data> data = std::make_unique<Data>();

// 共享所有权
std::shared_ptr<Resource> resource = std::make_shared<Resource>();

// Qt 父子关系（不需要智能指针）
QLabel* label = new QLabel(this);  // this 会自动删除 label
```

### 6.2 对象生命周期

```cpp
// 正确：Qt 父子关系自动管理
void setupUI() {
    d->label = new QLabel(this);  // 父对象会自动删除
    d->button = new QPushButton(this);
}

// 正确：智能指针
std::unique_ptr<Worker> worker = std::make_unique<Worker>();

// 错误：忘记删除
void badCode() {
    QLabel* label = new QLabel();  // 内存泄漏！
}
```

---

## 七、性能优化

### 7.1 字符串

```cpp
// 推荐：QStringLiteral（编译时优化）
return QStringLiteral("DashboardPage");

// 推荐：QString::arg()
QString msg = QString("Loaded %1 items").arg(count);

// 避免：字符串拼接
QString bad = "Item " + QString::number(id) + " loaded";
```

### 7.2 容器

```cpp
// 预分配空间
QVector<KLineData> data;
data.reserve(10000);

// 使用移动语义
QVector<Data> loadData() {
    QVector<Data> result;
    // ...
    return result;  // RVO 优化
}

// 避免不必要的拷贝
void processData(const QVector<KLineData>& data);  // const 引用
```

### 7.3 性能监控

```cpp
// 使用性能监控宏
PERF_START("loadData");
loadData();
PERF_END("loadData");

// 作用域自动计时
{
    PERF_SCOPE("processData");
    processData();
}
```

---

## 八、错误处理

### 8.1 空指针检查

```cpp
// 检查指针
QWidget* widget = findChild<QWidget*>("name");
if (!widget) {
    LOG_WARNING("Widget not found");
    return;
}

// 安全访问
if (d->klineChart) {
    d->klineChart->setData(data);
}
```

### 8.2 数据有效性

```cpp
// 价格有效性
if (price <= 0) {
    return;
}

// 时间有效性
if (!time.isValid()) {
    time = QDateTime::currentDateTime();
}

// 数组边界
if (index >= 0 && index < data.size()) {
    // 安全访问
}
```

### 8.3 日志记录

```cpp
#include "utils/Logger.h"

// 日志级别
LOG_DEBUG("Debug message");
LOG_INFO("Info message: %1").arg(value);
LOG_WARNING("Warning: null pointer");
LOG_ERROR("Error: failed to load data");
```

---

## 九、代码风格

### 9.1 缩进与空格

```cpp
// 4 空格缩进
void method()
{
    if (condition) {
        doSomething();
    }
}

// 运算符两侧空格
int result = a + b * c;

// 逗号后空格
function(arg1, arg2, arg3);
```

### 9.2 大括号

```cpp
// K&R 风格（推荐）
if (condition) {
    doSomething();
} else {
    doOther();
}

// 类定义
class MyClass : public BaseClass
{
    // ...
};
```

### 9.3 行长度

- 最大行长：120 字符
- 长行换行：

```cpp
// 函数调用
connect(d->toolBar, &ChartToolBar::periodChanged,
        this, &FuturesKLinePage::onPeriodChanged);

// 条件表达式
if (condition1 &&
    condition2 &&
    condition3) {
    doSomething();
}
```

---

## 十、最佳实践

### 10.1 单一职责

每个类只做一件事：

- `KLineChart` - 只负责K线绘制
- `ChartToolBar` - 只负责工具栏
- `MarketDepthWidget` - 只负责盘口显示

### 10.2 依赖注入

```cpp
// 通过 ServiceLocator 获取服务
ICTPPlugin* ctpPlugin = ServiceLocator::instance().tryResolve<ICTPPlugin>();
if (ctpPlugin) {
    connect(ctpPlugin, &ICTPPlugin::marketDataUpdated,
            this, &MyClass::onMarketDataUpdated);
}
```

### 10.3 信号驱动

```cpp
// 组件间通过信号通信
connect(d->depthWidget, &MarketDepthWidget::buyClicked,
        this, [this](double price) {
    emit tradeRequested(d->instrumentId, "buy", price, 1);
});
```

---

*文档版本：1.0*
*更新日期：2026-04-17*
