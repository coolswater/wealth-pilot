/**

* @file StyleMigrationGuide.md
* @brief 样式迁移指南 - 从硬编码迁移到 QSS
*
* @details 本文档记录样式迁移的策略和进度
* @author WealthPilot Team
* @version 1.0.0
  */

# 样式迁移指南

## 迁移策略

### 1. 使用属性选择器替代 setStyleSheet

**原则：** 尽量使用 QSS 中的属性选择器，减少代码中的 setStyleSheet 调用。

**示例：**

```cpp
// ❌ 硬编码方式（不推荐）
label->setStyleSheet(QString("color: %1; font-size: 14px;").arg(theme.textPrimary));

// ✅ 属性选择器方式（推荐）
label->setProperty("dataType", "label");
// QSS 中定义：QLabel[dataType="label"] { color: ${textSecondary}; font-size: 12px; }
```

### 2. 使用 Tokens 常量替代硬编码值

**原则：** 所有颜色、尺寸、间距值都应使用 Tokens.h 中定义的常量。

```cpp
// ❌ 硬编码方式
setStyleSheet("background-color: #161b22; padding: 8px;");

// ✅ Tokens 方式
setStyleSheet(QString("background-color: %1; padding: %2px;")
    .arg(Tokens::Colors::BgElevated)
    .arg(Tokens::Spacing::SM));
```

### 3. 组件样式类优先

**原则：** 使用 PageStyles.h / PageStyleHelper 提供的样式方法。

```cpp
// ❌ 直接写样式
setStyleSheet("background-color: #161b22; border: 1px solid #30363d;");

// ✅ 使用样式类
setStyleSheet(PageStyles::cardContainer());
```

## 迁移进度

### 已完成

- [x] 创建 components.qss 组件样式文件
- [x] 集成 components.qss 到 ThemeManager
- [x] 添加到资源文件

### 进行中

- [x] AIAssistantPanelWidget - 使用属性选择器，减少约 72 处硬编码
- [x] MarketDepthWidget - 使用属性选择器，动态颜色保留
- [x] NewsPanelWidget - 使用属性选择器
- [x] AlertSettingDialog - 使用 StyleHelper 和属性选择器
- [ ] ChartToolBar - 已使用 ChartStyles 集中管理
- [ ] OrderDialog - 已使用 StyleHelper

### 待迁移

- [ ] NewsPanelWidget
- [ ] OrderDialog
- [ ] AlertSettingDialog
- [ ] CTPBrokerDialog
- [ ] AddSymbolDialog
- [ ] PortfolioOptimizationDialog
- [ ] SignalDetailPanel
- [ ] StockInfoPanel
- [ ] RiskIndicatorWidget
- [ ] RecommendationListWidget
- [ ] NetworkIndicator

## 组件迁移模板

### 消息气泡组件

**QSS 定义：**

```css
AIAssistantPanelWidget QFrame[messageType="user"] {
    background-color: rgba(88, 166, 255, 0.15);
    border: 1px solid rgba(88, 166, 255, 0.3);
    border-radius: 12px;
}

AIAssistantPanelWidget QFrame[messageType="assistant"] {
    background-color: ${bgElevated};
    border: 1px solid ${border};
    border-radius: 12px;
}
```

**代码修改：**

```cpp
// 设置属性而非样式
frame->setProperty("messageType", isUser ? "user" : "assistant");
// 强制刷新样式
frame->style()->unpolish(frame);
frame->style()->polish(frame);
```

### 状态颜色组件

**QSS 定义：**

```css
QLabel[status="up"] {
    color: ${danger};
}

QLabel[status="down"] {
    color: ${success};
}

QLabel[status="flat"] {
    color: ${textSecondary};
}
```

**代码修改：**

```cpp
// 设置状态属性
label->setProperty("status", change > 0 ? "up" : (change < 0 ? "down" : "flat"));
label->style()->unpolish(label);
label->style()->polish(label);
```

## 注意事项

1. **动态颜色保留：** 涨跌颜色等需要根据数据动态变化的，保留代码中的 setStyleSheet
2. **主题切换：** 使用 ThemeManager::registerThemeChangeListener 监听主题变化
3. **性能优化：** 避免频繁调用 style()->polish()，只在必要时刷新

## 统计

- 原始硬编码样式总数：约 1012 处
- 当前硬编码数量：约 221 处
- 已迁移/优化：约 791 处（减少 78%）

### 已迁移组件

| 组件                          | 原硬编码数 | 迁移方式        | 减少数量 |
|-----------------------------|-------|-------------|------|
| AIAssistantPanelWidget      | ~80   | 属性选择器       | ~72  |
| MarketDepthWidget           | ~40   | 属性选择器       | ~31  |
| NewsPanelWidget             | ~20   | 属性选择器       | ~15  |
| AlertSettingDialog          | ~10   | StyleHelper | ~4   |
| ChartStatusBar              | ~15   | 属性选择器       | ~9   |
| AddSymbolDialog             | ~5    | StyleHelper | ~2   |
| CardWidget                  | ~5    | 属性选择器       | ~3   |
| SidebarWidget               | ~15   | 属性选择器       | ~10  |
| SignalDetailPanel           | ~15   | 属性选择器       | ~10  |
| RiskIndicatorWidget         | ~20   | 属性选择器       | ~15  |
| RecommendationListWidget    | ~15   | 属性选择器       | ~6   |
| PortfolioOptimizationDialog | ~15   | StyleHelper | ~5   |
| AnimationManager            | ~5    | 属性选择器       | ~5   |
| StatusBarWidget             | ~20   | 属性选择器       | ~12  |
| StockInfoPanel              | ~15   | 属性选择器       | ~11  |
| TickTableView               | ~10   | 属性选择器       | ~8   |
| AboutUSPage                 | ~15   | 属性选择器       | ~10  |
| AccountPage                 | ~10   | 属性选择器       | ~8   |
| AlertCenterPage             | ~25   | 属性选择器       | ~20  |
| BacktestPage                | ~30   | 属性选择器       | ~20  |

### 剩余待迁移文件

以下文件仍包含硬编码样式，但大部分是必要的动态样式（涨跌颜色、实时更新等）：

**大型页面（动态样式为主）：**

- DashboardPage.cpp（63处 - 涨跌颜色委托、实时数据更新）
- PortfolioPage.cpp（61处 - 图表样式、动态颜色）
- NewsPage.cpp（20处 - 卡片悬停效果）
- SignalCenterPage.cpp（20处 - 卡片悬停效果）
- FundPage.cpp（19处 - 表格样式）

**图表组件（自定义绘制）：**

- StockKLinePage.cpp（7处 - K线图工具栏）
- FuturesKLinePage.cpp（7处 - 期货K线）
- TreeMapWidget.cpp（1处 - 热力图）

**工具类（设计令牌定义）：**

- StyleHelper.cpp（5处 - 样式辅助方法）
- ThemeManager.cpp（2处 - 主题管理）
- ThemeEngine.cpp（1处 - 主题引擎）

**其他：**

- MainWindow.cpp（4处 - 窗口样式）
- SettingsPage.cpp（3处 - 滑块样式）
- WarningPage.cpp（7处 - 预警卡片）
- ChartToolBar.cpp（1处 - 使用 ChartStyles）

## 迁移后的代码风格

### 属性选择器模式

```cpp
// 设置属性
label->setProperty("status", "up");
label->setProperty("dataType", "value");

// 强制刷新样式（重要！）
label->style()->unpolish(label);
label->style()->polish(label);
```

### 对象名选择器模式

```cpp
// 设置对象名
button->setObjectName("sendBtn");
input->setObjectName("aiInputField");

// QSS 中使用 # 选择器
// #sendBtn { background-color: ${primary}; }
```

### 动态颜色保留

对于涨跌颜色等需要根据数据动态变化的场景，保留代码中的 setStyleSheet：

```cpp
// 动态颜色 - 保留在代码中
QColor color = change > 0 ? upColor : downColor;
label->setStyleSheet(QString("color: %1;").arg(color.name()));
```