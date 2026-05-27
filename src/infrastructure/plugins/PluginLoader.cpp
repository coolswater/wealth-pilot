/**
 * @file PluginLoader.cpp
 * @brief 插件加载器实现 - 高性能插件管理系统
 */

#include "PluginLoader.h"
#include "shared/utils/Logger.h"
#include <QDir>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QMutexLocker>

PluginLoader& PluginLoader::instance()
{
    static PluginLoader instance;
    return instance;
}

bool PluginLoader::initialize(const QString& pluginPath)
{
    QElapsedTimer timer;
    timer.start();
    
    m_pluginPath = pluginPath;
    
    // 扫描插件目录
    scanPluginDirectory();
    
    LOG_INFO(QString("PluginLoader initialized in %1ms, found %2 plugins")
        .arg(timer.elapsed()).arg(m_plugins.size()));
    
    return true;
}

bool PluginLoader::loadAll()
{
    QElapsedTimer timer;
    timer.start();
    
    QMutexLocker locker(&m_mutex);
    
    // 按优先级排序
    QStringList sortedPlugins = sortPluginsByPriority();
    
    int loadedCount = 0;
    int failedCount = 0;
    
    for (const QString& pluginName : sortedPlugins) {
        locker.unlock();
        if (loadPlugin(pluginName)) {
            loadedCount++;
        } else {
            failedCount++;
        }
        locker.relock();
    }
    
    LOG_INFO(QString("Loaded %1 plugins in %2ms (%3 failed)")
        .arg(loadedCount).arg(timer.elapsed()).arg(failedCount));
    
    return failedCount == 0;
}

void PluginLoader::unloadAll()
{
    QMutexLocker locker(&m_mutex);
    
    // 按优先级逆序卸载
    QStringList sortedPlugins = sortPluginsByPriority();
    std::reverse(sortedPlugins.begin(), sortedPlugins.end());
    
    for (const QString& pluginName : sortedPlugins) {
        locker.unlock();
        unloadPlugin(pluginName);
        locker.relock();
    }
    
    m_pluginCache.clear();
    LOG_INFO("All plugins unloaded");
}

bool PluginLoader::loadPlugin(const QString& pluginName)
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_plugins.find(pluginName);
    if (it == m_plugins.end()) {
        LOG_WARNING(QString("Plugin not found: %1").arg(pluginName));
        return false;
    }
    
    auto& info = it.value();
    
    // 已加载
    if (info.state == PluginState::Running || info.state == PluginState::Initialized) {
        return true;
    }
    
    // 解析依赖
    locker.unlock();
    if (!resolveDependencies(pluginName)) {
        LOG_ERROR(QString("Failed to resolve dependencies for plugin: %1").arg(pluginName));
        return false;
    }
    locker.relock();
    
    QElapsedTimer timer;
    timer.start();
    
    // 加载插件
    if (!info.isBuiltIn) {
        if (!info.loader->load()) {
            LOG_ERROR(QString("Failed to load plugin: %1, error: %2")
                .arg(pluginName).arg(info.loader->errorString()));
            info.state = PluginState::Error;
            return false;
        }
        
        QObject* instance = info.loader->instance();
        info.plugin = qobject_cast<IPlugin*>(instance);
        
        if (!info.plugin) {
            LOG_ERROR(QString("Invalid plugin interface: %1").arg(pluginName));
            info.loader->unload();
            info.state = PluginState::Error;
            return false;
        }
    }
    
    // 调用插件的 load() 方法（内置插件也需要调用）
    if (!info.plugin->load()) {
        LOG_ERROR(QString("Failed to load plugin: %1").arg(pluginName));
        info.state = PluginState::Error;
        return false;
    }
    
    info.state = PluginState::Loaded;
    
    // 初始化插件
    if (!info.plugin->initialize()) {
        LOG_ERROR(QString("Failed to initialize plugin: %1").arg(pluginName));
        info.state = PluginState::Error;
        return false;
    }
    
    info.state = PluginState::Initialized;
    
    // 启动插件
    if (!info.plugin->start()) {
        LOG_ERROR(QString("Failed to start plugin: %1").arg(pluginName));
        info.state = PluginState::Error;
        return false;
    }
    
    info.state = PluginState::Running;
    
    // 缓存插件
    m_pluginCache[pluginName] = info.plugin;
    
    LOG_INFO(QString("Plugin loaded: %1 in %2ms")
        .arg(pluginName).arg(timer.elapsed()));
    
    locker.unlock();
    emit pluginLoaded(pluginName);
    
    return true;
}

bool PluginLoader::unloadPlugin(const QString& pluginName)
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_plugins.find(pluginName);
    if (it == m_plugins.end()) {
        return false;
    }
    
    auto& info = it.value();
    
    if (info.state == PluginState::Unloaded) {
        return true;
    }
    
    // 停止插件
    if (info.plugin) {
        info.plugin->stop();
        info.plugin->unload();
    }
    
    // 卸载动态库
    if (!info.isBuiltIn && info.loader) {
        info.loader->unload();
    }
    
    info.state = PluginState::Unloaded;
    m_pluginCache.remove(pluginName);
    
    LOG_INFO(QString("Plugin unloaded: %1").arg(pluginName));
    
    locker.unlock();
    emit pluginUnloaded(pluginName);
    
    return true;
}

bool PluginLoader::reloadPlugin(const QString& pluginName)
{
    QElapsedTimer timer;
    timer.start();
    
    // 先卸载
    if (!unloadPlugin(pluginName)) {
        return false;
    }
    
    // 清除缓存
    m_pluginCache.remove(pluginName);
    
    // 重新加载
    bool result = loadPlugin(pluginName);
    
    LOG_INFO(QString("Plugin reloaded: %1 in %2ms, result: %3")
        .arg(pluginName).arg(timer.elapsed()).arg(result));
    
    return result;
}

QStringList PluginLoader::loadedPlugins() const
{
    QMutexLocker locker(&m_mutex);
    
    QStringList result;
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        if (it.value().state == PluginState::Running) {
            result.append(it.key());
        }
    }
    return result;
}

PluginState PluginLoader::pluginState(const QString& pluginName) const
{
    QMutexLocker locker(&m_mutex);
    return m_plugins.value(pluginName).state;
}

bool PluginLoader::isLoaded(const QString& pluginName) const
{
    QMutexLocker locker(&m_mutex);
    auto it = m_plugins.find(pluginName);
    return it != m_plugins.end() && it.value().state == PluginState::Running;
}

PluginMetaData PluginLoader::pluginMetaData(const QString& pluginName) const
{
    QMutexLocker locker(&m_mutex);
    auto it = m_plugins.find(pluginName);
    if (it != m_plugins.end() && it.value().plugin) {
        return it.value().plugin->metaData();
    }
    return PluginMetaData();
}

bool PluginLoader::registerBuiltInPlugin(IPlugin* plugin)
{
    if (!plugin) return false;
    
    QMutexLocker locker(&m_mutex);
    
    PluginMetaData metaData = plugin->metaData();
    QString pluginName = metaData.name;
    
    if (m_plugins.contains(pluginName)) {
        LOG_WARNING(QString("Plugin already registered: %1").arg(pluginName));
        return false;
    }
    
    PluginLoadInfo info;
    info.plugin = plugin;
    info.loader = nullptr;
    info.state = PluginState::Unloaded;
    info.isBuiltIn = true;
    
    m_plugins[pluginName] = info;
    
    LOG_INFO(QString("Built-in plugin registered: %1").arg(pluginName));
    return true;
}

void PluginLoader::scanPluginDirectory()
{
    QDir dir(m_pluginPath);
    if (!dir.exists()) {
        LOG_WARNING(QString("Plugin directory not found: %1").arg(m_pluginPath));
        return;
    }
    
    // 查找所有插件文件
    QStringList filters;
#ifdef Q_OS_WIN
    filters << "*.dll";
#else
    filters << "*.so";
#endif
    
    for (const QFileInfo& fileInfo : dir.entryInfoList(filters, QDir::Files)) {
        QString filePath = fileInfo.absoluteFilePath();
        QString pluginName = fileInfo.baseName();
        
        // 跳过已注册的插件
        if (m_plugins.contains(pluginName)) {
            continue;
        }
        
        // 创建插件加载器
        QPluginLoader* loader = new QPluginLoader(filePath);
        
        // 读取元数据
        QJsonObject metaData = loader->metaData().value("MetaData").toObject();
        
        PluginLoadInfo info;
        info.filePath = filePath;
        info.loader = loader;
        info.plugin = nullptr;
        info.state = PluginState::Unloaded;
        info.isBuiltIn = false;
        
        m_plugins[pluginName] = info;
        
        LOG_DEBUG(QString("Found plugin: %1 at %2").arg(pluginName).arg(filePath));
    }
}

bool PluginLoader::resolveDependencies(const QString& pluginName)
{
    auto it = m_plugins.find(pluginName);
    if (it == m_plugins.end()) {
        return false;
    }
    
    // 如果插件已加载，直接返回
    if (it.value().state == PluginState::Running) {
        return true;
    }
    
    // 获取插件实例（需要先加载）
    if (!it.value().plugin) {
        // 对于动态插件，需要先加载库
        if (!it.value().isBuiltIn && it.value().loader) {
            QObject* instance = it.value().loader->instance();
            it.value().plugin = qobject_cast<IPlugin*>(instance);
        }
    }
    
    if (!it.value().plugin) {
        return false;
    }
    
    QStringList deps = it.value().plugin->dependencies();
    
    for (const QString& dep : deps) {
        // 检查依赖是否已加载
        if (!isLoaded(dep)) {
            // 尝试加载依赖
            if (!loadPlugin(dep)) {
                LOG_ERROR(QString("Failed to load dependency: %1 for plugin: %2")
                    .arg(dep).arg(pluginName));
                return false;
            }
        }
    }
    
    return true;
}

QStringList PluginLoader::sortPluginsByPriority() const
{
    // 按优先级排序
    QMap<int, QStringList> priorityMap;
    
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        int priority = 100; // 默认优先级
        
        if (it.value().plugin) {
            priority = it.value().plugin->metaData().priority;
        }
        
        priorityMap[priority].append(it.key());
    }
    
    QStringList result;
    for (auto it = priorityMap.begin(); it != priorityMap.end(); ++it) {
        result.append(it.value());
    }
    
    return result;
}

PluginLoader::PluginLoader()
{
    LOG_DEBUG("PluginLoader created");
}

PluginLoader::~PluginLoader()
{
    unloadAll();
    LOG_DEBUG("PluginLoader destroyed");
}
