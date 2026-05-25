/**
 * @file ButtonStyles.h
 * @brief 按钮样式统一管理 - 设计规范与分类
 * 
 * @details 
 * 本文件定义 WealthPilot 项目中所有按钮的统一样式规范。
 * 解决问题：
 * 1. 按钮样式不统一
 * 2. 文字显示不全
 * 3. 尺寸不规范
 * 4. 语义不清晰
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef BUTTONSTYLES_H
#define BUTTONSTYLES_H

#include <QPushButton>
#include <QToolButton>
#include <QString>
#include <QIcon>
#include "infrastructure/config/Tokens.h"

/**
 * @brief 按钮类型枚举
 * @details 定义项目中所有按钮的语义类型
 */
enum class ButtonType
{
    // ========== 主要操作按钮 ==========
    Primary, ///< 主要操作（保存、提交、确认等）
    Secondary, ///< 次要操作（取消、关闭等）

    // ========== 状态按钮 ==========
    Success, ///< 成功/确认操作（买入、启用等）
    Danger, ///< 危险/警告操作（删除、卖出等）
    Warning, ///< 警告操作（重置等）
    Info, ///< 信息操作（详情、查看等）

    // ========== 功能按钮 ==========
    Refresh, ///< 刷新操作
    Add, ///< 添加操作
    Edit, ///< 编辑操作
    Delete, ///< 删除操作
    Export, ///< 导出操作
    Import, ///< 导入操作
    Search, ///< 搜索操作

    // ========== 导航按钮 ==========
    Navigation, ///< 导航按钮
    Back, ///< 返回按钮
    Next, ///< 下一步按钮

    // ========== 工具按钮 ==========
    Icon, ///< 纯图标按钮
    Text, ///< 纯文本按钮
    Link, ///< 链接样式按钮

    // ========== 对话框按钮 ==========
    DialogAccept, ///< 对话框确认
    DialogReject, ///< 对话框取消
    DialogApply, ///< 对话框应用
    DialogHelp, ///< 对话框帮助

    // ========== 特殊按钮 ==========
    Toggle, ///< 切换按钮
    Dropdown, ///< 下拉按钮
    Split, ///< 分割按钮
};

/**
 * @brief 按钮尺寸枚举
 */
enum class ButtonSize
{
    Small, ///< 小型按钮 (24px 高度)
    Medium, ///< 中型按钮 (32px 高度) - 默认
    Large, ///< 大型按钮 (40px 高度)
    ExtraLarge, ///< 超大按钮 (48px 高度)
};

/**
 * @brief 按钮样式管理器
 *
 * @details
 * 提供统一的按钮样式设置接口，确保整个应用的按钮样式一致。
 *
 * 使用示例：
 * @code
 * // 创建主要按钮
 * auto* saveBtn = new QPushButton("保存");
 * ButtonStyles::applyStyle(saveBtn, ButtonType::Primary);
 *
 * // 创建危险按钮
 * auto* deleteBtn = new QPushButton("删除");
 * ButtonStyles::applyStyle(deleteBtn, ButtonType::Danger);
 *
 * // 设置按钮尺寸
 * ButtonStyles::setSize(saveBtn, ButtonSize::Large);
 * @endcode
 */
class ButtonStyles
{
public:
    // ========================================================================
    // 样式应用
    // ========================================================================

    /**
     * @brief 应用按钮样式
     * @param button 按钮控件
     * @param type 按钮类型
     * @param size 按钮尺寸（默认中等）
     */
    static void applyStyle(QPushButton* button, ButtonType type, ButtonSize size = ButtonSize::Medium);

    /**
     * @brief 应用工具按钮样式
     * @param button 工具按钮控件
     * @param type 按钮类型
     */
    static void applyStyle(QToolButton* button, ButtonType type);

    /**
     * @brief 设置按钮尺寸
     * @param button 按钮控件
     * @param size 按钮尺寸
     */
    static void setSize(QPushButton* button, ButtonSize size);

    /**
     * @brief 设置按钮最小宽度（防止文字截断）
     * @param button 按钮控件
     * @param minChars 最小字符数
     */
    static void setMinWidth(QPushButton* button, int minChars = 4);

    // ========================================================================
    // 快捷方法 - 常用按钮类型
    // ========================================================================

    /// 设置为主要按钮样式
    static void setPrimary(QPushButton* button)
    {
        applyStyle(button, ButtonType::Primary);
    }

    /// 设置为次要按钮样式
    static void setSecondary(QPushButton* button)
    {
        applyStyle(button, ButtonType::Secondary);
    }

    /// 设置为成功按钮样式
    static void setSuccess(QPushButton* button)
    {
        applyStyle(button, ButtonType::Success);
    }

    /// 设置为危险按钮样式
    static void setDanger(QPushButton* button)
    {
        applyStyle(button, ButtonType::Danger);
    }

    /// 设置为警告按钮样式
    static void setWarning(QPushButton* button)
    {
        applyStyle(button, ButtonType::Warning);
    }

    /// 设置为信息按钮样式
    static void setInfo(QPushButton* button)
    {
        applyStyle(button, ButtonType::Info);
    }

    /// 设置为刷新按钮样式
    static void setRefresh(QPushButton* button)
    {
        applyStyle(button, ButtonType::Refresh);
    }

    /// 设置为添加按钮样式
    static void setAdd(QPushButton* button)
    {
        applyStyle(button, ButtonType::Add);
    }

    /// 设置为编辑按钮样式
    static void setEdit(QPushButton* button)
    {
        applyStyle(button, ButtonType::Edit);
    }

    /// 设置为删除按钮样式
    static void setDelete(QPushButton* button)
    {
        applyStyle(button, ButtonType::Delete);
    }

    /// 设置为导出按钮样式
    static void setExport(QPushButton* button)
    {
        applyStyle(button, ButtonType::Export);
    }

    /// 设置为图标按钮样式
    static void setIcon(QPushButton* button)
    {
        applyStyle(button, ButtonType::Icon);
    }

    /// 设置为链接按钮样式
    static void setLink(QPushButton* button)
    {
        applyStyle(button, ButtonType::Link);
    }

    // ========================================================================
    // 尺寸快捷方法
    // ========================================================================

    static void setSmall(QPushButton* button) { setSize(button, ButtonSize::Small); }
    static void setMedium(QPushButton* button) { setSize(button, ButtonSize::Medium); }
    static void setLarge(QPushButton* button) { setSize(button, ButtonSize::Large); }
    static void setExtraLarge(QPushButton* button) { setSize(button, ButtonSize::ExtraLarge); }

    // ========================================================================
    // 工具按钮快捷方法
    // ========================================================================

    static void setToolButton(QToolButton* button, ButtonType type = ButtonType::Icon)
    {
        applyStyle(button, type);
    }

    // ========================================================================
    // 样式字符串生成（用于 QSS）
    // ========================================================================

    /**
     * @brief 获取按钮类型的 QSS 属性值
     * @param type 按钮类型
     * @return QSS 属性值字符串
     */
    static QString getButtonTypeProperty(ButtonType type);

    /**
     * @brief 获取按钮尺寸的 QSS 属性值
     * @param size 按钮尺寸
     * @return QSS 属性值字符串
     */
    static QString getButtonSizeProperty(ButtonSize size);

    // ========================================================================
    // 尺寸常量
    // ========================================================================

    /// 小型按钮高度
    static constexpr int SmallHeight = 24;
    /// 中型按钮高度
    static constexpr int MediumHeight = 32;
    /// 大型按钮高度
    static constexpr int LargeHeight = 40;
    /// 超大按钮高度
    static constexpr int ExtraLargeHeight = 48;

    /// 小型按钮内边距
    static constexpr int SmallPadding = 6;
    /// 中型按钮内边距
    static constexpr int MediumPadding = 10;
    /// 大型按钮内边距
    static constexpr int LargePadding = 14;
    /// 超大按钮内边距
    static constexpr int ExtraLargePadding = 18;

    /// 小型按钮字体大小
    static constexpr int SmallFontSize = 11;
    /// 中型按钮字体大小
    static constexpr int MediumFontSize = 13;
    /// 大型按钮字体大小
    static constexpr int LargeFontSize = 14;
    /// 超大按钮字体大小
    static constexpr int ExtraLargeFontSize = 16;

    /// 最小按钮宽度（防止文字截断）
    static constexpr int MinButtonWidth = 60;

private:
    ButtonStyles() = delete;
    ~ButtonStyles() = delete;
};

// ============================================================================
// 便捷宏定义
// ============================================================================

/**
 * @brief 快速创建带样式的按钮
 * @param text 按钮文本
 * @param type 按钮类型
 * @param parent 父控件
 */
#define CREATE_BUTTON(text, type, parent) \
    ([](const QString& btnText, ButtonType btnType, QWidget* btnParent) -> QPushButton* { \
        auto* btn = new QPushButton(btnText, btnParent); \
        ButtonStyles::applyStyle(btn, btnType); \
        return btn; \
    })(text, type, parent)

/**
 * @brief 快速创建主要按钮
 */
#define CREATE_PRIMARY_BUTTON(text, parent) CREATE_BUTTON(text, ButtonType::Primary, parent)

/**
 * @brief 快速创建次要按钮
 */
#define CREATE_SECONDARY_BUTTON(text, parent) CREATE_BUTTON(text, ButtonType::Secondary, parent)

/**
 * @brief 快速创建危险按钮
 */
#define CREATE_DANGER_BUTTON(text, parent) CREATE_BUTTON(text, ButtonType::Danger, parent)

/**
 * @brief 快速创建成功按钮
 */
#define CREATE_SUCCESS_BUTTON(text, parent) CREATE_BUTTON(text, ButtonType::Success, parent)

#endif // BUTTONSTYLES_H
