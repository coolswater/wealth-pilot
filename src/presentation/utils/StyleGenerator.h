/**
 * @file StyleGenerator.h
 * @brief 样式生成工具 - 统一样式管理
 *
 * @details 提供样式生成功能：
 * - 样式表生成
 * - 颜色转换
 * - 统一样式应用
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef STYLEGENERATOR_H
#define STYLEGENERATOR_H

#include <QString>
#include <QColor>
#include <QWidget>
#include "infrastructure/config/Tokens.h"

/**
 * @brief 样式生成工具类
 */
class StyleGenerator
{
public:
    // ==================== 基础样式表生成 ====================

    /**
     * @brief 生成按钮样式
     */
    static QString buttonStyle(
        const QString& bgColor = Tokens::Colors::Primary,
        const QString& hoverColor = Tokens::Colors::PrimaryHover,
        const QString& textColor = Tokens::Colors::TextPrimary,
        int radius = Tokens::Radius::MD,
        int height = Tokens::Size::ButtonHeightMD
    );

    /**
     * @brief 生成输入框样式
     */
    static QString inputStyle(
        const QString& bgColor = Tokens::Colors::BgElevated,
        const QString& textColor = Tokens::Colors::TextPrimary,
        const QString& borderColor = Tokens::Colors::Border,
        const QString& focusBorderColor = Tokens::Colors::BorderFocus,
        int radius = Tokens::Radius::MD,
        int height = Tokens::Size::InputHeightMD
    );

    /**
     * @brief 生成卡片样式
     */
    static QString cardStyle(
        const QString& bgColor = Tokens::Colors::BgElevated,
        const QString& borderColor = Tokens::Colors::Border,
        int radius = Tokens::Radius::LG,
        int padding = Tokens::Spacing::MD
    );

    /**
     * @brief 生成表格样式
     */
    static QString tableStyle(
        const QString& bgColor = Tokens::Colors::BgSurface,
        const QString& textColor = Tokens::Colors::TextPrimary,
        const QString& borderColor = Tokens::Colors::Border,
        const QString& selectionColor = Tokens::Colors::Primary
    );

    /**
     * @brief 生成标签样式
     */
    static QString labelStyle(
        const QString& textColor = Tokens::Colors::TextPrimary,
        int fontSize = Tokens::Font::Size::Body
    );

    /**
     * @brief 生成分割线样式
     */
    static QString dividerStyle(
        const QString& color = Tokens::Colors::Border,
        int thickness = 1
    );

    // ==================== 特殊组件样式 ====================

    /**
     * @brief 生成涨跌颜色样式
     */
    static QString trendStyle(double change, bool useChineseStyle = true);

    /**
     * @brief 生成状态徽章样式
     */
    static QString badgeStyle(
        const QString& bgColor,
        const QString& textColor = Tokens::Colors::TextPrimary,
        int radius = Tokens::Radius::SM
    );

    /**
     * @brief 生成进度条样式
     */
    static QString progressBarStyle(
        const QString& bgColor = Tokens::Colors::BgElevated,
        const QString& chunkColor = Tokens::Colors::Primary,
        int radius = Tokens::Radius::SM
    );

    /**
     * @brief 生成滚动条样式
     */
    static QString scrollBarStyle(
        const QString& bgColor = Tokens::Colors::BgSurface,
        const QString& handleColor = Tokens::Colors::Border,
        int width = 10
    );

    /**
     * @brief 生成工具提示样式
     */
    static QString toolTipStyle(
        const QString& bgColor = Tokens::Colors::BgElevated,
        const QString& textColor = Tokens::Colors::TextPrimary,
        const QString& borderColor = Tokens::Colors::Border,
        int radius = Tokens::Radius::MD
    );

    // ==================== 颜色辅助函数 ====================

    /**
     * @brief 获取涨跌颜色（中国市场）
     */
    static QString getTrendColor(double change);

    /**
     * @brief 获取涨跌颜色（国际市场）
     */
    static QString getTrendColorInternational(double change);

    /**
     * @brief 获取风险等级颜色
     */
    static QString getRiskLevelColor(int level);

    /**
     * @brief 获取情感颜色
     */
    static QString getSentimentColor(int sentiment);

    // ==================== 应用样式 ====================

    /**
     * @brief 应用统一样式到Widget
     */
    static void applyStyle(QWidget* widget, const QString& style);

    /**
     * @brief 应用卡片样式
     */
    static void applyCardStyle(QWidget* widget);

    /**
     * @brief 应用输入框样式
     */
    static void applyInputStyle(QWidget* widget);

    /**
     * @brief 应用按钮样式
     */
    static void applyButtonStyle(QWidget* widget, bool isPrimary = true);

    // ==================== 字体辅助 ====================

    /**
     * @brief 设置字体大小
     */
    static QString fontSize(int size);

    /**
     * @brief 设置字体粗细
     */
    static QString fontWeight(int weight);

    /**
     * @brief 设置字体颜色
     */
    static QString fontColor(const QString& color);

    // ==================== 间距辅助 ====================

    /**
     * @brief 设置内边距
     */
    static QString padding(int value);

    /**
     * @brief 设置外边距
     */
    static QString margin(int value);

    /**
     * @brief 设置圆角
     */
    static QString borderRadius(int radius);

private:
    StyleGenerator() = delete; // 禁止实例化
};

#endif // STYLEGENERATOR_H