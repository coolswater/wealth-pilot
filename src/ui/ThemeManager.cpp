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

    // 设置默认主题
    setTheme(ThemeType::Dark);

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
    // 加载基础样式
    QString styleSheet = loadBaseQss();
    
    // 加载主题特定样式
    styleSheet += "\n" + loadThemeQss(m_currentType);
    
    // 替换颜色变量
    styleSheet = replaceColorVariables(styleSheet, m_currentTheme);
    
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
    
    return loadQssFile(fileName);
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
    // 深色主题 - WealthPilot设计规范
    ThemeColors darkTheme;
    darkTheme.name = QStringLiteral("深色主题");
    darkTheme.bgPrimary = QStringLiteral("#0d1117");      // 主背景色
    darkTheme.bgSecondary = QStringLiteral("#161b22");    // 卡片背景色
    darkTheme.bgElevated = QStringLiteral("#161b22");     // 提升背景色（卡片）
    darkTheme.bgSurface = QStringLiteral("#1c2128");      // 表面背景色
    darkTheme.bgHover = QStringLiteral("rgba(255, 255, 255, 0.05)");
    darkTheme.textPrimary = QStringLiteral("#e6edf3");    // 主文本色
    darkTheme.textSecondary = QStringLiteral("#8b949e");  // 次文本色
    darkTheme.textTertiary = QStringLiteral("#6e7681");   // 三级文本色
    darkTheme.textDisabled = QStringLiteral("#484f58");   // 禁用文本色
    darkTheme.primary = QStringLiteral("#58a6ff");        // 主品牌色
    darkTheme.primaryHover = QStringLiteral("#79c0ff");   // 主色调悬停
    darkTheme.primaryDark = QStringLiteral("#1f6feb");    // 主色调深色
    darkTheme.accent = QStringLiteral("#a371f7");         // 强调色
    darkTheme.success = QStringLiteral("#3fb950");        // 涨/成功色
    darkTheme.danger = QStringLiteral("#f85149");         // 跌/危险色
    darkTheme.warning = QStringLiteral("#f0883e");        // 警告色
    darkTheme.info = QStringLiteral("#58a6ff");           // 信息色
    darkTheme.border = QStringLiteral("#30363d");         // 边框色
    darkTheme.divider = QStringLiteral("#21262d");        // 分割线色
    darkTheme.chartUp = QStringLiteral("#3fb950");        // 图表上涨色
    darkTheme.chartDown = QStringLiteral("#f85149");      // 图表下跌色
    darkTheme.chartGrid = QStringLiteral("#30363d");      // 图表网格色

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
    eyecareTheme.bgPrimary = QStringLiteral("#1E1A14");
    eyecareTheme.bgSecondary = QStringLiteral("#2A251E");
    eyecareTheme.bgElevated = QStringLiteral("#352F26");
    eyecareTheme.bgSurface = QStringLiteral("#3D372D");
    eyecareTheme.bgHover = QStringLiteral("#453F35");
    eyecareTheme.textPrimary = QStringLiteral("#E8DCC8");
    eyecareTheme.textSecondary = QStringLiteral("#A89B85");
    eyecareTheme.textTertiary = QStringLiteral("#8B7D66");
    eyecareTheme.textDisabled = QStringLiteral("#6B5F4F");
    eyecareTheme.primary = QStringLiteral("#D4A574");
    eyecareTheme.primaryHover = QStringLiteral("#E5B785");
    eyecareTheme.primaryDark = QStringLiteral("#C49564");
    eyecareTheme.accent = QStringLiteral("#C49564");
    eyecareTheme.success = QStringLiteral("#7CB342");
    eyecareTheme.danger = QStringLiteral("#E57373");
    eyecareTheme.warning = QStringLiteral("#FFB74D");
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
