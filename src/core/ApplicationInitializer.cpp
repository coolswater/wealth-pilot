/**
 * @file ApplicationInitializer.cpp
 * @brief 应用初始化管理器实现
 */

#include "ApplicationInitializer.h"
#include "EnvironmentConfig.h"
#include "CacheManager.h"
#include "ServiceLocator.h"
#include "DatabaseManager.h"
#include "AsyncTaskManager.h"
#include "../plugins/PluginLoader.h"
#include "../ui/components/ThemeEngine.h"
#include "../utils/Logger.h"

#include <QElapsedTimer>
#include <QCoreApplication>
#include <QThread>

ApplicationInitializer& ApplicationInitializer::instance()
{
    static ApplicationInitializer instance;
    return instance;
}

ApplicationInitializer::ApplicationInitializer()
    : m_currentPhase(InitPhase::Core)
    , m_initialized(false)
{
    // 注册核心模块
    registerModule("EnvironmentConfig", InitPhase::Core, []() {
        return EnvironmentConfig::instance()->initialize();
    });

    registerModule("CacheManager", InitPhase::Core, []() {
        return CacheManager::instance()->initialize(
            100 * 1024 * 1024,  // 100MB内存缓存
            1024 * 1024 * 1024  // 1GB磁盘缓存
        );
    });

    registerModule("DatabaseManager", InitPhase::Core, []() {
        DatabaseConfig config;
        config.databaseName = "wealthpilot.db";
        config.enableWAL = true;
        config.maxConnections = 10;
        return DatabaseManager::instance()->initialize(config);
    });

    registerModule("AsyncTaskManager", InitPhase::Core, []() {
        return AsyncTaskManager::instance().initialize(QThread::idealThreadCount());
    });

    // 注册服务模块
    registerModule("ThemeEngine", InitPhase::Services, []() {
        return ThemeEngine::instance().initialize();
    });

    // 注册插件模块
    registerModule("PluginLoader", InitPhase::Plugins, []() {
        QString pluginPath = QCoreApplication::applicationDirPath() + "/plugins";
        if (!PluginLoader::instance().initialize(pluginPath)) {
            return false;
        }
        return PluginLoader::instance().loadAll();
    });

    // 注册UI模块
    registerModule("UIComponents", InitPhase::UI, []() {
        // UI组件初始化
        return true;
    });
}

ApplicationInitializer::~ApplicationInitializer()
{
    if (m_initialized) {
        shutdown();
    }
}

bool ApplicationInitializer::initialize()
{
    if (m_initialized) {
        LOG_WARNING("Application already initialized");
        return true;
    }

    QElapsedTimer totalTimer;
    totalTimer.start();

    LOG_INFO("Starting application initialization...");

    // 按阶段初始化
    if (!initializeCore()) {
        LOG_ERROR("Core initialization failed");
        return false;
    }

    if (!initializeServices()) {
        LOG_ERROR("Services initialization failed");
        return false;
    }

    if (!initializePlugins()) {
        LOG_ERROR("Plugins initialization failed");
        return false;
    }

    if (!initializeUI()) {
        LOG_ERROR("UI initialization failed");
        return false;
    }

    m_initialized = true;
    m_currentPhase = InitPhase::Complete;

    LOG_INFO(QString("Application initialized successfully in %1ms")
        .arg(totalTimer.elapsed()));

    emit initializationComplete(true);

    return true;
}

void ApplicationInitializer::shutdown()
{
    if (!m_initialized) {
        return;
    }

    LOG_INFO("Shutting down application...");

    // 按逆序关闭
    // UI层
    for (const auto& module : m_modules[InitPhase::UI]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    // 插件层
    for (const auto& module : m_modules[InitPhase::Plugins]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }
    PluginLoader::instance().unloadAll();

    // 服务层
    for (const auto& module : m_modules[InitPhase::Services]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    // 核心层
    for (const auto& module : m_modules[InitPhase::Core]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    ServiceLocator::instance().clear();

    m_initialized = false;
    LOG_INFO("Application shutdown complete");
}

QMap<QString, InitResult> ApplicationInitializer::results() const
{
    return m_results;
}

InitPhase ApplicationInitializer::currentPhase() const
{
    return m_currentPhase;
}

void ApplicationInitializer::registerModule(const QString& name,
                                           InitPhase phase,
                                           std::function<bool()> initFunc,
                                           std::function<void()> shutdownFunc)
{
    ModuleInfo info;
    info.name = name;
    info.phase = phase;
    info.initFunc = initFunc;
    info.shutdownFunc = shutdownFunc;

    m_modules[phase].append(info);
}

bool ApplicationInitializer::initializeCore()
{
    m_currentPhase = InitPhase::Core;
    emit phaseStarted(InitPhase::Core);

    LOG_INFO("Initializing core modules...");

    const auto& modules = m_modules[InitPhase::Core];
    int current = 0;
    int total = modules.size();

    for (const auto& module : modules) {
        QElapsedTimer timer;
        timer.start();

        LOG_INFO(QString("Initializing module: %1").arg(module.name));

        bool success = false;
        try {
            success = module.initFunc();
        } catch (const std::exception& e) {
            LOG_ERROR(QString("Exception in module %1: %2").arg(module.name).arg(e.what()));
        }

        InitResult result;
        result.success = success;
        result.moduleName = module.name;
        result.duration = timer.elapsed();

        if (!success) {
            result.error = "Initialization failed";
            m_results[module.name] = result;

            LOG_ERROR(QString("Failed to initialize module: %1").arg(module.name));
            emit moduleInitialized(module.name, false, result.duration);

            return false;
        }

        m_results[module.name] = result;

        LOG_INFO(QString("Module %1 initialized in %2ms")
            .arg(module.name).arg(result.duration));

        current++;
        emit progressUpdated(current, total, module.name);
        emit moduleInitialized(module.name, true, result.duration);
    }

    LOG_INFO("Core modules initialized successfully");
    return true;
}

bool ApplicationInitializer::initializeServices()
{
    m_currentPhase = InitPhase::Services;
    emit phaseStarted(InitPhase::Services);

    LOG_INFO("Initializing services...");

    const auto& modules = m_modules[InitPhase::Services];
    int current = 0;
    int total = modules.size();

    for (const auto& module : modules) {
        QElapsedTimer timer;
        timer.start();

        LOG_INFO(QString("Initializing service: %1").arg(module.name));

        bool success = false;
        try {
            success = module.initFunc();
        } catch (const std::exception& e) {
            LOG_ERROR(QString("Exception in service %1: %2").arg(module.name).arg(e.what()));
        }

        InitResult result;
        result.success = success;
        result.moduleName = module.name;
        result.duration = timer.elapsed();

        if (!success) {
            result.error = "Initialization failed";
            m_results[module.name] = result;

            LOG_ERROR(QString("Failed to initialize service: %1").arg(module.name));
            emit moduleInitialized(module.name, false, result.duration);

            return false;
        }

        m_results[module.name] = result;

        LOG_INFO(QString("Service %1 initialized in %2ms")
            .arg(module.name).arg(result.duration));

        current++;
        emit progressUpdated(current, total, module.name);
        emit moduleInitialized(module.name, true, result.duration);
    }

    LOG_INFO("Services initialized successfully");
    return true;
}

bool ApplicationInitializer::initializePlugins()
{
    m_currentPhase = InitPhase::Plugins;
    emit phaseStarted(InitPhase::Plugins);

    LOG_INFO("Initializing plugins...");

    const auto& modules = m_modules[InitPhase::Plugins];
    int current = 0;
    int total = modules.size();

    for (const auto& module : modules) {
        QElapsedTimer timer;
        timer.start();

        LOG_INFO(QString("Initializing plugin system: %1").arg(module.name));

        bool success = false;
        try {
            success = module.initFunc();
        } catch (const std::exception& e) {
            LOG_ERROR(QString("Exception in plugin system %1: %2").arg(module.name).arg(e.what()));
        }

        InitResult result;
        result.success = success;
        result.moduleName = module.name;
        result.duration = timer.elapsed();

        if (!success) {
            result.error = "Initialization failed";
            m_results[module.name] = result;

            LOG_WARNING(QString("Failed to initialize plugin system: %1").arg(module.name));
            // 插件系统失败不阻止应用启动
        } else {
            m_results[module.name] = result;

            LOG_INFO(QString("Plugin system %1 initialized in %2ms")
                .arg(module.name).arg(result.duration));
        }

        current++;
        emit progressUpdated(current, total, module.name);
        emit moduleInitialized(module.name, success, result.duration);
    }

    LOG_INFO("Plugins initialized");
    return true;
}

bool ApplicationInitializer::initializeUI()
{
    m_currentPhase = InitPhase::UI;
    emit phaseStarted(InitPhase::UI);

    LOG_INFO("Initializing UI...");

    const auto& modules = m_modules[InitPhase::UI];
    int current = 0;
    int total = modules.size();

    for (const auto& module : modules) {
        QElapsedTimer timer;
        timer.start();

        LOG_INFO(QString("Initializing UI component: %1").arg(module.name));

        bool success = false;
        try {
            success = module.initFunc();
        } catch (const std::exception& e) {
            LOG_ERROR(QString("Exception in UI component %1: %2").arg(module.name).arg(e.what()));
        }

        InitResult result;
        result.success = success;
        result.moduleName = module.name;
        result.duration = timer.elapsed();

        if (!success) {
            result.error = "Initialization failed";
            m_results[module.name] = result;

            LOG_ERROR(QString("Failed to initialize UI component: %1").arg(module.name));
            emit moduleInitialized(module.name, false, result.duration);

            return false;
        }

        m_results[module.name] = result;

        LOG_INFO(QString("UI component %1 initialized in %2ms")
            .arg(module.name).arg(result.duration));

        current++;
        emit progressUpdated(current, total, module.name);
        emit moduleInitialized(module.name, true, result.duration);
    }

    LOG_INFO("UI initialized successfully");
    return true;
}
