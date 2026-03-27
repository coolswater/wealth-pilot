#include "ThemeManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QApplication>
#include <QDebug>

// 静态成员初始化
ThemeManager* ThemeManager::m_instance = nullptr;

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
    , m_currentTheme(ThemeType::Light)
{
    loadBuiltinThemes();
}

ThemeManager::~ThemeManager() = default;

ThemeManager* ThemeManager::instance()
{
    if (!m_instance) {
        m_instance = new ThemeManager();
    }
    return m_instance;
}

ThemeManager::ThemeType ThemeManager::currentTheme() const
{
    return m_currentTheme;
}

void ThemeManager::setTheme(ThemeType type)
{
    if (m_currentTheme == type)
        return;

    m_currentTheme = type;
    applyTheme(type);
    emit themeChanged(type);
}

void ThemeManager::registerCustomTheme(const QString& name, const QJsonObject& config)
{
    // 为自定义主题分配一个临时类型（实际存储为 Custom，但通过名称区分）
    ThemeType customType = ThemeType::Custom;
    if (!m_customThemeMap.contains(name)) {
        // 注意：为了支持多个自定义主题，我们使用 QMap<QString, QMap<QString, QColor>> 存储额外配置
        // 但这里简化：将自定义主题配置存入同一个 Custom 条目，但用名称索引。
        // 实际上我们可以扩展为 QMap<QString, ThemeConfig>，但为了保持枚举简单，暂且这样。
        // 更严谨的做法是放弃枚举，用字符串标识主题，但为了兼容原有代码，保留枚举。
        // 这里采用：Custom 只存储当前激活的自定义主题配置，通过 m_customThemeMap 记录名称。
        // 在 setCustomTheme 中动态加载。
        m_customThemeMap[name] = ThemeType::Custom;
    }

    // 解析颜色
    QJsonObject colorsObj = config["colors"].toObject();
    QMap<QString, QColor> colorMap = parseColorsFromJson(colorsObj);

    // 存储配置到某个地方，这里为了简单，直接存入 m_colorMap[Custom] 但会覆盖，所以需要额外存储
    // 为支持多自定义主题，可以改为 QMap<QString, QMap<QString, QColor>> m_customColorMap。
    // 为简化，我们仅允许一个激活的自定义主题，通过 setCustomTheme 动态替换。
    // 这里我们将配置暂存，在 setCustomTheme 时应用。
    // 实际开发中建议改为字符串标识的主题系统，此处为了演示，仅提供框架。
    qWarning() << "Custom theme registration not fully implemented. Use setCustomTheme with JSON file.";
}

void ThemeManager::setCustomTheme(const QString& name)
{
    if (!m_customThemeMap.contains(name)) {
        qWarning() << "Custom theme not registered:" << name;
        return;
    }
    // 此处应该加载之前注册的自定义主题配置并应用。
    // 实际实现需要从存储中读取颜色和样式表。
    // 本示例简化，仅发出信号。
    qDebug() << "Switching to custom theme:" << name;
    // 实际应用中，需要设置 m_currentTheme = ThemeType::Custom 并更新内部配置。
    // emit themeChanged(ThemeType::Custom);
}

QColor ThemeManager::color(const QString& role) const
{
    if (m_colorMap.contains(m_currentTheme)) {
        return m_colorMap[m_currentTheme].value(role);
    }
    return QColor();
}

QString ThemeManager::styleSheet() const
{
    return m_styleSheets.value(m_currentTheme);
}

void ThemeManager::loadBuiltinThemes()
{
    // 内置亮色主题配置
    QJsonObject lightColors;
    lightColors["background"] = "#F8FAFC";
    lightColors["foreground"] = "#1F2937";
    lightColors["accent"] = "#3B82F6";
    lightColors["rise"] = "#10B981";
    lightColors["fall"] = "#EF4444";
    lightColors["neutral"] = "#9CA3AF";
    lightColors["border"] = "#E5E7EB";
    lightColors["card"] = "#FFFFFF";

    QJsonObject lightTheme;
    lightTheme["colors"] = lightColors;
    lightTheme["styleSheetPath"] = ":/style/theme_light.qss"; // 可选的样式表路径

    // 内置暗色主题配置
    QJsonObject darkColors;
    darkColors["background"] = "#1A1F2E";
    darkColors["foreground"] = "#FFFFFF";
    darkColors["accent"] = "#3B82F6";
    darkColors["rise"] = "#10B981";
    darkColors["fall"] = "#EF4444";
    darkColors["neutral"] = "#9CA3AF";
    darkColors["border"] = "rgba(255,255,255,0.1)";
    darkColors["card"] = "#242937";

    QJsonObject darkTheme;
    darkTheme["colors"] = darkColors;
    darkTheme["styleSheetPath"] = ":/style/theme_dark.qss";

    // 内置护眼主题配置
    QJsonObject eyeCareColors;
    eyeCareColors["background"] = "#1E1A14";
    eyeCareColors["foreground"] = "#E8DCC8";
    eyeCareColors["accent"] = "#D4A574";
    eyeCareColors["rise"] = "#7CB342";
    eyeCareColors["fall"] = "#E57373";
    eyeCareColors["neutral"] = "#A89B85";
    eyeCareColors["border"] = "#3D352B";
    eyeCareColors["card"] = "#2A241E";

    QJsonObject eyeCareTheme;
    eyeCareTheme["colors"] = eyeCareColors;
    eyeCareTheme["styleSheetPath"] = ":/style/theme_eyecare.qss";

    loadThemeFromJson(ThemeType::Light, lightTheme);
    loadThemeFromJson(ThemeType::Dark, darkTheme);
    loadThemeFromJson(ThemeType::EyeCare, eyeCareTheme);

    // 应用默认主题（亮色）
    applyTheme(ThemeType::Dark);
}

bool ThemeManager::loadThemeFromJson(ThemeType type, const QJsonObject& json)
{
    // 解析颜色
    QJsonObject colorsObj = json["colors"].toObject();
    QMap<QString, QColor> colorMap = parseColorsFromJson(colorsObj);
    m_colorMap[type] = colorMap;

    // 解析样式表（从文件加载）
    QString styleSheetPath = json["styleSheetPath"].toString();
    if (!styleSheetPath.isEmpty()) {
        QFile file(styleSheetPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString styleSheet = QString::fromUtf8(file.readAll());
            m_styleSheets[type] = styleSheet;
            file.close();
            return true;
        } else {
            qWarning() << "Failed to load style sheet:" << styleSheetPath;
            return false;
        }
    } else {
        // 如果没有样式表文件，可以使用默认样式或生成一个简单的样式
        m_styleSheets[type] = "";
        return true;
    }
}

void ThemeManager::applyTheme(ThemeType type)
{
    if (!m_styleSheets.contains(type)) {
        qWarning() << "No style sheet for theme" << static_cast<int>(type);
        return;
    }
    qApp->setStyleSheet(m_styleSheets[type]);
}

QMap<QString, QColor> ThemeManager::parseColorsFromJson(const QJsonObject& colorsObj) const
{
    QMap<QString, QColor> result;
    for (auto it = colorsObj.begin(); it != colorsObj.end(); ++it) {
        QString colorStr = it.value().toString();
        if (QColor::isValidColor(colorStr)) {
            result[it.key()] = QColor(colorStr);
        } else {
            qWarning() << "Invalid color value for key" << it.key() << ":" << colorStr;
        }
    }
    return result;
}
