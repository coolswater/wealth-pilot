/**
 * @file ChartStyles.h
 * @brief 图表样式配置 - 统一管理图表相关样式
 *
 * @details 定义：
 * - 颜色常量（与 Tokens.h 保持同步）
 * - 尺寸常量
 * - 样式字符串
 * - 布局比例
 *
 * @note 颜色值应与 core/config/Tokens.h 保持一致
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef CHARTSTYLES_H
#define CHARTSTYLES_H

#include <QColor>
#include <QString>
#include "core/config/Tokens.h"

/**
 * @brief 图表样式命名空间
 *
 * @details 提供统一的样式常量，确保整个应用视觉一致性
 */
namespace ChartStyles {

// ============================================================================
// 颜色系统（与 theme_dark.qss 保持一致）
// ============================================================================

namespace Colors {
    // 主色调
    constexpr const char* Primary     = "#3B82F6";   ///< 主蓝色
    constexpr const char* Secondary   = "#6366F1";   ///< 紫色
    constexpr const char* Success     = "#10B981";   ///< 涨/成功（绿色）
    constexpr const char* Danger      = "#EF4444";   ///< 跌/错误（红色）
    constexpr const char* Warning     = "#F97316";   ///< 警告（橙色）
    constexpr const char* Info        = "#0EA5E9";   ///< 信息（青色）

    // 背景色
    constexpr const char* BgBase      = "#1A1F2E";   ///< 主背景
    constexpr const char* BgSurface   = "#0F1419";   ///< 侧边栏/表头
    constexpr const char* BgElevated  = "#242937";   ///< 卡片/弹出层
    constexpr const char* BgHover     = "#374151";   ///< 悬停背景

    // 文字颜色
    constexpr const char* TextPrimary   = "#FFFFFF";   ///< 主文字
    constexpr const char* TextSecondary = "#9CA3AF";   ///< 次文字
    constexpr const char* TextTertiary  = "#6B7280";   ///< 三级文字
    constexpr const char* TextDisabled  = "#4B5563";   ///< 禁用文字

    // 边框颜色
    constexpr const char* Border       = "#374151";   ///< 边框
    constexpr const char* BorderLight  = "#4B5563";   ///< 浅边框
    constexpr const char* Divider      = "#2D3748";   ///< 分隔线

    // 指标颜色
    constexpr const char* MA5   = "#FFD700";   ///< MA5 金色
    constexpr const char* MA10  = "#00CED1";   ///< MA10 青色
    constexpr const char* MA20  = "#FF6B6B";   ///< MA20 珊瑚红
    constexpr const char* MA30  = "#9B59B6";   ///< MA30 紫色
    constexpr const char* MA60  = "#3498DB";   ///< MA60 蓝色
    constexpr const char* MACD  = "#FFD700";   ///< MACD 金色
    constexpr const char* RSI   = "#9B59B6";   ///< RSI 紫色
    constexpr const char* KDJ_K = "#FFD700";   ///< K值 金色
    constexpr const char* KDJ_D = "#00CED1";   ///< D值 青色
    constexpr const char* KDJ_J = "#FF6B6B";   ///< J值 珊瑚红
    constexpr const char* BOLL  = "#FFD700";   ///< BOLL 金色
}

// ============================================================================
// 尺寸常量
// ============================================================================

namespace Sizes {
    // 间距
    constexpr int SpacingXS   = 4;    ///< 超小间距
    constexpr int SpacingSM   = 8;    ///< 小间距
    constexpr int SpacingMD   = 16;   ///< 中间距
    constexpr int SpacingLG   = 24;   ///< 大间距
    constexpr int SpacingXL   = 32;   ///< 超大间距

    // 圆角
    constexpr int RadiusSM    = 4;    ///< 小圆角
    constexpr int RadiusMD    = 6;    ///< 中圆角
    constexpr int RadiusLG    = 8;    ///< 大圆角
    constexpr int RadiusXL    = 12;   ///< 超大圆角

    // 字体大小
    constexpr int FontXS      = 10;   ///< 超小字体
    constexpr int FontSM      = 12;   ///< 小字体
    constexpr int FontMD      = 14;   ///< 中字体
    constexpr int FontLG      = 16;   ///< 大字体
    constexpr int FontXL      = 20;   ///< 超大字体
    constexpr int FontXXL     = 24;   ///< 巨大字体

    // 组件高度
    constexpr int ToolBarHeight    = 48;   ///< 工具栏高度
    constexpr int StatusBarHeight  = 28;   ///< 状态栏高度
    constexpr int ButtonHeight     = 36;   ///< 按钮高度
    constexpr int InputHeight      = 36;   ///< 输入框高度
    constexpr int ListItemHeight   = 40;   ///< 列表项高度

    // K线图
    constexpr int CandleWidth      = 8;    ///< 蜡烛宽度
    constexpr int CandleSpacing    = 2;    ///< 蜡烛间距
    constexpr int ChartMarginLeft  = 60;   ///< 图表左边距
    constexpr int ChartMarginRight = 20;   ///< 图表右边距
    constexpr int ChartMarginTop   = 30;   ///< 图表上边距
    constexpr int ChartMarginBottom= 30;   ///< 图表下边距
}

// ============================================================================
// 布局比例
// ============================================================================

namespace Layout {
    constexpr double ChartRatio      = 0.70;   ///< K线图占比
    constexpr double DepthRatio      = 0.30;   ///< 盘口占比
    constexpr double VolumeRatio     = 0.20;   ///< 成交量图占比
    constexpr double IndicatorRatio  = 0.15;   ///< 指标图占比
}

// ============================================================================
// 样式字符串
// ============================================================================

namespace StyleSheets {

/**
 * @brief 图表工具栏样式
 */
inline QString chartToolBarStyle() {
    return R"(
        ChartToolBar {
            background-color: #0F1419;
            border-bottom: 1px solid #374151;
            padding: 0 16px;
        }
        ChartToolBar QComboBox {
            background-color: #1A1F2E;
            border: 1px solid #374151;
            border-radius: 6px;
            padding: 6px 12px;
            padding-right: 28px;
            color: #FFFFFF;
            font-size: 13px;
            min-width: 80px;
        }
        ChartToolBar QComboBox:hover {
            border-color: #4B5563;
            background-color: #242937;
        }
        ChartToolBar QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        ChartToolBar QComboBox QAbstractItemView {
            background-color: #0F1419;
            border: 1px solid #374151;
            border-radius: 6px;
            padding: 4px;
            selection-background-color: #3B82F6;
        }
        ChartToolBar QToolButton {
            background-color: transparent;
            color: #9CA3AF;
            border: none;
            border-radius: 6px;
            padding: 6px 12px;
            font-size: 13px;
        }
        ChartToolBar QToolButton:hover {
            background-color: #242937;
            color: #FFFFFF;
        }
        ChartToolBar QToolButton:pressed,
        ChartToolBar QToolButton:checked {
            background-color: #3B82F6;
            color: #FFFFFF;
        }
        ChartToolBar QFrame[frameShape="4"] {
            background-color: #374151;
            max-width: 1px;
        }
    )";
}

/**
 * @brief 盘口组件样式
 */
inline QString marketDepthStyle() {
    return R"(
        MarketDepthWidget {
            background-color: #0F1419;
            border: none;
        }
        MarketDepthWidget QLabel {
            color: #E4E6EB;
        }
        MarketDepthWidget QLabel#priceLabel {
            font-size: 24px;
            font-weight: bold;
        }
        MarketDepthWidget QLabel#instrumentLabel {
            font-size: 14px;
            font-weight: bold;
            color: #FFFFFF;
        }
        MarketDepthWidget QFrame[frameShape="4"] {
            background-color: #374151;
        }
    )";
}

/**
 * @brief 分笔成交表样式
 */
inline QString tickTableStyle() {
    return R"(
        TickTableView {
            background-color: #0F1419;
            border: none;
            gridline-color: transparent;
        }
        TickTableView::item {
            padding: 4px 8px;
            border-bottom: 1px solid #1A1F2E;
        }
        TickTableView::item:selected {
            background-color: #3B82F6;
        }
        TickTableView QHeaderView::section {
            background-color: #1A1F2E;
            color: #9CA3AF;
            padding: 6px 8px;
            border: none;
            border-bottom: 1px solid #374151;
            font-size: 12px;
            font-weight: 500;
        }
    )";
}

/**
 * @brief 状态栏样式
 */
inline QString chartStatusBarStyle() {
    return R"(
        ChartStatusBar {
            background-color: #0F1419;
            border-top: 1px solid #374151;
            padding: 0 16px;
        }
        ChartStatusBar QLabel {
            color: #9CA3AF;
            font-size: 12px;
        }
        ChartStatusBar QFrame[frameShape="4"] {
            background-color: #374151;
            max-width: 1px;
        }
    )";
}

/**
 * @brief K线图样式
 */
inline QString klineChartStyle() {
    return R"(
        KLineChart {
            background-color: #0F1419;
            border: none;
        }
    )";
}

/**
 * @brief 分割器样式
 */
inline QString splitterStyle() {
    return R"(
        QSplitter::handle {
            background-color: #374151;
        }
        QSplitter::handle:hover {
            background-color: #3B82F6;
        }
        QSplitter::handle:horizontal {
            width: 2px;
        }
        QSplitter::handle:vertical {
            height: 2px;
        }
    )";
}

} // namespace StyleSheets

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 获取涨跌颜色
 * @param change 涨跌值
 * @return 颜色
 */
inline QColor getChangeColor(double change) {
    if (change > 0) return QColor(Colors::Success);
    if (change < 0) return QColor(Colors::Danger);
    return QColor(Colors::TextSecondary);
}

/**
 * @brief 获取涨跌颜色（相对基准价）
 * @param current 当前价
 * @param base 基准价
 * @return 颜色
 */
inline QColor getChangeColor(double current, double base) {
    if (base <= 0) return QColor(Colors::TextSecondary);
    return getChangeColor(current - base);
}

/**
 * @brief 格式化价格
 * @param price 价格
 * @param precision 精度
 * @return 格式化字符串
 */
inline QString formatPrice(double price, int precision = 2) {
    if (price <= 0) return "--";
    return QString::number(price, 'f', precision);
}

/**
 * @brief 格式化成交量
 * @param volume 成交量
 * @return 格式化字符串
 */
inline QString formatVolume(qint64 volume) {
    if (volume <= 0) return "--";
    if (volume >= 100000000) {
        return QString("%1亿").arg(volume / 100000000.0, 0, 'f', 2);
    }
    if (volume >= 10000) {
        return QString("%1万").arg(volume / 10000.0, 0, 'f', 2);
    }
    return QString::number(volume);
}

/**
 * @brief 格式化涨跌幅
 * @param percent 百分比
 * @param showSign 是否显示符号
 * @return 格式化字符串
 */
inline QString formatChangePercent(double percent, bool showSign = true) {
    QString result = QString::number(qAbs(percent), 'f', 2) + "%";
    if (showSign) {
        if (percent > 0) return "+" + result;
        if (percent < 0) return "-" + result;
    }
    return result;
}

} // namespace ChartStyles

#endif // CHARTSTYLES_H
