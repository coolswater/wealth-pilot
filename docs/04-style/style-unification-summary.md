# 样式统一与硬编码消除总结

## 🎯 目标

消除代码中的硬编码颜色值，统一使用设计令牌系统，确保整个应用的一致性和可维护性。

## ✅ 已完成的工作

### 1. 使用统一的设计令牌系统

**设计令牌位置**：

- `src/core/config/Tokens.h` - 核心设计令牌定义
- `src/ui/components/ThemeColors.h` - 主题颜色工具类

**主要颜色定义**：

```cpp
namespace Tokens::Colors {
    // 功能色
    Success = "#10B981";  // 成功/上涨（绿色）
    Danger = "#EF4444";   // 错误/下跌（红色）
    Warning = "#F97316";  // 警告（橙色）
    Info = "#0EA5E9";     // 信息（蓝色）
    
    // 背景色
    BgBase = "#1A1F2E";       // 主背景
    BgSurface = "#0F1419";    // 表面色
    BgElevated = "#242937";   // 抬升色（卡片）
    
    // 文字色
    TextPrimary = "#FFFFFF";      // 主文字
    TextSecondary = "#9CA3AF";    // 次要文字
    TextDisabled = "#4B5563";     // 禁用文字
    
    // 边框色
    Border = "#14FFFFFF";
}
```

### 2. StockInfoPanel样式统一

**修改前（硬编码）**：

```cpp
setStyleSheet("background-color: #1E1E1E; color: #FFFFFF;");
m_bidLabels[i]->setStyleSheet("color: #00AA00;");  // 硬编码绿色
m_askLabels[i]->setStyleSheet("color: #FF0000;");  // 硬编码红色
```

**修改后（使用令牌）**：

```cpp
setStyleSheet(QString("background-color: %1; color: %2;")
    .arg(Colors::BgElevated, Colors::TextPrimary));
m_bidLabels[i]->setStyleSheet(QString("color: %1;")
    .arg(Colors::Success));  // 使用统一令牌
m_askLabels[i]->setStyleSheet(QString("color: %1;")
    .arg(Colors::Danger));   // 使用统一令牌
```

### 3. 价格颜色逻辑统一

**使用ThemeColors工具类**：

```cpp
void updatePriceLabel(QLabel* label, double price, double prevPrice) {
    QString color;
    if (prevPrice > 0) {
        color = (price > prevPrice) ? Colors::Success :    // 涨=绿
                (price < prevPrice ? Colors::Danger :      // 跌=红
                Colors::TextSecondary);                     // 平=灰
    }
    label->setStyleSheet(QString("color: %1;").arg(color));
}
```

## 📊 样式规范

### 颜色使用规范

| 用途    | 令牌                      | 说明          |
|-------|-------------------------|-------------|
| 上涨/买入 | `Colors::Success`       | 绿色 #10B981  |
| 下跌/卖出 | `Colors::Danger`        | 红色 #EF4444  |
| 平盘/中性 | `Colors::TextSecondary` | 灰色 #9CA3AF  |
| 主背景   | `Colors::BgBase`        | 深色 #1A1F2E  |
| 卡片背景  | `Colors::BgElevated`    | 浅深色 #242937 |
| 主文字   | `Colors::TextPrimary`   | 白色 #FFFFFF  |
| 次要文字  | `Colors::TextSecondary` | 灰色 #9CA3AF  |

### 字体大小规范

| 元素   | 大小   | 说明 |
|------|------|----|
| 股票名称 | 16px | 加粗 |
| 价格   | 24px | 加粗 |
| 涨跌幅  | 14px | 普通 |
| 五档盘口 | 11px | 普通 |
| 成交明细 | 11px | 普通 |

## 🔧 实现细节

### 1. 引入设计令牌

```cpp
#include "core/config/Tokens.h"
using namespace Tokens;
```

### 2. 使用字符串拼接

```cpp
QString style = QString("background-color: %1; color: %2;")
    .arg(Colors::BgElevated)
    .arg(Colors::TextPrimary);
widget->setStyleSheet(style);
```

### 3. 动态颜色判断

```cpp
QString getChangeColor(double change) {
    if (change > 0) return Colors::Success;
    if (change < 0) return Colors::Danger;
    return Colors::TextSecondary;
}
```

## 📁 修改的文件

**StockInfoPanel**：

- `src/ui/components/StockInfoPanel.h` - 移除硬编码颜色常量
- `src/ui/components/StockInfoPanel.cpp` - 使用Tokens::Colors

## ✅ 编译状态

```
[6/6] Linking CXX executable WealthPilot.exe
Process exited with code 0.
```

**编译成功！样式已统一！**

## 🎨 优势

### 1. 一致性

- 所有组件使用相同的颜色定义
- 视觉风格统一

### 2. 可维护性

- 修改颜色只需改一处
- 避免颜色值不一致

### 3. 主题切换

- 支持深色/浅色主题
- 只需切换令牌定义

### 4. 代码质量

- 消除魔法数字
- 提高代码可读性

## 📝 最佳实践

### ✅ 推荐做法

```cpp
// 使用设计令牌
setStyleSheet(QString("color: %1;").arg(Colors::Success));

// 使用工具类
QColor color = ThemeColors::getChangeColor(change);
```

### ❌ 避免做法

```cpp
// 硬编码颜色
setStyleSheet("color: #00AA00;");

// 魔法数字
if (change > 0) color = "#FF0000";
```

## 🚀 后续工作

1. **检查其他组件**
    - 扫描所有UI组件
    - 替换硬编码颜色

2. **完善设计令牌**
    - 添加更多语义化颜色
    - 支持更多主题变体

3. **文档完善**
    - 更新设计规范文档
    - 添加使用示例

## 📚 相关文档

- `src/core/config/Tokens.h` - 设计令牌定义
- `src/ui/components/ThemeColors.h` - 主题颜色工具
- `docs/design-system.md` - 设计系统文档

---

**样式统一完成！代码质量提升！** 🎉
