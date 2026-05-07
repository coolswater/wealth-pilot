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
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));

    qApp->setPalette(palette);

    // 应用样式表
    qApp->setStyleSheet(getThemeStyleSheet());

    // 通知所有监听器
    for (const auto& listener : m_listeners) {
        if (listener.first) {
            listener.second();
        }
    }
}

QString ThemeManager::getThemeStyleSheet() const
{
    QString style;

    // 全局样式
    style += QString(
        "QWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "}"
    ).arg(m_currentTheme.bgPrimary, m_currentTheme.textPrimary);

    // 按钮样式
    style += QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: white;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %2;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %3;"
        "}"
    ).arg(m_currentTheme.primary, m_currentTheme.primaryHover, m_currentTheme.primary);

    // 输入框样式
    style += QString(
        "QLineEdit, QTextEdit, QPlainTextEdit {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {"
        "  border-color: %4;"
        "}"
    ).arg(m_currentTheme.bgSecondary, m_currentTheme.textPrimary,
          m_currentTheme.border, m_currentTheme.primary);

    // 表格样式
    style += QString(
        "QTableWidget {"
        "  background-color: %1;"
        "  color: %2;"
        "  gridline-color: %3;"
        "}"
        "QTableWidget::item {"
        "  padding: 4px;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: %4;"
        "  color: white;"
        "}"
    ).arg(m_currentTheme.bgSecondary, m_currentTheme.textPrimary,
          m_currentTheme.border, m_currentTheme.primary);

    // 滚动条样式
    style += QString(
        "QScrollBar:vertical {"
        "  background-color: %1;"
        "  width: 10px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background-color: %2;"
        "  border-radius: 5px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    ).arg(m_currentTheme.bgSecondary, m_currentTheme.border);

    return style;
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
    // 深色主题
    ThemeColors darkTheme;
    darkTheme.name = QStringLiteral("深色主题");
    darkTheme.bgPrimary = QStringLiteral("#1E1E1E");
    darkTheme.bgSecondary = QStringLiteral("#252526");
    darkTheme.bgElevated = QStringLiteral("#2D2D30");
    darkTheme.bgSurface = QStringLiteral("#333333");
    darkTheme.textPrimary = QStringLiteral("#FFFFFF");
    darkTheme.textSecondary = QStringLiteral("#9CA3AF");
    darkTheme.textDisabled = QStringLiteral("#6B7280");
    darkTheme.primary = QStringLiteral("#3B82F6");
    darkTheme.primaryHover = QStringLiteral("#2563EB");
    darkTheme.accent = QStringLiteral("#8B5CF6");
    darkTheme.success = QStringLiteral("#10B981");
    darkTheme.danger = QStringLiteral("#EF4444");
    darkTheme.warning = QStringLiteral("#F59E0B");
    darkTheme.info = QStringLiteral("#3B82F6");
    darkTheme.border = QStringLiteral("#404040");
    darkTheme.divider = QStringLiteral("#303030");
    darkTheme.chartUp = QStringLiteral("#10B981");
    darkTheme.chartDown = QStringLiteral("#EF4444");
    darkTheme.chartGrid = QStringLiteral("#404040");

    m_themes[ThemeType::Dark] = darkTheme;

    // 浅色主题
    ThemeColors lightTheme;
    lightTheme.name = QStringLiteral("浅色主题");
    lightTheme.bgPrimary = QStringLiteral("#FFFFFF");
    lightTheme.bgSecondary = QStringLiteral("#F3F4F6");
    lightTheme.bgElevated = QStringLiteral("#FFFFFF");
    lightTheme.bgSurface = QStringLiteral("#F9FAFB");
    lightTheme.textPrimary = QStringLiteral("#1F2937");
    lightTheme.textSecondary = QStringLiteral("#6B7280");
    lightTheme.textDisabled = QStringLiteral("#9CA3AF");
    lightTheme.primary = QStringLiteral("#3B82F6");
    lightTheme.primaryHover = QStringLiteral("#2563EB");
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

    // 高对比度主题
    ThemeColors highContrastTheme;
    highContrastTheme.name = QStringLiteral("高对比度主题");
    highContrastTheme.bgPrimary = QStringLiteral("#000000");
    highContrastTheme.bgSecondary = QStringLiteral("#1A1A1A");
    highContrastTheme.bgElevated = QStringLiteral("#2A2A2A");
    highContrastTheme.bgSurface = QStringLiteral("#333333");
    highContrastTheme.textPrimary = QStringLiteral("#FFFFFF");
    highContrastTheme.textSecondary = QStringLiteral("#CCCCCC");
    highContrastTheme.textDisabled = QStringLiteral("#888888");
    highContrastTheme.primary = QStringLiteral("#00FF00");
    highContrastTheme.primaryHover = QStringLiteral("#00CC00");
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
