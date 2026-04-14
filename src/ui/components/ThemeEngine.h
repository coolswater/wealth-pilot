/**
 * @file ThemeEngine.h
 * @brief 主题引擎 - 管理应用主题和样式
 *
 * @details 功能：
 * - 主题配置管理（深色/浅色/护眼等）
 * - 动态主题切换
 * - 样式属性缓存和快速访问
 * - 性能优化：预编译样式表，减少运行时计算
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef THEMEENGINE_H
#define THEMEENGINE_H

#include "UIComponents.h"
#include <QSettings>
#include <QMap>
#include <QMutex>
#include <memory>

class ThemeEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentTheme READ currentTheme WRITE setCurrentTheme NOTIFY themeChanged)

public:
    /**
     * @brief 获取主题引擎单例
     */
    static ThemeEngine& instance();

    /**
     * @brief 初始化主题引擎
     */
    bool initialize();

    /**
     * @brief 获取当前主题名称
     */
    QString currentTheme() const;

    /**
     * @brief 设置当前主题
     */
    void setCurrentTheme(const QString& themeName);

    /**
     * @brief 获取主题配置
     */
    const UIComponents::ThemeConfig& themeConfig() const;

    /**
     * @brief 注册主题
     */
    void registerTheme(const QString& name, const UIComponents::ThemeConfig& config);

    /**
     * @brief 获取样式属性（高性能访问）
     */
    QVariant styleProperty(const QString& key, const QVariant& defaultValue = QVariant()) const;

    /**
     * @brief 应用主题到组件
     */
    void applyTheme(QWidget* widget);

    /**
     * @brief 预编译样式表（性能优化）
     */
    QString compiledStyleSheet() const;

signals:
    /**
     * @brief 主题切换信号
     */
    void themeChanged(const QString& newTheme);

private:
    ThemeEngine();
    ~ThemeEngine() = default;
    ThemeEngine(const ThemeEngine&) = delete;
    ThemeEngine& operator=(const ThemeEngine&) = delete;

    // 主题配置管理
    void loadThemes();
    void saveCurrentTheme();

    // 样式缓存
    void updateStyleCache();
    QString generateStyleSheet() const;

    // 性能优化：样式属性快速查找表
    void buildPropertyLookupTable();

    QString m_currentTheme;
    QMap<QString, UIComponents::ThemeConfig> m_themes;
    mutable UIComponents::ThemeConfig m_currentConfig;
    mutable QMutex m_mutex;
    QMap<QString, QVariant> m_styleCache;
    QMap<QString, int> m_propertyLookup;  // 性能优化：字符串到索引的映射
    QString m_compiledStyleSheet;         // 预编译样式表

    friend class ComponentRegistry;
};

#endif // THEMEENGINE_H