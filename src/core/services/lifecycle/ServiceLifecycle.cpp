/**
 * @file ServiceLifecycle.cpp
 * @brief 服务生命周期管理实现
 */

#include "ServiceLifecycle.h"
#include "shared/utils/Logger.h"

#include <QMutexLocker>
#include <algorithm>

namespace WealthPilot {

struct ServiceLifecycle::Impl {
    QVector<ServiceDescriptor> services;
    QHash<QString, int> serviceIndex;  ///< 名称到索引的映射
    QMutex mutex;
};

ServiceLifecycle* ServiceLifecycle::instance()
{
    static ServiceLifecycle* inst = new ServiceLifecycle();
    return inst;
}

ServiceLifecycle::ServiceLifecycle(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
}

ServiceLifecycle::~ServiceLifecycle()
{
    shutdownAll();
}

void ServiceLifecycle::registerService(const ServiceDescriptor& descriptor)
{
    QMutexLocker locker(&d->mutex);
    
    if (d->serviceIndex.contains(descriptor.name)) {
        LOG_WARN(QString("Service already registered: %1").arg(descriptor.name));
        return;
    }
    
    d->services.append(descriptor);
    d->serviceIndex[descriptor.name] = d->services.size() - 1;
    
    LOG_DEBUG(QString("Service registered: %1 (priority: %2)")
        .arg(descriptor.name).arg(descriptor.priority));
}

bool ServiceLifecycle::initializeAll()
{
    QMutexLocker locker(&d->mutex);
    
    // 按依赖关系排序
    sortServicesByDependency();
    
    int total = d->services.size();
    int completed = 0;
    bool allSuccess = true;
    
    for (auto& service : d->services) {
        service.state = ServiceState::Initializing;
        emit serviceStateChanged(service.name, service.state);
        emit initializationProgress(service.name, completed, total);
        
        // 检查依赖
        if (!checkDependencies(service.name)) {
            LOG_ERROR(QString("Dependencies not satisfied for: %1")
                .arg(service.name));
            service.state = ServiceState::Stopped;
            allSuccess = false;
            continue;
        }
        
        // 执行初始化
        bool success = false;
        if (service.initialize) {
            try {
                success = service.initialize();
            } catch (const std::exception& e) {
                LOG_ERROR(QString("Service initialization failed: %1 - %2")
                    .arg(service.name).arg(e.what()));
            }
        }
        
        if (success) {
            service.state = ServiceState::Running;
            LOG_INFO(QString("Service initialized: %1").arg(service.name));
        } else {
            service.state = ServiceState::Stopped;
            LOG_ERROR(QString("Service initialization failed: %1").arg(service.name));
            allSuccess = false;
        }
        
        emit serviceStateChanged(service.name, service.state);
        completed++;
    }
    
    return allSuccess;
}

void ServiceLifecycle::shutdownAll()
{
    QMutexLocker locker(&d->mutex);
    
    // 按逆序关闭（先关闭依赖者，再关闭被依赖者）
    int total = d->services.size();
    int completed = 0;
    
    for (int i = d->services.size() - 1; i >= 0; --i) {
        auto& service = d->services[i];
        
        if (service.state != ServiceState::Running) {
            continue;
        }
        
        service.state = ServiceState::Stopping;
        emit serviceStateChanged(service.name, service.state);
        emit shutdownProgress(service.name, completed, total);
        
        // 执行关闭
        if (service.shutdown) {
            try {
                service.shutdown();
            } catch (const std::exception& e) {
                LOG_ERROR(QString("Service shutdown failed: %1 - %2")
                    .arg(service.name).arg(e.what()));
            }
        }
        
        service.state = ServiceState::Stopped;
        LOG_INFO(QString("Service stopped: %1").arg(service.name));
        
        emit serviceStateChanged(service.name, service.state);
        completed++;
    }
}

ServiceState ServiceLifecycle::getServiceState(const QString& name) const
{
    QMutexLocker locker(&d->mutex);
    
    if (d->serviceIndex.contains(name)) {
        return d->services[d->serviceIndex[name]].state;
    }
    return ServiceState::Uninitialized;
}

bool ServiceLifecycle::isServiceRunning(const QString& name) const
{
    return getServiceState(name) == ServiceState::Running;
}

void ServiceLifecycle::sortServicesByDependency()
{
    // 使用拓扑排序，确保依赖的服务先启动
    // 简化实现：按优先级排序，依赖检查在初始化时进行
    
    std::sort(d->services.begin(), d->services.end(),
        [](const ServiceDescriptor& a, const ServiceDescriptor& b) {
            return a.priority < b.priority;
        });
    
    // 更新索引映射
    d->serviceIndex.clear();
    for (int i = 0; i < d->services.size(); ++i) {
        d->serviceIndex[d->services[i].name] = i;
    }
}

bool ServiceLifecycle::checkDependencies(const QString& serviceName) const
{
    if (!d->serviceIndex.contains(serviceName)) {
        return false;
    }
    
    const auto& service = d->services[d->serviceIndex[serviceName]];
    
    for (const QString& dep : service.dependencies) {
        if (!d->serviceIndex.contains(dep)) {
            LOG_WARN(QString("Dependency not registered: %1 -> %2")
                .arg(serviceName).arg(dep));
            return false;
        }
        
        if (d->services[d->serviceIndex[dep]].state != ServiceState::Running) {
            return false;
        }
    }
    
    return true;
}

} // namespace WealthPilot