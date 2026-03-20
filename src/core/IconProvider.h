#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include "../views/widgets/SvgColorIcon.h"
#include <QIcon>
#include <QMap>

/**
 * @class IconProvider
 * @brief 全局图标库 - 单例管理所有主题感知图标
 *
 * 特性：
 * 1. 所有图标自动跟随 ThemeManager 主题切换
 * 2. 统一配置各图标的语义角色（Normal/Disabled/Active/Selected）
 * 3. 延迟加载（首次获取时才创建 SvgColorIcon）
 */
class IconProvider
{
public:
    static IconProvider* instance();

    /**
     * @brief 初始化所有图标（应用启动时调用一次）
     * 配置各图标的颜色角色映射
     */
    void initialize();

    // 导航图标（对应 SidebarWidget 的菜单项）
    QIcon dashboard() const;      // 仪表盘 - 主题主色
    QIcon portfolio() const;      // 投资组合 - 主题主色
    QIcon market() const;         // 市场行情 - 主题主色
    QIcon trade() const;          // 交易 - 成功色（绿色）
    QIcon analysis() const;       // 分析 - 信息色（蓝色）
    QIcon settings() const;       // 设置 - 次要文字色
    QIcon risk() const;           // 风控 - 警告色（黄色/琥珀色）

    // 通用功能图标
    QIcon collapseLeft() const;   // 向左折叠
    QIcon collapseRight() const;  // 向右折叠
    QIcon logout() const;         // 退出 - 危险色（红色）

private:
    IconProvider() = default;
    ~IconProvider() = default;

    // 辅助方法：创建跟随主题的图标，带默认角色配置
    SvgColorIcon createThemedIcon(const QString& path,
                                  IconColorRole normalRole = IconColorRole::Primary,
                                  IconColorRole disabledRole = IconColorRole::TextSecondary) const;

    // 缓存（mutable 允许 const 方法修改）
    mutable QMap<QString, SvgColorIcon> m_cache;
};

#endif // ICONPROVIDER_H
