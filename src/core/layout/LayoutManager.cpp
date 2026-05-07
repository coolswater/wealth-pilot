/**
 * @file LayoutManager.cpp
 * @brief 布局管理器实现
 */

#include "LayoutManager.h"
#include "utils/Logger.h"
#include <QWidget>
#include <QMainWindow>
#include <QSplitter>
#include <QApplication>
#include <QScreen>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>

LayoutManager* LayoutManager::instance()
{
    static LayoutManager* inst = new LayoutManager();
    return inst;
}

LayoutManager::LayoutManager(QObject* parent)
    : QObject(parent)
    , m_settings(new QSettings(QStringLiteral("WealthPilot"), QStringLiteral("Layout"), this))
{
}

bool LayoutManager::initialize()
{
    if (m_initialized) return true;

    LOG_INFO("Initializing Layout Manager");

    // 初始化默认模板
    initDefaultTemplates();

    // 加载已保存的模板
    QStringList templateIds = m_settings->value(QStringLiteral("templates")).toStringList();
    for (const QString& id : templateIds) {
        LayoutTemplate layout = loadFromSettings(id);
        if (!layout.id.isEmpty()) {
            m_templates[id] = layout;
        }
    }

    // 恢复上次布局
    QString lastLayout = m_settings->value(QStringLiteral("lastLayout")).toString();
    if (!lastLayout.isEmpty()) {
        m_currentLayoutId = lastLayout;
    }

    m_initialized = true;
    LOG_INFO("Layout Manager initialized");
    return true;
}

bool LayoutManager::saveCurrentLayout(const QString& name)
{
    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(QApplication::topLevelWidgets().first());

    if (!mainWindow) {
        LOG_ERROR("No main window found");
        return false;
    }

    LayoutTemplate layout;
    layout.id = generateTemplateId();
    layout.name = name;
    layout.createTime = QDateTime::currentDateTime();
    layout.updateTime = QDateTime::currentDateTime();

    // 保存主窗口布局
    WindowLayout mainWindowLayout;
    mainWindowLayout.name = QStringLiteral("MainWindow");
    mainWindowLayout.className = QStringLiteral("QMainWindow");
    mainWindowLayout.geometry = mainWindow->geometry();
    mainWindowLayout.visible = mainWindow->isVisible();
    mainWindowLayout.maximized = mainWindow->isMaximized();
    mainWindowLayout.screenIndex = QApplication::screens().indexOf(mainWindow->screen());
    layout.windows.append(mainWindowLayout);

    // 保存分割器布局
    saveSplitters(mainWindow, layout);

    // 保存模板
    m_templates[layout.id] = layout;
    saveToSettings(layout);

    LOG_INFO(QString("Layout saved: %1 (%2)").arg(name, layout.id));
    emit layoutSaved(layout.id);

    return true;
}

bool LayoutManager::loadLayout(const QString& templateId)
{
    if (!m_templates.contains(templateId)) {
        LOG_ERROR(QString("Layout template not found: %1").arg(templateId));
        return false;
    }

    QMainWindow* mainWindow = qobject_cast<QMainWindow*>(QApplication::topLevelWidgets().first());
    if (!mainWindow) {
        LOG_ERROR("No main window found");
        return false;
    }

    const LayoutTemplate& layout = m_templates[templateId];
    applyLayout(mainWindow, layout);

    m_currentLayoutId = templateId;
    m_settings->setValue(QStringLiteral("lastLayout"), templateId);

    LOG_INFO(QString("Layout loaded: %1").arg(templateId));
    emit layoutLoaded(templateId);

    return true;
}

bool LayoutManager::deleteLayoutTemplate(const QString& templateId)
{
    if (!m_templates.contains(templateId)) {
        return false;
    }

    if (m_templates[templateId].isDefault) {
        LOG_WARNING(QString("Cannot delete default template: %1").arg(templateId));
        return false;
    }

    m_templates.remove(templateId);
    m_settings->remove(QStringLiteral("template_") + templateId);

    // 更新模板列表
    QStringList templateIds;
    for (const auto& layout : m_templates) {
        templateIds.append(layout.id);
    }
    m_settings->setValue(QStringLiteral("templates"), templateIds);

    LOG_INFO(QString("Layout deleted: %1").arg(templateId));
    emit layoutDeleted(templateId);

    return true;
}

QVector<LayoutTemplate> LayoutManager::getLayoutTemplates() const
{
    return m_templates.values().toVector();
}

LayoutTemplate LayoutManager::getLayoutTemplate(const QString& templateId) const
{
    return m_templates.value(templateId);
}

void LayoutManager::setDefaultLayout(const QString& templateId)
{
    if (!m_templates.contains(templateId)) {
        return;
    }

    for (auto& layout : m_templates) {
        layout.isDefault = (layout.id == templateId);
        saveToSettings(layout);
    }

    LOG_INFO(QString("Default layout set: %1").arg(templateId));
}

void LayoutManager::applyLayout(QMainWindow* mainWindow, const LayoutTemplate& layout)
{
    // 应用窗口布局
    for (const WindowLayout& windowLayout : layout.windows) {
        if (windowLayout.name == QStringLiteral("MainWindow")) {
            // 检查显示器是否存在
            QVector<QScreen*> screens = QApplication::screens();
            if (windowLayout.screenIndex >= 0 && windowLayout.screenIndex < screens.size()) {
                mainWindow->setScreen(screens[windowLayout.screenIndex]);
            }

            // 恢复几何信息
            if (windowLayout.maximized) {
                mainWindow->showMaximized();
            } else {
                mainWindow->setGeometry(windowLayout.geometry);
                mainWindow->showNormal();
            }

            mainWindow->setVisible(windowLayout.visible);
        }
    }

    // 恢复分割器布局
    restoreSplitters(mainWindow, layout);
}

WindowLayout LayoutManager::saveWindowLayout(QWidget* window) const
{
    WindowLayout layout;
    layout.name = window->objectName();
    layout.className = window->metaObject()->className();
    layout.geometry = window->geometry();
    layout.visible = window->isVisible();
    layout.maximized = window->isMaximized();
    layout.screenIndex = QApplication::screens().indexOf(window->screen());

    return layout;
}

void LayoutManager::restoreWindowLayout(QWidget* window, const WindowLayout& layout)
{
    window->setObjectName(layout.name);

    if (layout.maximized) {
        window->showMaximized();
    } else {
        window->setGeometry(layout.geometry);
        window->showNormal();
    }

    window->setVisible(layout.visible);
}

QVector<MonitorInfo> LayoutManager::getMonitorInfo() const
{
    QVector<MonitorInfo> monitors;
    QVector<QScreen*> screens = QApplication::screens();

    for (int i = 0; i < screens.size(); ++i) {
        QScreen* screen = screens[i];

        MonitorInfo info;
        info.index = i;
        info.name = screen->name();
        info.geometry = screen->geometry();
        info.availableGeometry = screen->availableGeometry();
        info.dpi = screen->logicalDotsPerInch();
        info.isPrimary = (screen == QApplication::primaryScreen());

        monitors.append(info);
    }

    return monitors;
}

int LayoutManager::getPrimaryMonitorIndex() const
{
    QScreen* primary = QApplication::primaryScreen();
    return QApplication::screens().indexOf(primary);
}

bool LayoutManager::exportLayoutTemplate(const QString& templateId, const QString& filePath)
{
    if (!m_templates.contains(templateId)) {
        return false;
    }

    const LayoutTemplate& layout = m_templates[templateId];

    QJsonObject json;
    json[QStringLiteral("id")] = layout.id;
    json[QStringLiteral("name")] = layout.name;
    json[QStringLiteral("description")] = layout.description;
    json[QStringLiteral("category")] = layout.category;
    json[QStringLiteral("createTime")] = layout.createTime.toString(Qt::ISODate);
    json[QStringLiteral("updateTime")] = layout.updateTime.toString(Qt::ISODate);

    // 窗口布局
    QJsonArray windowsArray;
    for (const WindowLayout& window : layout.windows) {
        QJsonObject windowObj;
        windowObj[QStringLiteral("name")] = window.name;
        windowObj[QStringLiteral("className")] = window.className;
        windowObj[QStringLiteral("geometry")] = QString("%1,%2,%3,%4")
            .arg(window.geometry.x())
            .arg(window.geometry.y())
            .arg(window.geometry.width())
            .arg(window.geometry.height());
        windowObj[QStringLiteral("visible")] = window.visible;
        windowObj[QStringLiteral("maximized")] = window.maximized;
        windowObj[QStringLiteral("screenIndex")] = window.screenIndex;
        windowsArray.append(windowObj);
    }
    json[QStringLiteral("windows")] = windowsArray;

    // 分割器布局
    QJsonArray splittersArray;
    for (const SplitterLayout& splitter : layout.splitters) {
        QJsonObject splitterObj;
        splitterObj[QStringLiteral("name")] = splitter.name;

        QJsonArray sizesArray;
        for (int size : splitter.sizes) {
            sizesArray.append(size);
        }
        splitterObj[QStringLiteral("sizes")] = sizesArray;
        splittersArray.append(splitterObj);
    }
    json[QStringLiteral("splitters")] = splittersArray;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file for export: %1").arg(filePath));
        return false;
    }

    file.write(QJsonDocument(json).toJson());
    file.close();

    LOG_INFO(QString("Layout exported: %1").arg(filePath));
    return true;
}

bool LayoutManager::importLayoutTemplate(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open file for import: %1").arg(filePath));
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

    QJsonObject json = doc.object();

    LayoutTemplate layout;
    layout.id = generateTemplateId(); // 生成新ID避免冲突
    layout.name = json[QStringLiteral("name")].toString();
    layout.description = json[QStringLiteral("description")].toString();
    layout.category = json[QStringLiteral("category")].toString();
    layout.createTime = QDateTime::currentDateTime();
    layout.updateTime = QDateTime::currentDateTime();

    // 解析窗口布局
    QJsonArray windowsArray = json[QStringLiteral("windows")].toArray();
    for (const QJsonValue& value : windowsArray) {
        QJsonObject windowObj = value.toObject();

        WindowLayout window;
        window.name = windowObj[QStringLiteral("name")].toString();
        window.className = windowObj[QStringLiteral("className")].toString();

        QString geoStr = windowObj[QStringLiteral("geometry")].toString();
        QStringList geoParts = geoStr.split(QStringLiteral(","));
        if (geoParts.size() == 4) {
            window.geometry = QRect(
                geoParts[0].toInt(),
                geoParts[1].toInt(),
                geoParts[2].toInt(),
                geoParts[3].toInt()
            );
        }

        window.visible = windowObj[QStringLiteral("visible")].toBool();
        window.maximized = windowObj[QStringLiteral("maximized")].toBool();
        window.screenIndex = windowObj[QStringLiteral("screenIndex")].toInt();

        layout.windows.append(window);
    }

    // 解析分割器布局
    QJsonArray splittersArray = json[QStringLiteral("splitters")].toArray();
    for (const QJsonValue& value : splittersArray) {
        QJsonObject splitterObj = value.toObject();

        SplitterLayout splitter;
        splitter.name = splitterObj[QStringLiteral("name")].toString();

        QJsonArray sizesArray = splitterObj[QStringLiteral("sizes")].toArray();
        for (const QJsonValue& sizeValue : sizesArray) {
            splitter.sizes.append(sizeValue.toInt());
        }

        layout.splitters.append(splitter);
    }

    m_templates[layout.id] = layout;
    saveToSettings(layout);

    LOG_INFO(QString("Layout imported: %1").arg(filePath));
    return true;
}

void LayoutManager::saveToSettings(const LayoutTemplate& layout)
{
    QString key = QStringLiteral("template_") + layout.id;

    m_settings->beginGroup(key);
    m_settings->setValue(QStringLiteral("id"), layout.id);
    m_settings->setValue(QStringLiteral("name"), layout.name);
    m_settings->setValue(QStringLiteral("description"), layout.description);
    m_settings->setValue(QStringLiteral("category"), layout.category);
    m_settings->setValue(QStringLiteral("isDefault"), layout.isDefault);
    m_settings->setValue(QStringLiteral("createTime"), layout.createTime);
    m_settings->setValue(QStringLiteral("updateTime"), layout.updateTime);
    m_settings->endGroup();

    // 更新模板列表
    QStringList templateIds = m_settings->value(QStringLiteral("templates")).toStringList();
    if (!templateIds.contains(layout.id)) {
        templateIds.append(layout.id);
        m_settings->setValue(QStringLiteral("templates"), templateIds);
    }
}

LayoutTemplate LayoutManager::loadFromSettings(const QString& templateId)
{
    LayoutTemplate layout;

    QString key = QStringLiteral("template_") + templateId;

    m_settings->beginGroup(key);
    layout.id = m_settings->value(QStringLiteral("id")).toString();
    layout.name = m_settings->value(QStringLiteral("name")).toString();
    layout.description = m_settings->value(QStringLiteral("description")).toString();
    layout.category = m_settings->value(QStringLiteral("category")).toString();
    layout.isDefault = m_settings->value(QStringLiteral("isDefault")).toBool();
    layout.createTime = m_settings->value(QStringLiteral("createTime")).toDateTime();
    layout.updateTime = m_settings->value(QStringLiteral("updateTime")).toDateTime();
    m_settings->endGroup();

    return layout;
}

void LayoutManager::saveSplitters(QWidget* parent, LayoutTemplate& layout)
{
    QList<QSplitter*> splitters = parent->findChildren<QSplitter*>();

    for (QSplitter* splitter : splitters) {
        SplitterLayout splitterLayout;
        splitterLayout.name = splitter->objectName().isEmpty()
            ? QStringLiteral("splitter_") + QString::number(layout.splitters.size())
            : splitter->objectName();
        splitterLayout.sizes = splitter->sizes().toVector();
        splitterLayout.state = splitter->saveState();

        layout.splitters.append(splitterLayout);
    }
}

void LayoutManager::restoreSplitters(QWidget* parent, const LayoutTemplate& layout)
{
    for (const SplitterLayout& splitterLayout : layout.splitters) {
        QSplitter* splitter = parent->findChild<QSplitter*>(splitterLayout.name);

        if (splitter) {
            splitter->setSizes(splitterLayout.sizes.toList());
            if (!splitterLayout.state.isEmpty()) {
                splitter->restoreState(splitterLayout.state);
            }
        }
    }
}

QString LayoutManager::generateTemplateId() const
{
    return QStringLiteral("layout_") + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

void LayoutManager::initDefaultTemplates()
{
    // 交易布局
    LayoutTemplate tradingLayout;
    tradingLayout.id = QStringLiteral("default_trading");
    tradingLayout.name = QStringLiteral("交易布局");
    tradingLayout.description = QStringLiteral("适合高频交易，左侧行情，右侧交易");
    tradingLayout.category = QStringLiteral("交易");
    tradingLayout.isDefault = true;
    m_templates[tradingLayout.id] = tradingLayout;

    // 分析布局
    LayoutTemplate analysisLayout;
    analysisLayout.id = QStringLiteral("default_analysis");
    analysisLayout.name = QStringLiteral("分析布局");
    analysisLayout.description = QStringLiteral("适合技术分析，大面积K线图");
    analysisLayout.category = QStringLiteral("分析");
    m_templates[analysisLayout.id] = analysisLayout;

    // 监控布局
    LayoutTemplate monitorLayout;
    monitorLayout.id = QStringLiteral("default_monitor");
    monitorLayout.name = QStringLiteral("监控布局");
    monitorLayout.description = QStringLiteral("适合多股票监控，多窗口布局");
    monitorLayout.category = QStringLiteral("监控");
    m_templates[monitorLayout.id] = monitorLayout;
}
