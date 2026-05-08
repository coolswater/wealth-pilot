# WealthPilot 样式迁移指南

## 概述

本文档说明如何将 WealthPilot 项目从硬编码样式迁移到 QSS 文件系统。

## 1. QSS 文件结构

```
resources/style/
├── base.qss           # 基础样式（所有主题共用）
├── theme_dark.qss     # 深色主题
├── theme_light.qss    # 浅色主题
└── theme_eyecare.qss  # 护眼主题
```

## 2. 颜色变量系统

QSS 文件使用变量占位符，由 ThemeManager 在运行时替换：

| 变量 | 说明 | 深色主题值 |
|------|------|-----------|
| `${bgPrimary}` | 主背景色 | #0d1117 |
| `${bgSecondary}` | 次背景色 | #161b22 |
| `${bgElevated}` | 提升背景色 | #161b22 |
| `${bgSurface}` | 表面背景色 | #1c2128 |
| `${textPrimary}` | 主文本色 | #e6edf3 |
| `${textSecondary}` | 次文本色 | #8b949e |
| `${primary}` | 主品牌色 | #58a6ff |
| `${success}` | 成功/上涨色 | #3fb950 |
| `${danger}` | 危险/下跌色 | #f85149 |
| `${warning}` | 警告色 | #f0883e |
| `${border}` | 边框色 | #30363d |

## 3. 属性选择器

使用 `setProperty()` 配合 QSS 属性选择器实现动态样式：

### 3.1 按钮类型

```cpp
// 主按钮
button->setProperty("primary", true);
button->style()->unpolish(button);
button->style()->polish(button);

// 次要按钮
button->setProperty("secondary", true);

// 危险按钮
button->setProperty("danger", true);

// 成功按钮
button->setProperty("success", true);

// 图标按钮
button->setProperty("icon", true);
```

### 3.2 涨跌状态

```cpp
// 上涨
label->setProperty("trend", "up");

// 下跌
label->setProperty("trend", "down");

// 平盘
label->setProperty("trend", "flat");

// 刷新样式
label->style()->unpolish(label);
label->style()->polish(label);
```

### 3.3 卡片主题

```cpp
card->setProperty("theme", "success");  // 成功主题
card->setProperty("theme", "warning");  // 警告主题
card->setProperty("theme", "danger");   // 危险主题
card->setProperty("theme", "primary");  // 主色调主题
```

## 4. 迁移步骤

### 4.1 移除 PageStyles 调用

**之前：**
```cpp
button->setStyleSheet(PageStyles::primaryButton());
lineEdit->setStyleSheet(PageStyles::inputField());
comboBox->setStyleSheet(PageStyles::comboBox());
```

**之后：**
```cpp
// 设置属性，让全局样式生效
button->setProperty("primary", true);
// 无需调用 setStyleSheet

// 如果需要刷新样式
button->style()->unpolish(button);
button->style()->polish(button);
```

### 4.2 移除硬编码颜色

**之前：**
```cpp
label->setStyleSheet("color: #EF4444;");  // 硬编码
```

**之后：**
```cpp
// 方法1：使用属性选择器
label->setProperty("trend", "up");
label->style()->unpolish(label);
label->style()->polish(label);

// 方法2：使用 ThemeManager 获取颜色
auto theme = ThemeManager::instance()->currentTheme();
label->setStyleSheet(QString("color: %1;").arg(theme.danger));
```

### 4.3 特殊组件处理

某些组件需要特殊样式，可以使用对象名选择器：

```cpp
// 设置对象名
widget->setObjectName("specialWidget");

// QSS 中定义
// #specialWidget { ... }
```

## 5. Qt 资源系统配置

### 5.1 更新 .qrc 文件

在 `resources/resources.qrc` 中添加：

```xml
<RCC>
    <qresource prefix="/style">
        <file>style/base.qss</file>
        <file>style/theme_dark.qss</file>
        <file>style/theme_light.qss</file>
        <file>style/theme_eyecare.qss</file>
    </qresource>
</RCC>
```

### 5.2 CMakeLists.txt 配置

确保资源文件被包含：

```cmake
set(RESOURCES
    resources/resources.qrc
)

add_executable(WealthPilot
    ${SOURCES}
    ${RESOURCES}
)
```

## 6. 热重载支持（开发模式）

ThemeManager 支持热重载 QSS 文件，方便开发调试：

```cpp
// 启用热重载（开发模式）
#ifdef QT_DEBUG
ThemeManager::instance()->enableHotReload(true);
#endif

// 当 QSS 文件修改后，自动重新加载
// 连接信号
connect(ThemeManager::instance(), &ThemeManager::themeChanged,
        this, &MainWindow::onThemeChanged);
```

## 7. 性能优化建议

### 7.1 避免频繁刷新样式

```cpp
// ❌ 错误：循环中频繁刷新
for (auto widget : widgets) {
    widget->setProperty("trend", "up");
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

// ✅ 正确：批量设置后统一刷新
for (auto widget : widgets) {
    widget->setProperty("trend", "up");
}
// 刷新父容器
parentWidget->update();
```

### 7.2 使用对象名选择器

对于需要特殊样式的组件，优先使用对象名选择器而非内联样式：

```cpp
// 设置对象名
widget->setObjectName("statusLabel");

// QSS 中定义
// QLabel#statusLabel { color: #10B981; }
```

## 8. 迁移检查清单

- [ ] 移除所有 `PageStyles::xxx()` 调用
- [ ] 移除所有硬编码颜色值
- [ ] 使用 `setProperty()` 设置按钮类型
- [ ] 使用 `setProperty()` 设置涨跌状态
- [ ] 更新 .qrc 文件包含 QSS 文件
- [ ] 测试所有主题切换
- [ ] 验证视觉效果无变化

## 9. 常见问题

### Q: 为什么样式没有生效？

A: 检查以下几点：
1. 确认 `setProperty()` 后调用了 `style()->unpolish()` 和 `style()->polish()`
2. 确认 QSS 文件已正确加载
3. 检查选择器是否正确

### Q: 如何调试 QSS？

A: 使用以下方法：
```cpp
// 打印当前样式表
qDebug() << qApp->styleSheet();

// 启用热重载
ThemeManager::instance()->enableHotReload(true);
```

### Q: 涨跌颜色为什么相反？

A: 中国市场使用红涨绿跌，QSS 中已正确配置：
- `${danger}` (#f85149) = 上涨/红色
- `${success}` (#3fb950) = 下跌/绿色

---

**版本历史：**

- v1.0.0 (2026-05-08) - 初始版本
