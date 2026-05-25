/**
 * @file ShortcutManager.h
 * @brief 快捷键管理器 - 全局快捷键系统
 *
 * @details 功能：
 * - 全局快捷键注册和管理
 * - 自定义快捷键配置
 * - 快捷键冲突检测
 * - 快捷键分组
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QObject>
#include <QHash>
#include <QKeySequence>
#include <QSettings>
#include <QShortcut>
#include <functional>

class QWidget;

/**
 * @brief 快捷键信息
 */
struct ShortcutInfo {
    QString id;                 ///< 快捷键ID
    QString name;              ///< 显示名称
    QString description;        ///< 描述
    QKeySequence defaultKey;   ///< 默认快捷键
    QKeySequence currentKey;   ///< 当前快捷键
    QString group;             ///< 分组
    bool enabled = true;       ///< 是否启用
};

/**
 * @brief 快捷键管理器
 *
 * 提供统一的快捷键管理：
 * - 注册/注销快捷键
 * - 自定义配置
 * - 冲突检测
 * - 分组管理
 */
class ShortcutManager : public QObject {
    Q_OBJECT

public:
    static ShortcutManager* instance();

    /**
     * @brief 初始化
     * @param parent 父窗口（用于创建 QShortcut）
     */
    void initialize(QWidget* parent);

    /**
     * @brief 注册快捷键
     * @param id 唯一ID
     * @param name 显示名称
     * @param defaultKey 默认快捷键
     * @param callback 回调函数
     * @param group 分组
     * @param description 描述
     */
    void registerShortcut(const QString& id,
                         const QString& name,
                         const QKeySequence& defaultKey,
                         std::function<void()> callback,
                         const QString& group = QStringLiteral("General"),
                         const QString& description = QString());

    /**
     * @brief 注销快捷键
     */
    void unregisterShortcut(const QString& id);

    /**
     * @brief 设置快捷键
     * @param id 快捷键ID
     * @param key 新的快捷键
     * @return 是否设置成功
     */
    bool setShortcut(const QString& id, const QKeySequence& key);

    /**
     * @brief 重置快捷键为默认值
     */
    void resetShortcut(const QString& id);

    /**
     * @brief 重置所有快捷键
     */
    void resetAllShortcuts();

    /**
     * @brief 获取快捷键信息
     */
    ShortcutInfo getShortcut(const QString& id) const;

    /**
     * @brief 获取所有快捷键
     */
    QHash<QString, ShortcutInfo> getAllShortcuts() const;

    /**
     * @brief 获取分组快捷键
     */
    QList<ShortcutInfo> getShortcutsByGroup(const QString& group) const;

    /**
     * @brief 检查快捷键冲突
     * @param key 要检查的快捷键
     * @param excludeId 排除的ID
     * @return 冲突的快捷键ID列表
     */
    QStringList checkConflict(const QKeySequence& key, const QString& excludeId = QString()) const;

    /**
     * @brief 保存配置
     */
    void saveConfig();

    /**
     * @brief 加载配置
     */
    void loadConfig();

    /**
     * @brief 启用/禁用快捷键
     */
    void setEnabled(const QString& id, bool enabled);

    /**
     * @brief 获取分组列表
     */
    QStringList getGroups() const;

signals:
    /**
     * @brief 快捷键触发信号
     */
    void shortcutTriggered(const QString& id);

    /**
     * @brief 快捷键变更信号
     */
    void shortcutChanged(const QString& id, const QKeySequence& newKey);

    /**
     * @brief 冲突检测信号
     */
    void conflictDetected(const QString& id, const QStringList& conflictingIds);

private:
    explicit ShortcutManager(QObject* parent = nullptr);
    ~ShortcutManager() override;

    void updateShortcut(const QString& id);

    QWidget* m_parent = nullptr;
    QHash<QString, ShortcutInfo> m_shortcuts;
    QHash<QString, QShortcut*> m_qshortcuts;
    QHash<QString, std::function<void()>> m_callbacks;
    QSettings* m_settings = nullptr;
};

#endif // SHORTCUTMANAGER_H
