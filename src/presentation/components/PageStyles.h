/**
 * @file PageStyles.h
 * @brief 页面样式工具类 - 基于 Tokens 提供统一样式
 * 
 * @details 所有样式都使用 Tokens::Colors 命名空间中的颜色常量
 * 避免硬编码颜色值，确保样式一致性
 */

#ifndef PAGESTYLES_H
#define PAGESTYLES_H

#include <QString>
#include "infrastructure/config/Tokens.h"

/**
 * @brief 页面样式工具类
 */
class PageStyles
{
public:
    // ========== 基础样式（使用 Tokens::Colors 命名空间）==========
    
    static QString pageBackground() {
        return QString("background-color: %1;").arg(Tokens::Colors::BgBase);
    }
    
    static QString cardContainer() {
        return QString(R"(
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
            padding: 12px;
        )").arg(Tokens::Colors::BgElevated, Tokens::Colors::Border);
    }
    
    // ========== 文字样式 ==========
    
    static QString titleText() {
        return QString("font-size: 20px; font-weight: bold; color: %1;")
            .arg(Tokens::Colors::TextPrimary);
    }
    
    static QString subtitleText() {
        return QString("font-size: 14px; color: %1;").arg(Tokens::Colors::TextSecondary);
    }
    
    static QString valueText(const QString &color = QString()) {
        QString c = color.isEmpty() ? Tokens::Colors::TextPrimary : color;
        return QString("font-size: 18px; font-weight: bold; color: %1;").arg(c);
    }
    
    static QString labelText() {
        return QString("font-size: 12px; color: %1;").arg(Tokens::Colors::TextSecondary);
    }
    
    // ========== 控件样式 ==========
    
    static QString primaryButton() {
        return QString(R"(
            QPushButton {
                background-color: %1;
                color: white;
                border: none;
                border-radius: 6px;
                padding: 8px 20px;
                font-weight: bold;
            }
            QPushButton:hover { background-color: %2; }
            QPushButton:pressed { background-color: %3; }
        )").arg(Tokens::Colors::Primary, Tokens::Colors::PrimaryHover, Tokens::Colors::PrimaryDark);
    }
    
    static QString inputField() {
        return QString(R"(
            QLineEdit, QSpinBox, QDoubleSpinBox {
                background-color: %1;
                color: %2;
                border: 1px solid %3;
                border-radius: 4px;
                padding: 6px 10px;
            }
            QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus {
                border-color: %4;
            }
        )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, 
                Tokens::Colors::Border, Tokens::Colors::Primary);
    }
    
    static QString comboBox() {
        return QString(R"(
            QComboBox {
                background-color: %1;
                color: %2;
                border: 1px solid %3;
                border-radius: 4px;
                padding: 6px 12px;
            }
            QComboBox:hover { border-color: %4; }
            QComboBox::drop-down { border: none; width: 20px; }
            QComboBox QAbstractItemView {
                background-color: %1;
                color: %2;
                selection-background-color: %4;
            }
        )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, 
                Tokens::Colors::Border, Tokens::Colors::Primary);
    }
    
    static QString table() {
        return QString(R"(
            QTableWidget {
                background-color: %1;
                color: %2;
                gridline-color: %3;
                border: 1px solid %3;
                border-radius: 6px;
            }
            QTableWidget::item { padding: 6px; }
            QTableWidget::item:selected { background-color: %4; }
            QHeaderView::section {
                background-color: %5;
                color: %6;
                padding: 8px;
                border: none;
                border-bottom: 1px solid %3;
                font-weight: bold;
            }
        )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, 
                Tokens::Colors::Border, Tokens::Colors::Primary,
                Tokens::Colors::BgSurface, Tokens::Colors::TextSecondary);
    }
    
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
        )").arg(Tokens::Colors::TextPrimary, Tokens::Colors::Border, Tokens::Colors::BgBase);
    }
    
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
        )").arg(Tokens::Colors::BgElevated, Tokens::Colors::TextPrimary, Tokens::Colors::Border,
                Tokens::Colors::BgHover, Tokens::Colors::Primary,
                Tokens::Colors::TextDisabled, Tokens::Colors::BgBase);
    }
    
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
                border: 2px solid %2;
                background-color: %3;
            }
            QCheckBox::indicator:hover {
                border-color: %4;
            }
            QCheckBox::indicator:checked {
                background-color: %4;
                border-color: %4;
            }
        )").arg(Tokens::Colors::TextPrimary, Tokens::Colors::Border, Tokens::Colors::BgElevated, Tokens::Colors::Primary);
    }
    
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
        )").arg(Tokens::Colors::Border, Tokens::Colors::BgBase,
                Tokens::Colors::BgElevated, Tokens::Colors::TextSecondary,
                Tokens::Colors::BgHover, Tokens::Colors::TextPrimary, Tokens::Colors::Primary);
    }
    
    static QString statCard(const QString &accentColor = QString()) {
        QString style = QString(R"(
            background-color: %1;
            border-radius: 8px;
            padding: 12px 16px;
        )").arg(Tokens::Colors::BgElevated);
        
        if (!accentColor.isEmpty()) {
            style = QString(R"(
                background-color: %1;
                border-left: 4px solid %2;
                border-radius: 8px;
                padding: 12px 16px;
            )").arg(Tokens::Colors::BgElevated, accentColor);
        }
        return style;
    }
    
    static QString dateEdit() {
        return comboBox();  // 样式相同
    }
    
    // ========== 颜色便捷访问 ==========
    
    static QString upColor() { return Tokens::Colors::Danger; }
    static QString downColor() { return Tokens::Colors::Success; }
    static QString flatColor() { return Tokens::Colors::TextSecondary; }
    static QString primaryColor() { return Tokens::Colors::Primary; }
    static QString warningColor() { return Tokens::Colors::Warning; }
    static QString errorColor() { return Tokens::Colors::Danger; }
    static QString successColor() { return Tokens::Colors::Success; }
    
    // ========== 主题样式 ==========
    
    // 浅色主题样式
    static QString lightPageBackground() {
        return QString("background-color: %1;").arg(Tokens::LightColors::BgBase);
    }
    
    static QString lightCardContainer() {
        return QString("background-color: %1; border: 1px solid %2; border-radius: 8px; padding: 12px;")
            .arg(Tokens::LightColors::BgCard, Tokens::LightColors::Border);
    }
    
    static QString lightTitleText() {
        return QString("font-size: 20px; font-weight: bold; color: %1;")
            .arg(Tokens::LightColors::TextPrimary);
    }
    
    // 护眼主题样式
    static QString eyeCarePageBackground() {
        return QString("background-color: %1;").arg(Tokens::EyeCareColors::BgBase);
    }
    
    static QString eyeCareCardContainer() {
        return QString("background-color: %1; border: 1px solid %2; border-radius: 8px; padding: 12px;")
            .arg(Tokens::EyeCareColors::BgCard, Tokens::EyeCareColors::Border);
    }
    
    static QString eyeCareTitleText() {
        return QString("font-size: 20px; font-weight: bold; color: %1;")
            .arg(Tokens::EyeCareColors::TextPrimary);
    }
};

#endif // PAGESTYLES_H
