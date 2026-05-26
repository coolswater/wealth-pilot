/**
 * @file ThemeManager.h
 * @brief 主题管理器 - 多主题支持
 *
 * @details 提供主题管理功能：
 * - 深色主题（默认，专业金融风格）
 * - 浅色主题
 * - 高对比度主题
 * - 护眼主题
 * - 自定义主题（JSON 配置）
 * - QSS 样式表动态加载和缓存
 *
 * @details 性能优化（v2.0.0）：
 * - 样式表编译缓存，避免重复解析
 * - 批量 UI 更新，减少重绘次数
 * - 异步监听器通知，避免阻塞主线程
 * - 使用 update() 替代 repaint()，允许 Qt 合并重绘请求
 *
 * @details 使用示例：
 * @code
 * // 切换主题
 * ThemeManager::instance()->setTheme(ThemeType::Light);
 *
 * // 注册主题变化监听器
 * ThemeManager::instance()->registerThemeChangeListener(this, [this]() {
 *     updateMyWidgetStyle();
 * });
 *
 * // 获取当前主题配色
 * ThemeColors colors = ThemeManager::instance()->currentTheme();
 * @endcode
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QHash>
#include <QMutex>

/**
 * @brief 主题类型
 */
enum class ThemeType {
    Dark,           ///< 深色主题
    Light,          ///< 浅色主题
    HighContrast,   ///< 高对比度主题
    EyeCare,        ///< 护眼主题
    Custom          ///< 自定义主题
};

/**
 * @brief 主题配色方案
 */
struct ThemeColors {
    QString name;                   ///< 主题名称

    // 背景色
    QString bgPrimary;              ///< 主背景色
    QString bgSecondary;            ///< 次背景色
    QString bgElevated;             ///< 提升背景色
    QString bgSurface;              ///< 表面背景色
    QString bgHover;                ///< 悬停背景色

    // 文本色
    QString textPrimary;            ///< 主文本色
    QString textSecondary;          ///< 次文本色
    QString textTertiary;           ///< 三级文本色
    QString textDisabled;           ///< 禁用文本色

    // 强调色
    QString primary;                ///< 主色调
    QString primaryHover;           ///< 主色调悬停
    QString primaryDark;            ///< 主色调深色
    QString accent;                 ///< 强调色

    // 状态色
    QString success;                ///< 成功色（涨）
    QString danger;                 ///< 危险色（跌）
    QString warning;                ///< 警告色
    QString info;                   ///< 信息色

    // 边框和分割线
    QString border;                 ///< 边框色
    QString divider;                ///< 分割线色

    // 图表色
    QString chartUp;                ///< 图表上涨色
    QString chartDown;              ///< 图表下跌色
    QString chartGrid;              ///< 图表网格色
};

/**
 * @brief 主题管理器
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager* instance();

    /**
     * @brief 初始化主题管理器
     */
    bool initialize();

    /**
     * @brief 设置当前主题
     */
    void setTheme(ThemeType type);

    /**
     * @brief 获取当前主题类型
     */
    ThemeType currentThemeType() const { return m_currentType; }

    /**
     * @brief 获取当前主题配色
     */
    ThemeColors currentTheme() const { return m_currentTheme; }

    /**
     * @brief 获取指定主题配色
     */
    ThemeColors getTheme(ThemeType type) const;

    /**
     * @brief 加载自定义主题
     */
    bool loadCustomTheme(const QString& filePath);

    /**
     * @brief 保存自定义主题
     */
    bool saveCustomTheme(const QString& filePath);

    /**
     * @brief 应用主题到应用
     */
    void applyTheme();

    /**
     * @brief 获取主题样式表
     */
    QString getThemeStyleSheet() const;

    /**
     * @brief 清除样式缓存
     */
    void clearCache();

    /**
     * @brief 注册主题变化监听器
     */
    void registerThemeChangeListener(QObject* object, const std::function<void()>& callback);

    /**
     * @brief 获取主题名称
     */
    static QString themeTypeToString(ThemeType type);

    /**
     * @brief 从名称获取主题类型
     */
    static ThemeType stringToThemeType(const QString& name);

signals:
    /**
     * @brief 主题变化信号
     */
    void themeChanged(ThemeType type);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override = default;

    // 初始化内置主题
    void initBuiltInThemes();

    // 加载QSS样式表
    QString loadQssFile(const QString& fileName) const;
    QString loadBaseQss() const;
    QString loadThemeQss(ThemeType type) const;

    // 替换QSS中的颜色变量
    QString replaceColorVariables(const QString& qss, const ThemeColors& theme) const;

    // 主题转换
    ThemeColors fromJsonObject(const QJsonObject& json) const;
    QJsonObject toJsonObject(const ThemeColors& theme) const;

    // 数据成员
    ThemeType m_currentType = ThemeType::Dark;
    ThemeColors m_currentTheme;
    QMap<ThemeType, ThemeColors> m_themes;

    QVector<QPair<QObject*, std::function<void()>>> m_listeners;

    bool m_initialized = false;

    // 性能优化：样式缓存
    mutable QHash<ThemeType, QString> m_styleCache;
    mutable QMutex m_cacheMutex;
};

#endif // THEMEMANAGER_H