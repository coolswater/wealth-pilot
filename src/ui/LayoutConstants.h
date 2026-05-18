/**
 * @file LayoutConstants.h
 * @brief 统一布局常量 - 避免硬编码
 *
 * @details 定义全局布局常量，确保UI一致性
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef WEALTHPILOT_LAYOUTCONSTANTS_H
#define WEALTHPILOT_LAYOUTCONSTANTS_H

#include <QtGlobal>

namespace WealthPilot {
namespace Layout {

// ============================================================================
// 窗口尺寸
// ============================================================================
namespace Window {
    constexpr int MinWidth = 1280;
    constexpr int MinHeight = 720;
    constexpr int DefaultWidth = 1600;
    constexpr int DefaultHeight = 900;
}

// ============================================================================
// 侧边栏
// ============================================================================
namespace Sidebar {
    constexpr int Width = 200;
    constexpr int MinWidth = 180;
    constexpr int MaxWidth = 280;
    constexpr int ItemHeight = 48;
    constexpr int IconSize = 24;
}

// ============================================================================
// 工具栏
// ============================================================================
namespace Toolbar {
    constexpr int Height = 48;
    constexpr int ButtonWidth = 40;
    constexpr int ButtonHeight = 32;
    constexpr int IconSize = 20;
    constexpr int Spacing = 8;
}

// ============================================================================
// 内容区域
// ============================================================================
namespace Content {
    constexpr int Padding = 16;
    constexpr int Spacing = 12;
    constexpr int BorderRadius = 8;
}

// ============================================================================
// 输入控件
// ============================================================================
namespace Input {
    constexpr int MinWidth = 120;
    constexpr int MaxWidth = 300;
    constexpr int Height = 32;
    constexpr int SearchWidth = 200;
    constexpr int ComboBoxWidth = 150;
    constexpr int ApiKeyWidth = 250;
    constexpr int ApiUrlWidth = 300;
}

// ============================================================================
// 面板
// ============================================================================
namespace Panel {
    constexpr int InfoMinWidth = 280;
    constexpr int InfoMaxWidth = 320;
    constexpr int ChartMinHeight = 400;
    constexpr int ListMinHeight = 200;
}

// ============================================================================
// 对话框
// ============================================================================
namespace Dialog {
    constexpr int MinWidth = 400;
    constexpr int MinHeight = 300;
    constexpr int MaxWidth = 800;
    constexpr int MaxHeight = 600;
    constexpr int ButtonWidth = 80;
    constexpr int ButtonHeight = 32;
}

// ============================================================================
// 表格
// ============================================================================
namespace Table {
    constexpr int RowHeight = 36;
    constexpr int HeaderHeight = 40;
    constexpr int MinColumnWidth = 80;
    constexpr int MaxColumnWidth = 200;
}

// ============================================================================
// 图表
// ============================================================================
namespace Chart {
    constexpr int MinWidth = 600;
    constexpr int MinHeight = 400;
    constexpr int LegendHeight = 30;
    constexpr int AxisWidth = 60;
}

// ============================================================================
// 动画
// ============================================================================
namespace Animation {
    constexpr int Duration = 200;       // 毫秒
    constexpr int FastDuration = 100;
    constexpr int SlowDuration = 300;
}

// ============================================================================
// 间距
// ============================================================================
namespace Spacing {
    constexpr int Tight = 4;
    constexpr int Normal = 8;
    constexpr int Medium = 12;
    constexpr int Loose = 16;
    constexpr int Wide = 24;
    constexpr int ExtraWide = 32;
}

} // namespace Layout
} // namespace WealthPilot

#endif // WEALTHPILOT_LAYOUTCONSTANTS_H