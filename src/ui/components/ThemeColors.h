/**
 * @file ThemeColors.h
 * @brief 主题颜色配置 - 统一管理应用颜色方案
 *
 * @details 功能：
 * - 定义统一的颜色常量
 * - 支持深色/浅色主题切换
 * - 提供颜色工具方法
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef THEMECOLORS_H
#define THEMECOLORS_H

#include <QColor>
#include <QString>

/**
 * @brief 主题颜色配置类
 *
 * @details 提供统一的颜色定义，确保整个应用颜色一致性。
 * 支持深色主题（默认）和浅色主题。
 *
 * @example
 * @code
 * QColor upColor = ThemeColors::upColor();      // 上涨颜色
 * QColor downColor = ThemeColors::downColor();  // 下跌颜色
 * QString style = ThemeColors::cardStyle();     // 卡片样式
 * @endcode
 */
class ThemeColors
{
public:
    // ========== 价格颜色 ==========

    /**
     * @brief 上涨颜色（红色）
     */
    static QColor upColor() { return QColor("#EF4444"); }

    /**
     * @brief 下跌颜色（绿色）
     */
    static QColor downColor() { return QColor("#10B981"); }

    /**
     * @brief 平盘颜色（灰色）
     */
    static QColor flatColor() { return QColor("#9CA3AF"); }

    // ========== 背景颜色 ==========

    /**
     * @brief 主背景色（深灰）
     */
    static QColor backgroundPrimary() { return QColor("#111827"); }

    /**
     * @brief 次背景色（中灰）
     */
    static QColor backgroundSecondary() { return QColor("#1F2937"); }

    /**
     * @brief 卡片背景色
     */
    static QColor backgroundCard() { return QColor("#374151"); }

    /**
     * @brief 悬停背景色
     */
    static QColor backgroundHover() { return QColor("#4B5563"); }

    // ========== 文字颜色 ==========

    /**
     * @brief 主文字颜色（白色）
     */
    static QColor textPrimary() { return QColor("#F3F4F6"); }

    /**
     * @brief 次文字颜色（浅灰）
     */
    static QColor textSecondary() { return QColor("#9CA3AF"); }

    /**
     * @brief 禁用文字颜色
     */
    static QColor textDisabled() { return QColor("#6B7280"); }

    // ========== 边框颜色 ==========

    /**
     * @brief 边框颜色
     */
    static QColor border() { return QColor("#374151"); }

    /**
     * @brief 分隔线颜色
     */
    static QColor divider() { return QColor("#4B5563"); }

    // ========== 状态颜色 ==========

    /**
     * @brief 成功颜色
     */
    static QColor success() { return QColor("#10B981"); }

    /**
     * @brief 警告颜色
     */
    static QColor warning() { return QColor("#F59E0B"); }

    /**
     * @brief 错误颜色
     */
    static QColor error() { return QColor("#EF4444"); }

    /**
     * @brief 信息颜色
     */
    static QColor info() { return QColor("#3B82F6"); }

    // ========== 品牌颜色 ==========

    /**
     * @brief 主品牌色（蓝色）
     */
    static QColor primary() { return QColor("#3B82F6"); }

    /**
     * @brief 次品牌色
     */
    static QColor secondary() { return QColor("#8B5CF6"); }

    // ========== 指标颜色 ==========

    /**
     * @brief MA5 颜色（金色）
     */
    static QColor ma5() { return QColor("#FFD700"); }

    /**
     * @brief MA10 颜色（青色）
     */
    static QColor ma10() { return QColor("#00CED1"); }

    /**
     * @brief MA20 颜色（珊瑚红）
     */
    static QColor ma20() { return QColor("#FF6B6B"); }

    /**
     * @brief MA30 颜色（紫色）
     */
    static QColor ma30() { return QColor("#9B59B6"); }

    /**
     * @brief MA60 颜色（蓝色）
     */
    static QColor ma60() { return QColor("#3498DB"); }

    // ========== 样式字符串 ==========

    /**
     * @brief 卡片样式
     */
    static QString cardStyle() {
        return R"(
            background-color: #1F2937;
            border: 1px solid #374151;
            border-radius: 8px;
        )";
    }

    /**
     * @brief 按钮样式
     */
    static QString buttonStyle() {
        return R"(
            QPushButton {
                background-color: #374151;
                color: #F3F4F6;
                border: none;
                border-radius: 4px;
                padding: 8px 16px;
            }
            QPushButton:hover {
                background-color: #4B5563;
            }
            QPushButton:pressed {
                background-color: #6B7280;
            }
        )";
    }

    /**
     * @brief 输入框样式
     */
    static QString lineEditStyle() {
        return R"(
            QLineEdit {
                background-color: #1F2937;
                color: #F3F4F6;
                border: 1px solid #374151;
                border-radius: 4px;
                padding: 8px;
            }
            QLineEdit:focus {
                border-color: #3B82F6;
            }
        )";
    }

    /**
     * @brief 表格样式
     */
    static QString tableStyle() {
        return R"(
            QTableWidget {
                background-color: #111827;
                color: #F3F4F6;
                gridline-color: #374151;
                border: none;
            }
            QTableWidget::item {
                padding: 4px;
            }
            QTableWidget::item:selected {
                background-color: #3B82F6;
            }
            QHeaderView::section {
                background-color: #1F2937;
                color: #9CA3AF;
                padding: 8px;
                border: none;
                border-bottom: 1px solid #374151;
            }
        )";
    }

    /**
     * @brief 下拉框样式
     */
    static QString comboBoxStyle() {
        return R"(
            QComboBox {
                background-color: #1F2937;
                color: #F3F4F6;
                border: 1px solid #374151;
                border-radius: 4px;
                padding: 6px 12px;
            }
            QComboBox:hover {
                border-color: #4B5563;
            }
            QComboBox::drop-down {
                border: none;
                width: 20px;
            }
            QComboBox QAbstractItemView {
                background-color: #1F2937;
                color: #F3F4F6;
                selection-background-color: #3B82F6;
            }
        )";
    }

    // ========== 工具方法 ==========

    /**
     * @brief 根据涨跌获取颜色
     * @param change 涨跌值
     * @return 对应颜色
     */
    static QColor getChangeColor(double change) {
        if (change > 0) return upColor();
        if (change < 0) return downColor();
        return flatColor();
    }

    /**
     * @brief 根据涨跌获取颜色
     * @param current 当前价
     * @param base 基准价
     * @return 对应颜色
     */
    static QColor getChangeColor(double current, double base) {
        if (base <= 0) return flatColor();
        return getChangeColor(current - base);
    }
};

#endif // THEMECOLORS_H
