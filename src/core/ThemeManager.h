#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H
#include <QObject>
#include <QMap>
#include <QColor>
#include <QString>
#include <QJsonObject>
/**
     * @brief 主题管理器 - 单例模式
     *
     * 功能：
     * 1. 管理应用程序的主题配置（亮色/暗色）
     * 2. 提供统一颜色接口（背景、前景、强调色、涨跌色等）
     * 3. 支持主题动态切换并通知所有监听控件
     * 4. 支持自定义主题扩展
     */
class ThemeManager : public QObject
{
    Q_OBJECT
public:
    // 主题类型枚举
    enum class ThemeType {
        Light,  // 亮色主题
        Dark,   // 暗色主题
        Custom  // 自定义主题
    };
    Q_ENUM(ThemeType)
    // 获取单例实例
    static ThemeManager* instance();
    // 获取当前主题类型
    ThemeType currentTheme() const;
    // 切换主题
    void setTheme(ThemeType type);
    // 获取主题颜色配置
    QColor backgroundColor() const;
    QColor foregroundColor() const;
    QColor accentColor() const;
    QColor riseColor() const;      // 股票上涨颜色（红色）
    QColor fallColor() const;      // 股票下跌颜色（绿色）
    QColor neutralColor() const;   // 中性颜色（持平）
    QColor borderColor() const;
    QColor cardColor() const;
    // 获取指定角色的颜色
    QColor color(const QString& role) const;
    // 获取主题样式表
    QString themeStyleSheet() const;
    // 注册自定义主题
    void registerCustomTheme(const QString& name, const QJsonObject& config);
signals:
    // 主题切换信号
    void themeChanged(ThemeType newTheme);
protected:
    // 禁用拷贝和赋值
    ThemeManager(QObject* parent = nullptr);
    ~ThemeManager();
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
private:
    // 加载内置主题配置
    void loadBuiltinThemes();

    // 新增：从文件加载主题配置的辅助函数
    bool loadThemeFromFile(ThemeType type, const QString& filePath);

    // 应用主题配置
    void applyTheme(ThemeType type);
private:
    static ThemeManager* m_instance;
    ThemeType m_currentTheme;
    // 主题配置数据结构
    struct ThemeConfig {
        QMap<QString, QColor> colors;
        QString styleSheet;
    };
    QMap<ThemeType, ThemeConfig> m_themes;
    QMap<QString, ThemeType> m_customThemeMap;
};
#endif // THEMEMANAGER_H
