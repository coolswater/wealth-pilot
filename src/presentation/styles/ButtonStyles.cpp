/**
 * @file ButtonStyles.cpp
 * @brief 按钮样式统一管理 - 实现
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ButtonStyles.h"
#include <QStyle>

namespace
{
    /**
 * @brief 获取按钮尺寸高度
 */
    int getButtonHeight(ButtonSize size)
    {
        switch (size)
        {
        case ButtonSize::Small: return ButtonStyles::SmallHeight;
        case ButtonSize::Medium: return ButtonStyles::MediumHeight;
        case ButtonSize::Large: return ButtonStyles::LargeHeight;
        case ButtonSize::ExtraLarge: return ButtonStyles::ExtraLargeHeight;
        default: return ButtonStyles::MediumHeight;
        }
    }

    /**
 * @brief 获取按钮内边距
 */
    int getButtonPadding(ButtonSize size)
    {
        switch (size)
        {
        case ButtonSize::Small: return ButtonStyles::SmallPadding;
        case ButtonSize::Medium: return ButtonStyles::MediumPadding;
        case ButtonSize::Large: return ButtonStyles::LargePadding;
        case ButtonSize::ExtraLarge: return ButtonStyles::ExtraLargePadding;
        default: return ButtonStyles::MediumPadding;
        }
    }

    /**
 * @brief 获取按钮字体大小
 */
    // 按钮字体大小辅助函数（内部使用）
    [[maybe_unused]] int getButtonFontSize(ButtonSize size)
    {
        switch (size)
        {
        case ButtonSize::Small: return ButtonStyles::SmallFontSize;
        case ButtonSize::Medium: return ButtonStyles::MediumFontSize;
        case ButtonSize::Large: return ButtonStyles::LargeFontSize;
        case ButtonSize::ExtraLarge: return ButtonStyles::ExtraLargeFontSize;
        default: return ButtonStyles::MediumFontSize;
        }
    }

    /**
 * @brief 刷新控件样式
 */
    void refreshStyle(QWidget* widget)
    {
        if (!widget) return;
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
    }
} // anonymous namespace

// ============================================================================
// 样式应用实现
// ============================================================================

void ButtonStyles::applyStyle(QPushButton* button, ButtonType type, ButtonSize size)
{
    if (!button) return;

    // 设置按钮类型属性（用于 QSS 选择器）
    button->setProperty("buttonType", getButtonTypeProperty(type));

    // 设置按钮尺寸属性
    button->setProperty("buttonSize", getButtonSizeProperty(size));

    // 设置固定高度
    button->setFixedHeight(getButtonHeight(size));

    // 设置最小宽度（防止文字截断）
    setMinWidth(button);

    // 刷新样式
    refreshStyle(button);
}

void ButtonStyles::applyStyle(QToolButton* button, ButtonType type)
{
    if (!button) return;

    // 设置按钮类型属性
    button->setProperty("buttonType", getButtonTypeProperty(type));

    // 工具按钮默认使用图标样式
    if (type == ButtonType::Icon)
    {
        button->setAutoRaise(true);
        button->setFixedSize(32, 32);
    }

    // 刷新样式
    refreshStyle(button);
}

void ButtonStyles::setSize(QPushButton* button, ButtonSize size)
{
    if (!button) return;

    button->setProperty("buttonSize", getButtonSizeProperty(size));
    button->setFixedHeight(getButtonHeight(size));

    refreshStyle(button);
}

void ButtonStyles::setMinWidth(QPushButton* button, int minChars)
{
    if (!button) return;

    // 计算最小宽度：字符数 × 字体平均宽度 + 内边距
    QFontMetrics fm(button->font());
    int textWidth = fm.horizontalAdvance(QString(minChars, 'W'));
    int padding = getButtonPadding(ButtonSize::Medium);
    int minWidth = textWidth + padding * 2;

    button->setMinimumWidth(qMax(minWidth, MinButtonWidth));
}

// ============================================================================
// 属性值生成
// ============================================================================

QString ButtonStyles::getButtonTypeProperty(ButtonType type)
{
    switch (type)
    {
    case ButtonType::Primary: return QStringLiteral("primary");
    case ButtonType::Secondary: return QStringLiteral("secondary");
    case ButtonType::Success: return QStringLiteral("success");
    case ButtonType::Danger: return QStringLiteral("danger");
    case ButtonType::Warning: return QStringLiteral("warning");
    case ButtonType::Info: return QStringLiteral("info");
    case ButtonType::Refresh: return QStringLiteral("refresh");
    case ButtonType::Add: return QStringLiteral("add");
    case ButtonType::Edit: return QStringLiteral("edit");
    case ButtonType::Delete: return QStringLiteral("delete");
    case ButtonType::Export: return QStringLiteral("export");
    case ButtonType::Import: return QStringLiteral("import");
    case ButtonType::Search: return QStringLiteral("search");
    case ButtonType::Navigation: return QStringLiteral("navigation");
    case ButtonType::Back: return QStringLiteral("back");
    case ButtonType::Next: return QStringLiteral("next");
    case ButtonType::Icon: return QStringLiteral("icon");
    case ButtonType::Text: return QStringLiteral("text");
    case ButtonType::Link: return QStringLiteral("link");
    case ButtonType::DialogAccept: return QStringLiteral("accept");
    case ButtonType::DialogReject: return QStringLiteral("reject");
    case ButtonType::DialogApply: return QStringLiteral("apply");
    case ButtonType::DialogHelp: return QStringLiteral("help");
    case ButtonType::Toggle: return QStringLiteral("toggle");
    case ButtonType::Dropdown: return QStringLiteral("dropdown");
    case ButtonType::Split: return QStringLiteral("split");
    default: return QStringLiteral("default");
    }
}

QString ButtonStyles::getButtonSizeProperty(ButtonSize size)
{
    switch (size)
    {
    case ButtonSize::Small: return QStringLiteral("small");
    case ButtonSize::Medium: return QStringLiteral("medium");
    case ButtonSize::Large: return QStringLiteral("large");
    case ButtonSize::ExtraLarge: return QStringLiteral("xlarge");
    default: return QStringLiteral("medium");
    }
}
