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

#include <QColor>
#include <QString>
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
    // ========== 价格颜色（金融标准：红涨绿跌）==========
    static QColor upColor() { return QColor(Tokens::Colors::Danger); }     // 红色=涨
    static QColor downColor() { return QColor(Tokens::Colors::Success); } // 绿色=跌
    static QColor flatColor() { return QColor(Tokens::Colors::TextSecondary); }
    
    // ========== 背景颜色 ==========
    static QColor backgroundPrimary() { return QColor(Tokens::Colors::BgBase); }
    static QColor backgroundSecondary() { return QColor(Tokens::Colors::BgSurface); }
    static QColor backgroundCard() { return QColor(Tokens::Colors::BgElevated); }
    static QColor backgroundHover() { return QColor(Tokens::Colors::BgHover); }
    
    // ========== 文字颜色 ==========
    static QColor textPrimary() { return QColor(Tokens::Colors::TextPrimary); }
    static QColor textSecondary() { return QColor(Tokens::Colors::TextSecondary); }
    static QColor textDisabled() { return QColor(Tokens::Colors::TextDisabled); }
    
    // ========== 边框颜色 ==========
    static QColor border() { return QColor(Tokens::Colors::Border); }
    
    // ========== 状态颜色 ==========
    static QColor success() { return QColor(Tokens::Colors::Success); }
    static QColor warning() { return QColor(Tokens::Colors::Warning); }
    static QColor error() { return QColor(Tokens::Colors::Danger); }
    static QColor info() { return QColor(Tokens::Colors::Info); }
    
    // ========== 品牌颜色 ==========
    static QColor primary() { return QColor(Tokens::Colors::Primary); }
    static QColor secondary() { return QColor(Tokens::Colors::Secondary); }
    
    // ========== 工具方法 ==========
    static QColor getChangeColor(double change) {
        if (change > 0) return upColor();
        if (change < 0) return downColor();
        return flatColor();
    }
    
    static QColor getChangeColor(double current, double base) {
        if (base <= 0) return flatColor();
        return getChangeColor(current - base);
    }
};

#endif // THEMECOLORS_H
