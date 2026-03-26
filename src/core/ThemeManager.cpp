#include "ThemeManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QDebug>
#include <QApplication>

// 单例实例
ThemeManager* ThemeManager::m_instance = nullptr;
ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
    , m_currentTheme(ThemeType::Light)
{
    loadBuiltinThemes();
}
ThemeManager::~ThemeManager()
{
}
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
    if (m_currentTheme == type) {
        return;
    }
    m_currentTheme = type;
    applyTheme(type);
    emit themeChanged(type);
}
void ThemeManager::loadBuiltinThemes()
{
    // 将 qss 文件添加到资源文件 中
    loadThemeFromFile(ThemeType::Light, ":/style/theme_light.qss");
    loadThemeFromFile(ThemeType::Dark, ":/style/theme_dark.qss");
}

// 文件解析与加载实现
bool ThemeManager::loadThemeFromFile(ThemeType type, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load theme file:" << filePath;
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    ThemeConfig config;

    // 1. 解析颜色变量
    // 我们在 QSS 文件头部定义颜色变量，格式为：/* @key: #RRGGBB */
    // 示例: /* @rise: #E74C3C */
    QRegularExpression colorRegex(R"(\/\*\s*@(\w+):\s*(#[A-Fa-f0-9]{6,8})\s*\*\/)");
    QRegularExpressionMatchIterator it = colorRegex.globalMatch(content);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1);    // 捕获键，如 "rise"
        QString hex = match.captured(2);    // 捕获值，如 "#E74C3C"
        config.colors[key] = QColor(hex);
    }

    // 2. 设置样式表
    // 移除注释块，保留纯净的 CSS，或者直接使用全部内容（Qt 会忽略注释）
    // 为了性能和纯净性，这里直接使用全部内容，Qt 引擎会自动处理注释
    config.styleSheet = content;

    // 3. 存入主题表
    m_themes[type] = config;

    return true;
}

void ThemeManager::applyTheme(ThemeType type)
{
    // 这里可以添加额外的主题应用逻辑
    // 例如更新QApplication的样式表等
    qApp->setStyleSheet(m_themes[type].styleSheet);
}
QColor ThemeManager::backgroundColor() const
{
    return color("background");
}
QColor ThemeManager::foregroundColor() const
{
    return color("foreground");
}
QColor ThemeManager::accentColor() const
{
    return color("accent");
}
QColor ThemeManager::riseColor() const
{
    return color("rise");
}
QColor ThemeManager::fallColor() const
{
    return color("fall");
}
QColor ThemeManager::neutralColor() const
{
    return color("neutral");
}
QColor ThemeManager::borderColor() const
{
    return color("border");
}
QColor ThemeManager::cardColor() const
{
    return color("card");
}
QColor ThemeManager::color(const QString& role) const
{
    if (m_themes.contains(m_currentTheme)) {
        return m_themes[m_currentTheme].colors.value(role);
    }
    return QColor();
}
QString ThemeManager::themeStyleSheet() const
{
    if (m_themes.contains(m_currentTheme)) {
        return m_themes[m_currentTheme].styleSheet;
    }
    return QString();
}
void ThemeManager::registerCustomTheme(const QString& name, const QJsonObject& config)
{
    ThemeConfig customTheme;
    // 解析颜色配置
    QJsonObject colors = config["colors"].toObject();
    for (auto it = colors.begin(); it != colors.end(); ++it) {
        customTheme.colors[it.key()] = QColor(it.value().toString());
    }
    // 解析样式表
    customTheme.styleSheet = config["styleSheet"].toString();
    m_themes[ThemeType::Custom] = customTheme;
    m_customThemeMap[name] = ThemeType::Custom;
}
