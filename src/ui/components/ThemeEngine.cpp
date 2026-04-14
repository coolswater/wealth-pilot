/**
 * @file ThemeEngine.cpp
 * @brief 主题引擎实现 - 高性能主题管理系统
 */

#include "ThemeEngine.h"
#include "../../utils/Logger.h"
#include <QApplication>
#include <QWidget>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QElapsedTimer>

/**
 * @brief 构造函数
 */
ThemeEngine::ThemeEngine()
    : m_currentTheme("dark")
{
    LOG_DEBUG("ThemeEngine created");
}

ThemeEngine& ThemeEngine::instance()
{
    static ThemeEngine instance;
    return instance;
}

bool ThemeEngine::initialize()
{
    QElapsedTimer timer;
    timer.start();
    
    QMutexLocker locker(&m_mutex);
    
    // 加载主题配置
    loadThemes();
    
    // 加载当前主题
    QSettings settings;
    m_currentTheme = settings.value("theme/current", "dark").toString();
    
    // 应用当前主题
    if (m_themes.contains(m_currentTheme)) {
        m_currentConfig = m_themes[m_currentTheme];
    } else {
        // 默认使用深色主题
        m_currentTheme = "dark";
        m_currentConfig = m_themes["dark"];
    }
    
    // 构建样式缓存（性能优化）
    updateStyleCache();
    buildPropertyLookupTable();
    
    LOG_INFO(QString("ThemeEngine initialized in %1ms, current theme: %2")
        .arg(timer.elapsed()).arg(m_currentTheme));
    
    return true;
}

QString ThemeEngine::currentTheme() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentTheme;
}

void ThemeEngine::setCurrentTheme(const QString& themeName)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_currentTheme == themeName) {
        return;
    }
    
    if (!m_themes.contains(themeName)) {
        LOG_WARNING(QString("Theme not found: %1").arg(themeName));
        return;
    }
    
    m_currentTheme = themeName;
    m_currentConfig = m_themes[themeName];
    
    // 更新样式缓存
    updateStyleCache();
    
    // 保存设置
    QSettings settings;
    settings.setValue("theme/current", themeName);
    
    LOG_INFO(QString("Theme changed to: %1").arg(themeName));
    
    locker.unlock();
    emit themeChanged(themeName);
}

const UIComponents::ThemeConfig& ThemeEngine::themeConfig() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentConfig;
}

void ThemeEngine::registerTheme(const QString& name, const UIComponents::ThemeConfig& config)
{
    QMutexLocker locker(&m_mutex);
    m_themes[name] = config;
    LOG_DEBUG(QString("Theme registered: %1").arg(name));
}

QVariant ThemeEngine::styleProperty(const QString& key, const QVariant& defaultValue) const
{
    // 使用查找表快速访问（性能优化）
    auto it = m_propertyLookup.find(key);
    if (it != m_propertyLookup.end()) {
        // 直接索引访问，避免字符串比较
        return m_styleCache.value(key, defaultValue);
    }
    return defaultValue;
}

void ThemeEngine::applyTheme(QWidget* widget)
{
    if (!widget) return;
    
    // 应用预编译样式表
    widget->setStyleSheet(m_compiledStyleSheet);
}

QString ThemeEngine::compiledStyleSheet() const
{
    return m_compiledStyleSheet;
}

void ThemeEngine::loadThemes()
{
    // 深色主题
    UIComponents::ThemeConfig darkTheme;
    darkTheme.name = "dark";
    darkTheme.primaryColor = QColor("#3B82F6");
    darkTheme.secondaryColor = QColor("#8B5CF6");
    darkTheme.backgroundColor = QColor("#1A1A2E");
    darkTheme.surfaceColor = QColor("#16213E");
    darkTheme.onPrimaryColor = QColor("#FFFFFF");
    darkTheme.onSecondaryColor = QColor("#FFFFFF");
    darkTheme.onBackgroundColor = QColor("#E2E8F0");
    darkTheme.onSurfaceColor = QColor("#E2E8F0");
    darkTheme.baseFont = QFont("Microsoft YaHei", 10);
    darkTheme.titleFont = QFont("Microsoft YaHei", 14, QFont::Bold);
    darkTheme.bodyFont = QFont("Microsoft YaHei", 10);
    darkTheme.borderRadius = 8;
    darkTheme.spacing = 8;
    darkTheme.padding = 16;
    darkTheme.elevation = 4.0;
    m_themes["dark"] = darkTheme;
    
    // 浅色主题
    UIComponents::ThemeConfig lightTheme;
    lightTheme.name = "light";
    lightTheme.primaryColor = QColor("#2563EB");
    lightTheme.secondaryColor = QColor("#7C3AED");
    lightTheme.backgroundColor = QColor("#F8FAFC");
    lightTheme.surfaceColor = QColor("#FFFFFF");
    lightTheme.onPrimaryColor = QColor("#FFFFFF");
    lightTheme.onSecondaryColor = QColor("#FFFFFF");
    lightTheme.onBackgroundColor = QColor("#1E293B");
    lightTheme.onSurfaceColor = QColor("#1E293B");
    lightTheme.baseFont = QFont("Microsoft YaHei", 10);
    lightTheme.titleFont = QFont("Microsoft YaHei", 14, QFont::Bold);
    lightTheme.bodyFont = QFont("Microsoft YaHei", 10);
    lightTheme.borderRadius = 8;
    lightTheme.spacing = 8;
    lightTheme.padding = 16;
    lightTheme.elevation = 2.0;
    m_themes["light"] = lightTheme;
    
    // 护眼主题
    UIComponents::ThemeConfig eyecareTheme;
    eyecareTheme.name = "eyecare";
    eyecareTheme.primaryColor = QColor("#059669");
    eyecareTheme.secondaryColor = QColor("#0891B2");
    eyecareTheme.backgroundColor = QColor("#F0FDF4");
    eyecareTheme.surfaceColor = QColor("#ECFDF5");
    eyecareTheme.onPrimaryColor = QColor("#FFFFFF");
    eyecareTheme.onSecondaryColor = QColor("#FFFFFF");
    eyecareTheme.onBackgroundColor = QColor("#1F2937");
    eyecareTheme.onSurfaceColor = QColor("#1F2937");
    eyecareTheme.baseFont = QFont("Microsoft YaHei", 10);
    eyecareTheme.titleFont = QFont("Microsoft YaHei", 14, QFont::Bold);
    eyecareTheme.bodyFont = QFont("Microsoft YaHei", 10);
    eyecareTheme.borderRadius = 8;
    eyecareTheme.spacing = 8;
    eyecareTheme.padding = 16;
    eyecareTheme.elevation = 2.0;
    m_themes["eyecare"] = eyecareTheme;
}

void ThemeEngine::saveCurrentTheme()
{
    QSettings settings;
    settings.setValue("theme/current", m_currentTheme);
}

void ThemeEngine::updateStyleCache()
{
    m_styleCache.clear();
    
    // 缓存常用样式属性
    m_styleCache["primaryColor"] = m_currentConfig.primaryColor.name();
    m_styleCache["secondaryColor"] = m_currentConfig.secondaryColor.name();
    m_styleCache["backgroundColor"] = m_currentConfig.backgroundColor.name();
    m_styleCache["surfaceColor"] = m_currentConfig.surfaceColor.name();
    m_styleCache["textColor"] = m_currentConfig.onSurfaceColor.name();
    m_styleCache["borderRadius"] = m_currentConfig.borderRadius;
    m_styleCache["spacing"] = m_currentConfig.spacing;
    m_styleCache["padding"] = m_currentConfig.padding;
    
    // 生成预编译样式表
    m_compiledStyleSheet = generateStyleSheet();
}

void ThemeEngine::buildPropertyLookupTable()
{
    m_propertyLookup.clear();
    
    // 建立字符串到索引的映射（性能优化）
    int index = 0;
    for (auto it = m_styleCache.begin(); it != m_styleCache.end(); ++it) {
        m_propertyLookup[it.key()] = index++;
    }
}

QString ThemeEngine::generateStyleSheet() const
{
    QString style;
    
    // 生成全局样式
    style = QString(R"(
        QWidget {
            background-color: %1;
            color: %2;
            font-family: "Microsoft YaHei";
            font-size: 10pt;
        }
        
        QPushButton {
            background-color: %3;
            color: %4;
            border: none;
            border-radius: %5px;
            padding: 8px 16px;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: %6;
        }
        
        QPushButton:pressed {
            background-color: %7;
        }
        
        QLineEdit, QTextEdit {
            background-color: %8;
            color: %9;
            border: 1px solid %10;
            border-radius: %11px;
            padding: 8px;
        }
        
        QLineEdit:focus, QTextEdit:focus {
            border: 2px solid %12;
        }
        
        QTableWidget {
            background-color: %13;
            color: %14;
            gridline-color: %15;
            border: none;
        }
        
        QHeaderView::section {
            background-color: %16;
            color: %17;
            padding: 8px;
            border: none;
            border-bottom: 1px solid %18;
        }
    )")
    .arg(m_currentConfig.backgroundColor.name())
    .arg(m_currentConfig.onBackgroundColor.name())
    .arg(m_currentConfig.primaryColor.name())
    .arg(m_currentConfig.onPrimaryColor.name())
    .arg(m_currentConfig.borderRadius)
    .arg(m_currentConfig.primaryColor.lighter(110).name())
    .arg(m_currentConfig.primaryColor.darker(110).name())
    .arg(m_currentConfig.surfaceColor.name())
    .arg(m_currentConfig.onSurfaceColor.name())
    .arg(m_currentConfig.primaryColor.lighter(150).name())
    .arg(m_currentConfig.borderRadius)
    .arg(m_currentConfig.primaryColor.name())
    .arg(m_currentConfig.surfaceColor.name())
    .arg(m_currentConfig.onSurfaceColor.name())
    .arg(m_currentConfig.primaryColor.lighter(180).name())
    .arg(m_currentConfig.surfaceColor.name())
    .arg(m_currentConfig.onSurfaceColor.name())
    .arg(m_currentConfig.primaryColor.lighter(150).name());
    
    return style;
}
