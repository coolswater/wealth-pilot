# 样式迁移操作指南

## 快速迁移步骤

### 1. 更新头文件引用

将：
```cpp
#include "ui/components/PageStyles.h"
```

改为：
```cpp
#include "ui/components/StyleHelper.h"
```

### 2. 移除样式变量

删除这些行：
```cpp
QString groupBoxStyle = PageStyles::groupBox();
QString inputStyle = PageStyles::inputField();
QString btnStyle = PageStyles::secondaryButton();
```

### 3. 按钮样式迁移

| 旧代码 | 新代码 |
|--------|--------|
| `button->setStyleSheet(PageStyles::primaryButton());` | `StyleHelper::setPrimaryButton(button);` |
| `button->setStyleSheet(PageStyles::secondaryButton());` | `StyleHelper::setSecondaryButton(button);` |
| `button->setStyleSheet(PageStyles::dangerButton());` | `StyleHelper::setDangerButton(button);` |

### 4. 输入框/下拉框/分组框

**直接删除** `setStyleSheet` 调用，全局样式会自动生效：

```cpp
// 删除这些行
lineEdit->setStyleSheet(PageStyles::inputField());
comboBox->setStyleSheet(PageStyles::comboBox());
groupBox->setStyleSheet(PageStyles::groupBox());
checkBox->setStyleSheet(PageStyles::checkBox());
table->setStyleSheet(PageStyles::table());
```

### 5. 涨跌颜色迁移

| 旧代码 | 新代码 |
|--------|--------|
| `label->setStyleSheet(PageStyles::valueText(PageStyles::upColor()));` | `StyleHelper::setTrendUp(label);` |
| `label->setStyleSheet(PageStyles::valueText(PageStyles::downColor()));` | `StyleHelper::setTrendDown(label);` |
| `label->setStyleSheet(PageStyles::valueText(PageStyles::flatColor()));` | `StyleHelper::setTrendFlat(label);` |

### 6. 文本样式迁移

| 旧代码 | 新代码 |
|--------|--------|
| `label->setStyleSheet(PageStyles::titleText());` | `StyleHelper::setTitleLabel(label);` |
| `label->setStyleSheet(PageStyles::subtitleText());` | `StyleHelper::setSubtitleLabel(label);` |
| `label->setStyleSheet(PageStyles::valueText());` | `StyleHelper::setValueLabel(label);` |
| `label->setStyleSheet(PageStyles::labelText());` | `StyleHelper::setLabelText(label);` |

### 7. 动态颜色（需要保留）

如果颜色需要根据数值动态变化，使用 ThemeManager：

```cpp
// 获取当前主题颜色
ThemeColors theme = ThemeManager::instance()->currentTheme();

// 根据条件选择颜色
QString color;
if (value > threshold) {
    color = theme.danger;  // 上涨/危险
} else if (value < -threshold) {
    color = theme.success; // 下跌/成功
} else {
    color = theme.warning; // 警告
}

// 应用颜色
label->setStyleSheet(QString("color: %1;").arg(color));
```

## 批量替换正则表达式

在 VS Code 或其他编辑器中使用：

### 查找并删除（替换为空）
```
->setStyleSheet\(PageStyles::inputField\(\)\)
->setStyleSheet\(PageStyles::comboBox\(\)\)
->setStyleSheet\(PageStyles::groupBox\(\)\)
->setStyleSheet\(PageStyles::checkBox\(\)\)
->setStyleSheet\(PageStyles::table\(\)\)
->setStyleSheet\(PageStyles::pageBackground\(\)\)
```

### 查找并替换
```
查找: button->setStyleSheet\(PageStyles::primaryButton\(\)\);
替换: StyleHelper::setPrimaryButton(button);

查找: button->setStyleSheet\(PageStyles::secondaryButton\(\)\);
替换: StyleHelper::setSecondaryButton(button);
```

## 注意事项

1. **编码问题**：确保文件保存为 UTF-8 编码
2. **备份文件**：迁移前先备份
3. **逐个文件**：建议逐个文件迁移并测试
4. **编译验证**：每次迁移后编译验证

## 文件迁移顺序建议

1. 简单页面（AboutUSPage, SettingsPage）
2. 核心组件（OrderDialog, AlertSettingDialog）
3. 复杂页面（DashboardPage, StockKLinePage）
4. 其他页面

---

**创建时间**: 2026-05-08
