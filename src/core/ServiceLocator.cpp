/**
 * @file ServiceLocator.cpp
 * @brief 服务定位器实现
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "ServiceLocator.h"
#include "../utils/Logger.h"
#include <QElapsedTimer>

ServiceLocator& ServiceLocator::instance()
{
    static ServiceLocator instance;
    return instance;
}

ServiceLocator::ServiceLocator()
{
    LOG_DEBUG("ServiceLocator created");
}

ServiceLocator::~ServiceLocator()
{
    clear();
    LOG_DEBUG("ServiceLocator destroyed");
}

void ServiceLocator::registerServiceInternal(const std::type_index& type, ServiceDescriptor descriptor)
{
    QMutexLocker locker(&m_mutex);
    
    // 如果已存在，先清理旧实例
    if (m_services.contains(type)) {
        auto& oldDescriptor = m_services[type];
        if (oldDescriptor.instance && oldDescriptor.lifetime == ServiceLifetime::Singleton) {
            oldDescriptor.instance->deleteLater();
        }
        m_cache.remove(type);
    }
    
    m_services[type] = descriptor;
    
    QString typeName = QString::fromStdString(type.name());
    LOG_DEBUG(QString("Service registered: %1, lifetime: %2")
        .arg(typeName)
        .arg(descriptor.lifetime == ServiceLifetime::Singleton ? "Singleton" : "Transient"));
    
    locker.unlock();
    emit serviceRegistered(typeName);
}

QObject* ServiceLocator::resolveInternal(const std::type_index& type)
{
    QMutexLocker locker(&m_mutex);
    
    // 性能优化：先检查缓存
    if (m_cache.contains(type)) {
        return m_cache[type];
    }
    
    // 查找服务描述符
    auto it = m_services.find(type);
    if (it == m_services.end()) {
        return nullptr;
    }
    
    auto& descriptor = it.value();
    QObject* instance = nullptr;
    
    switch (descriptor.lifetime) {
        case ServiceLifetime::Singleton:
            // 单例模式：如果已初始化，直接返回
            if (descriptor.initialized && descriptor.instance) {
                instance = descriptor.instance;
            } else {
                // 创建新实例
                QElapsedTimer timer;
                timer.start();
                
                instance = descriptor.factory();
                descriptor.instance = instance;
                descriptor.initialized = true;
                
                // 缓存实例
                m_cache[type] = instance;
                
                LOG_DEBUG(QString("Singleton service created: %1, time: %2ms")
                    .arg(QString::fromStdString(type.name()))
                    .arg(timer.elapsed()));
            }
            break;
            
        case ServiceLifetime::Transient:
            // 瞬态模式：每次创建新实例
            instance = descriptor.factory();
            LOG_DEBUG(QString("Transient service created: %1")
                .arg(QString::fromStdString(type.name())));
            break;
            
        case ServiceLifetime::Scoped:
            // 作用域模式：暂时按单例处理
            if (descriptor.initialized && descriptor.instance) {
                instance = descriptor.instance;
            } else {
                instance = descriptor.factory();
                descriptor.instance = instance;
                descriptor.initialized = true;
                m_cache[type] = instance;
            }
            break;
    }
    
    locker.unlock();
    emit serviceResolved(QString::fromStdString(type.name()));
    
    return instance;
}

bool ServiceLocator::isRegisteredInternal(const std::type_index& type) const
{
    QMutexLocker locker(&m_mutex);
    return m_services.contains(type);
}

void ServiceLocator::clear()
{
    QMutexLocker locker(&m_mutex);
    
    // 清理所有单例实例
    for (auto it = m_services.begin(); it != m_services.end(); ++it) {
        auto& descriptor = it.value();
        if (descriptor.instance && descriptor.lifetime == ServiceLifetime::Singleton) {
            descriptor.instance->deleteLater();
        }
    }
    
    m_services.clear();
    m_cache.clear();
    
    LOG_INFO("All services cleared");
}

int ServiceLocator::count() const
{
    QMutexLocker locker(&m_mutex);
    return m_services.size();
}
