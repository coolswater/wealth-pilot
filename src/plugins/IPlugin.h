/**
 * @file IPlugin.h
 * @brief 插件接口 - 定义所有插件必须实现的接口
 *
 * @details 功能：
 * - 插件生命周期管理
 * - 插件元数据
 * - 插件依赖管理
 * - 插件配置
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonObject>

/**
 * @brief 插件状态
 */
enum class PluginState {
    Unloaded,       // 未加载
    Loading,        // 加载中
    Loaded,         // 已加载
    Initialized,    // 已初始化
    Running,        // 运行中
    Stopped,        // 已停止
    Error          // 错误
};

/**
 * @brief 插件元数据
 */
struct PluginMetaData {
    QString name;               // 插件名称
    QString version;            // 插件版本
    QString description;        // 插件描述
    QString author;             // 作者
    QString license;            // 许可证
    QString website;            // 网站
    QStringList dependencies;   // 依赖的其他插件
    int priority;              // 加载优先级（数字越小越先加载）
    bool enableHotReload;      // 是否支持热重载
};

/**
 * @brief 插件接口 - 所有插件必须实现此接口
 */
class IPlugin : public QObject
{
    Q_OBJECT

public:
    virtual ~IPlugin() = default;

    // ========== 插件元数据 ==========

    /**
     * @brief 获取插件元数据
     */
    virtual PluginMetaData metaData() const = 0;

    /**
     * @brief 获取插件状态
     */
    virtual PluginState state() const = 0;

    // ========== 生命周期管理 ==========

    /**
     * @brief 加载插件
     * @return 是否成功
     */
    virtual bool load() = 0;

    /**
     * @brief 初始化插件
     * @param config 配置参数
     * @return 是否成功
     */
    virtual bool initialize(const QJsonObject& config = QJsonObject()) = 0;

    /**
     * @brief 启动插件
     * @return 是否成功
     */
    virtual bool start() = 0;

    /**
     * @brief 停止插件
     */
    virtual void stop() = 0;

    /**
     * @brief 卸载插件
     */
    virtual void unload() = 0;

    // ========== 配置管理 ==========

    /**
     * @brief 获取配置
     */
    virtual QJsonObject configuration() const = 0;

    /**
     * @brief 设置配置
     */
    virtual void setConfiguration(const QJsonObject& config) = 0;

    // ========== 依赖管理 ==========

    /**
     * @brief 检查依赖是否满足
     */
    virtual bool checkDependencies() const = 0;

    /**
     * @brief 获取依赖的插件列表
     */
    virtual QStringList dependencies() const = 0;

signals:
    /**
     * @brief 状态改变信号
     */
    void stateChanged(PluginState newState);

    /**
     * @brief 错误信号
     */
    void errorOccurred(const QString& error);

    /**
     * @brief 配置改变信号
     */
    void configurationChanged();
};

Q_DECLARE_INTERFACE(IPlugin, "com.wealthpilot.IPlugin/2.0")

#endif // IPLUGIN_H
