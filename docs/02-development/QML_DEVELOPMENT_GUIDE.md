# QML 开发指南

## 概述

WealthPilot 采用 Widgets + QML 混合架构，QML 用于高性能图表渲染，Widgets 用于复杂业务窗口。

---

## 1. 快速开始

### 1.1 QML 文件位置

```
qml/
├── charts/           # 图表组件
│   ├── KLineChart.qml
│   └── TimeShareChart.qml
├── components/       # UI 组件
│   ├── StockCard.qml
│   └── PriceLabel.qml
├── models/           # 数据模型
└── main.qml          # 入口文件
```

### 1.2 资源文件

QML 文件通过 `qml/qml.qrc` 注册：

```xml
<RCC>
    <qresource prefix="/qml">
        <file>main.qml</file>
        <file>charts/KLineChart.qml</file>
        <file>charts/TimeShareChart.qml</file>
    </qresource>
</RCC>
```

---

## 2. 数据绑定

### 2.1 C++ 数据模型

```cpp
// QmlDataBridge.h
class KLineQmlModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    
public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        OpenRole, HighRole, LowRole, CloseRole, VolumeRole,
        Ma5Role, Ma10Role, Ma20Role
    };
    
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void setData(const QVector<KLineData>& data);
};
```

### 2.2 QML 使用模型

```qml
// KLineChart.qml
Item {
    property var model: null  // 外部传入的数据模型
    
    function updateData() {
        if (!model || model.count === 0) return;
        
        for (var i = 0; i < model.count; i++) {
            var candle = model.get(i);
            // 使用 candle.open, candle.high, candle.close 等
        }
    }
    
    onModelChanged: updateData()
}
```

---

## 3. 在 Widgets 中嵌入 QML

### 3.1 使用 QmlKLineWidget

```cpp
#include "ui/components/QmlKLineWidget.h"

// 创建 QML 图表容器
auto* qmlChart = new QmlKLineWidget(this);

// 设置数据
QVector<KLineData> data;
qmlChart->setKLineData(data);

// 切换图表类型
qmlChart->setChartType(QmlKLineWidget::ChartType::TimeShare);
```

### 3.2 直接使用 QQuickWidget

```cpp
#include <QQuickWidget>

auto* quickWidget = new QQuickWidget(this);
quickWidget->setSource(QUrl("qrc:/qml/charts/KLineChart.qml"));

// 设置上下文属性
QQmlContext* context = quickWidget->rootContext();
context->setContextProperty("klineModel", myModel);
```

---

## 4. 主题支持

### 4.1 颜色定义

```qml
// 深色主题
property color upColor: "#EF4444"      // 红涨
property color downColor: "#10B981"    // 绿跌
property color gridColor: "#2D3748"
property color textColor: "#9CA3AF"

// 浅色主题
property color upColor: "#E8463A"
property color downColor: "#14B143"
property color gridColor: "#D0D7DE"
property color textColor: "#24292F"
```

### 4.2 C++ 端应用主题

```cpp
void QmlKLineWidget::applyTheme(bool isDark) {
    if (isDark) {
        updateQmlProperty("upColor", "#EF4444");
        updateQmlProperty("downColor", "#10B981");
    } else {
        updateQmlProperty("upColor", "#E8463A");
        updateQmlProperty("downColor", "#14B143");
    }
}
```

---

## 5. 性能优化

### 5.1 数据更新策略

```cpp
// 批量更新 - 使用 beginResetModel/endResetModel
void KLineQmlModel::setData(const QVector<KLineData>& data) {
    beginResetModel();
    m_data = data;
    endResetModel();
    emit countChanged();
}

// 增量更新 - 使用 beginInsertRows/endInsertRows
void KLineQmlModel::appendData(const KLineData& data) {
    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    m_data.append(data);
    endInsertRows();
    emit countChanged();
}
```

### 5.2 QML 端优化

```qml
// 避免频繁重绘
function updateData() {
    if (!model || model.count === 0) return;
    
    // 只处理可见区域
    var startIndex = Math.max(0, model.count - visibleCount);
    var endIndex = model.count;
    
    for (var i = startIndex; i < endIndex; i++) {
        // ...
    }
}

// 使用 Behavior 添加平滑动画
Behavior on visibleCount {
    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
}
```

---

## 6. 调试技巧

### 6.1 查看加载错误

```cpp
connect(m_quickWidget, &QQuickWidget::statusChanged, [](QQuickWidget::Status status) {
    if (status == QQuickWidget::Error) {
        qWarning() << "QML load error:" << m_quickWidget->errors();
    }
});
```

### 6.2 QML 调试输出

```qml
Component.onCompleted: {
    console.log("KLineChart loaded, model count:", model ? model.count : 0);
}
```

### 6.3 Qt Charts 模块导入

确保在 CMakeLists.txt 中添加：

```cmake
find_package(Qt6 REQUIRED COMPONENTS Charts Quick QuickWidgets Qml)
target_link_libraries(WealthPilot PRIVATE Qt6::Charts Qt6::Quick Qt6::QuickWidgets Qt6::Qml)
```

运行时设置 QML 导入路径：

```cpp
m_quickWidget->engine()->addImportPath("C:/Qt/6.10.2/mingw_64/qml");
```

---

## 7. 常见问题

### Q1: QML 文件找不到

**原因**: 资源文件未正确注册  
**解决**: 检查 `qml.qrc` 和 CMakeLists.txt 中的 `QML_RESOURCES`

### Q2: QtCharts 模块未安装

**原因**: QML 无法找到 QtCharts 插件  
**解决**: 添加导入路径或使用 windeployqt 部署

### Q3: 数据不显示

**原因**: 模型未正确绑定  
**解决**: 确保 `Q_INVOKABLE get(int)` 方法正确实现

### Q4: 性能问题

**原因**: 频繁重绘或数据量过大  
**解决**: 使用虚拟滚动、批量更新、限制可见数量

---

## 8. 最佳实践

1. **分离关注点**: QML 只负责 UI 渲染，业务逻辑放在 C++
2. **数据驱动**: 使用模型-视图模式，避免在 QML 中处理复杂逻辑
3. **性能优先**: 限制可见数据量，使用增量更新
4. **主题一致**: 通过 C++ 端统一管理主题颜色
5. **错误处理**: 检查模型有效性，处理空数据情况

---

**文档版本**: 1.0.0  
**最后更新**: 2026-05-11
