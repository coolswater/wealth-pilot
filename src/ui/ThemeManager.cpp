/**
 * @file ThemeManager.cpp
 * @brief 主题管理器实现
 */

#include "ThemeManager.h"
#include "utils/Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QApplication>
#include <QPalette>
#include <QWidget>
#include <QMutexLocker>

ThemeManager* ThemeManager::instance()
{
    static ThemeManager* inst = new ThemeManager();
    return inst;
}

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
}

bool ThemeManager::initialize()
{
    if (m_initialized) return true;

    LOG_INFO("Initializing Theme Manager");

    initBuiltInThemes();

    // 设置默认主题（不发射信号，因为此时没有监听器）
    m_currentType = ThemeType::Dark;
    m_currentTheme = getTheme(ThemeType::Dark);
    applyTheme();

    m_initialized = true;
    LOG_INFO("Theme Manager initialized");
    return true;
}

void ThemeManager::setTheme(ThemeType type)
{
    if (type == m_currentType) return;

    m_currentType = type;
    m_currentTheme = getTheme(type);

    applyTheme();

    LOG_INFO(QString("Theme changed to: %1").arg(static_cast<int>(type)));
    emit themeChanged(type);
}

void ThemeManager::clearCache()
{
    QMutexLocker locker(&m_cacheMutex);
    m_styleCache.clear();
    LOG_DEBUG("Theme style cache cleared");
}

ThemeColors ThemeManager::getTheme(ThemeType type) const
{
    return m_themes.value(type);
}

bool ThemeManager::loadCustomTheme(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open theme file: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return false;
    }

    ThemeColors theme = fromJsonObject(doc.object());
    m_themes[ThemeType::Custom] = theme;

    LOG_INFO(QString("Custom theme loaded: %1").arg(filePath));
    return true;
}

bool ThemeManager::saveCustomTheme(const QString& filePath)
{
    if (!m_themes.contains(ThemeType::Custom)) {
        return false;
    }

    QJsonObject json = toJsonObject(m_themes[ThemeType::Custom]);
    QJsonDocument doc(json);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file for writing: %1").arg(filePath));
        return false;
    }

    file.write(doc.toJson());
    file.close();

    LOG_INFO(QString("Custom theme saved: %1").arg(filePath));
    return true;
}

void ThemeManager::applyTheme()
{
    // 检查缓存
    QString styleSheet;
    {
        QMutexLocker locker(&m_cacheMutex);
        if (m_styleCache.contains(m_currentType))
        {
            styleSheet = m_styleCache[m_currentType];
            LOG_DEBUG("Using cached stylesheet for theme");
        }
    }

    // 如果没有缓存，编译样式表
    if (styleSheet.isEmpty())
    {
        // 加载基础样式
        styleSheet = loadBaseQss();

        // 加载主题特定样式
        styleSheet += "\n" + loadThemeQss(m_currentType);

        // 替换颜色变量
        styleSheet = replaceColorVariables(styleSheet, m_currentTheme);

        // 缓存编译后的样式表
        {
            QMutexLocker locker(&m_cacheMutex);
            m_styleCache[m_currentType] = styleSheet;
        }
    }

    // 应用QPalette
    QPalette palette;

    palette.setColor(QPalette::Window, QColor(m_currentTheme.bgPrimary));
    palette.setColor(QPalette::WindowText, QColor(m_currentTheme.textPrimary));
    palette.setColor(QPalette::Base, QColor(m_currentTheme.bgSecondary));
    palette.setColor(QPalette::AlternateBase, QColor(m_currentTheme.bgElevated));
    palette.setColor(QPalette::ToolTipBase, QColor(m_currentTheme.bgElevated));
    palette.setColor(QPalette::ToolTipText, QColor(m_currentTheme.textPrimary));
    palette.setColor(QPalette::Text, QColor(m_currentTheme.textPrimary));
    palette.setColor(QPalette::Button, QColor(m_currentTheme.bgSurface));
    palette.setColor(QPalette::ButtonText, QColor(m_currentTheme.textPrimary));
    palette.setColor(QPalette::BrightText, QColor(m_currentTheme.danger));
    palette.setColor(QPalette::Highlight, QColor(m_currentTheme.primary));
    palette.setColor(QPalette::HighlightedText, QStringLiteral("#FFFFFF"));

    qApp->setPalette(palette);

    // 应用样式表
    qApp->setStyleSheet(styleSheet);

    // 通知所有监听器
    for (const auto& listener : m_listeners) {
        if (listener.first) {
            listener.second();
        }
    }
    
    // 强制刷新所有顶级窗口
    for (QWidget* widget : qApp->topLevelWidgets()) {
        if (widget) {
            widget->setStyleSheet(widget->styleSheet());
            widget->update();
            widget->repaint();
        }
    }
    
    LOG_INFO(QString("Theme applied: %1").arg(m_currentTheme.name));
}

QString ThemeManager::getThemeStyleSheet() const
{
    // 返回当前已加载的样式表
    return qApp->styleSheet();
}

QString ThemeManager::loadQssFile(const QString& fileName) const
{
    // 尝试从资源文件加载
    QString resourcePath = QStringLiteral(":/style/%1").arg(fileName);
    QFile resourceFile(resourcePath);
    
    if (resourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(resourceFile.readAll());
        resourceFile.close();
        LOG_DEBUG(QString("Loaded QSS from resource: %1").arg(fileName));
        return content;
    }
    
    // 尝试从文件系统加载
    QString filePath = QStringLiteral("resources/style/%1").arg(fileName);
    QFile file(filePath);
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        LOG_DEBUG(QString("Loaded QSS from file: %1").arg(filePath));
        return content;
    }
    
    LOG_WARNING(QString("Failed to load QSS file: %1").arg(fileName));
    return QString();
}

QString ThemeManager::loadBaseQss() const
{
    // 加载基础样式（已包含所有通用样式）
    return loadQssFile(QStringLiteral("base.qss"));
}

QString ThemeManager::loadThemeQss(ThemeType type) const
{
    QString fileName;
    
    switch (type) {
        case ThemeType::Dark:
            fileName = QStringLiteral("theme_dark.qss");
            break;
        case ThemeType::Light:
            fileName = QStringLiteral("theme_light.qss");
            break;
        case ThemeType::EyeCare:
            fileName = QStringLiteral("theme_eyecare.qss");
            break;
        case ThemeType::HighContrast:
            fileName = QStringLiteral("theme_dark.qss"); // 使用深色主题
            break;
        default:
            fileName = QStringLiteral("theme_dark.qss");
            break;
    }

    // 加载主题样式
    QString themeQss = loadQssFile(fileName);

    // 加载组件样式（所有主题共用）
    QString componentsQss = loadQssFile(QStringLiteral("components.qss"));

    // 加载按钮样式（所有主题共用）
    QString buttonsQss = loadQssFile(QStringLiteral("buttons.qss"));

    // 加载页面样式（所有主题共用）
    QString pagesQss = loadQssFile(QStringLiteral("pages.qss"));

    return themeQss + "\n" + componentsQss + "\n" + buttonsQss + "\n" + pagesQss;
}

QString ThemeManager::replaceColorVariables(const QString& qss, const ThemeColors& theme) const
{
    QString result = qss;
    
    // 背景色
    result.replace(QStringLiteral("${bgPrimary}"), theme.bgPrimary);
    result.replace(QStringLiteral("${bgSecondary}"), theme.bgSecondary);
    result.replace(QStringLiteral("${bgElevated}"), theme.bgElevated);
    result.replace(QStringLiteral("${bgSurface}"), theme.bgSurface);
    result.replace(QStringLiteral("${bgHover}"), theme.bgHover);
    
    // 文本色
    result.replace(QStringLiteral("${textPrimary}"), theme.textPrimary);
    result.replace(QStringLiteral("${textSecondary}"), theme.textSecondary);
    result.replace(QStringLiteral("${textTertiary}"), theme.textTertiary);
    result.replace(QStringLiteral("${textDisabled}"), theme.textDisabled);
    
    // 品牌色
    result.replace(QStringLiteral("${primary}"), theme.primary);
    result.replace(QStringLiteral("${primaryHover}"), theme.primaryHover);
    result.replace(QStringLiteral("${primaryDark}"), theme.primaryDark);
    result.replace(QStringLiteral("${accent}"), theme.accent);
    
    // 状态色
    result.replace(QStringLiteral("${success}"), theme.success);
    result.replace(QStringLiteral("${danger}"), theme.danger);
    result.replace(QStringLiteral("${warning}"), theme.warning);
    result.replace(QStringLiteral("${info}"), theme.info);
    
    // 边框色
    result.replace(QStringLiteral("${border}"), theme.border);
    result.replace(QStringLiteral("${divider}"), theme.divider);
    
    // 图表色
    result.replace(QStringLiteral("${chartUp}"), theme.chartUp);
    result.replace(QStringLiteral("${chartDown}"), theme.chartDown);
    result.replace(QStringLiteral("${chartGrid}"), theme.chartGrid);
    
    return result;
}

QString ThemeManager::themeTypeToString(ThemeType type)
{
    switch (type) {
        case ThemeType::Dark: return QStringLiteral("dark");
        case ThemeType::Light: return QStringLiteral("light");
        case ThemeType::HighContrast: return QStringLiteral("highcontrast");
        case ThemeType::EyeCare: return QStringLiteral("eyecare");
        case ThemeType::Custom: return QStringLiteral("custom");
        default: return QStringLiteral("dark");
    }
}

ThemeType ThemeManager::stringToThemeType(const QString& name)
{
    if (name == QStringLiteral("light")) return ThemeType::Light;
    if (name == QStringLiteral("highcontrast")) return ThemeType::HighContrast;
    if (name == QStringLiteral("eyecare")) return ThemeType::EyeCare;
    if (name == QStringLiteral("custom")) return ThemeType::Custom;
    return ThemeType::Dark;
}

void ThemeManager::registerThemeChangeListener(QObject* object, const std::function<void()>& callback)
{
    m_listeners.append({object, callback});

    // 当对象销毁时自动移除
    connect(object, &QObject::destroyed, this, [this, object]() {
        for (int i = m_listeners.size() - 1; i >= 0; --i) {
            if (m_listeners[i].first == object) {
                m_listeners.removeAt(i);
            }
        }
    });
}

void ThemeManager::initBuiltInThemes()
{
    // 深色主题 - 专业金融风格
    ThemeColors darkTheme;
    darkTheme.name = QStringLiteral("深色主题");
    darkTheme.bgPrimary = QStringLiteral("#0A0E17"); // 主背景色（深蓝黑）
    darkTheme.bgSecondary = QStringLiteral("#111827"); // 卡片背景色（深灰蓝）
    darkTheme.bgElevated = QStringLiteral("#1A2332"); // 提升背景色（中灰蓝）
    darkTheme.bgSurface = QStringLiteral("#1A2332"); // 表面背景色
    darkTheme.bgHover = QStringLiteral("rgba(255, 255, 255, 0.05)");
    darkTheme.textPrimary = QStringLiteral("#F3F4F6"); // 主文本色（亮白）
    darkTheme.textSecondary = QStringLiteral("#9CA3AF"); // 次文本色（中灰）
    darkTheme.textTertiary = QStringLiteral("#6B7280"); // 三级文本色
    darkTheme.textDisabled = QStringLiteral("#4B5563"); // 禁用文本色
    darkTheme.primary = QStringLiteral("#3B82F6"); // 主品牌色（专业蓝）
    darkTheme.primaryHover = QStringLiteral("#60A5FA"); // 主色调悬停
    darkTheme.primaryDark = QStringLiteral("#2563EB"); // 主色调深色
    darkTheme.accent = QStringLiteral("#F59E0B"); // 强调色（金色）
    darkTheme.success = QStringLiteral("#10B981"); // 涨/成功色（翠绿）
    darkTheme.danger = QStringLiteral("#EF4444"); // 跌/危险色（鲜红）
    darkTheme.warning = QStringLiteral("#F59E0B"); // 警告色（金色）
    darkTheme.info = QStringLiteral("#3B82F6"); // 信息色
    darkTheme.border = QStringLiteral("#2D3748"); // 边框色
    darkTheme.divider = QStringLiteral("#1F2937"); // 分割线色
    darkTheme.chartUp = QStringLiteral("#EF4444"); // 图表上涨色（鲜红）
    darkTheme.chartDown = QStringLiteral("#10B981"); // 图表下跌色（翠绿）
    darkTheme.chartGrid = QStringLiteral("#2D3748"); // 图表网格色

    m_themes[ThemeType::Dark] = darkTheme;

    // 浅色主题
    ThemeColors lightTheme;
    lightTheme.name = QStringLiteral("浅色主题");
    lightTheme.bgPrimary = QStringLiteral("#F8FAFC");
    lightTheme.bgSecondary = QStringLiteral("#FFFFFF");
    lightTheme.bgElevated = QStringLiteral("#FFFFFF");
    lightTheme.bgSurface = QStringLiteral("#F1F5F9");
    lightTheme.bgHover = QStringLiteral("#E2E8F0");
    lightTheme.textPrimary = QStringLiteral("#1F2937");
    lightTheme.textSecondary = QStringLiteral("#6B7280");
    lightTheme.textTertiary = QStringLiteral("#9CA3AF");
    lightTheme.textDisabled = QStringLiteral("#D1D5DB");
    lightTheme.primary = QStringLiteral("#3B82F6");
    lightTheme.primaryHover = QStringLiteral("#2563EB");
    lightTheme.primaryDark = QStringLiteral("#1D4ED8");
    lightTheme.accent = QStringLiteral("#8B5CF6");
    lightTheme.success = QStringLiteral("#10B981");
    lightTheme.danger = QStringLiteral("#EF4444");
    lightTheme.warning = QStringLiteral("#F59E0B");
    lightTheme.info = QStringLiteral("#3B82F6");
    lightTheme.border = QStringLiteral("#E5E7EB");
    lightTheme.divider = QStringLiteral("#F3F4F6");
    lightTheme.chartUp = QStringLiteral("#10B981");
    lightTheme.chartDown = QStringLiteral("#EF4444");
    lightTheme.chartGrid = QStringLiteral("#E5E7EB");

    m_themes[ThemeType::Light] = lightTheme;

    // 护眼主题
    ThemeColors eyecareTheme;
    eyecareTheme.name = QStringLiteral("护眼主题");
    eyecareTheme.bgPrimary = QStringLiteral("#1A1A2E"); // 主背景（深紫灰）
    eyecareTheme.bgSecondary = QStringLiteral("#16213E"); // 卡片背景（深蓝灰）
    eyecareTheme.bgElevated = QStringLiteral("#0F3460"); // 提升背景（中蓝）
    eyecareTheme.bgSurface = QStringLiteral("#0F3460"); // 表面背景
    eyecareTheme.bgHover = QStringLiteral("#1F4068");
    eyecareTheme.textPrimary = QStringLiteral("#EEEEEE"); // 主文本（暖白）
    eyecareTheme.textSecondary = QStringLiteral("#B8B8B8"); // 次文本（暖灰）
    eyecareTheme.textTertiary = QStringLiteral("#888888"); // 三级文本
    eyecareTheme.textDisabled = QStringLiteral("#666666"); // 禁用文本
    eyecareTheme.primary = QStringLiteral("#4ECCA3"); // 主色（青绿）
    eyecareTheme.primaryHover = QStringLiteral("#5FDAB4"); // 主色悬停
    eyecareTheme.primaryDark = QStringLiteral("#3DBB92"); // 主色深色
    eyecareTheme.accent = QStringLiteral("#C4A35A"); // 强调色（暖金）
    eyecareTheme.success = QStringLiteral("#4ECCA3"); // 涨/成功色（青绿）
    eyecareTheme.danger = QStringLiteral("#E8505B"); // 跌/危险色（暖红）
    eyecareTheme.warning = QStringLiteral("#FFC93C"); // 警告色（暖黄）
    eyecareTheme.info = QStringLiteral("#4ECCA3"); // 信息色
    eyecareTheme.border = QStringLiteral("#1F4068"); // 边框色
    eyecareTheme.divider = QStringLiteral("#16213E"); // 分割线色
    eyecareTheme.chartUp = QStringLiteral("#E8505B"); // 图表上涨色（暖红）
    eyecareTheme.chartDown = QStringLiteral("#4ECCA3"); // 图表下跌色（青绿）
    eyecareTheme.chartGrid = QStringLiteral("#1F4068"); // 图表网格色
    eyecareTheme.info = QStringLiteral("#D4A574");
    eyecareTheme.border = QStringLiteral("#3D372D");
    eyecareTheme.divider = QStringLiteral("#2A251E");
    eyecareTheme.chartUp = QStringLiteral("#7CB342");
    eyecareTheme.chartDown = QStringLiteral("#E57373");
    eyecareTheme.chartGrid = QStringLiteral("#3D372D");

    m_themes[ThemeType::EyeCare] = eyecareTheme;

    // 高对比度主题
    ThemeColors highContrastTheme;
    highContrastTheme.name = QStringLiteral("高对比度主题");
    highContrastTheme.bgPrimary = QStringLiteral("#000000");
    highContrastTheme.bgSecondary = QStringLiteral("#1A1A1A");
    highContrastTheme.bgElevated = QStringLiteral("#2A2A2A");
    highContrastTheme.bgSurface = QStringLiteral("#333333");
    highContrastTheme.bgHover = QStringLiteral("#444444");
    highContrastTheme.textPrimary = QStringLiteral("#FFFFFF");
    highContrastTheme.textSecondary = QStringLiteral("#CCCCCC");
    highContrastTheme.textTertiary = QStringLiteral("#999999");
    highContrastTheme.textDisabled = QStringLiteral("#666666");
    highContrastTheme.primary = QStringLiteral("#00FF00");
    highContrastTheme.primaryHover = QStringLiteral("#00CC00");
    highContrastTheme.primaryDark = QStringLiteral("#009900");
    highContrastTheme.accent = QStringLiteral("#FFFF00");
    highContrastTheme.success = QStringLiteral("#00FF00");
    highContrastTheme.danger = QStringLiteral("#FF0000");
    highContrastTheme.warning = QStringLiteral("#FFFF00");
    highContrastTheme.info = QStringLiteral("#00FFFF");
    highContrastTheme.border = QStringLiteral("#FFFFFF");
    highContrastTheme.divider = QStringLiteral("#666666");
    highContrastTheme.chartUp = QStringLiteral("#00FF00");
    highContrastTheme.chartDown = QStringLiteral("#FF0000");
    highContrastTheme.chartGrid = QStringLiteral("#666666");

    m_themes[ThemeType::HighContrast] = highContrastTheme;
}

ThemeColors ThemeManager::fromJsonObject(const QJsonObject& json) const
{
    ThemeColors theme;

    theme.name = json[QStringLiteral("name")].toString();
    theme.bgPrimary = json[QStringLiteral("bgPrimary")].toString();
    theme.bgSecondary = json[QStringLiteral("bgSecondary")].toString();
    theme.bgElevated = json[QStringLiteral("bgElevated")].toString();
    theme.bgSurface = json[QStringLiteral("bgSurface")].toString();
    theme.textPrimary = json[QStringLiteral("textPrimary")].toString();
    theme.textSecondary = json[QStringLiteral("textSecondary")].toString();
    theme.textDisabled = json[QStringLiteral("textDisabled")].toString();
    theme.primary = json[QStringLiteral("primary")].toString();
    theme.primaryHover = json[QStringLiteral("primaryHover")].toString();
    theme.accent = json[QStringLiteral("accent")].toString();
    theme.success = json[QStringLiteral("success")].toString();
    theme.danger = json[QStringLiteral("danger")].toString();
    theme.warning = json[QStringLiteral("warning")].toString();
    theme.info = json[QStringLiteral("info")].toString();
    theme.border = json[QStringLiteral("border")].toString();
    theme.divider = json[QStringLiteral("divider")].toString();
    theme.chartUp = json[QStringLiteral("chartUp")].toString();
    theme.chartDown = json[QStringLiteral("chartDown")].toString();
    theme.chartGrid = json[QStringLiteral("chartGrid")].toString();

    return theme;
}

QJsonObject ThemeManager::toJsonObject(const ThemeColors& theme) const
{
    QJsonObject json;

    json[QStringLiteral("name")] = theme.name;
    json[QStringLiteral("bgPrimary")] = theme.bgPrimary;
    json[QStringLiteral("bgSecondary")] = theme.bgSecondary;
    json[QStringLiteral("bgElevated")] = theme.bgElevated;
    json[QStringLiteral("bgSurface")] = theme.bgSurface;
    json[QStringLiteral("textPrimary")] = theme.textPrimary;
    json[QStringLiteral("textSecondary")] = theme.textSecondary;
    json[QStringLiteral("textDisabled")] = theme.textDisabled;
    json[QStringLiteral("primary")] = theme.primary;
    json[QStringLiteral("primaryHover")] = theme.primaryHover;
    json[QStringLiteral("accent")] = theme.accent;
    json[QStringLiteral("success")] = theme.success;
    json[QStringLiteral("danger")] = theme.danger;
    json[QStringLiteral("warning")] = theme.warning;
    json[QStringLiteral("info")] = theme.info;
    json[QStringLiteral("border")] = theme.border;
    json[QStringLiteral("divider")] = theme.divider;
    json[QStringLiteral("chartUp")] = theme.chartUp;
    json[QStringLiteral("chartDown")] = theme.chartDown;
    json[QStringLiteral("chartGrid")] = theme.chartGrid;

    return json;
}
