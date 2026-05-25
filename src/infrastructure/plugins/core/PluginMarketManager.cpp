/**
 * @file PluginMarketManager.cpp
 * @brief 插件市场管理器实现
 */

#include "PluginMarketManager.h"
#include "shared/utils/Logger.h"
#include <QSettings>
#include <QDir>
#include <QFile>
#include <algorithm>

PluginMarketManager* PluginMarketManager::instance()
{
    static PluginMarketManager* inst = new PluginMarketManager();
    return inst;
}

PluginMarketManager::PluginMarketManager(QObject* parent)
    : QObject(parent)
{
    // 设置插件目录
    m_pluginDirectory = QDir::currentPath() + "/plugins";

    // 加载已安装插件
    QSettings settings("WealthPilot", "Plugins");
    QStringList installedPlugins = settings.value("installed").toStringList();

    for (const QString& pluginId : installedPlugins) {
        if (m_plugins.contains(pluginId)) {
            m_plugins[pluginId].installed = true;
            m_plugins[pluginId].enabled = settings.value(QString("enabled_%1").arg(pluginId)).toBool();
        }
    }

    LOG_INFO("PluginMarketManager initialized");
}

QVector<MarketPluginInfo> PluginMarketManager::getHotPlugins(int limit) const
{
    QVector<MarketPluginInfo> result;
    for (const auto& plugin : m_plugins) {
        result.append(plugin);
    }

    // 按下载次数排序
    std::sort(result.begin(), result.end(),
              [](const MarketPluginInfo& a, const MarketPluginInfo& b) {
                  return a.downloads > b.downloads;
              });

    if (result.size() > limit) {
        result = result.mid(0, limit);
    }

    return result;
}

QVector<MarketPluginInfo> PluginMarketManager::getLatestPlugins(int limit) const
{
    QVector<MarketPluginInfo> result;
    for (const auto& plugin : m_plugins) {
        result.append(plugin);
    }

    // 按发布时间排序
    std::sort(result.begin(), result.end(),
              [](const MarketPluginInfo& a, const MarketPluginInfo& b) {
                  return a.publishTime > b.publishTime;
              });

    if (result.size() > limit) {
        result = result.mid(0, limit);
    }

    return result;
}

QVector<MarketPluginInfo> PluginMarketManager::searchPlugins(const QString& keyword,
                                                       const QString& category) const
{
    QVector<MarketPluginInfo> result;

    for (const auto& plugin : m_plugins) {
        bool match = keyword.isEmpty() ||
            plugin.name.contains(keyword, Qt::CaseInsensitive) ||
            plugin.description.contains(keyword, Qt::CaseInsensitive);

        if (!category.isEmpty() && plugin.category != category) {
            match = false;
        }

        if (match) {
            result.append(plugin);
        }
    }

    return result;
}

MarketPluginInfo PluginMarketManager::getPlugin(const QString& pluginId) const
{
    return m_plugins.value(pluginId);
}

QStringList PluginMarketManager::getCategories() const
{
    QStringList categories;
    for (const auto& plugin : m_plugins) {
        if (!categories.contains(plugin.category)) {
            categories.append(plugin.category);
        }
    }
    return categories;
}

QVector<MarketPluginInfo> PluginMarketManager::getPluginsByCategory(const QString& category) const
{
    QVector<MarketPluginInfo> result;
    for (const auto& plugin : m_plugins) {
        if (plugin.category == category) {
            result.append(plugin);
        }
    }
    return result;
}

bool PluginMarketManager::downloadPlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        LOG_WARNING(QString("Plugin not found: %1").arg(pluginId));
        return false;
    }

    // TODO: 实际下载逻辑
    LOG_INFO(QString("Downloading plugin: %1").arg(pluginId));

    // 模拟下载进度
    for (int i = 0; i <= 100; i += 10) {
        emit downloadProgress(pluginId, i);
    }

    emit pluginDownloaded(pluginId);
    LOG_INFO(QString("Plugin downloaded: %1").arg(pluginId));

    return true;
}

bool PluginMarketManager::installPlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    MarketPluginInfo& plugin = m_plugins[pluginId];

    // 检查依赖
    if (!checkDependencies(plugin.dependencies)) {
        LOG_ERROR(QString("Dependencies not satisfied for plugin: %1").arg(pluginId));
        return false;
    }

    // 下载插件
    if (!plugin.installed) {
        if (!downloadPlugin(pluginId)) {
            return false;
        }
    }

    // 安装
    plugin.installed = true;
    plugin.localPath = m_pluginDirectory + "/" + pluginId;

    // 保存安装信息
    QSettings settings("WealthPilot", "Plugins");
    QStringList installed = settings.value("installed").toStringList();
    if (!installed.contains(pluginId)) {
        installed.append(pluginId);
        settings.setValue("installed", installed);
    }

    emit pluginInstalled(pluginId);
    LOG_INFO(QString("Plugin installed: %1").arg(pluginId));

    return true;
}

bool PluginMarketManager::uninstallPlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    MarketPluginInfo& plugin = m_plugins[pluginId];
    if (!plugin.installed) {
        return false;
    }

    // 先卸载
    if (plugin.enabled) {
        unloadPlugin(pluginId);
    }

    // 删除文件
    if (!plugin.localPath.isEmpty()) {
        QDir dir(plugin.localPath);
        dir.removeRecursively();
    }

    plugin.installed = false;
    plugin.enabled = false;
    plugin.localPath.clear();

    // 更新设置
    QSettings settings("WealthPilot", "Plugins");
    QStringList installed = settings.value("installed").toStringList();
    installed.removeAll(pluginId);
    settings.setValue("installed", installed);
    settings.remove(QString("enabled_%1").arg(pluginId));

    emit pluginUninstalled(pluginId);
    LOG_INFO(QString("Plugin uninstalled: %1").arg(pluginId));

    return true;
}

bool PluginMarketManager::updatePlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    // TODO: 检查版本并更新
    LOG_INFO(QString("Updating plugin: %1").arg(pluginId));

    emit pluginUpdated(pluginId);
    return true;
}

QVector<MarketPluginInfo> PluginMarketManager::checkUpdates() const
{
    QVector<MarketPluginInfo> updates;

    for (const auto& plugin : m_plugins) {
        if (plugin.installed) {
            // TODO: 检查版本
            // 如果有新版本，添加到更新列表
        }
    }

    return updates;
}

bool PluginMarketManager::enablePlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    MarketPluginInfo& plugin = m_plugins[pluginId];
    if (!plugin.installed) {
        return false;
    }

    // 加载插件
    if (!loadPlugin(pluginId)) {
        return false;
    }

    plugin.enabled = true;

    QSettings settings("WealthPilot", "Plugins");
    settings.setValue(QString("enabled_%1").arg(pluginId), true);

    emit pluginStateChanged(pluginId, true);
    LOG_INFO(QString("Plugin enabled: %1").arg(pluginId));

    return true;
}

bool PluginMarketManager::disablePlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    MarketPluginInfo& plugin = m_plugins[pluginId];
    if (!plugin.enabled) {
        return false;
    }

    // 卸载插件
    unloadPlugin(pluginId);

    plugin.enabled = false;

    QSettings settings("WealthPilot", "Plugins");
    settings.setValue(QString("enabled_%1").arg(pluginId), false);

    emit pluginStateChanged(pluginId, false);
    LOG_INFO(QString("Plugin disabled: %1").arg(pluginId));

    return true;
}

QVector<MarketPluginInfo> PluginMarketManager::getInstalledPlugins() const
{
    QVector<MarketPluginInfo> result;
    for (const auto& plugin : m_plugins) {
        if (plugin.installed) {
            result.append(plugin);
        }
    }
    return result;
}

QVector<MarketPluginInfo> PluginMarketManager::getEnabledPlugins() const
{
    QVector<MarketPluginInfo> result;
    for (const auto& plugin : m_plugins) {
        if (plugin.enabled) {
            result.append(plugin);
        }
    }
    return result;
}

bool PluginMarketManager::loadPlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    const MarketPluginInfo& plugin = m_plugins[pluginId];

    // TODO: 实际加载插件
    LOG_INFO(QString("Loading plugin: %1").arg(plugin.name));

    return true;
}

bool PluginMarketManager::unloadPlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    // TODO: 实际卸载插件
    LOG_INFO(QString("Unloading plugin: %1").arg(pluginId));

    return true;
}

bool PluginMarketManager::ratePlugin(const QString& pluginId, double rating, const QString& comment)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    PluginRating r;
    r.pluginId = pluginId;
    r.rating = qBound(1.0, rating, 5.0);
    r.comment = comment;
    r.time = QDateTime::currentDateTime();

    m_ratings[pluginId].append(r);
    updatePluginRating(pluginId);

    LOG_INFO(QString("Plugin rated: %1 -> %2").arg(pluginId).arg(rating));

    return true;
}

QVector<PluginRating> PluginMarketManager::getPluginRatings(const QString& pluginId) const
{
    return m_ratings.value(pluginId);
}

bool PluginMarketManager::publishPlugin(const MarketPluginInfo& plugin)
{
    if (m_plugins.contains(plugin.id)) {
        LOG_WARNING(QString("Plugin already exists: %1").arg(plugin.id));
        return false;
    }

    MarketPluginInfo newPlugin = plugin;
    newPlugin.publishTime = QDateTime::currentDateTime();
    newPlugin.updateTime = newPlugin.publishTime;

    m_plugins[plugin.id] = newPlugin;

    LOG_INFO(QString("Plugin published: %1 (%2)").arg(plugin.name).arg(plugin.id));

    return true;
}

bool PluginMarketManager::updatePluginInfo(const MarketPluginInfo& plugin)
{
    if (!m_plugins.contains(plugin.id)) {
        return false;
    }

    MarketPluginInfo updated = plugin;
    updated.updateTime = QDateTime::currentDateTime();

    m_plugins[plugin.id] = updated;

    LOG_INFO(QString("Plugin info updated: %1").arg(plugin.id));

    return true;
}

void PluginMarketManager::updatePluginRating(const QString& pluginId)
{
    QVector<PluginRating> ratings = m_ratings.value(pluginId);
    if (ratings.isEmpty()) return;

    double total = 0;
    for (const PluginRating& r : ratings) {
        total += r.rating;
    }

    m_plugins[pluginId].rating = total / ratings.size();
    m_plugins[pluginId].ratingCount = ratings.size();
}

bool PluginMarketManager::checkDependencies(const QStringList& dependencies)
{
    for (const QString& dep : dependencies) {
        if (!m_plugins.contains(dep) || !m_plugins[dep].installed) {
            return false;
        }
    }
    return true;
}