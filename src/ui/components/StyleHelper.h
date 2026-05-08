/**
 * @file StyleHelper.h
 * @brief 样式辅助工具类 - 简化样式迁移
 *
 * @details 提供便捷方法设置控件样式属性，
 *          配合全局 QSS 样式系统使用
 */

#ifndef STYLEHELPER_H
#define STYLEHELPER_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QString>

/**
 * @brief 样式辅助工具类
 *
 * 使用方法：
 * @code
 * // 设置按钮为主按钮样式
 * StyleHelper::setPrimaryButton(saveBtn);
 *
 * // 设置标签为上涨样式
 * StyleHelper::setTrendUp(priceLabel);
 *
 * // 刷新控件样式
 * StyleHelper::refreshStyle(widget);
 * @endcode
 */
class StyleHelper
{
public:
    // ========================================================================
    // 按钮样式
    // ========================================================================

    /**
     * @brief 设置为主按钮样式
     */
    static void setPrimaryButton(QPushButton* button)
    {
        if (!button) return;
        button->setProperty("primary", true);
        refreshStyle(button);
    }

    /**
     * @brief 设置为次要按钮样式
     */
    static void setSecondaryButton(QPushButton* button)
    {
        if (!button) return;
        button->setProperty("secondary", true);
        refreshStyle(button);
    }

    /**
     * @brief 设置为危险按钮样式
     */
    static void setDangerButton(QPushButton* button)
    {
        if (!button) return;
        button->setProperty("danger", true);
        refreshStyle(button);
    }

    /**
     * @brief 设置为成功按钮样式
     */
    static void setSuccessButton(QPushButton* button)
    {
        if (!button) return;
        button->setProperty("success", true);
        refreshStyle(button);
    }

    /**
     * @brief 设置为图标按钮样式
     */
    static void setIconButton(QPushButton* button)
    {
        if (!button) return;
        button->setProperty("icon", true);
        refreshStyle(button);
    }

    // ========================================================================
    // 涨跌状态
    // ========================================================================

    /**
     * @brief 设置为上涨样式（红色）
     */
    static void setTrendUp(QLabel* label)
    {
        if (!label) return;
        label->setProperty("trend", "up");
        refreshStyle(label);
    }

    /**
     * @brief 设置为下跌样式（绿色）
     */
    static void setTrendDown(QLabel* label)
    {
        if (!label) return;
        label->setProperty("trend", "down");
        refreshStyle(label);
    }

    /**
     * @brief 设置为平盘样式（灰色）
     */
    static void setTrendFlat(QLabel* label)
    {
        if (!label) return;
        label->setProperty("trend", "flat");
        refreshStyle(label);
    }

    /**
     * @brief 根据涨跌值设置样式
     * @param label 标签控件
     * @param change 涨跌值（正数上涨，负数下跌，零平盘）
     */
    static void setTrendByValue(QLabel* label, double change)
    {
        if (!label) return;
        
        if (change > 0.0001) {
            setTrendUp(label);
        } else if (change < -0.0001) {
            setTrendDown(label);
        } else {
            setTrendFlat(label);
        }
    }

    // ========================================================================
    // 卡片主题
    // ========================================================================

    /**
     * @brief 设置卡片主题
     * @param widget 卡片控件
     * @param theme 主题类型：success, warning, danger, primary
     */
    static void setCardTheme(QWidget* widget, const QString& theme)
    {
        if (!widget) return;
        widget->setProperty("theme", theme);
        refreshStyle(widget);
    }

    /**
     * @brief 设置为成功主题卡片
     */
    static void setSuccessCard(QWidget* widget)
    {
        setCardTheme(widget, QStringLiteral("success"));
    }

    /**
     * @brief 设置为警告主题卡片
     */
    static void setWarningCard(QWidget* widget)
    {
        setCardTheme(widget, QStringLiteral("warning"));
    }

    /**
     * @brief 设置为危险主题卡片
     */
    static void setDangerCard(QWidget* widget)
    {
        setCardTheme(widget, QStringLiteral("danger"));
    }

    /**
     * @brief 设置为主色调主题卡片
     */
    static void setPrimaryCard(QWidget* widget)
    {
        setCardTheme(widget, QStringLiteral("primary"));
    }

    // ========================================================================
    // 样式刷新
    // ========================================================================

    /**
     * @brief 刷新控件样式
     *
     * @details 调用此方法让 QSS 属性选择器生效
     */
    static void refreshStyle(QWidget* widget)
    {
        if (!widget) return;
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
    }

    /**
     * @brief 批量刷新控件样式
     */
    static void refreshStyles(const QList<QWidget*>& widgets)
    {
        for (QWidget* widget : widgets) {
            refreshStyle(widget);
        }
    }

    /**
     * @brief 刷新父容器及其所有子控件
     */
    static void refreshAll(QWidget* parent)
    {
        if (!parent) return;
        
        // 刷新父容器
        refreshStyle(parent);
        
        // 刷新所有子控件
        QList<QWidget*> children = parent->findChildren<QWidget*>();
        for (QWidget* child : children) {
            refreshStyle(child);
        }
    }

    // ========================================================================
    // 属性设置
    // ========================================================================

    /**
     * @brief 设置属性并刷新样式
     */
    static void setProperty(QWidget* widget, const QString& property, const QVariant& value)
    {
        if (!widget) return;
        widget->setProperty(property.toUtf8().constData(), value);
        refreshStyle(widget);
    }

    /**
     * @brief 批量设置属性
     */
    static void setProperty(const QList<QWidget*>& widgets, const QString& property, const QVariant& value)
    {
        for (QWidget* widget : widgets) {
            if (widget) {
                widget->setProperty(property.toUtf8().constData(), value);
            }
        }
        refreshStyles(widgets);
    }

    // ========================================================================
    // 对象名设置（用于 QSS 选择器）
    // ========================================================================

    /**
     * @brief 设置为标题样式（通过对象名）
     */
    static void setTitleLabel(QLabel* label)
    {
        if (!label) return;
        label->setObjectName(QStringLiteral("titleLabel"));
        refreshStyle(label);
    }

    /**
     * @brief 设置为副标题样式（通过对象名）
     */
    static void setSubtitleLabel(QLabel* label)
    {
        if (!label) return;
        label->setObjectName(QStringLiteral("subtitleLabel"));
        refreshStyle(label);
    }

    /**
     * @brief 设置为数值样式（通过对象名）
     */
    static void setValueLabel(QLabel* label)
    {
        if (!label) return;
        label->setObjectName(QStringLiteral("valueLabel"));
        refreshStyle(label);
    }

    /**
     * @brief 设置为标签文本样式（通过对象名）
     */
    static void setLabelText(QLabel* label)
    {
        if (!label) return;
        label->setObjectName(QStringLiteral("labelText"));
        refreshStyle(label);
    }

    // ========================================================================
    // 状态样式
    // ========================================================================

    /**
     * @brief 设置状态样式
     * @param widget 控件
     * @param status 状态：success, warning, danger, info
     */
    static void setStatus(QWidget* widget, const QString& status)
    {
        if (!widget) return;
        widget->setProperty("status", status);
        refreshStyle(widget);
    }

    /**
     * @brief 设置为成功状态
     */
    static void setSuccess(QWidget* widget)
    {
        setStatus(widget, QStringLiteral("success"));
    }

    /**
     * @brief 设置为警告状态
     */
    static void setWarning(QWidget* widget)
    {
        setStatus(widget, QStringLiteral("warning"));
    }

    /**
     * @brief 设置为危险状态
     */
    static void setDanger(QWidget* widget)
    {
        setStatus(widget, QStringLiteral("danger"));
    }

    /**
     * @brief 设置为信息状态
     */
    static void setInfo(QWidget* widget)
    {
        setStatus(widget, QStringLiteral("info"));
    }

private:
    // 禁止实例化
    StyleHelper() = delete;
    ~StyleHelper() = delete;
};

#endif // STYLEHELPER_H
