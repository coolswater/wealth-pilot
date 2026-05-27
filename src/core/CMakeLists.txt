# ==================== Core 模块 CMakeLists.txt ====================
# 核心业务模块：领域模型、服务、交易系统

add_library(Core STATIC)

# ==================== Domain 领域模型 ====================

# Analysis 分析模块
target_sources(Core PRIVATE
        domain/analysis/StockScreener.h
        domain/analysis/StockScreener.cpp
        domain/analysis/ExampleStrategies.h
        domain/analysis/RiskAnalyzer.h
        domain/analysis/RiskAnalyzer.cpp
        domain/analysis/NewsSentimentAnalyzer.h
        domain/analysis/NewsSentimentAnalyzer.cpp
)

# Analysis Legacy - 缠论分析
target_sources(Core PRIVATE
        domain/analysis/legacy/chanlun/ChanLunTypes.h
        domain/analysis/legacy/chanlun/ChanLunAnalyzer.h
        domain/analysis/legacy/chanlun/ChanLunAnalyzer.cpp
        domain/analysis/legacy/chanlun/ChanLunIndicator.h
        domain/analysis/legacy/chanlun/ChanLunIndicator.cpp
        domain/analysis/legacy/chanlun/ChanLunIntegration.h
        domain/analysis/legacy/chanlun/ChanLunIntegration.cpp
)

# Analysis Legacy - 波浪理论
target_sources(Core PRIVATE
        domain/analysis/legacy/elliottwave/ElliottWaveTypes.h
        domain/analysis/legacy/elliottwave/ElliottWaveAnalyzer.h
        domain/analysis/legacy/elliottwave/ElliottWaveAnalyzer.cpp
)

# Analysis Legacy - 道氏理论
target_sources(Core PRIVATE
        domain/analysis/legacy/dowtheory/DowTheoryTypes.h
        domain/analysis/legacy/dowtheory/DowTheoryAnalyzer.h
        domain/analysis/legacy/dowtheory/DowTheoryAnalyzer.cpp
)

# Analysis Legacy - 量价形态
target_sources(Core PRIVATE
        domain/analysis/legacy/volumepattern/VolumePatternTypes.h
        domain/analysis/legacy/volumepattern/VolumePatternAnalyzer.h
        domain/analysis/legacy/volumepattern/VolumePatternAnalyzer.cpp
)

# Analysis Legacy - 信号系统
target_sources(Core PRIVATE
        domain/analysis/legacy/AnalysisTypes.h
        domain/analysis/legacy/IAnalyzer.h
        domain/analysis/legacy/AnalysisManager.h
        domain/analysis/legacy/AnalysisManager.cpp
        domain/analysis/legacy/signal/SignalFilter.h
        domain/analysis/legacy/signal/SignalFilter.cpp
        domain/analysis/legacy/signal/SignalService.h
        domain/analysis/legacy/signal/SignalService.cpp
)

# Backtest 回测模块
target_sources(Core PRIVATE
        domain/backtest/BacktestEngine.h
        domain/backtest/BacktestEngine.cpp
)

# Portfolio 组合优化
target_sources(Core PRIVATE
        domain/portfolio/PortfolioOptimizer.h
        domain/portfolio/PortfolioOptimizer.cpp
)

# Quant 量化交易
target_sources(Core PRIVATE
        domain/quant/QuantTradingEngine.h
        domain/quant/QuantTradingEngine.cpp
)

# Recommendation 推荐
target_sources(Core PRIVATE
        domain/recommendation/PersonalizedRecommendation.h
        domain/recommendation/PersonalizedRecommendation.cpp
)

# Risk 风险管理
target_sources(Core PRIVATE
        domain/risk/RiskWarningSystem.h
        domain/risk/RiskWarningSystem.cpp
)

# ==================== Services 服务层 ====================

# Account 多账户
target_sources(Core PRIVATE
        services/account/MultiAccountManager.h
        services/account/MultiAccountManager.cpp
)

# Alert 预警系统
target_sources(Core PRIVATE
        services/alert/SmartAlertSystem.h
        services/alert/SmartAlertSystem.cpp
        services/alert/AlertNotificationService.h
        services/alert/AlertNotificationService.cpp
)

# Cache 缓存系统
target_sources(Core PRIVATE
        services/cache/CacheManager.h
        services/cache/CacheManager.cpp
        services/cache/DataCacheManager.h
        services/cache/DataCacheManager.cpp
)

# DI 依赖注入
target_sources(Core PRIVATE
        services/di/ServiceLocator.h
        services/di/ServiceLocator.cpp
        services/di/ServiceRegistry.h
        services/di/ServiceRegistry.cpp
)

# Feedback 用户反馈
target_sources(Core PRIVATE
        services/feedback/UserFeedbackManager.h
        services/feedback/UserFeedbackManager.cpp
        services/feedback/FeedbackTranslations.h
        services/feedback/FeedbackSystem.h
        services/feedback/FeedbackSystem.cpp
)

# Lifecycle 服务生命周期
target_sources(Core PRIVATE
        services/lifecycle/ServiceLifecycle.h
        services/lifecycle/ServiceLifecycle.cpp
)

# Storage 数据存储（占位实现）
target_sources(Core PRIVATE
        services/storage/DataStorageService.h
)

# CTP 交易接口（占位实现）
target_sources(Core PRIVATE
        trading/ctp/CTPService.h
)

# Monitoring 性能监控
target_sources(Core PRIVATE
        services/monitoring/PerformanceMonitor.h
        services/monitoring/PerformanceMonitor.cpp
)

# Navigation 页面导航
target_sources(Core PRIVATE
        services/navigation/PageNavigator.h
        services/navigation/PageNavigator.cpp
        services/navigation/PageNavigatorManager.h
        services/navigation/PageNavigatorManager.cpp
        services/navigation/PageFactoryRegistry.h
        services/navigation/PageFactoryRegistry.cpp
)

# Performance 性能管理
target_sources(Core PRIVATE
        services/performance/PerformanceManager.h
        services/performance/PerformanceManager.cpp
)

# Security 权限管理
target_sources(Core PRIVATE
        services/security/PermissionManager.h
        services/security/PermissionManager.cpp
)

# Social 社交交易
target_sources(Core PRIVATE
        services/social/StrategyShareManager.h
        services/social/StrategyShareManager.cpp
)

# Task 异步任务
target_sources(Core PRIVATE
        services/task/AsyncTaskManager.h
        services/task/AsyncTaskManager.cpp
)

# ==================== Trading 交易系统 ====================
target_sources(Core PRIVATE
        trading/TradingTypes.h
        trading/OrderManager.h
        trading/OrderManager.cpp
        trading/PositionManager.h
        trading/PositionManager.cpp
        trading/RiskController.h
        trading/RiskController.cpp
        trading/TradingService.h
        trading/TradingService.cpp
        trading/ConditionOrderEngine.h
        trading/ConditionOrderEngine.cpp
)

# ==================== 包含目录 ====================
target_include_directories(Core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# ==================== 链接库 ====================
target_link_libraries(Core PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        WealthPilot::Shared
)

# 添加别名，允许其他模块使用 WealthPilot::Core
add_library(WealthPilot::Core ALIAS Core)

# ==================== 编译选项 ====================
if (MSVC)
    target_compile_options(Core PRIVATE /W4)
else ()
    target_compile_options(Core PRIVATE -Wall -Wextra -Wpedantic)
endif ()
