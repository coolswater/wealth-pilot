/**
 * @file LayoutManager.cpp
 * @brief 窗口布局管理器实现
 */

#include "LayoutManager.h"
#include "../utils/Logger.h"
#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>
#include <QSettings>
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

LayoutManager* LayoutManager::instance()
{
    static LayoutManager* inst = new LayoutManager();
    return inst;
}

LayoutManager::LayoutManager(QObject* parent)
    : QObject(parent)
{
    connect(qApp, &QApplication::aboutToQuit, this, &LayoutManager::onAboutToQuit);
    LOG_INFO("LayoutManager initialized");
}

LayoutManager::~LayoutManager()
{
    if (m_autoSave && !m_currentLayout.isEmpty()) {
        saveLayout(m_currentLayout);
    }
}

void LayoutManager::initialize(QMainWindow* mainWindow)
{
    m_mainWindow = mainWindow;

    // 加载默认布局
    restoreLayout();

    LOG_INFO("LayoutManager initialized with main window");
}

void LayoutManager::saveLayout(const QString& name)
{
    if (!m_mainWindow) return;

    QString layoutName = name.isEmpty() ? QStringLiteral("default") : name;
    LayoutInfo info;
    info.name = layoutName;

    // 保存窗口几何和状态
    info.geometry = m_mainWindow->saveGeometry();
    info.state = m_mainWindow->saveState();

    // 保存分割器状态
    for (auto it = m_splitters.begin(); it != m_splitters.end(); ++it) {
        info.splitterStates[it.key()] = it.value()->saveState();
    }

    // 保存选项卡顺序
    for (auto it = m_tabWidgets.begin(); it != m_tabWidgets.end(); ++it) {
        QStringList tabOrder;
        QTabWidget* tab = it.value();
        for (int i = 0; i < tab->count(); ++i) {
            tabOrder.append(tab->tabText(i));
        }
        info.tabOrders[it.key()] = tabOrder;
    }

    m_layouts[layoutName] = info;
    m_currentLayout = layoutName;

    // 保存到设置
    QSettings settings("WealthPilot", "Layout");
    settings.beginGroup(layoutName);
    settings.setValue("geometry", info.geometry.toBase64());
    settings.setValue("state", info.state.toBase64());

    settings.beginGroup("Splitters");
    for (auto it = info.splitterStates.begin(); it != info.splitterStates.end(); ++it) {
        settings.setValue(it.key(), it.value().toBase64());
    }
    settings.endGroup();

    settings.beginGroup("Tabs");
    for (auto it = info.tabOrders.begin(); it != info.tabOrders.end(); ++it) {
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();

    settings.endGroup();
    settings.sync();

    emit layoutSaved(layoutName);
    LOG_INFO(QString("Layout saved: %1").arg(layoutName));
}

bool LayoutManager::restoreLayout(const QString& name)
{
    if (!m_mainWindow) return false;

    QString layoutName = name.isEmpty() ? QStringLiteral("default") : name;

    QSettings settings("WealthPilot", "Layout");
    if (!settings.childGroups().contains(layoutName)) {
        LOG_WARNING(QString("Layout not found: %1").arg(layoutName));
        return false;
    }

    settings.beginGroup(layoutName);

    // 恢复窗口几何和状态
    QByteArray geometry = QByteArray::fromBase64(settings.value("geometry").toByteArray());
    QByteArray state = QByteArray::fromBase64(settings.value("state").toByteArray());

    if (!geometry.isEmpty()) {
        m_mainWindow->restoreGeometry(geometry);
    }
    if (!state.isEmpty()) {
        m_mainWindow->restoreState(state);
    }

    // 恢复分割器状态
    settings.beginGroup("Splitters");
    for (const QString& id : settings.childKeys()) {
        QByteArray splitterState = QByteArray::fromBase64(settings.value(id).toByteArray());
        if (m_splitters.contains(id) && !splitterState.isEmpty()) {
            m_splitters[id]->restoreState(splitterState);
        }
    }
    settings.endGroup();

    // 恢复选项卡顺序
    settings.beginGroup("Tabs");
    for (const QString& id : settings.childKeys()) {
        QStringList tabOrder = settings.value(id).toStringList();
        if (m_tabWidgets.contains(id) && !tabOrder.isEmpty()) {
            // TODO: 实现选项卡重排序
        }
    }
    settings.endGroup();

    settings.endGroup();

    m_currentLayout = layoutName;
    emit layoutRestored(layoutName);
    LOG_INFO(QString("Layout restored: %1").arg(layoutName));

    return true;
}

void LayoutManager::saveSplitterState(const QString& id, QSplitter* splitter)
{
    if (!splitter) return;

    QSettings settings("WealthPilot", "Layout");
    settings.beginGroup(m_currentLayout.isEmpty() ? "default" : m_currentLayout);
    settings.beginGroup("Splitters");
    settings.setValue(id, splitter->saveState().toBase64());
    settings.endGroup();
    settings.endGroup();
    settings.sync();

    LOG_DEBUG(QString("Splitter state saved: %1").arg(id));
}

void LayoutManager::restoreSplitterState(const QString& id, QSplitter* splitter)
{
    if (!splitter) return;

    QSettings settings("WealthPilot", "Layout");
    settings.beginGroup(m_currentLayout.isEmpty() ? "default" : m_currentLayout);
    settings.beginGroup("Splitters");
    QByteArray state = QByteArray::fromBase64(settings.value(id).toByteArray());
    settings.endGroup();
    settings.endGroup();

    if (!state.isEmpty()) {
        splitter->restoreState(state);
        LOG_DEBUG(QString("Splitter state restored: %1").arg(id));
    }
}

void LayoutManager::saveTabOrder(const QString& id, QTabWidget* tabWidget)
{
    if (!tabWidget) return;

    QStringList tabOrder;
    for (int i = 0; i < tabWidget->count(); ++i) {
        tabOrder.append(tabWidget->tabText(i));
    }

    QSettings settings("WealthPilot", "Layout");
    settings.beginGroup(m_currentLayout.isEmpty() ? "default" : m_currentLayout);
    settings.beginGroup("Tabs");
    settings.setValue(id, tabOrder);
    settings.endGroup();
    settings.endGroup();
    settings.sync();

    LOG_DEBUG(QString("Tab order saved: %1").arg(id));
}

void LayoutManager::restoreTabOrder(const QString& id, QTabWidget* tabWidget)
{
    // TODO: 实现选项卡重排序
    Q_UNUSED(id)
    Q_UNUSED(tabWidget)
}

QStringList LayoutManager::getLayoutNames() const
{
    QSettings settings("WealthPilot", "Layout");
    return settings.childGroups();
}

void LayoutManager::deleteLayout(const QString& name)
{
    QSettings settings("WealthPilot", "Layout");
    settings.remove(name);
    m_layouts.remove(name);

    LOG_INFO(QString("Layout deleted: %1").arg(name));
}

bool LayoutManager::exportLayout(const QString& name, const QString& filePath)
{
    QSettings settings("WealthPilot", "Layout");
    settings.beginGroup(name);

    QJsonObject root;
    root["name"] = name;
    root["geometry"] = QString(settings.value("geometry").toByteArray().toBase64());
    root["state"] = QString(settings.value("state").toByteArray().toBase64());

    settings.beginGroup("Splitters");
    QJsonObject splitters;
    for (const QString& key : settings.childKeys()) {
        splitters[key] = QString(settings.value(key).toByteArray().toBase64());
    }
    root["splitters"] = splitters;
    settings.endGroup();

    settings.beginGroup("Tabs");
    QJsonObject tabs;
    for (const QString& key : settings.childKeys()) {
        tabs[key] = QJsonArray::fromStringList(settings.value(key).toStringList());
    }
    root["tabs"] = tabs;
    settings.endGroup();

    settings.endGroup();

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file for export: %1").arg(filePath));
        return false;
    }

    file.write(QJsonDocument(root).toJson());
    file.close();

    LOG_INFO(QString("Layout exported: %1 -> %2").arg(name).arg(filePath));
    return true;
}

bool LayoutManager::importLayout(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open file for import: %1").arg(filePath));
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull()) {
        LOG_ERROR(QString("Invalid layout file: %1").arg(filePath));
        return false;
    }

    QJsonObject root = doc.object();
    QString name = root["name"].toString();

    QSettings settings("WealthPilot", "Layout");
    settings.beginGroup(name);
    settings.setValue("geometry", QByteArray::fromBase64(root["geometry"].toString().toLatin1()));
    settings.setValue("state", QByteArray::fromBase64(root["state"].toString().toLatin1()));

    QJsonObject splitters = root["splitters"].toObject();
    settings.beginGroup("Splitters");
    for (const QString& key : splitters.keys()) {
        settings.setValue(key, QByteArray::fromBase64(splitters[key].toString().toLatin1()));
    }
    settings.endGroup();

    QJsonObject tabs = root["tabs"].toObject();
    settings.beginGroup("Tabs");
    for (const QString& key : tabs.keys()) {
        settings.setValue(key, tabs[key].toVariant().toStringList());
    }
    settings.endGroup();

    settings.endGroup();
    settings.sync();

    LOG_INFO(QString("Layout imported: %1 <- %2").arg(name).arg(filePath));
    return true;
}

void LayoutManager::setAutoSave(bool enabled)
{
    m_autoSave = enabled;
    LOG_INFO(QString("Auto save %1").arg(enabled ? "enabled" : "disabled"));
}

void LayoutManager::registerSplitter(const QString& id, QSplitter* splitter)
{
    m_splitters[id] = splitter;
    LOG_DEBUG(QString("Splitter registered: %1").arg(id));
}

void LayoutManager::registerTabWidget(const QString& id, QTabWidget* tabWidget)
{
    m_tabWidgets[id] = tabWidget;
    LOG_DEBUG(QString("Tab widget registered: %1").arg(id));
}

void LayoutManager::onAboutToQuit()
{
    if (m_autoSave && !m_currentLayout.isEmpty()) {
        saveLayout(m_currentLayout);
        LOG_INFO("Auto-saved layout on quit");
    }
}

QString LayoutManager::layoutKey(const QString& name) const
{
    return QString("Layout/%1").arg(name);
}
