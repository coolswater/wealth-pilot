# ==================== Application 模块 CMakeLists.txt ====================
# 应用服务层 - 协调 Domain 和 Infrastructure
# 包含：用例编排、应用初始化、功能集成

add_library(AppLib STATIC)

# ==================== Application Services 应用服务 ====================
target_sources(AppLib PRIVATE
        # 市场数据服务 - 协调 DataHub 和数据源
        market/MarketDataService.h
        market/MarketDataService.cpp

        # 交易执行服务 - 协调 OrderManager 和交易接口
        trading/TradingExecutionService.h
        trading/TradingExecutionService.cpp

        # 投资组合服务 - 协调 PortfolioOptimizer 和数据存储
        portfolio/PortfolioAppService.h
        portfolio/PortfolioAppService.cpp

        # 分析服务 - 协调各种分析器和数据
        analysis/AnalysisAppService.h
        analysis/AnalysisAppService.cpp

        # 风控服务 - 协调 RiskWarningSystem 和交易
        risk/RiskAppService.h
        risk/RiskAppService.cpp
)

# ==================== Use Cases 用例编排 ====================
target_sources(AppLib PRIVATE
        usecases/UseCaseBase.h
        usecases/UseCaseBase.cpp
        usecases/StockAnalysisUseCase.h
        usecases/StockAnalysisUseCase.cpp
        usecases/PortfolioOptimizeUseCase.h
        usecases/PortfolioOptimizeUseCase.cpp
        usecases/BacktestStrategyUseCase.h
        usecases/BacktestStrategyUseCase.cpp
        usecases/TradeExecutionUseCase.h
        usecases/TradeExecutionUseCase.cpp
)

# ==================== 初始化和集成 ====================
target_sources(AppLib PRIVATE
        ApplicationInitializer.h
        ApplicationInitializer.cpp
        FeatureIntegration.h
        FeatureIntegration.cpp
)

# ==================== 包含目录 ====================
target_include_directories(AppLib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# ==================== 链接库 ====================
target_link_libraries(AppLib PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        WealthPilot::Shared
        WealthPilot::Domain
        WealthPilot::Services
        WealthPilot::Data
        WealthPilot::Infrastructure
)

# 添加别名
add_library(WealthPilot::AppLib ALIAS AppLib)

# ==================== 编译选项 ====================
if (MSVC)
    target_compile_options(AppLib PRIVATE /W4)
else ()
    target_compile_options(AppLib PRIVATE -Wall -Wextra -Wpedantic)
endif ()
