# ==================== Services 模块 CMakeLists.txt ====================
# 基硎服务层 - 提供通用服务能力
# 包含：缓存、依赖注入、生命周期、导航、性能、任务调度等

add_library(Services STATIC)

# ==================== Account 多账户 ====================
target_sources(Services PRIVATE
        account/MultiAccountManager.h
        account/MultiAccountManager.cpp
)

# ==================== Alert 预警系统 ====================
target_sources(Services PRIVATE
        alert/SmartAlertSystem.h
        alert/SmartAlertSystem.cpp
        alert/AlertNotificationService.h
        alert/AlertNotificationService.cpp
)

# ==================== Cache 缓存系统 ====================
target_sources(Services PRIVATE
        cache/CacheManager.h
        cache/CacheManager.cpp
        cache/DataCacheManager.h
        cache/DataCacheManager.cpp
)

# ==================== DI 依赖注入 ====================
target_sources(Services PRIVATE
        di/ServiceLocator.h
        di/ServiceLocator.cpp
        di/ServiceRegistry.h
        di/ServiceRegistry.cpp
)

# ==================== Feedback 用户反馈 ====================
target_sources(Services PRIVATE
        feedback/UserFeedbackManager.h
        feedback/UserFeedbackManager.cpp
        feedback/FeedbackTranslations.h
        feedback/FeedbackSystem.h
        feedback/FeedbackSystem.cpp
)

# ==================== Lifecycle 服务生命周期 ====================
target_sources(Services PRIVATE
        lifecycle/ServiceLifecycle.h
        lifecycle/ServiceLifecycle.cpp
)

# ==================== Monitoring 性能监控 ====================
target_sources(Services PRIVATE
        monitoring/PerformanceMonitor.h
        monitoring/PerformanceMonitor.cpp
)

# ==================== Navigation 页面导航 ====================
target_sources(Services PRIVATE
        navigation/PageNavigator.h
        navigation/PageNavigator.cpp
        navigation/PageNavigatorManager.h
        navigation/PageNavigatorManager.cpp
        navigation/PageFactoryRegistry.h
        navigation/PageFactoryRegistry.cpp
)

# ==================== Performance 性能管理 ====================
target_sources(Services PRIVATE
        performance/PerformanceManager.h
        performance/PerformanceManager.cpp
)

# ==================== Security 权限管理 ====================
target_sources(Services PRIVATE
        security/PermissionManager.h
        security/PermissionManager.cpp
)

# ==================== Social 社交交易 ====================
target_sources(Services PRIVATE
        social/StrategyShareManager.h
        social/StrategyShareManager.cpp
)

# ==================== Task 异步任务 ====================
target_sources(Services PRIVATE
        task/AsyncTaskManager.h
        task/AsyncTaskManager.cpp
)

# ==================== Trading Service 交易服务 ====================
target_sources(Services PRIVATE
        trading/TradingService.h
        trading/TradingService.cpp
)

# ==================== 包含目录 ====================
target_include_directories(Services PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# ==================== 链接库 ====================
target_link_libraries(Services PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        WealthPilot::Shared
        WealthPilot::Domain
)

# 添加别名
add_library(WealthPilot::Services ALIAS Services)

# ==================== 编译选项 ====================
if (MSVC)
    target_compile_options(Services PRIVATE /W4)
else ()
    target_compile_options(Services PRIVATE -Wall -Wextra -Wpedantic)
endif ()
