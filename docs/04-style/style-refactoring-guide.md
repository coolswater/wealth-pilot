# WealthPilot 样式统一重构指南

## 📋 重构目标

1. **消除硬编码颜色值** - 所有颜色使用Tokens::Colors
2. **统一页面样式** - 使用PageStyleHelper统一样式
3. **去除冗余代码** - 合并重复的样式定义
4. **提高可维护性** - 集中管理样式，便于主题切换

## 🔧 样式系统架构

### 1. 设计令牌系统（Tokens.h）

**位置**：`src/core/config/Tokens.h`

**包含内容**：

- 颜色系统（Colors）
- 间距系统（Spacing）
- 圆角系统（Radius）
- 字体系统（Font）
- 阴影系统（Shadow）
- 动画系统（Animation）
- 尺寸系统（Size）

### 2. 样式辅助工具（StyleHelper）

**位置**：`src/ui/utils/StyleHelper.h/cpp`

**功能**：

- 基础组件样式生成（按钮、输入框、卡片等）
- 颜色辅助函数（涨跌颜色、风险等级颜色等）
- 样式应用函数

### 3. 页面样式辅助（PageStyleHelper）

**位置**：`src/ui/utils/PageStyleHelper.h/cpp`

**功能**：

- 页面基础样式
- 卡片样式
- 数据展示样式
- 表格样式
- 状态样式

## 📝 重构规则

### 规则1：禁止硬编码颜色

❌ **错误示例**：

```cpp
label->setStyleSheet("color: #FFFFFF;");
widget->setStyleSheet("background-color: #1A1F2E;");
```

✅ **正确示例**：

```cpp
label->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::TextPrimary));
widget->setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgBase));
```

✅ **更佳示例**：

```cpp
label->setStyleSheet(PageStyleHelper::dataLabelStyle());
widget->setStyleSheet(PageStyleHelper::cardContainerStyle());
```

### 规则2：使用统一的样式辅助类

❌ **错误示例**：

```cpp
QString style = "QPushButton {"
    "  background-color: #3B82F6;"
    "  color: white;"
    "  border-radius: 8px;"
    "}";
button->setStyleSheet(style);
```

✅ **正确示例**：

```cpp
button->setStyleSheet(PageStyleHelper::primaryButtonStyle());
```

### 规则3：涨跌颜色使用辅助函数

❌ **错误示例**：

```cpp
if (change > 0) {
    label->setStyleSheet("color: #EF4444;");
} else {
    label->setStyleSheet("color: #10B981;");
}
```

✅ **正确示例**：

```cpp
label->setStyleSheet(PageStyleHelper::dataChangeStyle(change));
// 或
label->setStyleSheet(QString("color: %1;").arg(StyleHelper::getTrendColor(change)));
```

### 规则4：表格样式统一

❌ **错误示例**：

```cpp
table->setStyleSheet(
    "QTableWidget {"
    "  background-color: #0F1419;"
    "  color: #FFFFFF;"
    "  border: 1px solid #374151;"
    "}"
);
```

✅ **正确示例**：

```cpp
table->setStyleSheet(PageStyleHelper::tableStyle());
```

## 🔍 需要重构的文件

### 高优先级（大量硬编码）

1. **ChartStatusBar.cpp**
    - 硬编码颜色：#374151, #9CA3AF, #10B981, #F59E0B, #EF4444, #1F2937
    - 需要替换：使用Tokens::Colors和PageStyleHelper

2. **MarketDepthWidget.cpp**
    - 硬编码颜色：#374151, #10B981, #9CA3AF, #EF4444
    - 需要替换：使用StyleHelper::getTrendColor()

3. **ChartConfig.cpp**
    - 硬编码颜色：#FFFFFF, #EF4444, #10B981, #9CA3AF, #FF6B6B
    - 需要替换：使用Tokens::Colors

### 中优先级（部分硬编码）

4. **RiskIndicatorWidget.cpp**
5. **RecommendationListWidget.cpp**
6. **PortfolioOptimizationDialog.cpp**
7. **NewsPanelWidget.cpp**
8. **AlertSettingDialog.cpp**

### 低优先级（已使用Tokens）

9. **StockInfoPanel.cpp** - 已使用Tokens，需检查完整性
10. **ThemeManager.cpp** - 已使用Tokens，需检查完整性

## 📊 重构步骤

### 步骤1：添加样式工具到CMakeLists.txt

```cmake
# UI Utils
set(UI_UTILS_SOURCES
    src/ui/utils/StyleHelper.h
    src/ui/utils/StyleHelper.cpp
    src/ui/utils/PageStyleHelper.h
    src/ui/utils/PageStyleHelper.cpp
)
```

### 步骤2：重构单个文件

以ChartStatusBar.cpp为例：

**重构前**：

```cpp
d->connectionLabel->setStyleSheet("color: #EF4444;");
```

**重构后**：

```cpp
d->connectionLabel->setStyleSheet(PageStyleHelper::errorStyle());
```

### 步骤3：测试验证

编译并运行，确保样式显示正确。

## 🎯 重构优先级

### 第一阶段：核心组件（本周）

- [ ] ChartStatusBar.cpp
- [ ] MarketDepthWidget.cpp
- [ ] ChartConfig.cpp

### 第二阶段：新增组件（下周）

- [ ] RiskIndicatorWidget.cpp
- [ ] RecommendationListWidget.cpp
- [ ] PortfolioOptimizationDialog.cpp
- [ ] NewsPanelWidget.cpp
- [ ] AlertSettingDialog.cpp

### 第三阶段：页面组件（后续）

- [ ] StockInfoPanel.cpp
- [ ] 所有页面文件

## 📈 预期效果

### 代码质量提升

- 消除约200+处硬编码颜色
- 减少约50%的样式代码
- 提高代码可维护性

### 功能增强

- 支持主题切换（深色/浅色/护眼）
- 统一的视觉风格
- 更好的可扩展性

### 性能优化

- 减少样式表解析
- 样式缓存复用
- 更快的渲染速度

## 🔧 使用示例

### 示例1：创建卡片

**重构前**：

```cpp
QWidget* card = new QWidget();
card->setStyleSheet(
    "background-color: #242937;"
    "border: 1px solid rgba(255, 255, 255, 0.1);"
    "border-radius: 12px;"
    "padding: 16px;"
);
```

**重构后**：

```cpp
QWidget* card = new QWidget();
StyleHelper::applyCardStyle(card);
// 或
card->setStyleSheet(PageStyleHelper::cardContainerStyle());
```

### 示例2：显示涨跌数据

**重构前**：

```cpp
QLabel* changeLabel = new QLabel();
if (change > 0) {
    changeLabel->setStyleSheet("color: #EF4444; font-weight: bold;");
} else if (change < 0) {
    changeLabel->setStyleSheet("color: #10B981; font-weight: bold;");
} else {
    changeLabel->setStyleSheet("color: #9CA3AF;");
}
```

**重构后**：

```cpp
QLabel* changeLabel = new QLabel();
changeLabel->setStyleSheet(PageStyleHelper::dataChangeStyle(change));
```

### 示例3：创建表格

**重构前**：

```cpp
QTableWidget* table = new QTableWidget();
table->setStyleSheet(
    "QTableWidget {"
    "  background-color: #0F1419;"
    "  color: #FFFFFF;"
    "  border: 1px solid rgba(255, 255, 255, 0.1);"
    "  gridline-color: rgba(255, 255, 255, 0.05);"
    "}"
    "QTableWidget::item:selected {"
    "  background-color: #3B82F6;"
    "}"
);
```

**重构后**：

```cpp
QTableWidget* table = new QTableWidget();
table->setStyleSheet(PageStyleHelper::tableStyle());
```

## 📚 参考资料

- Tokens.h - 设计令牌定义
- StyleHelper.h - 样式辅助工具
- PageStyleHelper.h - 页面样式辅助
- WealthPilot UI设计规范文档

---

**样式统一重构指南 v1.0**
