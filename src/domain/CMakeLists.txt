# ==================== Domain 模块 CMakeLists.txt ====================
# 纯领域模型层 - 无 Qt 依赖（除类型定义外）
# 包含：分析算法、交易策略、风险管理模型

add_library(Domain STATIC)

# ==================== Analysis 分析模块 ====================
target_sources(Domain PRIVATE
        # 核心分析
        analysis/StockScreener.h
        analysis/StockScreener.cpp
        analysis/ExampleStrategies.h
        analysis/RiskAnalyzer.h
        analysis/RiskAnalyzer.cpp
        analysis/NewsSentimentAnalyzer.h
        analysis/NewsSentimentAnalyzer.cpp

        # 缠论分析
        analysis/legacy/chanlun/ChanLunTypes.h
        analysis/legacy/chanlun/ChanLunAnalyzer.h
        analysis/legacy/chanlun/ChanLunAnalyzer.cpp
        analysis/legacy/chanlun/ChanLunIndicator.h
        analysis/legacy/chanlun/ChanLunIndicator.cpp
        analysis/legacy/chanlun/ChanLunIntegration.h
        analysis/legacy/chanlun/ChanLunIntegration.cpp

        # 波浪理论
        analysis/legacy/elliottwave/ElliottWaveTypes.h
        analysis/legacy/elliottwave/ElliottWaveAnalyzer.h
        analysis/legacy/elliottwave/ElliottWaveAnalyzer.cpp

        # 道氏理论
        analysis/legacy/dowtheory/DowTheoryTypes.h
        analysis/legacy/dowtheory/DowTheoryAnalyzer.h
        analysis/legacy/dowtheory/DowTheoryAnalyzer.cpp

        # 量价形态
        analysis/legacy/volumepattern/VolumePatternTypes.h
        analysis/legacy/volumepattern/VolumePatternAnalyzer.h
        analysis/legacy/volumepattern/VolumePatternAnalyzer.cpp

        # 信号系统
        analysis/legacy/AnalysisTypes.h
        analysis/legacy/IAnalyzer.h
        analysis/legacy/AnalysisManager.h
        analysis/legacy/AnalysisManager.cpp
        analysis/legacy/signal/SignalFilter.h
        analysis/legacy/signal/SignalFilter.cpp
        analysis/legacy/signal/SignalService.h
        analysis/legacy/signal/SignalService.cpp
)

# ==================== Backtest 回测模块 ====================
target_sources(Domain PRIVATE
        backtest/BacktestEngine.h
        backtest/BacktestEngine.cpp
)

# ==================== Portfolio 组合优化 ====================
target_sources(Domain PRIVATE
        portfolio/PortfolioOptimizer.h
        portfolio/PortfolioOptimizer.cpp
)

# ==================== Quant 量化交易 ====================
target_sources(Domain PRIVATE
        quant/QuantTradingEngine.h
        quant/QuantTradingEngine.cpp
)

# ==================== Recommendation 推荐 ====================
target_sources(Domain PRIVATE
        recommendation/PersonalizedRecommendation.h
        recommendation/PersonalizedRecommendation.cpp
)

# ==================== Risk 风险管理 ====================
target_sources(Domain PRIVATE
        risk/RiskWarningSystem.h
        risk/RiskWarningSystem.cpp
)

# ==================== Trading 交易模型 ====================
target_sources(Domain PRIVATE
        trading/TradingTypes.h
        trading/TradingService.h
        trading/TradingService.cpp
        trading/OrderManager.h
        trading/OrderManager.cpp
        trading/PositionManager.h
        trading/PositionManager.cpp
        trading/RiskController.h
        trading/RiskController.cpp
        trading/ConditionOrderEngine.h
        trading/ConditionOrderEngine.cpp
)

# ==================== 包含目录 ====================
target_include_directories(Domain PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# ==================== 链接库 ====================
target_link_libraries(Domain PUBLIC
        Qt6::Core
        Qt6::Sql
        WealthPilot::Shared
)

# 添加别名
add_library(WealthPilot::Domain ALIAS Domain)

# ==================== 编译选项 ====================
if (MSVC)
    target_compile_options(Domain PRIVATE /W4)
else ()
    target_compile_options(Domain PRIVATE -Wall -Wextra -Wpedantic)
endif ()
