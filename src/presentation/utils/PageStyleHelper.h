/**
 * @file PageStyleHelper.h
 * @brief 页面样式辅助 - 统一页面样式
 *
 * @details 提供页面统一样式：
 * - 页面基础样式
 * - 标题样式
 * - 卡片样式
 * - 表格样式
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PAGESTYLEHELPER_H
#define PAGESTYLEHELPER_H

#include <QString>
#include "infrastructure/config/Tokens.h"

/**
 * @brief 页面样式辅助类
 */
class PageStyleHelper
{
public:
    // ==================== 页面基础样式 ====================

    /**
     * @brief 获取页面主样式
     */
    static QString pageStyle();

    /**
     * @brief 获取页面标题样式
     */
    static QString pageTitleStyle();

    /**
     * @brief 获取页面副标题样式
     */
    static QString pageSubtitleStyle();

    // ==================== 卡片样式 ====================

    /**
     * @brief 获取卡片容器样式
     */
    static QString cardContainerStyle();

    /**
     * @brief 获取卡片标题样式
     */
    static QString cardTitleStyle();

    /**
     * @brief 获取卡片内容样式
     */
    static QString cardContentStyle();

    // ==================== 数据展示样式 ====================

    /**
     * @brief 获取数据标签样式
     */
    static QString dataLabelStyle();

    /**
     * @brief 获取数据值样式（大号）
     */
    static QString dataValueLargeStyle();

    /**
     * @brief 获取数据值样式（普通）
     */
    static QString dataValueStyle();

    /**
     * @brief 获取数据变化样式
     */
    static QString dataChangeStyle(double change, bool useChineseStyle = true);

    // ==================== 表格样式 ====================

    /**
     * @brief 获取表格样式
     */
    static QString tableStyle();

    /**
     * @brief 获取表格标题样式
     */
    static QString tableHeaderStyle();

    // ==================== 按钮样式 ====================

    /**
     * @brief 获取主要按钮样式
     */
    static QString primaryButtonStyle();

    /**
     * @brief 获取次要按钮样式
     */
    static QString secondaryButtonStyle();

    /**
     * @brief 获取危险按钮样式
     */
    static QString dangerButtonStyle();

    // ==================== 输入框样式 ====================

    /**
     * @brief 获取输入框样式
     */
    static QString inputStyle();

    /**
     * @brief 获取下拉框样式
     */
    static QString comboBoxStyle();

    // ==================== 分组样式 ====================

    /**
     * @brief 获取分组框样式
     */
    static QString groupBoxStyle();

    /**
     * @brief 获取分割线样式
     */
    static QString dividerStyle();

    // ==================== 状态样式 ====================

    /**
     * @brief 获取成功状态样式
     */
    static QString successStyle();

    /**
     * @brief 获取警告状态样式
     */
    static QString warningStyle();

    /**
     * @brief 获取错误状态样式
     */
    static QString errorStyle();

    /**
     * @brief 获取信息状态样式
     */
    static QString infoStyle();

private:
    PageStyleHelper() = delete;
};

#endif // PAGESTYLEHELPER_H