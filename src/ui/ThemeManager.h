#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QMap>
#include <QColor>
#include <QString>
#include <QJsonObject>

/**
 * @brief 主题管理器（单例模式）
 *
 * 负责管理应用程序的主题配置，包括颜色定义和样式表。
 * 支持内置主题（亮色、暗色、护眼）和动态切换，并提供颜色角色查询接口。
 * 主题切换时发出信号，便于其他组件响应。
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    // 主题类型枚举
    enum class ThemeType {
        Light,      // 亮色主题
        Dark,       // 暗色主题
        EyeCare,    // 护眼主题
        Custom      // 自定义主题
    };
    Q_ENUM(ThemeType)

    // 获取单例实例
    static ThemeManager* instance();

    // 禁止拷贝和赋值
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    // 获取当前主题类型
    ThemeType currentTheme() const;

    // 切换主题（内置类型）
    void setTheme(ThemeType type);

    // 注册自定义主题（从 JSON 配置）
    void registerCustomTheme(const QString& name, const QJsonObject& config);

    // 切换至已注册的自定义主题
    void setCustomTheme(const QString& name);

    // 获取指定颜色角色的颜色值（例如 "background", "foreground", "accent", "rise", "fall" 等）
    QColor color(const QString& role) const;

    // 获取当前主题的样式表
    QString styleSheet() const;

signals:
    // 主题切换信号，参数为新主题类型
    void themeChanged(ThemeType newTheme);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager();

    // 加载内置主题配置（从内置数据或资源文件）
    void loadBuiltinThemes();

    // 从 JSON 对象解析主题配置
    bool loadThemeFromJson(ThemeType type, const QJsonObject& json);

    // 应用主题（设置样式表、更新内部缓存）
    void applyTheme(ThemeType type);

    // 辅助：从 JSON 对象构建颜色映射
    QMap<QString, QColor> parseColorsFromJson(const QJsonObject& colorsObj) const;

private:
    static ThemeManager* m_instance;

    ThemeType m_currentTheme;
    QMap<ThemeType, QString> m_styleSheets;          // 主题 -> 样式表内容
    QMap<ThemeType, QMap<QString, QColor>> m_colorMap; // 主题 -> 颜色角色映射
    QMap<QString, ThemeType> m_customThemeMap;       // 自定义主题名称 -> 类型映射
};

#endif // THEMEMANAGER_H
