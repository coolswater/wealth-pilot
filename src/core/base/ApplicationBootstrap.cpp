/**
 * @file ApplicationBootstrap.cpp
 * @brief 应用启动引导实现
 */

#include "ApplicationBootstrap.h"
#include "../config/ConfigManager.h"
#include "../cache/CacheManager.h"
#include "../database/DatabaseManager.h"
#include "../di/ServiceLocator.h"
#include "../performance/PerformanceManager.h"
#include "utils/Logger.h"
#include <QElapsedTimer>
#include <QMainWindow>

namespace WealthPilot {

ApplicationBootstrap* ApplicationBootstrap::instance()
{
    static ApplicationBootstrap* inst = new ApplicationBootstrap();
    return inst;
}

ApplicationBootstrap::ApplicationBootstrap(QObject* parent)
    : QObject(parent)
{
    LOG_INFO("ApplicationBootstrap created");
}

InitResult ApplicationBootstrap::initialize(QMainWindow* mainWindow)
{
    if (m_initialized) {
        LOG_WARNING("ApplicationBootstrap already initialized");
        return {true, "complete", "", 0};
    }

    m_mainWindow = mainWindow;
    m_phaseResults.clear();

    QElapsedTimer totalTimer;
    totalTimer.start();

    LOG_INFO("Starting application bootstrap...");

    // Phase 1: 基础服务
    auto result1 = initializePhase(InitPhase::CoreServices);
    if (!result1.success) return result1;

    // Phase 2: 数据层
    auto result2 = initializePhase(InitPhase::DataLayer);
    if (!result2.success) return result2;

    // Phase 3: 业务层
    auto result3 = initializePhase(InitPhase::BusinessLayer);
    if (!result3.success) return result3;

    // Phase 4: UI层
    auto result4 = initializePhase(InitPhase::UILayer);
    if (!result4.success) return result4;

    m_currentPhase = InitPhase::Complete;
    m_initialized = true;

    qint64 totalMs = totalTimer.elapsed();
    LOG_INFO(QString("Application bootstrap completed in %1ms").arg(totalMs));

    return {true, "complete", "", totalMs};
}

InitResult ApplicationBootstrap::initializePhase(InitPhase phase)
{
    QElapsedTimer timer;
    timer.start();

    m_currentPhase = phase;
    emit phaseStarted(phase);

    InitResult result;

    switch (phase) {
    case InitPhase::CoreServices:
        result = initializeCoreServices();
        break;
    case InitPhase::DataLayer:
        result = initializeDataLayer();
        break;
    case InitPhase::BusinessLayer:
        result = initializeBusinessLayer();
        break;
    case InitPhase::UILayer:
        result = initializeUILayer();
        break;
    default:
        result = {false, "unknown", "Unknown phase", 0};
        break;
    }

    result.elapsedMs = timer.elapsed();
    m_phaseResults.append(result);

    emit phaseCompleted(phase, result);
    emit progressChanged(static_cast<int>(phase), 4, result.phase);

    return result;
}

InitResult ApplicationBootstrap::initializeCoreServices()
{
    LOG_INFO("Phase 1: Initializing core services...");

    try {
        // 初始化配置管理器
        ConfigManager::instance();

        // 初始化缓存管理器
        CacheManager::instance();

        // 初始化性能管理器
        PerformanceManager::instance();

        // 初始化服务定位器
        ServiceLocator::instance();

        LOG_INFO("Core services initialized");
        return {true, "CoreServices", "", 0};
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Core services init failed: %1").arg(e.what()));
        return {false, "CoreServices", e.what(), 0};
    }
}

InitResult ApplicationBootstrap::initializeDataLayer()
{
    LOG_INFO("Phase 2: Initializing data layer...");

    try {
        // 初始化数据库管理器
        DatabaseManager::instance();

        // TODO: 初始化数据源管理器
        // DataSourceManager::instance();

        LOG_INFO("Data layer initialized");
        return {true, "DataLayer", "", 0};
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Data layer init failed: %1").arg(e.what()));
        return {false, "DataLayer", e.what(), 0};
    }
}

InitResult ApplicationBootstrap::initializeBusinessLayer()
{
    LOG_INFO("Phase 3: Initializing business layer...");

    try {
        // 初始化业务模块（延迟加载，按需初始化）
        // 这些模块使用单例模式，首次调用时自动初始化

        LOG_INFO("Business layer initialized");
        return {true, "BusinessLayer", "", 0};
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Business layer init failed: %1").arg(e.what()));
        return {false, "BusinessLayer", e.what(), 0};
    }
}

InitResult ApplicationBootstrap::initializeUILayer()
{
    LOG_INFO("Phase 4: Initializing UI layer...");

    try {
        if (m_mainWindow) {
            // 初始化快捷键管理器
            // ShortcutManager::instance()->initialize(m_mainWindow);

            // 初始化布局管理器
            // LayoutManager::instance()->initialize(m_mainWindow);
        }

        LOG_INFO("UI layer initialized");
        return {true, "UILayer", "", 0};
    } catch (const std::exception& e) {
        LOG_ERROR(QString("UI layer init failed: %1").arg(e.what()));
        return {false, "UILayer", e.what(), 0};
    }
}

void ApplicationBootstrap::registerPhaseCallback(InitPhase phase, std::function<InitResult()> callback)
{
    m_phaseCallbacks[phase] = callback;
}

QString ApplicationBootstrap::getStartupReport() const
{
    QString report;
    report += QStringLiteral("========== 应用启动报告 ==========\n\n");

    report += QStringLiteral("初始化阶段:\n");
    report += QString("  当前阶段: %1\n").arg(static_cast<int>(m_currentPhase));
    report += QString("  初始化状态: %1\n\n").arg(m_initialized ? "完成" : "未完成");

    report += QStringLiteral("各阶段耗时:\n");
    for (const auto& result : m_phaseResults) {
        report += QString("  %1: %2ms (%3)\n")
            .arg(result.phase)
            .arg(result.elapsedMs)
            .arg(result.success ? "成功" : "失败");
        if (!result.error.isEmpty()) {
            report += QString("    错误: %1\n").arg(result.error);
        }
    }

    return report;
}

} // namespace WealthPilot
