/**
 * @file ServiceLocator.h
 * @brief 服务定位器 - 高性能依赖注入容器
 * @author WealthPilot Team
 * @version 2.0.0
 * 
 * @details 功能：
 * - 服务注册和解析
 * - 生命周期管理（单例、瞬态）
 * - 延迟初始化
 * - 线程安全
 * - 性能优化：使用快速查找表
 * 
 * @thread_safe 所有公共方法都是线程安全的
 */
#ifndef SERVICELOCATOR_H
#define SERVICELOCATOR_H

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <functional>
#include <memory>
#include <typeindex>
#include <type_traits>

/**
 * @brief 服务生命周期
 */
enum class ServiceLifetime {
    Singleton,      // 单例模式
    Transient,      // 瞬态模式（每次创建新实例）
    Scoped         // 作用域模式
};

/**
 * @brief 服务描述符
 */
struct ServiceDescriptor {
    std::type_index type = std::type_index(typeid(void));  // 服务类型
    ServiceLifetime lifetime = ServiceLifetime::Singleton; // 生命周期
    std::function<QObject*()> factory;                      // 工厂函数
    QObject* instance = nullptr;                            // 单例实例
    bool initialized = false;                               // 是否已初始化
};

/**
 * @brief 服务定位器 - 高性能DI容器
 * @thread_safe 所有公共方法都是线程安全的
 */
class ServiceLocator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取服务定位器单例
     */
    static ServiceLocator& instance();

    /**
     * @brief 注册服务（单例模式）
     */
    template<typename TInterface, typename TImplementation>
    void registerSingleton();

    /**
     * @brief 注册服务（瞬态模式）
     */
    template<typename TInterface, typename TImplementation>
    void registerTransient();

    /**
     * @brief 注册服务（带工厂函数）
     */
    template<typename TInterface>
    void registerFactory(std::function<TInterface*()> factory, ServiceLifetime lifetime = ServiceLifetime::Singleton);

    /**
     * @brief 注册已存在的实例
     */
    template<typename TInterface>
    void registerInstance(TInterface* instance);

    /**
     * @brief 解析服务
     */
    template<typename TInterface>
    TInterface* resolve();

    /**
     * @brief 尝试解析服务（失败返回nullptr）
     */
    template<typename TInterface>
    TInterface* tryResolve();

    /**
     * @brief 检查服务是否已注册
     */
    template<typename TInterface>
    bool isRegistered() const;

    /**
     * @brief 注销服务
     */
    template<typename TInterface>
    void unregister();

    /**
     * @brief 清空所有服务
     */
    void clear();

    /**
     * @brief 获取已注册服务数量
     */
    int count() const;

signals:
    /**
     * @brief 服务注册信号
     */
    void serviceRegistered(const QString& serviceName);

    /**
     * @brief 服务解析信号
     */
    void serviceResolved(const QString& serviceName);

private:
    ServiceLocator();
    ~ServiceLocator();
    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;

    // 内部实现
    void registerServiceInternal(const std::type_index& type, ServiceDescriptor descriptor);
    QObject* resolveInternal(const std::type_index& type);
    bool isRegisteredInternal(const std::type_index& type) const;

    // 性能优化：使用type_index作为key，避免字符串比较
    QHash<std::type_index, ServiceDescriptor> m_services;
    mutable QMutex m_mutex;
    
    // 性能优化：缓存常用服务
    QHash<std::type_index, QObject*> m_cache;
};

// ========== 模板实现 ==========

template<typename TInterface, typename TImplementation>
void ServiceLocator::registerSingleton()
{
    static_assert(std::is_base_of<TInterface, TImplementation>::value, 
        "TImplementation must inherit from TInterface");
    
    ServiceDescriptor descriptor;
    descriptor.type = std::type_index(typeid(TInterface));
    descriptor.lifetime = ServiceLifetime::Singleton;
    descriptor.factory = []() -> QObject* {
        return new TImplementation();
    };
    
    registerServiceInternal(std::type_index(typeid(TInterface)), descriptor);
}

template<typename TInterface, typename TImplementation>
void ServiceLocator::registerTransient()
{
    static_assert(std::is_base_of<TInterface, TImplementation>::value, 
        "TImplementation must inherit from TInterface");
    
    ServiceDescriptor descriptor;
    descriptor.type = std::type_index(typeid(TInterface));
    descriptor.lifetime = ServiceLifetime::Transient;
    descriptor.factory = []() -> QObject* {
        return new TImplementation();
    };
    
    registerServiceInternal(std::type_index(typeid(TInterface)), descriptor);
}

template<typename TInterface>
void ServiceLocator::registerFactory(std::function<TInterface*()> factory, ServiceLifetime lifetime)
{
    ServiceDescriptor descriptor;
    descriptor.type = std::type_index(typeid(TInterface));
    descriptor.lifetime = lifetime;
    descriptor.factory = [factory]() -> QObject* {
        return factory();
    };
    
    registerServiceInternal(std::type_index(typeid(TInterface)), descriptor);
}

template<typename TInterface>
void ServiceLocator::registerInstance(TInterface* instance)
{
    ServiceDescriptor descriptor;
    descriptor.type = std::type_index(typeid(TInterface));
    descriptor.lifetime = ServiceLifetime::Singleton;
    descriptor.instance = instance;
    descriptor.initialized = true;
    
    registerServiceInternal(std::type_index(typeid(TInterface)), descriptor);
}

template<typename TInterface>
TInterface* ServiceLocator::resolve()
{
    QObject* obj = resolveInternal(std::type_index(typeid(TInterface)));
    if (!obj) {
        throw std::runtime_error(QString("Service not registered: %1")
            .arg(typeid(TInterface).name()).toStdString());
    }
    return qobject_cast<TInterface*>(obj);
}

template<typename TInterface>
TInterface* ServiceLocator::tryResolve()
{
    QObject* obj = resolveInternal(std::type_index(typeid(TInterface)));
    return obj ? qobject_cast<TInterface*>(obj) : nullptr;
}

template<typename TInterface>
bool ServiceLocator::isRegistered() const
{
    return isRegisteredInternal(std::type_index(typeid(TInterface)));
}

template<typename TInterface>
void ServiceLocator::unregister()
{
    QMutexLocker locker(&m_mutex);
    auto type = std::type_index(typeid(TInterface));
    m_services.remove(type);
    m_cache.remove(type);
}

#endif // SERVICELOCATOR_H
