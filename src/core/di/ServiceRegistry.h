/**
 * @file ServiceRegistry.h
 * @brief 服务注册助手 - 简化服务层依赖注入
 *
 * @details 功能：
 * - 集中管理服务注册
 * - 提供服务初始化顺序
 * - 支持服务依赖声明
 * - 服务健康检查
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SERVICEREGISTRY_H
#define SERVICEREGISTRY_H

#include "ServiceLocator.h"
#include "utils/Logger.h"
#include <QObject>
#include <QString>
#include <QVector>
#include <functional>

namespace WealthPilot {

/**
 * @brief 服务注册信息
 */
struct ServiceRegistrationInfo {
    QString name;                           ///< 服务名称
    QString description;                    ///< 服务描述
    QVector<QString> dependencies;          ///< 依赖的服务
    int priority = 0;                       ///< 注册优先级（越高越先注册）
    bool lazyInit = false;                  ///< 是否延迟初始化
    bool isCritical = false;                ///< 是否关键服务
    std::function<bool()> healthCheck;      ///< 健康检查函数
};

/**
 * @brief 服务注册助手类
 *
 * @details 简化服务注册流程，提供服务依赖管理和健康检查
 */
class ServiceRegistry : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例
     */
    static ServiceRegistry& instance();

    /**
     * @brief 声明服务（用于依赖分析）
     * @param info 服务注册信息
     */
    void declareService(const ServiceRegistrationInfo& info);

    /**
     * @brief 注册所有已声明的服务
     * @return 注册成功的服务数量
     */
    int registerAll();

    /**
     * @brief 按优先级注册服务
     * @param priority 优先级阈值（只注册优先级 >= threshold 的服务）
     */
    int registerByPriority(int threshold);

    /**
     * @brief 初始化所有服务
     * @return 初始化成功的服务数量
     */
    int initializeAll();

    /**
     * @brief 执行健康检查
     * @return 健康的服务数量
     */
    int healthCheckAll();

    /**
     * @brief 获取服务注册信息
     */
    QVector<ServiceRegistrationInfo> getRegistrationInfos() const;

    /**
     * @brief 获取服务依赖图
     */
    QString getDependencyGraph() const;

    /**
     * @brief 检查服务是否健康
     */
    bool isServiceHealthy(const QString& serviceName) const;

    /**
     * @brief 获取不健康的服务列表
     */
    QVector<QString> getUnhealthyServices() const;

signals:
    /**
     * @brief 服务注册完成信号
     */
    void serviceRegistered(const QString& serviceName);

    /**
     * @brief 服务初始化完成信号
     */
    void serviceInitialized(const QString& serviceName);

    /**
     * @brief 服务健康检查失败信号
     */
    void healthCheckFailed(const QString& serviceName, const QString& reason);

private:
    ServiceRegistry();
    ~ServiceRegistry() = default;

    // 服务注册信息
    QVector<ServiceRegistrationInfo> m_registrations;

    // 服务健康状态
    QHash<QString, bool> m_healthStatus;

    // 已注册的服务
    QHash<QString, bool> m_registeredServices;
};

// ============================================================================
// 常用服务注册宏
// ============================================================================

/**
 * @brief 声明单例服务
 */
#define DECLARE_SINGLETON_SERVICE(Name, Interface, Impl, Priority, Critical) \
    ServiceRegistry::instance().declareService({ \
        QStringLiteral(Name), \
        QStringLiteral(#Interface), \
        {}, \
        Priority, \
        false, \
        Critical \
    });

/**
 * @brief 声明带依赖的服务
 */
#define DECLARE_SERVICE_WITH_DEPS(Name, Interface, Impl, Deps, Priority) \
    ServiceRegistry::instance().declareService({ \
        QStringLiteral(Name), \
        QStringLiteral(#Interface), \
        Deps, \
        Priority, \
        false, \
        false \
    });

// ============================================================================
// 服务注册便捷函数
// ============================================================================

namespace Services {

/**
 * @brief 注册核心服务
 */
inline void registerCoreServices()
{
    auto& locator = ServiceLocator::instance();

    // 注册核心服务
    // locator.registerSingleton<DataHub, DataHub>();
    // locator.registerSingleton<DatabaseManager, DatabaseManager>();
    // locator.registerSingleton<ConfigManager, ConfigManager>();

    LOG_INFO("[ServiceRegistry] Core services registered");
}

/**
 * @brief 注册数据服务
 */
inline void registerDataServices()
{
    auto& locator = ServiceLocator::instance();

    // 注册数据服务
    // locator.registerSingleton<MarketDataStorage, MarketDataStorage>();
    // locator.registerSingleton<DataCacheManager, DataCacheManager>();

    LOG_INFO("[ServiceRegistry] Data services registered");
}

/**
 * @brief 注册交易服务
 */
inline void registerTradingServices()
{
    auto& locator = ServiceLocator::instance();

    // 注册交易服务
    // locator.registerSingleton<OrderManager, OrderManager>();
    // locator.registerSingleton<TradingService, TradingService>();
    // locator.registerSingleton<RiskManager, RiskManager>();

    LOG_INFO("[ServiceRegistry] Trading services registered");
}

/**
 * @brief 注册所有服务
 */
inline void registerAllServices()
{
    registerCoreServices();
    registerDataServices();
    registerTradingServices();

    LOG_INFO("[ServiceRegistry] All services registered");
}

/**
 * @brief 初始化所有服务
 */
inline void initializeAllServices()
{
    auto& locator = ServiceLocator::instance();

    // 按依赖顺序初始化
    // auto* dataHub = locator.resolve<DataHub>();
    // if (dataHub) dataHub->initialize();

    // auto* dbManager = locator.resolve<DatabaseManager>();
    // if (dbManager) dbManager->initialize();

    LOG_INFO("[ServiceRegistry] All services initialized");
}

} // namespace Services

} // namespace WealthPilot

#endif // SERVICEREGISTRY_H