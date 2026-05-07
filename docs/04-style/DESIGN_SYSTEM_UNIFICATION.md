# WealthPilot 设计系统统一方案

## 当前问题

项目存在 **4 个** 颜色/样式定义文件，存在大量重复：

| 文件            | 路径                 | 使用情况   | 内容       |
|---------------|--------------------|--------|----------|
| Tokens.h      | src/core/config/   | ❌ 未使用  | 最完整的设计系统 |
| ChartStyles.h | src/ui/components/ | ✅ 图表相关 | 图表专用样式   |
| ThemeColors.h | src/ui/components/ | ✅ 通用   | 颜色常量     |
| PageStyles.h  | src/ui/components/ | ✅ 页面   | 样式字符串    |

## 推荐方案：统一到 Tokens.h

### 理由

1. `Tokens.h` 最完整，包含颜色、间距、字体、圆角、动画、尺寸等
2. 符合设计系统（Design Tokens）最佳实践
3. 位于 `core/config/` 目录，适合作为核心配置

### 迁移步骤

#### 第一阶段：建立引用关系

```cpp
// ThemeColors.h - 引用 Tokens
#include "core/config/Tokens.h"

namespace Colors = Tokens::Colors;  // 别名
```

#### 第二阶段：更新 ChartStyles.h

```cpp
// ChartStyles.h - 引用 Tokens
#include "core/config/Tokens.h"

namespace ChartStyles {
    // 使用 Tokens::Colors
    using namespace Tokens::Colors;
    
    // 图表专用的额外定义
    namespace IndicatorColors {
        using namespace Tokens::Colors;
    }
}
```

#### 第三阶段：逐步迁移使用代码

- `ThemeColors::` → `Tokens::Colors::`
- `ChartStyles::Colors::` → `Tokens::Colors::`
- `Colors::` (全局) → `Tokens::Colors::`

### 颜色值对比

| 用途   | Tokens.h | ChartStyles.h | ThemeColors.h |
|------|----------|---------------|---------------|
| 主色   | #3B82F6  | #3B82F6       | #3B82F6       |
| 涨/成功 | #10B981  | #10B981       | #EF4444 ❌     |
| 跌/危险 | #EF4444  | #EF4444       | #10B981 ❌     |
| 背景   | #1A1F2E  | #1A1F2E       | #1A1F2E       |

**注意：** ThemeColors.h 中涨跌颜色与 Tokens.h 相反！需要统一。

## 立即可做的优化

### 1. 修复颜色不一致

- 统一涨跌颜色：涨=#EF4444(红)，跌=#10B981(绿)
- 更新 ThemeColors.h 使用正确的颜色

### 2. 添加引用关系

让 ThemeColors.h 和 ChartStyles.h 引用 Tokens.h

### 3. 文档化

- 在每个文件头部注明"颜色定义请参考 Tokens.h"
- 添加迁移指南

## 长期目标

1. **单一来源原则**：所有设计令牌从 Tokens.h 获取
2. **类型安全**：使用 constexpr 和 inline 确保编译期检查
3. **主题支持**：Tokens.h 支持多主题切换

## 文件职责划分

```
Tokens.h          → 核心设计令牌（颜色、间距、字体、动画）
ThemeColors.h     → 颜色工具类（QColor 转换、便捷方法）
ChartStyles.h     → 图表专用样式字符串
PageStyles.h      → 页面样式字符串
```

## 下一步行动

- [ ] 确认涨跌颜色标准（红涨绿跌）
- [ ] 更新 ThemeColors.h 引用 Tokens
- [ ] 更新 ChartStyles.h 引用 Tokens
- [ ] 扫描并更新所有硬编码颜色
- [ ] 添加迁移文档
