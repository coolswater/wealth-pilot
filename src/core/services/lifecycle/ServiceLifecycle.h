/**
 * @file ServiceLifecycle.h
 * @brief 服务生命周期管理 - 优雅启动和关闭
 *
 * @details 提供服务生命周期管理：
 * - 统一的初始化顺序
 * - 优雅关闭机制
 * - 资源清理保证
 * - 依赖关系管理
 */

#ifndef SERVICELIFECYCLE_H
#define SERVICELIFECYCLE_H

#include <QObject>
#include <QVector>
#include <functional>
#include <memory>

namespace WealthPilot {

/**
 * @brief 服务状态
 */
enum class ServiceState {
    Uninitialized,  ///< 未初始化
    Initializing,   ///< 初始化中
    Running,        ///< 运行中
    Stopping,       ///< 停止中
    Stopped         ///< 已停止
};

/**
 * @brief 服务描述
 */
struct ServiceDescriptor {
    QString name;                           ///< 服务名称
    int priority = 0;                       ///< 启动优先级（越小越先启动）
    QStringList dependencies;               ///< 依赖的服务
    std::function<bool()> initialize;       ///< 初始化函数
    std::function<void()> shutdown;         ///< 关闭函数
    ServiceState state = ServiceState::Uninitialized;
};

/**
 * @brief 服务生命周期管理器
 * 
 * 单例模式，负责管理所有服务的启动和关闭顺序
 */
class ServiceLifecycle : public QObject {
    Q_OBJECT

public:
    static ServiceLifecycle* instance();

    /**
     * @brief 注册服务
     */
    void registerService(const ServiceDescriptor& descriptor);

    /**
     * @brief 初始化所有服务
     * @return 是否全部成功
     */
    bool initializeAll();

    /**
     * @brief 关闭所有服务
     * 
     * 按依赖关系的逆序关闭，确保资源正确释放
     */
    void shutdownAll();

    /**
     * @brief 获取服务状态
     */
    ServiceState getServiceState(const QString& name) const;

    /**
     * @brief 检查服务是否运行
     */
    bool isServiceRunning(const QString& name) const;

signals:
    /**
     * @brief 服务状态变化信号
     */
    void serviceStateChanged(const QString& name, ServiceState state);

    /**
     * @brief 初始化进度信号
     */
    void initializationProgress(const QString& current, int completed, int total);

    /**
     * @brief 关闭进度信号
     */
    void shutdownProgress(const QString& current, int completed, int total);

private:
    ServiceLifecycle(QObject* parent = nullptr);
    ~ServiceLifecycle() override;

    void sortServicesByDependency();
    bool checkDependencies(const QString& serviceName) const;

    struct Impl;
    std::unique_ptr<Impl> d;
};

// ============================================================================
// 服务注册宏，简化服务注册
// ============================================================================

#define REGISTER_SERVICE(Name, Priority, Deps, InitFunc, ShutdownFunc) \
    ServiceLifecycle::instance()->registerService({ \
        Name, Priority, Deps, InitFunc, ShutdownFunc \
    })

} // namespace WealthPilot

#endif // SERVICELIFECYCLE_H
