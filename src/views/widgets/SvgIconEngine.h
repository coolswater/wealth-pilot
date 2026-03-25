#ifndef SVGCOLORICON_H
#define SVGCOLORICON_H

#include <QIcon>
#include <QIconEngine>
#include <QColor>
#include <QString>
#include <QByteArray>
#include <QScopedPointer>
#include <array>
#include <optional>
#include <QSharedPointer>

// 前向声明
class ThemeManager;

/**
 * @brief 图标颜色角色枚举
 * 对应 ThemeManager 的语义化颜色接口
 */
enum class IconColorRole {
    Primary,        ///< 主题主色（primaryColor）
    Secondary,      ///< 主题次要色（secondaryColor）
    TextPrimary,    ///< 主要文字色（textPrimaryColor）
    TextSecondary,  ///< 次要文字色（textSecondaryColor）
    Success,        ///< 成功色（successColor）
    Warning,        ///< 警告色（warningColor）
    Danger,         ///< 危险色（errorColor）
    Info,           ///< 信息色（infoColor）
    Up,             ///< 上涨色（upColor）
    Down,           ///< 下跌色（downColor）
    Custom          ///< 自定义固定颜色（不跟随主题）
};

/**
 * @class SvgIconEngine
 * @brief SVG 着色图标引擎（支持 ThemeManager 联动）
 *
 * 特性：
 * 1. 自动注册到 ThemeManager，生命周期自动管理
 * 2. 支持批量更新，主题切换时高性能刷新
 * 3. 语义化角色映射，适配不同主题模式
 * 4. 线程安全的 LRU 缓存，优化渲染性能
 */
class SvgIconEngine : public QIconEngine {
public:
    explicit SvgIconEngine(const QString& svgPath);
    ~SvgIconEngine() override;

    // QIconEngine 接口实现
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
    QSize actualSize(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine* clone() const override;
    QString key() const override;
    bool read(QDataStream& in) override;
    bool write(QDataStream& out) const override;

    // 基础颜色设置（固定颜色模式）
    void setColor(QIcon::Mode mode, const QColor& color);
    QColor color(QIcon::Mode mode) const;

    // 缓存管理
    void clearCache();
    void setCacheLimit(int maxCostKB);

    // ========== 主题联动核心接口 ==========

    /**
     * @brief 设置是否跟随主题
     * @param follow true: 自动跟随 ThemeManager 主题变化
     *               false: 使用 setColor 设置的固定颜色
     */
    void setFollowTheme(bool follow);
    bool isFollowingTheme() const noexcept { return m_followTheme; }

    /**
     * @brief 设置指定状态的颜色角色
     * @param mode QIcon 状态（Normal/Disabled/Active/Selected）
     * @param role 颜色角色（如 Primary、Success 等）
     */
    void setColorRole(QIcon::Mode mode, IconColorRole role);
    IconColorRole colorRole(QIcon::Mode mode) const;

    /**
     * @brief 应用当前主题颜色（由 ThemeManager 批量调用）
     * 内部根据 m_colorRoles 和 ThemeManager 当前主题更新 m_colors
     */
    void applyThemeColors();

    /**
     * @brief 批量更新控制（由 ThemeManager 统一调度）
     * 避免主题切换时 N 次重复清空缓存
     */
    void beginThemeUpdate();
    void endThemeUpdate();

    bool isValid() const noexcept { return m_isValid; }
    QString svgPath() const { return m_svgPath; }

private:
    // 友元声明，允许 ThemeManager 访问私有批量更新接口
    friend class ThemeManager;

    // 将颜色角色解析为实际 QColor（访问 ThemeManager）
    QColor resolveColor(IconColorRole role) const;

    // 生成缓存键（尺寸+模式+颜色+DPR）
    static QString cacheKey(const QSize& size, QIcon::Mode mode, const QColor& color, qreal dpr);

    // 渲染逻辑
    QPixmap renderPixmap(const QSize& size, QIcon::Mode mode, qreal dpr);
    static void tintImage(QImage& image, const QColor& color);

    // 自动注册/注销到 ThemeManager
    void registerToThemeManager();
    void unregisterFromThemeManager();

    // 成员变量
    QString m_svgPath;
    QByteArray m_svgData;           // SVG 原始数据缓存（延迟加载）
    std::array<QColor, 4> m_colors;        // 各状态实际颜色（Normal/Disabled/Active/Selected）
    std::array<IconColorRole, 4> m_roles;  // 各状态映射的角色
    bool m_isValid = false;
    bool m_followTheme = false;     // 是否跟随主题
    bool m_batchUpdating = false;   // 是否处于批量更新模式（暂停缓存清理）
    bool m_pendingRefresh = false;   // 批量期间有颜色更新待应用

    // PIMPL：LRU 缓存实现（线程安全）
    class CachePrivate;
    QScopedPointer<CachePrivate> m_cache;
};

/**
 * @class SvgColorIcon
 * @brief SVG 着色图标包装类（流式 API）
 *
 * 使用示例：
 * @code
 * SvgColorIcon icon(":/icons/save.svg");
 * icon.followTheme()  // 启用主题跟随
 *       .setNormalRole(IconColorRole::Primary)
 *       .setDisabledRole(IconColorRole::TextSecondary);
 *
 * button->setIcon(icon);  // 自动转换
 * @endcode
 */
class SvgColorIcon {
public:
    explicit SvgColorIcon(const QString& svgPath = QString());

    // 隐式转换为 QIcon（用于 QAction、QToolButton 等）
    operator QIcon() const { return toIcon(); }
    QIcon toIcon() const;

    // 固定颜色模式接口（与旧代码兼容）
    SvgColorIcon& setColor(QIcon::Mode mode, const QColor& color);
    SvgColorIcon& setNormalColor(const QColor& color);
    SvgColorIcon& setDisabledColor(const QColor& color);
    SvgColorIcon& setActiveColor(const QColor& color);
    SvgColorIcon& setSelectedColor(const QColor& color);
    QColor color(QIcon::Mode mode) const;

    // ========== 主题联动流式接口 ==========

    /** @brief 启用/禁用主题跟随 */
    SvgColorIcon& followTheme(bool enabled = true);

    /** @brief 设置各状态的颜色角色（链式调用） */
    SvgColorIcon& setNormalRole(IconColorRole role);
    SvgColorIcon& setDisabledRole(IconColorRole role);
    SvgColorIcon& setActiveRole(IconColorRole role);
    SvgColorIcon& setSelectedRole(IconColorRole role);
    SvgColorIcon& setRole(QIcon::Mode mode, IconColorRole role);

    // 查询
    bool isFollowingTheme() const;
    bool isValid() const noexcept;
    QString svgPath() const;
    void clearCache();

private:
    // mutable 允许在 const 方法中延迟初始化（逻辑 const）
    mutable QSharedPointer<SvgIconEngine> m_engine;
    mutable QScopedPointer<QIcon> m_iconCache;  // 确保是指针类型

    void ensureEngine() const;
};

#endif // SVGCOLORICON_H
