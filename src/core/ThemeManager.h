/**
 * @file ThemeManager.h
 * @brief 主题管理器 - 统一管理应用主题（单例）
 */

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include "Singleton.h"
#include "Tokens.h"

#include <QObject>
#include <QColor>
#include <QMap>
#include <memory>

/**
 * @class ThemeManager
 * @brief 主题管理器
 */
class ThemeManager : public QObject, public Singleton<ThemeManager>
{
    Q_OBJECT
    Q_ENUMS(Theme)
    friend class Singleton<ThemeManager>;

public:
    enum Theme {
        Dark,     ///< 深色主题
        Light,    ///< 浅色主题
        EyeCare   ///< 护眼主题
    };

    void initialize();
    void setTheme(Theme theme);
    void setTheme(const QString& themeName);
    Theme currentTheme() const;
    QString currentThemeName() const;

    void applyCurrentTheme();

    // 颜色获取
    QColor backgroundColor() const;
    QColor surfaceColor() const;
    QColor primaryColor() const;
    QColor secondaryColor() const;
    QColor textPrimaryColor() const;
    QColor textSecondaryColor() const;
    QColor borderColor() const;

    QColor upColor() const;
    QColor downColor() const;
    QColor warningColor() const;
    QColor successColor() const;
    QColor errorColor() const;
    QColor infoColor() const;

signals:
    void themeChanged(const QString& theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager();

    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    void loadThemeColors();
    QString loadStylesheetFromFile() const;

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // THEMEMANAGER_H
