# WealthPilot 按钮样式架构设计总结

## 一、问题分析

### 当前存在的问题

1. **样式不统一**
    - 不同页面按钮样式各异
    - 硬编码样式散落在各处
    - 同类型按钮视觉表现不一致

2. **文字显示不全**
    - 未设置最小宽度
    - 按钮尺寸与文字不匹配
    - 紧凑布局时文字截断

3. **尺寸不规范**
    - 按钮高度随意设置
    - 内边距不统一
    - 字体大小不一致

4. **语义不清晰**
    - 按钮类型与功能不对应
    - 颜色使用混乱（如红色既用于删除又用于买入）
    - 缺乏统一的分类标准

---

## 二、解决方案架构

### 1. 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层 (Pages)                          │
│  DashboardPage, StockQuotesPage, TradingPanel, Settings...  │
└─────────────────────────────────────────────────────────────┘
                              ↓ 使用
┌─────────────────────────────────────────────────────────────┐
│                      API层 (ButtonStyles)                    │
│  setPrimary(), setSecondary(), setDanger(), setSize()...    │
└─────────────────────────────────────────────────────────────┘
                              ↓ 设置属性
┌─────────────────────────────────────────────────────────────┐
│                      QSS层 (buttons.qss)                     │
│  QPushButton[buttonType="primary"] { ... }                  │
│  QPushButton[buttonSize="large"] { ... }                    │
└─────────────────────────────────────────────────────────────┘
                              ↓ 使用变量
┌─────────────────────────────────────────────────────────────┐
│                    主题层 (ThemeManager)                     │
│  ${primary}, ${danger}, ${success}, ${bgElevated}...        │
└─────────────────────────────────────────────────────────────┘
                              ↓ 定义
┌─────────────────────────────────────────────────────────────┐
│                    Token层 (Tokens.h)                        │
│  Colors::Primary, Colors::Danger, Colors::Success...        │
└─────────────────────────────────────────────────────────────┘
```

### 2. 核心组件

| 组件                      | 文件                                      | 职责       |
|-------------------------|-----------------------------------------|----------|
| ButtonStyles            | `src/ui/styles/ButtonStyles.h`          | 按钮样式 API |
| buttons.qss             | `resources/style/buttons.qss`           | 按钮样式定义   |
| ButtonDesignSpec.md     | `docs/design/ButtonDesignSpec.md`       | 设计规范文档   |
| ButtonStylesExample.cpp | `docs/examples/ButtonStylesExample.cpp` | 使用示例     |

---

## 三、按钮分类体系

### 按语义分类（6大类）

#### 1. 主要操作按钮 (Primary)

- **用途**：保存、提交、确认、运行
- **样式**：蓝色背景、白色文字、加粗
- **示例**：保存设置、提交订单、运行回测

#### 2. 次要操作按钮 (Secondary)

- **用途**：取消、关闭、返回
- **样式**：透明背景、灰色边框、灰色文字
- **示例**：取消、关闭、返回

#### 3. 状态按钮

| 类型      | 用途   | 颜色 | 示例       |
|---------|------|----|----------|
| Success | 正向操作 | 绿色 | 买入、启用、订阅 |
| Danger  | 危险操作 | 红色 | 卖出、删除、清空 |
| Warning | 警告操作 | 橙色 | 重置、清除缓存  |
| Info    | 信息操作 | 蓝色 | 详情、查看、帮助 |

#### 4. 功能按钮

| 类型      | 用途   | 示例   |
|---------|------|------|
| Refresh | 刷新数据 | 刷新行情 |
| Add     | 添加项目 | 添加预警 |
| Edit    | 编辑项目 | 编辑配置 |
| Delete  | 删除项目 | 删除选中 |
| Export  | 导出数据 | 导出报告 |

#### 5. 工具按钮

| 类型   | 样式      | 用途    |
|------|---------|-------|
| Icon | 无背景、无边框 | 纯图标按钮 |
| Text | 无背景、无边框 | 纯文本按钮 |
| Link | 蓝色下划线   | 链接样式  |

#### 6. 对话框按钮

| 类型           | 样式     | 用途    |
|--------------|--------|-------|
| DialogAccept | 主按钮样式  | 确定、保存 |
| DialogReject | 次要按钮样式 | 取消、关闭 |
| DialogApply  | 成功按钮样式 | 应用    |

### 按尺寸分类（4种）

| 尺寸         | 高度   | 字体   | 适用场景     |
|------------|------|------|----------|
| Small      | 24px | 11px | 表格内、紧凑布局 |
| Medium     | 32px | 13px | 默认尺寸     |
| Large      | 40px | 14px | 重要操作、对话框 |
| ExtraLarge | 48px | 16px | 引导页、空状态  |

---

## 四、关键设计决策

### 1. 属性选择器方案

使用 QSS 属性选择器实现样式分类：

```css
QPushButton[buttonType="primary"] {
    background-color: ${primary};
    color: white;
}

QPushButton[buttonType="danger"] {
    background-color: ${danger};
    color: white;
}
```

**优点**：

- 样式与代码分离
- 支持动态切换
- 易于维护和扩展

### 2. 最小宽度防截断

```cpp
void ButtonStyles::setMinWidth(QPushButton* button, int minChars = 4)
{
    QFontMetrics fm(button->font());
    int textWidth = fm.horizontalAdvance(QString(minChars, 'W'));
    int minWidth = textWidth + padding * 2;
    button->setMinimumWidth(qMax(minWidth, 60));
}
```

**解决问题**：

- 文字显示不全
- 短文本按钮过小

### 3. 语义化命名

```cpp
// 清晰的语义
ButtonStyles::setPrimary(saveBtn);    // 主要操作
ButtonStyles::setDanger(deleteBtn);   // 危险操作
ButtonStyles::setSuccess(buyBtn);     // 正向操作

// 而非模糊的
button->setStyleSheet("background: blue;");
```

---

## 五、迁移路径

### Phase 1：基础设施（已完成）

- 创建 ButtonStyles 类
- 创建 buttons.qss 样式文件
- 创建设计规范文档

### Phase 2：核心页面迁移（推荐优先）

- TradingPanel（交易面板）
- AlertCenterPage（预警中心）
- BacktestPage（回测页面）
- SettingsPage（设置页面）

### Phase 3：其他页面迁移

- DashboardPage
- StockQuotesPage
- SignalCenterPage
- 各对话框

### Phase 4：验证与优化

- 视觉一致性检查
- 文字截断测试
- 主题切换测试

---

## 六、使用指南

### 快速开始

```cpp
#include "ButtonStyles.h"

// 1. 创建按钮并应用样式
QPushButton* saveBtn = new QPushButton("保存");
ButtonStyles::setPrimary(saveBtn);

// 2. 设置尺寸
ButtonStyles::setLarge(saveBtn);

// 3. 设置最小宽度
ButtonStyles::setMinWidth(saveBtn, 4);
```

### 常见场景

#### 工具栏按钮

```cpp
ButtonStyles::setAdd(addBtn);
ButtonStyles::setEdit(editBtn);
ButtonStyles::setDelete(deleteBtn);
ButtonStyles::setRefresh(refreshBtn);
```

#### 对话框按钮

```cpp
ButtonStyles::setPrimary(okBtn);
ButtonStyles::setSecondary(cancelBtn);
okBtn->setMinimumWidth(90);
cancelBtn->setMinimumWidth(90);
```

#### 交易按钮

```cpp
ButtonStyles::setSuccess(buyBtn);   // 绿色买入
ButtonStyles::setDanger(sellBtn);   // 红色卖出
ButtonStyles::setLarge(buyBtn);
ButtonStyles::setLarge(sellBtn);
```

---

## 七、文件清单

### 新增文件

| 文件                      | 位置                 | 说明         |
|-------------------------|--------------------|------------|
| ButtonStyles.h          | `src/ui/styles/`   | 按钮样式管理类头文件 |
| ButtonStyles.cpp        | `src/ui/styles/`   | 按钮样式管理类实现  |
| buttons.qss             | `resources/style/` | 按钮样式定义     |
| ButtonDesignSpec.md     | `docs/design/`     | 设计规范文档     |
| ButtonStylesExample.cpp | `docs/examples/`   | 使用示例代码     |

### 修改文件

| 文件               | 修改内容           |
|------------------|----------------|
| ThemeManager.cpp | 加载 buttons.qss |

---

## 八、效果预期

### 解决的问题

| 问题    | 解决方案                 | 效果       |
|-------|----------------------|----------|
| 样式不统一 | 统一的 ButtonStyles API | 所有按钮风格一致 |
| 文字截断  | setMinWidth() 方法     | 文字完整显示   |
| 尺寸不规范 | 四档尺寸系统               | 视觉层次清晰   |
| 语义不清  | 语义化分类                | 功能一目了然   |

### 带来的改进

1. **开发效率**
    - 一行代码设置样式
    - 无需手写 QSS
    - 减少样式调试时间

2. **维护效率**
    - 样式集中管理
    - 修改一处全局生效
    - 易于扩展新类型

3. **用户体验**
    - 视觉一致性
    - 操作语义清晰
    - 主题切换无缝

---

## 九、后续优化建议

### 1. 动画效果

- 添加按钮点击动画
- 悬停过渡动画
- 加载状态动画

### 2. 无障碍支持

- 焦点状态优化
- 键盘导航支持
- 高对比度模式

### 3. 扩展类型

- 更多功能按钮类型
- 自定义颜色支持
- 按钮组组件

---

## 十、总结

本设计方案通过分层架构、语义化分类、统一 API 和 QSS 属性选择器，系统性地解决了 WealthPilot 项目中按钮样式不统一、文字截断、尺寸不规范等问题。

核心价值：

- **一致性**：统一视觉风格
- **可维护性**：集中管理样式
- **可扩展性**：易于添加新类型
- **开发效率**：简化样式设置