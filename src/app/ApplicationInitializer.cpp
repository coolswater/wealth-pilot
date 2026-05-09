/**
 * @file ApplicationInitializer.cpp
 * @brief Application Initializer Implementation
 */

#include "ApplicationInitializer.h"
#include "../core/config/EnvironmentConfig.h"
#include "../core/cache/CacheManager.h"
#include "../core/di/ServiceLocator.h"
#include "../core/database/DatabaseManager.h"
#include "../core/task/AsyncTaskManager.h"
#include "../plugins/PluginLoader.h"
#include "../plugins/CTPPlugin.h"
#include "../ai/plugin/AIPlugin.h"
#include "../ui/ThemeManager.h"
#include "../utils/Logger.h"

#include <QElapsedTimer>
#include <QCoreApplication>
#include <QThread>
#include <QtConcurrent>

ApplicationInitializer& ApplicationInitializer::instance()
{
    static ApplicationInitializer instance;
    return instance;
}

ApplicationInitializer::ApplicationInitializer()
    : m_currentPhase(InitPhase::Core)
    , m_initialized(false)
{
    LOG_DEBUG("ApplicationInitializer created");
}

ApplicationInitializer::~ApplicationInitializer()
{
    if (m_initialized) {
        shutdown();
    }
    LOG_DEBUG("ApplicationInitializer destroyed");
}

bool ApplicationInitializer::initialize()
{
    if (m_initialized) {
        LOG_WARNING("Application already initialized");
        return true;
    }

    LOG_INFO("Starting application initialization...");

    // Initialize in order
    if (!initializeCore()) {
        LOG_ERROR("Core initialization failed");
        emit initializationComplete(false);
        return false;
    }

    if (!initializeServices()) {
        LOG_ERROR("Services initialization failed");
        emit initializationComplete(false);
        return false;
    }

    if (!initializePlugins()) {
        LOG_WARNING("Plugins initialization had issues");
    }

    if (!initializeUI()) {
        LOG_WARNING("UI initialization had issues");
    }

    m_initialized = true;
    m_currentPhase = InitPhase::Complete;

    emit initializationComplete(true);
    return true;
}

void ApplicationInitializer::shutdown()
{
    if (!m_initialized) {
        return;
    }

    LOG_INFO("Starting application shutdown...");
    QElapsedTimer timer;
    timer.start();

    // Shutdown in reverse order
    emit phaseStarted(InitPhase::UI);
    for (const auto& module : m_modules[InitPhase::UI]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    emit phaseStarted(InitPhase::Plugins);
    for (const auto& module : m_modules[InitPhase::Plugins]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    emit phaseStarted(InitPhase::Services);
    for (const auto& module : m_modules[InitPhase::Services]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    emit phaseStarted(InitPhase::Core);
    for (const auto& module : m_modules[InitPhase::Core]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    ServiceLocator::instance().clear();

    m_initialized = false;
    LOG_INFO(QString("Application shutdown complete in %1ms").arg(timer.elapsed()));
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

    LOG_DEBUG(QString("Module registered: %1 (phase %2)").arg(name).arg(static_cast<int>(phase)));
}

bool ApplicationInitializer::initializeCore()
{
    emit phaseStarted(InitPhase::Core);
    m_currentPhase = InitPhase::Core;

    QElapsedTimer timer;
    int current = 0;
    int total = m_modules[InitPhase::Core].size() + 3; // +3 for Logger, Env, Cache

    // Initialize Logger (must be first, synchronous)
    timer.start();
    Logger::instance()->init();
    LOG_DEBUG("Logger initialized");
    emit moduleInitialized("Logger", true, timer.elapsed());
    emit progressUpdated(++current, total, "Logger");

    // 并行初始化 EnvironmentConfig 和 CacheManager
    timer.restart();

    QFuture<bool> envFuture = QtConcurrent::run([this]()
    {
        QElapsedTimer t;
        t.start();
        (void)EnvironmentConfig::instance();
        emit moduleInitialized("EnvironmentConfig", true, t.elapsed());
        return true;
    });

    QFuture<bool> cacheFuture = QtConcurrent::run([this]()
    {
        QElapsedTimer t;
        t.start();
        bool result = CacheManager::instance()->initialize();
        emit moduleInitialized("CacheManager", result, t.elapsed());
        return result;
    });

    // 等待并行任务完成
    envFuture.waitForFinished();
    cacheFuture.waitForFinished();

    emit progressUpdated(++current, total, "EnvironmentConfig");
    emit progressUpdated(++current, total, "CacheManager");

    if (!cacheFuture.result())
    {
        LOG_ERROR("Failed to initialize CacheManager");
        return false;
    }

    // Note: DatabaseManager is initialized by DataStorageService
    LOG_DEBUG("DatabaseManager will be initialized by DataStorageService");

    // Initialize registered modules
    for (const auto& module : m_modules[InitPhase::Core]) {
        timer.restart();
        bool success = true;
        if (module.initFunc) {
            success = module.initFunc();
        }
        
        InitResult result;
        result.success = success;
        result.moduleName = module.name;
        result.duration = timer.elapsed();
        m_results[module.name] = result;

        emit moduleInitialized(module.name, success, result.duration);
        emit progressUpdated(++current, total, module.name);

        if (!success) {
            LOG_ERROR(QString("Core module failed: %1").arg(module.name));
            return false;
        }
    }

    return true;
}

bool ApplicationInitializer::initializeServices()
{
    emit phaseStarted(InitPhase::Services);
    m_currentPhase = InitPhase::Services;

    QElapsedTimer timer;
    int current = 0;
    int total = m_modules[InitPhase::Services].size();

    // Initialize AsyncTaskManager
    timer.start();
    AsyncTaskManager::instance().initialize();
    emit moduleInitialized("AsyncTaskManager", true, timer.elapsed());
    emit progressUpdated(++current, total, "AsyncTaskManager");

    // Initialize registered modules
    for (const auto& module : m_modules[InitPhase::Services]) {
        timer.restart();
        bool success = true;
        if (module.initFunc) {
            success = module.initFunc();
        }
        
        InitResult result;
        result.success = success;
        result.moduleName = module.name;
        result.duration = timer.elapsed();
        m_results[module.name] = result;

        emit moduleInitialized(module.name, success, result.duration);
        emit progressUpdated(++current, total, module.name);

        if (!success) {
            LOG_ERROR(QString("Service module failed: %1").arg(module.name));
            // Services can fail without blocking
        }
    }

    return true;
}

bool ApplicationInitializer::initializePlugins()
{
    emit phaseStarted(InitPhase::Plugins);
    m_currentPhase = InitPhase::Plugins;

    QElapsedTimer timer;
    int current = 0;
    int total = m_modules[InitPhase::Plugins].size();

    // Initialize PluginLoader
    timer.start();
    
    // 注册并启动内置插件
    auto& pluginLoader = PluginLoader::instance();
    
    // 注册并启动 CTP 插件
    auto ctpPlugin = new CTP::CTPPlugin();
    if (pluginLoader.registerBuiltInPlugin(ctpPlugin)) {
        LOG_INFO("CTPPlugin registered as built-in plugin");
        if (pluginLoader.loadPlugin("CTPPlugin")) {
            LOG_INFO("CTPPlugin loaded and started successfully");
        } else {
            LOG_WARNING("CTPPlugin failed to load");
        }
    }
    
    // 注册并启动 AI 插件
    auto aiPlugin = new AI::AIPlugin();
    if (pluginLoader.registerBuiltInPlugin(aiPlugin)) {
        LOG_INFO("AIPlugin registered as built-in plugin");
        if (pluginLoader.loadPlugin("AIPlugin")) {
            LOG_INFO("AIPlugin loaded and started successfully");
        } else {
            LOG_WARNING("AIPlugin failed to load");
        }
    }
    
    emit moduleInitialized("PluginLoader", true, timer.elapsed());
    emit progressUpdated(++current, total, "PluginLoader");

    // Initialize registered modules
    for (const auto& module : m_modules[InitPhase::Plugins]) {
        timer.restart();
        bool success = true;
        if (module.initFunc) {
            success = module.initFunc();
        }
        
        InitResult result;
        result.success = success;
        result.moduleName = module.name;
        result.duration = timer.elapsed();
        m_results[module.name] = result;

        emit moduleInitialized(module.name, success, result.duration);
        emit progressUpdated(++current, total, module.name);
    }

    return true;
}

bool ApplicationInitializer::initializeUI()
{
    emit phaseStarted(InitPhase::UI);
    m_currentPhase = InitPhase::UI;

    QElapsedTimer timer;
    int current = 0;
    int total = m_modules[InitPhase::UI].size();

    // Initialize ThemeManager
    timer.start();
    // ThemeManager auto-initializes on first access
    (void)ThemeManager::instance();
    emit moduleInitialized("ThemeManager", true, timer.elapsed());
    emit progressUpdated(++current, total, "ThemeManager");

    // Initialize registered modules
    for (const auto& module : m_modules[InitPhase::UI]) {
        timer.restart();
        bool success = true;
        if (module.initFunc) {
            success = module.initFunc();
        }
        
        InitResult result;
        result.success = success;
        result.moduleName = module.name;
        result.duration = timer.elapsed();
        m_results[module.name] = result;

        emit moduleInitialized(module.name, success, result.duration);
        emit progressUpdated(++current, total, module.name);
    }

    return true;
}
