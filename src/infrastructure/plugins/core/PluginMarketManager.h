/**
 * @file PluginMarketManager.h
 * @brief 插件市场管理器 - 第三方插件生态系统
 *
 * @details 功能：
 * - 插件发布
 * - 插件下载
 * - 插件管理
 * - 插件评分
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PLUGINMARKETMANAGER_H
#define PLUGINMARKETMANAGER_H

#include <QObject>
#include <QVector>
#include <QHash>
#include <QString>
#include <QDateTime>

/**
 * @brief 插件信息（市场）
 */
struct MarketPluginInfo {
    QString id;                 ///< 插件ID
    QString name;               ///< 插件名称
    QString version;            ///< 版本
    QString author;             ///< 作者
    QString description;        ///< 描述
    QString category;           ///< 分类
    QString downloadUrl;        ///< 下载地址
    QString iconUrl;            ///< 图标地址
    int downloads = 0;          ///< 下载次数
    double rating = 0.0;        ///< 评分
    int ratingCount = 0;        ///< 评分人数
    QDateTime publishTime;      ///< 发布时间
    QDateTime updateTime;       ///< 更新时间
    QStringList tags;           ///< 标签
    QStringList dependencies;   ///< 依赖
    bool installed = false;     ///< 是否已安装
    bool enabled = false;       ///< 是否启用
    QString localPath;          ///< 本地路径
};

/**
 * @brief 插件评分
 */
struct PluginRating {
    QString pluginId;           ///< 插件ID
    QString userId;             ///< 用户ID
    double rating = 0.0;        ///< 评分
    QString comment;            ///< 评论
    QDateTime time;             ///< 时间
};

/**
 * @brief 插件市场管理器
 *
 * 提供插件生态系统：
 * - 插件发布和下载
 * - 插件管理
 * - 评分系统
 */
class PluginMarketManager : public QObject {
    Q_OBJECT

public:
    static PluginMarketManager* instance();

    // ========== 插件浏览 ==========

    /**
     * @brief 获取热门插件
     */
    QVector<MarketPluginInfo> getHotPlugins(int limit = 20) const;

    /**
     * @brief 获取最新插件
     */
    QVector<MarketPluginInfo> getLatestPlugins(int limit = 20) const;

    /**
     * @brief 搜索插件
     */
    QVector<MarketPluginInfo> searchPlugins(const QString& keyword,
                                      const QString& category = QString()) const;

    /**
     * @brief 获取插件详情
     */
    MarketPluginInfo getPlugin(const QString& pluginId) const;

    /**
     * @brief 获取分类列表
     */
    QStringList getCategories() const;

    /**
     * @brief 获取分类插件
     */
    QVector<MarketPluginInfo> getPluginsByCategory(const QString& category) const;

    // ========== 插件安装 ==========

    /**
     * @brief 下载插件
     */
    bool downloadPlugin(const QString& pluginId);

    /**
     * @brief 安装插件
     */
    bool installPlugin(const QString& pluginId);

    /**
     * @brief 卸载插件
     */
    bool uninstallPlugin(const QString& pluginId);

    /**
     * @brief 更新插件
     */
    bool updatePlugin(const QString& pluginId);

    /**
     * @brief 检查插件更新
     */
    QVector<MarketPluginInfo> checkUpdates() const;

    // ========== 插件管理 ==========

    /**
     * @brief 启用插件
     */
    bool enablePlugin(const QString& pluginId);

    /**
     * @brief 禁用插件
     */
    bool disablePlugin(const QString& pluginId);

    /**
     * @brief 获取已安装插件
     */
    QVector<MarketPluginInfo> getInstalledPlugins() const;

    /**
     * @brief 获取已启用插件
     */
    QVector<MarketPluginInfo> getEnabledPlugins() const;

    /**
     * @brief 加载插件
     */
    bool loadPlugin(const QString& pluginId);

    /**
     * @brief 卸载插件（运行时）
     */
    bool unloadPlugin(const QString& pluginId);

    // ========== 评分系统 ==========

    /**
     * @brief 评分插件
     */
    bool ratePlugin(const QString& pluginId, double rating, const QString& comment = QString());

    /**
     * @brief 获取插件评分
     */
    QVector<PluginRating> getPluginRatings(const QString& pluginId) const;

    // ========== 插件发布 ==========

    /**
     * @brief 发布插件
     */
    bool publishPlugin(const MarketPluginInfo& plugin);

    /**
     * @brief 更新插件信息
     */
    bool updatePluginInfo(const MarketPluginInfo& plugin);

signals:
    /**
     * @brief 插件下载完成
     */
    void pluginDownloaded(const QString& pluginId);

    /**
     * @brief 插件安装完成
     */
    void pluginInstalled(const QString& pluginId);

    /**
     * @brief 插件卸载完成
     */
    void pluginUninstalled(const QString& pluginId);

    /**
     * @brief 插件更新完成
     */
    void pluginUpdated(const QString& pluginId);

    /**
     * @brief 插件状态变化
     */
    void pluginStateChanged(const QString& pluginId, bool enabled);

    /**
     * @brief 下载进度
     */
    void downloadProgress(const QString& pluginId, int progress);

private:
    explicit PluginMarketManager(QObject* parent = nullptr);
    ~PluginMarketManager() override = default;

    void updatePluginRating(const QString& pluginId);
    bool checkDependencies(const QStringList& dependencies);

    QHash<QString, MarketPluginInfo> m_plugins;
    QHash<QString, QVector<PluginRating>> m_ratings;
    QString m_pluginDirectory;
};

#endif // PLUGINMARKETMANAGER_H