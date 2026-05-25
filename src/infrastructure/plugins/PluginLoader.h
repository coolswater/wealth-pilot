/**
 * @file PluginLoader.h
 * @brief 插件加载器 - 动态加载和管理插件
 *
 * @details 功能：
 * - 动态加载插件（DLL/SO）
 * - 插件生命周期管理
 * - 插件依赖解析
 * - 热插拔支持
 * - 性能优化：延迟加载
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include "IPlugin.h"
#include <QObject>
#include <QHash>
#include <QMap>
#include <QMutex>
#include <QPluginLoader>
#include <memory>

/**
 * @brief 插件加载信息
 */
struct PluginLoadInfo {
    QString filePath;           // 插件文件路径
    IPlugin* plugin;           // 插件实例
    QPluginLoader* loader;     // Qt插件加载器
    PluginState state;         // 插件状态
    bool isBuiltIn;            // 是否内置插件
};

/**
 * @brief 插件加载器 - 高性能插件管理系统
 */
class PluginLoader : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取插件加载器单例
     */
    static PluginLoader& instance();

    /**
     * @brief 初始化插件加载器
     * @param pluginPath 插件目录路径
     */
    bool initialize(const QString& pluginPath = "plugins");

    /**
     * @brief 加载所有插件
     */
    bool loadAll();

    /**
     * @brief 卸载所有插件
     */
    void unloadAll();

    /**
     * @brief 加载指定插件
     */
    bool loadPlugin(const QString& pluginName);

    /**
     * @brief 卸载指定插件
     */
    bool unloadPlugin(const QString& pluginName);

    /**
     * @brief 重载插件（热插拔）
     */
    bool reloadPlugin(const QString& pluginName);

    /**
     * @brief 获取插件
     */
    template<typename T>
    T* getPlugin(const QString& pluginName);

    /**
     * @brief 获取所有已加载的插件
     */
    QStringList loadedPlugins() const;

    /**
     * @brief 获取插件状态
     */
    PluginState pluginState(const QString& pluginName) const;

    /**
     * @brief 检查插件是否已加载
     */
    bool isLoaded(const QString& pluginName) const;

    /**
     * @brief 获取插件元数据
     */
    PluginMetaData pluginMetaData(const QString& pluginName) const;

    /**
     * @brief 注册内置插件
     */
    bool registerBuiltInPlugin(IPlugin* plugin);

signals:
    /**
     * @brief 插件加载信号
     */
    void pluginLoaded(const QString& pluginName);

    /**
     * @brief 插件卸载信号
     */
    void pluginUnloaded(const QString& pluginName);

    /**
     * @brief 插件错误信号
     */
    void pluginError(const QString& pluginName, const QString& error);

private:
    PluginLoader();
    ~PluginLoader();
    PluginLoader(const PluginLoader&) = delete;
    PluginLoader& operator=(const PluginLoader&) = delete;

    // 扫描插件目录
    void scanPluginDirectory();
    
    // 解析插件依赖
    bool resolveDependencies(const QString& pluginName);
    
    // 按优先级排序插件
    QStringList sortPluginsByPriority() const;

    QString m_pluginPath;
    QMap<QString, PluginLoadInfo> m_plugins;
    mutable QMutex m_mutex;
    
    // 性能优化：插件缓存
    QHash<QString, IPlugin*> m_pluginCache;
};

// ========== 模板实现 ==========

template<typename T>
T* PluginLoader::getPlugin(const QString& pluginName)
{
    QMutexLocker locker(&m_mutex);
    
    // 先检查缓存
    if (m_pluginCache.contains(pluginName)) {
        return qobject_cast<T*>(m_pluginCache[pluginName]);
    }
    
    // 查找插件
    auto it = m_plugins.find(pluginName);
    if (it == m_plugins.end()) {
        return nullptr;
    }
    
    // 检查状态
    if (it.value().state != PluginState::Running) {
        return nullptr;
    }
    
    // 缓存并返回
    T* plugin = qobject_cast<T*>(it.value().plugin);
    if (plugin) {
        m_pluginCache[pluginName] = it.value().plugin;
    }
    
    return plugin;
}

#endif // PLUGINLOADER_H
