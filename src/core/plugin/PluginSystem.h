/**
 * @file PluginSystem.h
 * @brief 插件系统 - 支持Python脚本自定义指标
 *
 * @details 提供插件功能：
 * - Python解释器集成
 * - 脚本API接口
 * - 指标计算
 * - 脚本管理
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PLUGINSYSTEM_H
#define PLUGINSYSTEM_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QString>
#include <QVariant>

/**
 * @brief 插件类型
 */
enum class PluginType {
    Indicator,      ///< 指标插件
    Strategy,       ///< 策略插件
    Alert,          ///< 预警插件
    Utility         ///< 工具插件
};

/**
 * @brief 插件信息
 */
struct PluginInfo {
    QString id;                 ///< 插件ID
    QString name;               ///< 插件名称
    QString version;            ///< 版本号
    QString author;             ///< 作者
    QString description;        ///< 描述
    PluginType type;            ///< 插件类型
    QString scriptPath;         ///< 脚本路径
    QString iconPath;           ///< 图标路径
    bool enabled = true;        ///< 是否启用
    QDateTime installTime;      ///< 安装时间
    QDateTime updateTime;       ///< 更新时间
};

/**
 * @brief 指标计算结果
 */
struct IndicatorResult {
    QString name;               ///< 指标名称
    QVector<double> values;     ///< 指标值数组
    QString color;              ///< 显示颜色
    int lineWidth = 1;          ///< 线宽
    QString lineStyle;          ///< 线型（solid/dash/dot）
    bool visible = true;        ///< 是否可见
};

/**
 * @brief 插件API上下文
 */
struct PluginContext {
    QString symbol;             ///< 股票代码
    QVector<double> prices;     ///< 价格数据（收盘价）
    QVector<double> highs;      ///< 最高价
    QVector<double> lows;       ///< 最低价
    QVector<double> opens;      ///< 开盘价
    QVector<qint64> volumes;    ///< 成交量
    QVector<QDateTime> times;   ///< 时间序列
    QVariantMap params;         ///< 自定义参数
};

/**
 * @brief 插件系统
 */
class PluginSystem : public QObject
{
    Q_OBJECT

public:
    static PluginSystem* instance();

    /**
     * @brief 初始化插件系统
     */
    bool initialize();

    /**
     * @brief 安装插件
     */
    bool installPlugin(const QString& scriptPath);

    /**
     * @brief 卸载插件
     */
    bool uninstallPlugin(const QString& pluginId);

    /**
     * @brief 启用/禁用插件
     */
    void setPluginEnabled(const QString& pluginId, bool enabled);

    /**
     * @brief 获取插件列表
     */
    QVector<PluginInfo> getPlugins(PluginType type = PluginType::Indicator) const;

    /**
     * @brief 获取插件信息
     */
    PluginInfo getPluginInfo(const QString& pluginId) const;

    /**
     * @brief 执行指标计算
     */
    IndicatorResult calculateIndicator(const QString& pluginId,
                                       const PluginContext& context);

    /**
     * @brief 批量计算指标
     */
    QMap<QString, IndicatorResult> calculateIndicators(
        const QVector<QString>& pluginIds,
        const PluginContext& context);

    /**
     * @brief 验证脚本
     */
    bool validateScript(const QString& scriptPath, QString& errorMsg);

    /**
     * @brief 获取脚本模板
     */
    QString getScriptTemplate(PluginType type) const;

    /**
     * @brief 设置Python路径
     */
    void setPythonPath(const QString& path);

signals:
    void pluginInstalled(const PluginInfo& plugin);
    void pluginUninstalled(const QString& pluginId);
    void pluginEnabledChanged(const QString& pluginId, bool enabled);
    void pluginError(const QString& pluginId, const QString& error);

private:
    explicit PluginSystem(QObject* parent = nullptr);
    ~PluginSystem() override = default;

    // Python执行
    bool executePython(const QString& script, const QVariantMap& params, QVariantMap& result);
    QString buildScript(const QString& scriptPath, const PluginContext& context);

    // 插件管理
    QString generatePluginId() const;
    bool parsePluginMetadata(const QString& scriptPath, PluginInfo& info);

    // 数据成员
    QMap<QString, PluginInfo> m_plugins;
    QString m_pythonPath;
    bool m_initialized = false;
};

#endif // PLUGINSYSTEM_H