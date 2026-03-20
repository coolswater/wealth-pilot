/**
 * @file ThemeManager.cpp
 * @brief 主题管理器实现
 */

#include "ThemeManager.h"
#include "ConfigManager.h"
#include "../utils/Logger.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include "src/views/widgets/SvgColorIcon.h"

struct ThemeManager::Impl {
    Theme currentTheme = Dark;
    QMap<QString, QColor> colors;
    QSet<SvgColorIconEngine*> iconEngines;  // 新增：注册的图标引擎集合

    // 使用数组而不是QMap来存储颜色，提高访问速度
    const std::array<QColor, 13> darkColors = {
        QColor(Tokens::Colors::BgBase),       // background
        QColor(Tokens::Colors::BgSurface),    // surface
        QColor(Tokens::Colors::Primary),      // primary
        QColor(Tokens::Colors::PrimaryLight), // secondary
        QColor(Tokens::Colors::TextPrimary),  // textPrimary
        QColor(Tokens::Colors::TextSecondary),// textSecondary
        QColor(255,255,255),                 // border
        QColor(Tokens::Colors::Success),      // up
        QColor(Tokens::Colors::Danger),       // down
        QColor(Tokens::Colors::Warning),      // warning
        QColor(Tokens::Colors::Success),      // success
        QColor(Tokens::Colors::Danger),       // error
        QColor(Tokens::Colors::Info)         // info
    };

    const std::array<QColor, 13> lightColors = {
        QColor(248,250,252),                 // background
        QColor(255,255,255),                 // surface
        QColor(Tokens::Colors::Primary),      // primary
        QColor(Tokens::Colors::PrimaryLight), // secondary
        QColor(31,41,55),                    // textPrimary
        QColor(107,114,128),                 // textSecondary
        QColor(229,231,235),                 // border
        QColor(Tokens::Colors::Success),      // up
        QColor(Tokens::Colors::Danger),       // down
        QColor(Tokens::Colors::Warning),      // warning
        QColor(Tokens::Colors::Success),      // success
        QColor(Tokens::Colors::Danger),       // error
        QColor(Tokens::Colors::Info)         // info
    };

    const std::array<QColor, 13> eyeCareColors = {
        QColor(30,26,20),                    // background
        QColor(21,18,15),                    // surface
        QColor(212,165,116),                 // primary
        QColor(196,154,108),                 // secondary
        QColor(232,220,200),                 // textPrimary
        QColor(168,155,133),                 // textSecondary
        QColor(61,53,43),                    // border
        QColor(124,179,66),                  // up
        QColor(229,115,115),                 // down
        QColor(255,183,77),                  // warning
        QColor(129,199,132),                 // success
        QColor(229,115,115),                 // error
        QColor(100,181,246)                  // info
    };

    // 颜色名称到索引的映射
    const QMap<QString, int> colorIndexMap = {
        {"background", 0},
        {"surface", 1},
        {"primary", 2},
        {"secondary", 3},
        {"textPrimary", 4},
        {"textSecondary", 5},
        {"border", 6},
        {"up", 7},
        {"down", 8},
        {"warning", 9},
        {"success", 10},
        {"error", 11},
        {"info", 12}
    };
};

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
}

ThemeManager::~ThemeManager() = default;

void ThemeManager::initialize()
{
    LOG_INFO("ThemeManager initializing...");

    QString savedTheme = ConfigManager::instance()->getString(ConfigKeys::Theme, "Dark");

    if (savedTheme == "Dark")
        d->currentTheme = Dark;
    else if (savedTheme == "Light")
        d->currentTheme = Light;
    else if (savedTheme == "EyeCare")
        d->currentTheme = EyeCare;
    else
        d->currentTheme = Dark;

    loadThemeColors();
    applyCurrentTheme();

    LOG_INFO(QString("ThemeManager initialized with theme: %1").arg(currentThemeName()));
}

void ThemeManager::setTheme(Theme theme)
{
    if (d->currentTheme == theme)
        return;

    d->currentTheme = theme;
    loadThemeColors();

    ConfigManager::instance()->set(ConfigKeys::Theme, currentThemeName());

    // 先让所有引擎准备批量更新（暂停缓存清理），再统一刷新，最后统一清理缓存
    notifyIconEnginesThemeChanged();

    emit themeChanged(currentThemeName());
    applyCurrentTheme();

    LOG_INFO(QString("Theme switched to: %1 (updated %2 icon engines)")
                 .arg(currentThemeName()).arg(d->iconEngines.size()));
}

void ThemeManager::setTheme(const QString& themeName)
{
    if (themeName == "Dark")
        setTheme(Dark);
    else if (themeName == "Light")
        setTheme(Light);
    else if (themeName == "EyeCare")
        setTheme(EyeCare);
    else
        LOG_WARNING(QString("Unknown theme name: %1").arg(themeName));
}

ThemeManager::Theme ThemeManager::currentTheme() const
{
    return d->currentTheme;
}

QString ThemeManager::currentThemeName() const
{
    switch (d->currentTheme) {
    case Dark:    return "Dark";
    case Light:   return "Light";
    case EyeCare: return "EyeCare";
    default:      return "Dark";
    }
}

void ThemeManager::applyCurrentTheme()
{
    if (!qApp) {
        LOG_WARNING("QApplication not available, cannot apply stylesheet.");
        return;
    }

    QString style = loadStylesheetFromFile();
    if (style.isEmpty()) {
        LOG_WARNING("Failed to load stylesheet, using empty style.");
        qApp->setStyleSheet(QString());
        return;
    }

    qApp->setStyleSheet(style);
    LOG_INFO(QString("Applied stylesheet for theme: %1").arg(currentThemeName()));
}

QString ThemeManager::loadStylesheetFromFile() const
{
    QString themeName = currentThemeName().toLower();
    QString path = QString(":/style/theme_%1.qss").arg(themeName);

    QFile file(path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        LOG_WARNING(QString("Cannot open stylesheet file: %1").arg(path));
        return QString();
    }

    QTextStream stream(&file);
    QString style = stream.readAll();
    file.close();
    return style;
}

void ThemeManager::loadThemeColors()
{
    const std::array<QColor, 13>* colorArray = nullptr;

    switch (d->currentTheme) {
    case Dark:
        colorArray = &d->darkColors;
        break;
    case Light:
        colorArray = &d->lightColors;
        break;
    case EyeCare:
        colorArray = &d->eyeCareColors;
        break;
    }

    if (colorArray) {
        d->colors.clear();
        for (auto it = d->colorIndexMap.begin(); it != d->colorIndexMap.end(); ++it) {
            d->colors[it.key()] = (*colorArray)[it.value()];
        }
    }
}

void ThemeManager::registerIconEngine(SvgColorIconEngine* engine)
{
    if (!engine || d->iconEngines.contains(engine))
        return;

    d->iconEngines.insert(engine);
    LOG_DEBUG(QString("Registered icon engine, total: %1").arg(d->iconEngines.size()));
}

void ThemeManager::unregisterIconEngine(SvgColorIconEngine* engine)
{
    if (!engine)
        return;

    d->iconEngines.remove(engine);
    LOG_DEBUG(QString("Unregistered icon engine, remaining: %1").arg(d->iconEngines.size()));
}

void ThemeManager::notifyIconEnginesThemeChanged()
{
    if (d->iconEngines.isEmpty())
        return;

    // 阶段1：所有引擎进入批量更新模式（暂停缓存清理）
    for (auto* engine : d->iconEngines) {
        engine->beginThemeUpdate();
    }

    // 阶段2：应用新主题颜色（此时不清理缓存，只更新颜色配置）
    for (auto* engine : d->iconEngines) {
        engine->applyThemeColors();
    }

    // 阶段3：结束批量更新（统一清空缓存，触发重绘）
    for (auto* engine : d->iconEngines) {
        engine->endThemeUpdate();
    }
}


QColor ThemeManager::backgroundColor() const    { return d->colors.value("background"); }
QColor ThemeManager::surfaceColor() const       { return d->colors.value("surface"); }
QColor ThemeManager::primaryColor() const       { return d->colors.value("primary"); }
QColor ThemeManager::secondaryColor() const     { return d->colors.value("secondary"); }
QColor ThemeManager::textPrimaryColor() const   { return d->colors.value("textPrimary"); }
QColor ThemeManager::textSecondaryColor() const { return d->colors.value("textSecondary"); }
QColor ThemeManager::borderColor() const        { return d->colors.value("border"); }

QColor ThemeManager::upColor() const      { return d->colors.value("up"); }
QColor ThemeManager::downColor() const    { return d->colors.value("down"); }
QColor ThemeManager::warningColor() const { return d->colors.value("warning"); }
QColor ThemeManager::successColor() const { return d->colors.value("success"); }
QColor ThemeManager::errorColor() const   { return d->colors.value("error"); }
QColor ThemeManager::infoColor() const    { return d->colors.value("info"); }
