/**
 * @file ApplicationInitializer.cpp
 * @brief 应用初始化管理器实现
 *
 * @details 初始化流程：
 * 1. Core 阶段：Logger -> EnvironmentConfig -> CacheManager -> DataHub
 * 2. Services 阶段：AsyncTaskManager -> 数据源服务
 * 3. Plugins 阶段：PluginLoader -> CTPPlugin -> AIPlugin
 * 4. UI 阶段：ThemeManager -> 页面注册
 */

#include "ApplicationInitializer.h"
#include "infrastructure/config/EnvironmentConfig.h"
#include "core/services/cache/CacheManager.h"
#include "data/datahub/DataHubBootstrap.h"
#include "core/services/di/ServiceLocator.h"
#include "infrastructure/database/DatabaseManager.h"
#include "core/services/task/AsyncTaskManager.h"
#include "infrastructure/plugins/PluginLoader.h"
#include "infrastructure/plugins/CTPPlugin.h"
#include "infrastructure/ai/plugin/AIPlugin.h"
#include "presentation/styles/ThemeManager.h"
#include "shared/utils/Logger.h"

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
    LOG_INFO("Application initialization complete");
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

    // ============================================================
    // 关闭顺序：反向释放资源
    // UI -> Plugins -> Services -> Core
    // ============================================================
    
    // 1. 关闭 UI 层
    emit phaseStarted(InitPhase::UI);
    for (const auto& module : m_modules[InitPhase::UI]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    // 2. 关闭插件系统
    emit phaseStarted(InitPhase::Plugins);
    for (const auto& module : m_modules[InitPhase::Plugins]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    // 3. 关闭服务层
    emit phaseStarted(InitPhase::Services);
    for (const auto& module : m_modules[InitPhase::Services]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    // 4. 关闭核心模块
    emit phaseStarted(InitPhase::Core);
    for (const auto& module : m_modules[InitPhase::Core]) {
        if (module.shutdownFunc) {
            module.shutdownFunc();
        }
    }

    // 5. 关闭 DataHub（新增）
    // DataHub 会在 ServiceLocator::clear() 之前自动清理
    LOG_INFO("DataHub will be cleaned up automatically");

    // 6. 清理服务定位器
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
    // +4 for Logger, Env, Cache, DataHub
    int total = m_modules[InitPhase::Core].size() + 4;

    // ============================================================
    // 1. 初始化 Logger（必须第一个，同步）
    // ============================================================
    timer.start();
    Logger::instance()->init();
    emit moduleInitialized("Logger", true, timer.elapsed());
    emit progressUpdated(++current, total, "Logger");

    // ============================================================
    // 2. 并行初始化 EnvironmentConfig 和 CacheManager
    // ============================================================
    timer.restart();

    QFuture<bool> envFuture = QtConcurrent::run([this]()
    {
        (void)EnvironmentConfig::instance();
        return true;
    });

    QFuture<bool> cacheFuture = QtConcurrent::run([this]()
    {
        bool result = CacheManager::instance()->initialize();
        return result;
    });

    // 等待并行任务完成
    envFuture.waitForFinished();
    cacheFuture.waitForFinished();

    // 在主线程中发射信号
    emit moduleInitialized("EnvironmentConfig", true, 0);
    emit moduleInitialized("CacheManager", cacheFuture.result(), 0);
    emit progressUpdated(++current, total, "EnvironmentConfig");
    emit progressUpdated(++current, total, "CacheManager");

    if (!cacheFuture.result())
    {
        LOG_ERROR("Failed to initialize CacheManager");
        return false;
    }

    // ============================================================
    // 3. 初始化 DataHub 数据中心（新增）
    // ============================================================
    timer.restart();
    WealthPilot::DataHubBootstrap dataHubBootstrap;
    bool dataHubResult = dataHubBootstrap.initialize();
    emit moduleInitialized("DataHub", dataHubResult, timer.elapsed());
    emit progressUpdated(++current, total, "DataHub");

    if (!dataHubResult) {
        LOG_ERROR("Failed to initialize DataHub");
        return false;
    }

    // ============================================================
    // 4. 初始化注册的核心模块
    // ============================================================
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
