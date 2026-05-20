/**
 * @file ServiceRegistry.cpp
 * @brief 服务注册助手实现
 */

#include "ServiceRegistry.h"
#include <QMutexLocker>
#include <algorithm>

namespace WealthPilot {

ServiceRegistry& ServiceRegistry::instance()
{
    static ServiceRegistry instance;
    return instance;
}

ServiceRegistry::ServiceRegistry()
{
}

void ServiceRegistry::declareService(const ServiceRegistrationInfo& info)
{
    m_registrations.append(info);
    m_healthStatus[info.name] = true; // 默认健康
}

int ServiceRegistry::registerAll()
{
    // 按优先级排序
    std::sort(m_registrations.begin(), m_registrations.end(),
              [](const ServiceRegistrationInfo& a, const ServiceRegistrationInfo& b) {
                  return a.priority > b.priority;
              });

    int count = 0;
    for (const auto& info : m_registrations) {
        if (!m_registeredServices.value(info.name, false)) {
            m_registeredServices[info.name] = true;
            emit serviceRegistered(info.name);
            count++;
        }
    }

    LOG_INFO(QString("[ServiceRegistry] Registered %1 services").arg(count));
    return count;
}

int ServiceRegistry::registerByPriority(int threshold)
{
    int count = 0;
    for (const auto& info : m_registrations) {
        if (info.priority >= threshold && !m_registeredServices.value(info.name, false)) {
            m_registeredServices[info.name] = true;
            emit serviceRegistered(info.name);
            count++;
        }
    }

    LOG_INFO(QString("[ServiceRegistry] Registered %1 services with priority >= %2")
             .arg(count).arg(threshold));
    return count;
}

int ServiceRegistry::initializeAll()
{
    int count = 0;
    for (const auto& info : m_registrations) {
        if (m_registeredServices.value(info.name, false) && !info.lazyInit) {
            emit serviceInitialized(info.name);
            count++;
        }
    }

    LOG_INFO(QString("[ServiceRegistry] Initialized %1 services").arg(count));
    return count;
}

int ServiceRegistry::healthCheckAll()
{
    int healthy = 0;
    for (const auto& info : m_registrations) {
        if (info.healthCheck) {
            bool isHealthy = info.healthCheck();
            m_healthStatus[info.name] = isHealthy;

            if (!isHealthy) {
                emit healthCheckFailed(info.name, QStringLiteral("Health check failed"));
                LOG_WARNING(QString("[ServiceRegistry] Service %1 health check failed").arg(info.name));
            } else {
                healthy++;
            }
        } else {
            healthy++; // 没有健康检查函数的服务默认健康
        }
    }

    LOG_INFO(QString("[ServiceRegistry] Health check: %1/%2 services healthy")
             .arg(healthy).arg(m_registrations.size()));
    return healthy;
}

QVector<ServiceRegistrationInfo> ServiceRegistry::getRegistrationInfos() const
{
    return m_registrations;
}

QString ServiceRegistry::getDependencyGraph() const
{
    QString graph = QStringLiteral("Service Dependency Graph:\n");

    for (const auto& info : m_registrations) {
        graph += QString("  %1 (priority: %2)\n").arg(info.name).arg(info.priority);

        if (!info.dependencies.isEmpty()) {
            for (const auto& dep : info.dependencies) {
                graph += QString("    -> %1\n").arg(dep);
            }
        }
    }

    return graph;
}

bool ServiceRegistry::isServiceHealthy(const QString& serviceName) const
{
    return m_healthStatus.value(serviceName, true);
}

QVector<QString> ServiceRegistry::getUnhealthyServices() const
{
    QVector<QString> unhealthy;

    for (auto it = m_healthStatus.constBegin(); it != m_healthStatus.constEnd(); ++it) {
        if (!it.value()) {
            unhealthy.append(it.key());
        }
    }

    return unhealthy;
}

} // namespace WealthPilot