# 页面样式规范

## 一、编码规范

### 1.1 文件编码
- 所有源文件使用 **UTF-8** 编码
- 文件头部添加标准注释块

### 1.2 注释格式

```cpp
/**
 * @file FileName.cpp
 * @brief 文件描述
 * @author WealthPilot Team
 * @version 1.0.0
 */

// 单行注释使用 // 

/**
 * @brief 函数描述
 * @param param1 参数描述
 * @return 返回值描述
 */
```

### 1.3 命名规范
- 类名：PascalCase（如 `DashboardPage`）
- 函数名：camelCase（如 `loadData()`）
- 变量名：camelCase（如 `totalCount`）
- 常量：UPPER_CASE（如 `MAX_COUNT`）
- 成员变量：m_ 前缀（如 `m_data`）

## 二、样式规范

### 2.1 颜色使用

**必须使用 Tokens::Colors 命名空间**：

```cpp
// ✅ 正确
QColor color(Tokens::Colors::Primary);
label->setStyleSheet(QString("color: %1;").arg(Tokens::Colors::TextPrimary));

// ❌ 错误
QColor color("#3B82F6");
label->setStyleSheet("color: white;");
```

### 2.2 页面样式

**使用 PageStyles 类**：

```cpp
#include "ui/components/PageStyles.h"

// 标题
titleLabel->setStyleSheet(PageStyles::titleText());

// 按钮
button->setStyleSheet(PageStyles::primaryButton());

// 输入框
lineEdit->setStyleSheet(PageStyles::inputField());

// 表格
table->setStyleSheet(PageStyles::table());

// 卡片
card->setStyleSheet(PageStyles::statCard());
```

### 2.3 涨跌颜色

**中国市场：红涨绿跌**

```cpp
// 使用 Tokens::getTrendColor() 函数
QString color = Tokens::getTrendColor(changePercent);

// 或直接使用
if (change > 0) {
    // 上涨 - 红色
    item->setForeground(QColor(Tokens::Colors::Danger));
} else if (change < 0) {
    // 下跌 - 绿色
    item->setForeground(QColor(Tokens::Colors::Success));
}
```

### 2.4 字体规范

```cpp
// 标题：20px 粗体
titleLabel->setStyleSheet(PageStyles::titleText());

// 副标题：14px
subtitleLabel->setStyleSheet(PageStyles::subtitleText());

// 数值：18px 粗体
valueLabel->setStyleSheet(PageStyles::valueText());

// 标签：12px
label->setStyleSheet(PageStyles::labelText());
```

## 三、布局规范

### 3.1 间距

```cpp
// 使用 Tokens::Spacing
layout->setSpacing(Tokens::Spacing::Md);  // 12px
layout->setContentsMargins(
    Tokens::Spacing::Md,   // left
    Tokens::Spacing::Md,   // top
    Tokens::Spacing::Md,   // right
    Tokens::Spacing::Md    // bottom
);
```

### 3.2 圆角

```cpp
// 使用 Tokens::Radius
widget->setStyleSheet(QString("border-radius: %1px;").arg(Tokens::Radius::Md));
```

## 四、组件规范

### 4.1 表格

```cpp
// 使用委托实现涨跌颜色
table->setItemDelegate(new PriceColorDelegate(this));

// 样式
table->setStyleSheet(PageStyles::table());
```

### 4.2 按钮

```cpp
// 主要按钮
primaryBtn->setStyleSheet(PageStyles::primaryButton());

// 次要按钮
secondaryBtn->setStyleSheet(PageStyles::secondaryButton());

// 危险按钮
dangerBtn->setStyleSheet(PageStyles::dangerButton());
```

### 4.3 输入控件

```cpp
// 文本输入
lineEdit->setStyleSheet(PageStyles::inputField());

// 下拉框
comboBox->setStyleSheet(PageStyles::comboBox());

// 日期选择
dateEdit->setStyleSheet(PageStyles::dateEdit());
```

## 五、页面模板

```cpp
/**
 * @file XxxPage.cpp
 * @brief 页面描述
 */

#include "XxxPage.h"
#include "ui/components/PageStyles.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

struct XxxPage::Impl {
    // 成员变量
};

XxxPage::XxxPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

void XxxPage::setupUI()
{
    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(Tokens::Spacing::Md);
    layout->setContentsMargins(
        Tokens::Spacing::Md,
        Tokens::Spacing::Md,
        Tokens::Spacing::Md,
        Tokens::Spacing::Md
    );
    
    // 使用 PageStyles 设置样式
    // ...
}
```

## 六、检查清单

- [ ] 所有颜色使用 Tokens::Colors
- [ ] 所有样式使用 PageStyles
- [ ] 涨跌颜色使用红涨绿跌
- [ ] 注释使用标准格式
- [ ] 文件编码为 UTF-8
- [ ] 命名遵循规范
