/**
 * @file ThemeColors.h
 * @brief 统一主题颜色配置 - 全应用颜色方案
 * 
 * @details 本文件是 Tokens.h 的便捷别名，保持向后兼容
 * 所有颜色定义请参考 core/config/Tokens.h
 * 
 * @author WealthPilot Team
 * @version 3.0.0
 */

#ifndef THEMECOLORS_H
#define THEMECOLORS_H

#include "core/config/Tokens.h"

/**
 * @brief 颜色命名空间（引用 Tokens）
 */
namespace Colors = Tokens::Colors;

/**
 * @brief 主题颜色工具类
 *
 * 提供颜色获取、样式生成等工具方法
 */
class ThemeColors
{
public:
    // 获取颜色
    static QColor primary() { return QColor(Tokens::Colors::Primary); }
    static QColor danger() { return QColor(Tokens::Colors::Danger); }
    static QColor success() { return QColor(Tokens::Colors::Success); }
    static QColor warning() { return QColor(Tokens::Colors::Warning); }

    // 背景色
    static QColor background() { return QColor(Tokens::Colors::BgBase); }
    static QColor surface() { return QColor(Tokens::Colors::BgSurface); }
    static QColor elevated() { return QColor(Tokens::Colors::BgElevated); }

    // 文本色
    static QColor textPrimary() { return QColor(Tokens::Colors::TextPrimary); }
    static QColor textSecondary() { return QColor(Tokens::Colors::TextSecondary); }
    static QColor textTertiary() { return QColor(Tokens::Colors::TextTertiary); }

    // 边框色
    static QColor border() { return QColor(Tokens::Colors::Border); }
    static QColor borderLight() { return QColor(Tokens::Colors::BorderLight); }
};

#endif // THEMECOLORS_H
