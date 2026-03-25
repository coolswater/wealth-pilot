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

// 前向声明，避免循环包含
class SvgIconEngine;

/**
 * @class ThemeManager
 * @brief 主题管理器
 */
class ThemeManager : public QObject, public Singleton<ThemeManager>
{
    Q_OBJECT
    Q_ENUMS(Theme)
    friend class Singleton<ThemeManager>;
    // 允许图标引擎访问私有注册接口
    friend class SvgIconEngine;

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
    void themeChanged(const ::QString& theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager();

    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    void loadThemeColors();
    QString loadStylesheetFromFile() const;

    /**
     * @brief 注册图标引擎（由 SvgIconEngine 构造函数自动调用）
     * 非线程安全，必须在主线程调用
     */
    void registerIconEngine(SvgIconEngine* engine);

    /**
     * @brief 注销图标引擎（由 SvgIconEngine 析构函数自动调用）
     */
    void unregisterIconEngine(SvgIconEngine* engine);

    /**
     * @brief 通知所有注册的图标引擎主题已变更
     * 实现批量更新策略：begin → apply colors → end，避免重复渲染
     */
    void notifyIconEnginesThemeChanged();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // THEMEMANAGER_H
