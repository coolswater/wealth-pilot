/**
 * @file LayoutConstants.h
 * @brief 布局常量配置 - 统一管理应用布局参数
 *
 * @details 定义：
 * - 窗口尺寸
 * - 组件尺寸
 * - 间距和边距
 * - 布局比例
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef LAYOUTCONSTANTS_H
#define LAYOUTCONSTANTS_H

#include <QMargins>
#include <QSize>

/**
 * @brief 布局常量命名空间
 *
 * @details 提供统一的布局参数，确保整个应用布局一致性
 */
namespace Layout {

// ============================================================================
// 窗口尺寸
// ============================================================================

namespace Window {
    constexpr int MinWidth = 1280;      ///< 最小宽度
    constexpr int MinHeight = 800;      ///< 最小高度
    constexpr int DefaultWidth = 1600;  ///< 默认宽度
    constexpr int DefaultHeight = 900;  ///< 默认高度
}

// ============================================================================
// 组件高度
// ============================================================================

namespace Height {
    constexpr int TitleBar = 40;        ///< 标题栏
    constexpr int ToolBar = 48;         ///< 工具栏
    constexpr int StatusBar = 28;       ///< 状态栏
    constexpr int Sidebar = 0;          ///< 侧边栏（自动高度）
    constexpr int Button = 36;          ///< 按钮
    constexpr int Input = 36;           ///< 输入框
    constexpr int ListItem = 40;        ///< 列表项
    constexpr int TabBar = 40;          ///< 标签栏
    constexpr int Header = 44;          ///< 表头
}

// ============================================================================
// 组件宽度
// ============================================================================

namespace Width {
    constexpr int Sidebar = 80;         ///< 侧边栏
    constexpr int AIPanel = 300;        ///< AI面板
    constexpr int DepthPanel = 280;     ///< 盘口面板
    constexpr int Button = 120;         ///< 默认按钮
    constexpr int Input = 200;          ///< 默认输入框
    constexpr int ComboBox = 100;       ///< 下拉框
}

// ============================================================================
// 间距
// ============================================================================

namespace Spacing {
    constexpr int None = 0;             ///< 无间距
    constexpr int XS = 4;               ///< 超小间距
    constexpr int SM = 8;               ///< 小间距
    constexpr int MD = 12;              ///< 中间距
    constexpr int LG = 16;              ///< 大间距
    constexpr int XL = 24;              ///< 超大间距
    constexpr int XXL = 32;             ///< 巨大间距
}

// ============================================================================
// 边距
// ============================================================================

namespace Margin {
    inline QMargins None() { return QMargins(0, 0, 0, 0); }
    inline QMargins XS() { return QMargins(4, 4, 4, 4); }
    inline QMargins SM() { return QMargins(8, 8, 8, 8); }
    inline QMargins MD() { return QMargins(12, 12, 12, 12); }
    inline QMargins LG() { return QMargins(16, 16, 16, 16); }
    inline QMargins XL() { return QMargins(24, 24, 24, 24); }
    
    // 特殊边距
    inline QMargins ToolBar() { return QMargins(16, 8, 16, 8); }
    inline QMargins Content() { return QMargins(16, 16, 16, 16); }
    inline QMargins Card() { return QMargins(16, 12, 16, 12); }
    inline QMargins Dialog() { return QMargins(24, 20, 24, 24); }
}

// ============================================================================
// 圆角
// ============================================================================

namespace Radius {
    constexpr int None = 0;             ///< 无圆角
    constexpr int SM = 4;               ///< 小圆角
    constexpr int MD = 6;               ///< 中圆角
    constexpr int LG = 8;               ///< 大圆角
    constexpr int XL = 12;              ///< 超大圆角
    constexpr int Round = 9999;         ///< 完全圆角
}

// ============================================================================
// 字体大小
// ============================================================================

namespace FontSize {
    constexpr int XS = 10;              ///< 超小字体
    constexpr int SM = 12;              ///< 小字体
    constexpr int MD = 14;              ///< 中字体（默认）
    constexpr int LG = 16;              ///< 大字体
    constexpr int XL = 18;              ///< 超大字体
    constexpr int XXL = 24;             ///< 巨大字体
    constexpr int Title = 20;           ///< 标题字体
    constexpr int Heading = 18;         ///< 标题字体
}

// ============================================================================
// 布局比例
// ============================================================================

namespace Ratio {
    constexpr double Sidebar = 0.05;    ///< 侧边栏占比
    constexpr double Content = 0.70;    ///< 内容区占比
    constexpr double AIPanel = 0.25;    ///< AI面板占比
    
    // K线页面
    constexpr double Chart = 0.70;      ///< K线图占比
    constexpr double Depth = 0.30;      ///< 盘口占比
    constexpr double Volume = 0.20;     ///< 成交量图占比
    constexpr double Indicator = 0.15;  ///< 指标图占比
}

// ============================================================================
// 动画时长
// ============================================================================

namespace Animation {
    constexpr int Fast = 150;           ///< 快速动画
    constexpr int Normal = 250;         ///< 正常动画
    constexpr int Slow = 400;           ///< 慢速动画
    constexpr int Toast = 3000;         ///< 提示显示时长
}

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 计算分割器尺寸
 * @param totalWidth 总宽度
 * @param ratio 比例
 * @return 分割尺寸列表
 */
inline QList<int> calculateSplitSizes(int totalWidth, double ratio) {
    int first = static_cast<int>(totalWidth * ratio);
    int second = totalWidth - first;
    return {first, second};
}

/**
 * @brief 计算三栏布局尺寸
 * @param totalWidth 总宽度
 * @param leftRatio 左侧比例
 * @param rightRatio 右侧比例
 * @return 分割尺寸列表
 */
inline QList<int> calculateThreeColumnSizes(int totalWidth, double leftRatio, double rightRatio) {
    int left = static_cast<int>(totalWidth * leftRatio);
    int right = static_cast<int>(totalWidth * rightRatio);
    int center = totalWidth - left - right;
    return {left, center, right};
}

/**
 * @brief 根据屏幕尺寸计算合适的字体大小
 * @param screenWidth 屏幕宽度
 * @return 字体大小
 */
inline int calculateFontSize(int screenWidth) {
    if (screenWidth >= 1920) return FontSize::MD;
    if (screenWidth >= 1600) return FontSize::SM;
    return FontSize::XS;
}

/**
 * @brief 根据屏幕尺寸计算合适的间距
 * @param screenWidth 屏幕宽度
 * @return 间距
 */
inline int calculateSpacing(int screenWidth) {
    if (screenWidth >= 1920) return Spacing::LG;
    if (screenWidth >= 1600) return Spacing::MD;
    return Spacing::SM;
}

} // namespace Layout

#endif // LAYOUTCONSTANTS_H
