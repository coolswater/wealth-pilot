#include "IconProvider.h"

IconProvider* IconProvider::instance()
{
    static IconProvider inst;
    return &inst;
}

void IconProvider::initialize()
{
    // 预创建常用图标（可选，也可以延迟加载）
    // 配置导航图标：Normal 状态使用主题主色，Disabled 使用次要文字色
    // m_cache["dashboard"] = createThemedIcon(":/icons/dashboard.svg");
    m_cache["portfolio"] = createThemedIcon(":/icons/portfolio.svg");
    m_cache["market"] = createThemedIcon(":/icons/market.svg");

    // 特殊语义色配置
    m_cache["dashboard"] = SvgColorIcon(":/icons/dashboard.svg")
                           .followTheme()
                           .setNormalRole(IconColorRole::Primary)    // 交易图标用成功色（绿色）
                           .setDisabledRole(IconColorRole::TextPrimary);

    m_cache["risk"] = SvgColorIcon(":/icons/risk.svg")
                          .followTheme()
                          .setNormalRole(IconColorRole::Warning)    // 风控用警告色（黄色）
                          .setActiveRole(IconColorRole::Danger);    // 激活时用危险色（红色强调）

    m_cache["settings"] = createThemedIcon(":/icons/settings.svg",
                                           IconColorRole::TextSecondary);  // 设置用次要色

    // 折叠按钮图标（固定颜色，不跟随主题，或跟随文字色）
    m_cache["chevron_left"] = SvgColorIcon(":/icons/chevron_left.svg")
                                  .followTheme()
                                  .setNormalRole(IconColorRole::TextPrimary);

    m_cache["chevron_right"] = SvgColorIcon(":/icons/chevron_right.svg")
                                   .followTheme()
                                   .setNormalRole(IconColorRole::TextPrimary);
}

QIcon IconProvider::dashboard() const
{
    return m_cache.value("dashboard").toIcon();
}

QIcon IconProvider::portfolio() const
{
    return m_cache.value("portfolio").toIcon();
}

QIcon IconProvider::market() const
{
    return m_cache.value("market").toIcon();
}

QIcon IconProvider::trade() const
{
    return m_cache.value("trade").toIcon();
}

QIcon IconProvider::analysis() const
{
    return m_cache.value("analysis").toIcon();
}

QIcon IconProvider::settings() const
{
    return m_cache.value("settings").toIcon();
}

QIcon IconProvider::risk() const
{
    return m_cache.value("risk").toIcon();
}

QIcon IconProvider::collapseLeft() const
{
    return m_cache.value("chevron_left").toIcon();
}

QIcon IconProvider::collapseRight() const
{
    return m_cache.value("chevron_right").toIcon();
}

SvgColorIcon IconProvider::createThemedIcon(const QString& path,
                                            IconColorRole normalRole,
                                            IconColorRole disabledRole) const
{
    return SvgColorIcon(path)
    .followTheme()
        .setNormalRole(normalRole)
        .setDisabledRole(disabledRole)
        .setActiveRole(IconColorRole::Secondary)     // 悬停/激活用次要色
        .setSelectedRole(IconColorRole::Primary);    // 选中时用主色（加亮）
}
