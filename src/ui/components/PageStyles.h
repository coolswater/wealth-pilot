/**
 * @file PageStyles.h
 * @brief 页面样式工具类 - 统一管理所有页面样式
 */

#ifndef PAGESTYLES_H
#define PAGESTYLES_H

#include <QString>
#include "ThemeColors.h"

/**
 * @brief 页面样式工具类
 * @details 提供统一的页面样式字符串，避免重复代码
 */
class PageStyles
{
public:
    // ========== 页面主样式 ==========
    
    /**
     * @brief 页面主背景样式
     */
    static QString pageBackground() {
        return QString("background-color: %1;")
            .arg(ThemeColors::backgroundPrimary().name());
    }
    
    /**
     * @brief 卡片容器样式
     */
    static QString cardContainer() {
        return QString(R"(
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
            padding: 12px;
        )").arg(ThemeColors::backgroundSecondary().name(),
                ThemeColors::border().name());
    }
    
    /**
     * @brief 统计卡片样式
     * @param accentColor 强调色（可选）
     */
    static QString statCard(const QString &accentColor = QString()) {
        QString style = QString(R"(
            background-color: %1;
            border-radius: 8px;
            padding: 12px 16px;
        )").arg(ThemeColors::backgroundSecondary().name());
        
        if (!accentColor.isEmpty()) {
            style = QString(R"(
                background-color: %1;
                border-left: 4px solid %2;
                border-radius: 8px;
                padding: 12px 16px;
            )").arg(ThemeColors::backgroundSecondary().name(), accentColor);
        }
        return style;
    }
    
    // ========== 文字样式 ==========
    
    /**
     * @brief 标题文字样式
     */
    static QString titleText() {
        return QString("font-size: 20px; font-weight: bold; color: %1;")
            .arg(ThemeColors::textPrimary().name());
    }
    
    /**
     * @brief 副标题文字样式
     */
    static QString subtitleText() {
        return QString("font-size: 14px; color: %1;")
            .arg(ThemeColors::textSecondary().name());
    }
    
    /**
     * @brief 数值文字样式
     * @param color 颜色（可选，默认主文字色）
     */
    static QString valueText(const QString &color = QString()) {
        QString c = color.isEmpty() ? ThemeColors::textPrimary().name() : color;
        return QString("font-size: 18px; font-weight: bold; color: %1;").arg(c);
    }
    
    /**
     * @brief 小标签文字样式
     */
    static QString labelText() {
        return QString("font-size: 12px; color: %1;")
            .arg(ThemeColors::textSecondary().name());
    }
    
    // ========== 控件样式 ==========
    
    /**
     * @brief 主按钮样式
     */
    static QString primaryButton() {
        QColor primaryColor = ThemeColors::primary();
        QColor hoverColor = primaryColor.lighter(120);
        QColor pressedColor = primaryColor.darker(120);
        
        return QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: none;
                border-radius: 6px;
                padding: 8px 20px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: %3;
            }
            QPushButton:pressed {
                background-color: %4;
            }
        )").arg(primaryColor.name(),
                ThemeColors::textPrimary().name(),
                hoverColor.name(),
                pressedColor.name());
    }
    
    /**
     * @brief 次要按钮样式
     */
    static QString secondaryButton() {
        return QString(R"(
            QPushButton {
                background-color: %1;
                color: %2;
                border: 1px solid %3;
                border-radius: 4px;
                padding: 6px 16px;
            }
            QPushButton:hover {
                background-color: %4;
                border-color: %5;
            }
            QPushButton:disabled {
                color: %6;
                background-color: %7;
            }
        )").arg(ThemeColors::backgroundSecondary().name(),
                ThemeColors::textPrimary().name(),
                ThemeColors::border().name(),
                ThemeColors::backgroundHover().name(),
                ThemeColors::primary().name(),
                ThemeColors::textDisabled().name(),
                ThemeColors::backgroundPrimary().name());
    }
    
    /**
     * @brief 输入框样式
     */
    static QString inputField() {
        return QString(R"(
            QLineEdit, QSpinBox, QDoubleSpinBox {
                background-color: %1;
                color: %2;
                border: 1px solid %3;
                border-radius: 4px;
                padding: 6px 10px;
                min-height: 28px;
            }
            QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus {
                border-color: %4;
            }
        )").arg(ThemeColors::backgroundSecondary().name(),
                ThemeColors::textPrimary().name(),
                ThemeColors::border().name(),
                ThemeColors::primary().name());
    }
    
    /**
     * @brief 下拉框样式
     */
    static QString comboBox() {
        return QString(R"(
            QComboBox {
                background-color: %1;
                color: %2;
                border: 1px solid %3;
                border-radius: 4px;
                padding: 6px 12px;
                min-width: 80px;
            }
            QComboBox:hover {
                border-color: %4;
            }
            QComboBox::drop-down {
                border: none;
                width: 20px;
            }
            QComboBox QAbstractItemView {
                background-color: %1;
                color: %2;
                selection-background-color: %4;
            }
        )").arg(ThemeColors::backgroundSecondary().name(),
                ThemeColors::textPrimary().name(),
                ThemeColors::border().name(),
                ThemeColors::primary().name());
    }
    
    /**
     * @brief 日期选择器样式
     */
    static QString dateEdit() {
        return comboBox();  // 样式相同
    }
    
    /**
     * @brief 复选框样式
     */
    static QString checkBox() {
        return QString(R"(
            QCheckBox {
                color: %1;
                spacing: 8px;
            }
            QCheckBox::indicator {
                width: 18px;
                height: 18px;
                border-radius: 4px;
                border: 1px solid %2;
                background-color: %3;
            }
            QCheckBox::indicator:checked {
                background-color: %4;
                border-color: %4;
            }
        )").arg(ThemeColors::textPrimary().name(),
                ThemeColors::border().name(),
                ThemeColors::backgroundSecondary().name(),
                ThemeColors::primary().name());
    }
    
    /**
     * @brief 分组框样式
     */
    static QString groupBox() {
        return QString(R"(
            QGroupBox {
                color: %1;
                font-weight: bold;
                border: 1px solid %2;
                border-radius: 6px;
                margin-top: 12px;
                padding-top: 8px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                padding: 0 8px;
                background-color: %3;
            }
        )").arg(ThemeColors::textPrimary().name(),
                ThemeColors::border().name(),
                ThemeColors::backgroundPrimary().name());
    }
    
    /**
     * @brief 表格样式
     */
    static QString table() {
        return QString(R"(
            QTableWidget {
                background-color: %1;
                color: %2;
                gridline-color: %3;
                border: 1px solid %3;
                border-radius: 6px;
            }
            QTableWidget::item {
                padding: 6px;
            }
            QTableWidget::item:selected {
                background-color: %4;
            }
            QHeaderView::section {
                background-color: %5;
                color: %6;
                padding: 8px;
                border: none;
                border-bottom: 1px solid %3;
                font-weight: bold;
            }
        )").arg(ThemeColors::backgroundSecondary().name(),
                ThemeColors::textPrimary().name(),
                ThemeColors::border().name(),
                ThemeColors::primary().name(),
                ThemeColors::backgroundSecondary().name(),
                ThemeColors::textSecondary().name());
    }
    
    /**
     * @brief 标签页样式
     */
    static QString tabWidget() {
        return QString(R"(
            QTabWidget::pane {
                border: 1px solid %1;
                border-radius: 6px;
                background-color: %2;
            }
            QTabBar::tab {
                background-color: %3;
                color: %4;
                padding: 8px 20px;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
                margin-right: 2px;
            }
            QTabBar::tab:selected {
                background-color: %5;
                color: %6;
            }
            QTabBar::tab:hover {
                color: %7;
            }
        )").arg(ThemeColors::border().name(),
                ThemeColors::backgroundPrimary().name(),
                ThemeColors::backgroundSecondary().name(),
                ThemeColors::textSecondary().name(),
                ThemeColors::backgroundHover().name(),
                ThemeColors::textPrimary().name(),
                ThemeColors::primary().name());
    }
    
    // ========== 组合样式 ==========
    
    /**
     * @brief 完整页面样式
     */
    static QString fullPageStyle() {
        return QString(R"(
            QWidget {
                background-color: %1;
                color: %2;
            }
            QLabel {
                color: %2;
            }
            %3
            %4
            %5
            %6
            %7
            %8
        )").arg(ThemeColors::backgroundPrimary().name(),
                ThemeColors::textPrimary().name(),
                secondaryButton(),
                inputField(),
                comboBox(),
                checkBox(),
                groupBox(),
                table());
    }
    
    // ========== 颜色常量（便捷访问）==========
    
    static QString upColor() { return ThemeColors::upColor().name(); }
    static QString downColor() { return ThemeColors::downColor().name(); }
    static QString flatColor() { return ThemeColors::flatColor().name(); }
    static QString primaryColor() { return ThemeColors::primary().name(); }
    static QString warningColor() { return ThemeColors::warning().name(); }
    static QString errorColor() { return ThemeColors::error().name(); }
    static QString successColor() { return ThemeColors::success().name(); }
};

#endif // PAGESTYLES_H
