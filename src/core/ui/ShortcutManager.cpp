/**
 * @file ShortcutManager.cpp
 * @brief 快捷键管理器实现
 */

#include "ShortcutManager.h"
#include "utils/Logger.h"
#include <QWidget>
#include <QApplication>

ShortcutManager* ShortcutManager::instance()
{
    static ShortcutManager* inst = new ShortcutManager();
    return inst;
}

ShortcutManager::ShortcutManager(QObject* parent)
    : QObject(parent)
{
    m_settings = new QSettings("WealthPilot", "Shortcuts", this);
    LOG_INFO("ShortcutManager initialized");
}

ShortcutManager::~ShortcutManager()
{
    saveConfig();
}

void ShortcutManager::initialize(QWidget* parent)
{
    m_parent = parent;
    loadConfig();
    LOG_INFO("ShortcutManager initialized with parent widget");
}

void ShortcutManager::registerShortcut(const QString& id,
                                       const QString& name,
                                       const QKeySequence& defaultKey,
                                       std::function<void()> callback,
                                       const QString& group,
                                       const QString& description)
{
    ShortcutInfo info;
    info.id = id;
    info.name = name;
    info.description = description;
    info.defaultKey = defaultKey;
    info.currentKey = defaultKey;
    info.group = group;

    // 从配置加载
    if (m_settings->contains(id)) {
        QString keyStr = m_settings->value(id).toString();
        info.currentKey = QKeySequence(keyStr);
    }

    m_shortcuts[id] = info;
    m_callbacks[id] = callback;

    // 创建 QShortcut
    updateShortcut(id);

    LOG_INFO(QString("Shortcut registered: %1 [%2] -> %3")
        .arg(name).arg(info.currentKey.toString()).arg(id));
}

void ShortcutManager::unregisterShortcut(const QString& id)
{
    if (m_qshortcuts.contains(id)) {
        delete m_qshortcuts.take(id);
    }
    m_shortcuts.remove(id);
    m_callbacks.remove(id);

    LOG_INFO(QString("Shortcut unregistered: %1").arg(id));
}

bool ShortcutManager::setShortcut(const QString& id, const QKeySequence& key)
{
    if (!m_shortcuts.contains(id)) {
        return false;
    }

    // 检查冲突
    QStringList conflicts = checkConflict(key, id);
    if (!conflicts.isEmpty()) {
        emit conflictDetected(id, conflicts);
        LOG_WARNING(QString("Shortcut conflict: %1 conflicts with %2")
            .arg(id).arg(conflicts.join(", ")));
        return false;
    }

    m_shortcuts[id].currentKey = key;
    updateShortcut(id);
    saveConfig();

    emit shortcutChanged(id, key);
    LOG_INFO(QString("Shortcut changed: %1 -> %2").arg(id).arg(key.toString()));

    return true;
}

void ShortcutManager::resetShortcut(const QString& id)
{
    if (!m_shortcuts.contains(id)) return;

    m_shortcuts[id].currentKey = m_shortcuts[id].defaultKey;
    updateShortcut(id);
    saveConfig();

    emit shortcutChanged(id, m_shortcuts[id].currentKey);
    LOG_INFO(QString("Shortcut reset: %1").arg(id));
}

void ShortcutManager::resetAllShortcuts()
{
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        it.value().currentKey = it.value().defaultKey;
        updateShortcut(it.key());
    }
    saveConfig();

    LOG_INFO("All shortcuts reset to defaults");
}

ShortcutInfo ShortcutManager::getShortcut(const QString& id) const
{
    return m_shortcuts.value(id);
}

QHash<QString, ShortcutInfo> ShortcutManager::getAllShortcuts() const
{
    return m_shortcuts;
}

QList<ShortcutInfo> ShortcutManager::getShortcutsByGroup(const QString& group) const
{
    QList<ShortcutInfo> result;
    for (const auto& info : m_shortcuts) {
        if (info.group == group) {
            result.append(info);
        }
    }
    return result;
}

QStringList ShortcutManager::checkConflict(const QKeySequence& key, const QString& excludeId) const
{
    QStringList conflicts;

    if (key.isEmpty()) return conflicts;

    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        if (it.key() != excludeId && it.value().currentKey == key) {
            conflicts.append(it.key());
        }
    }

    return conflicts;
}

void ShortcutManager::saveConfig()
{
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        m_settings->setValue(it.key(), it.value().currentKey.toString());
    }
    m_settings->sync();
    LOG_DEBUG("Shortcuts config saved");
}

void ShortcutManager::loadConfig()
{
    // 配置在 registerShortcut 时加载
    LOG_DEBUG("Shortcuts config loaded");
}

void ShortcutManager::setEnabled(const QString& id, bool enabled)
{
    if (!m_shortcuts.contains(id)) return;

    m_shortcuts[id].enabled = enabled;
    if (m_qshortcuts.contains(id)) {
        m_qshortcuts[id]->setEnabled(enabled);
    }

    LOG_DEBUG(QString("Shortcut %1 %2").arg(id).arg(enabled ? "enabled" : "disabled"));
}

QStringList ShortcutManager::getGroups() const
{
    QStringList groups;
    for (const auto& info : m_shortcuts) {
        if (!groups.contains(info.group)) {
            groups.append(info.group);
        }
    }
    return groups;
}

void ShortcutManager::updateShortcut(const QString& id)
{
    if (!m_parent) return;

    // 删除旧的
    if (m_qshortcuts.contains(id)) {
        delete m_qshortcuts.take(id);
    }

    // 创建新的
    const ShortcutInfo& info = m_shortcuts[id];
    if (!info.currentKey.isEmpty()) {
        QShortcut* shortcut = new QShortcut(info.currentKey, m_parent);
        shortcut->setEnabled(info.enabled);
        connect(shortcut, &QShortcut::activated, this, [this, id]() {
            if (m_callbacks.contains(id)) {
                m_callbacks[id]();
            }
            emit shortcutTriggered(id);
        });
        m_qshortcuts[id] = shortcut;
    }
}
