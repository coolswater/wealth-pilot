cmake_minimum_required(VERSION 3.16)
project(WealthPilot VERSION 1.0.0 LANGUAGES CXX)

# 设置Qt6路径（如果未设置）
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.2/mingw_64")

# 设置默认构建类型
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif ()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 查找Qt6组件
find_package(Qt6 REQUIRED COMPONENTS
        Core
        Widgets
        Network
        WebSockets
        Sql
        TextToSpeech
        Charts
        Concurrent
        Svg
        StateMachine
        Core5Compat
)

# ==================== 源文件分组 ====================

# App 应用层
set(APP_SOURCES
        src/app/ApplicationInitializer.h
        src/app/ApplicationInitializer.cpp
)

# Core - Base 基础类
set(CORE_BASE_SOURCES
        src/core/base/Singleton.h
        src/core/base/BasePage.h
        src/core/base/ErrorCode.h
        src/core/base/NamespaceGuide.h
        src/core/types/MarketTypes.h
)

# Core - Config 配置管理
set(CORE_CONFIG_SOURCES
        src/core/config/ConfigManager.h
        src/core/config/ConfigManager.cpp
        src/core/config/EnvironmentConfig.h
        src/core/config/EnvironmentConfig.cpp
        src/core/config/Tokens.h
)

# Core - Cache 缓存系统
set(CORE_CACHE_SOURCES
        src/core/cache/CacheManager.h
        src/core/cache/CacheManager.cpp
)

# Core - Database 数据库
set(CORE_DATABASE_SOURCES
        src/core/database/DatabaseManager.h
        src/core/database/DatabaseManager.cpp
        src/data/DataStorage.h
        src/data/DataStorage.cpp
)

# Core - Task 异步任务
set(CORE_TASK_SOURCES
        src/core/task/AsyncTaskManager.h
        src/core/task/AsyncTaskManager.cpp
)

# Core - DI 依赖注入
set(CORE_DI_SOURCES
        src/core/di/ServiceLocator.h
        src/core/di/ServiceLocator.cpp
)

# Core - Navigation 页面导航
set(CORE_NAVIGATION_SOURCES
        src/core/navigation/PageNavigator.h
        src/core/navigation/PageNavigator.cpp
        src/core/navigation/PageNavigatorManager.h
        src/core/navigation/PageNavigatorManager.cpp
        src/core/navigation/PageFactoryRegistry.h
        src/core/navigation/PageFactoryRegistry.cpp
)

# CTP - API
set(CTP_API_SOURCES
        src/ctp/api/CtpApiLoader.h
        src/ctp/api/CtpDataBuffer.h
        src/ctp/api/CtpMarketSpi.h
        src/ctp/api/CtpMarketSpi.cpp
        src/ctp/api/CtpTradingSpi.h
        src/ctp/api/CtpTradingSpi.cpp
)

# CTP - Config
set(CTP_CONFIG_SOURCES
        src/ctp/config/CTPConfigManager.h
        src/ctp/config/CTPConfigManager.cpp
)

# CTP - Service
set(CTP_SERVICE_SOURCES
        src/ctp/service/CTPService.h
        src/ctp/service/CTPService.cpp
)

# AI - Service
set(AI_SERVICE_SOURCES
        src/ai/service/AIService.h
        src/ai/service/AIService.cpp
)

# AI - Plugin
set(AI_PLUGIN_SOURCES
        src/ai/plugin/AIPlugin.h
        src/ai/plugin/AIPlugin.cpp
)

# Market 行情数据
set(MARKET_SOURCES
        src/market/QuoteDataManager.h
        src/market/QuoteDataManager.cpp
        src/market/FavoritesManager.h
        src/market/FavoritesManager.cpp
        src/market/MarketDataHub.h
        src/market/MarketDataHub.cpp
        src/market/StockDataSource.h
        src/market/StockDataSource.cpp
        src/market/NewsDataSource.h
        src/market/NewsDataSource.cpp
)

# Models 数据模型
set(MODEL_SOURCES
        src/models/FuturesQuoteItem.h
        src/models/FuturesQuoteModel.h
        src/models/FuturesQuoteModel.cpp
        src/models/FuturesQuoteDelegate.h
        src/models/FuturesQuoteDelegate.cpp
        src/models/StockQuoteItem.h
        src/models/StockQuoteItem.cpp
)

# Utils 工具
set(UTIL_SOURCES
        src/utils/Logger.cpp
        src/utils/Logger.h
        src/utils/FuturesMockDataGenerator.h
        src/utils/FuturesMockDataGenerator.cpp
        src/utils/Result.h
        src/utils/TechnicalIndicators.h
        src/utils/TechnicalIndicators.cpp
        src/utils/PerformanceMonitor.h
        src/utils/PerformanceMonitor.cpp
        src/utils/ExcelExporter.h
        src/utils/ExcelExporter.cpp
        src/utils/ReportGenerator.h
        src/utils/ReportGenerator.cpp
)

# Network 网络
set(NETWORK_SOURCES
        src/network/NetworkManager.cpp
        src/network/NetworkManager.h
        src/network/NetworkCache.h
        src/network/NetworkCache.cpp
)

# Plugins 插件框架
set(PLUGIN_SOURCES
        src/plugins/IPlugin.h
        src/plugins/PluginLoader.h
        src/plugins/PluginLoader.cpp
        src/plugins/ICTPPlugin.h
        src/plugins/IAIPlugin.h
        src/plugins/CTPPlugin.h
        src/plugins/CTPPlugin.cpp
)

# UI Components UI组件库
set(UI_COMPONENT_SOURCES
        src/ui/components/UIComponents.h
        src/ui/components/ThemeEngine.h
        src/ui/components/ThemeEngine.cpp
        src/ui/components/ThemeColors.h
        src/ui/components/PageStyles.h
        src/ui/components/ChartStyles.h
        src/ui/components/LayoutConstants.h
        src/ui/components/BaseChartWidget.h
        src/ui/components/BaseChartWidget.cpp
        src/ui/components/ComponentFactory.h
        src/ui/components/ComponentFactory.cpp
        src/ui/components/ChartConfig.h
        src/ui/components/ChartConfig.cpp
        src/ui/components/KLineChart.h
        src/ui/components/KLineChart.cpp
        src/ui/components/ChartToolBar.h
        src/ui/components/ChartToolBar.cpp
        src/ui/components/MarketDepthWidget.h
        src/ui/components/MarketDepthWidget.cpp
        src/ui/components/TickTableView.h
        src/ui/components/TickTableView.cpp
        src/ui/components/ChartStatusBar.h
        src/ui/components/ChartStatusBar.cpp
        src/ui/components/BaseWidget.h
        src/ui/components/BaseWidget.cpp
        src/ui/components/SidebarWidget.h
        src/ui/components/SidebarWidget.cpp
        src/ui/components/SvgIconEngine.h
        src/ui/components/SvgIconEngine.cpp
        src/ui/components/SvgIconWidget.h
        src/ui/components/SvgIconWidget.cpp
        src/ui/components/TitleBarWidget.h
        src/ui/components/TitleBarWidget.cpp
        src/ui/components/ThemeToggleButton.h
        src/ui/components/ThemeToggleButton.cpp
        src/ui/components/CardWidget.h
        src/ui/components/CardWidget.cpp
        src/ui/components/StatusBarWidget.h
        src/ui/components/StatusBarWidget.cpp
        src/ui/components/AssetCardWidget.h
        src/ui/components/AssetCardWidget.cpp
        src/ui/components/AssetPieChart.h
        src/ui/components/AssetPieChart.cpp
        src/ui/components/AIAssistantPanelWidget.cpp
        src/ui/components/AIAssistantPanelWidget.h
        src/ui/components/NetworkIndicator.cpp
        src/ui/components/NetworkIndicator.h
        src/ui/components/TreeMapWidget.cpp
        src/ui/components/TreeMapWidget.h
        src/ui/components/AddSymbolDialog.cpp
        src/ui/components/AddSymbolDialog.h
        src/ui/components/DividerWidget.h
        src/ui/components/DividerWidget.cpp
        src/ui/components/OrderDialog.h
        src/ui/components/OrderDialog.cpp
        src/ui/ThemeManager.h
        src/ui/ThemeManager.cpp
)

# UI Animation 动画系统
set(UI_ANIMATION_SOURCES
        src/ui/animation/AnimationManager.h
        src/ui/animation/AnimationManager.cpp
)

# Views 视图
set(VIEW_SOURCES
        src/views/mainWindow/MainWindow.cpp
        src/views/mainWindow/MainWindow.h
        src/views/dashboard/DashboardPage.h
        src/views/dashboard/DashboardPage.cpp
        src/views/watchList/WatchListPage.h
        src/views/watchList/WatchListPage.cpp
        src/views/signalCenter/SignalCenterPage.h
        src/views/signalCenter/SignalCenterPage.cpp
        src/views/news/NewsPage.h
        src/views/news/NewsPage.cpp
        src/views/portfolio/PortfolioPage.h
        src/views/portfolio/PortfolioPage.cpp
        src/views/futures/FuturesQuotesPage.h
        src/views/futures/FuturesQuotesPage.cpp
        src/views/futures/FuturesKLinePage.h
        src/views/futures/FuturesKLinePage.cpp
        src/views/futures/FuturesPageIntegration.h
        src/views/futures/FuturesPageIntegration.cpp
        src/views/futures/FuturesPagesRegistration.cpp
        src/views/stock/StockQuotesPage.h
        src/views/stock/StockQuotesPage.cpp
        src/views/stock/StockKLinePage.h
        src/views/stock/StockKLinePage.cpp
        src/views/settings/SettingsPage.h
        src/views/settings/SettingsPage.cpp
        src/views/settings/CTPBrokerDialog.h
        src/views/settings/CTPBrokerDialog.cpp
        src/views/settings/RiskSettingsPage.h
        src/views/settings/RiskSettingsPage.cpp
        src/views/warning/WarningPage.h
        src/views/warning/WarningPage.cpp
        src/views/aboutus/AboutUSPage.h
        src/views/aboutus/AboutUSPage.cpp
        src/views/account/AccountPage.h
        src/views/account/AccountPage.cpp
        src/views/trading/TradeHistoryPage.h
        src/views/trading/TradeHistoryPage.cpp
        src/views/trading/ConditionOrderPage.h
        src/views/trading/ConditionOrderPage.cpp
        src/views/trading/TradingIntegration.h
        src/views/trading/TradingIntegration.cpp
        src/views/trading/TradingPanel.h
        src/views/trading/TradingPanel.cpp
        src/views/TradingPagesRegistration.cpp
)

# Trading 交易系统
set(TRADING_SOURCES
        src/trading/TradingTypes.h
        src/trading/OrderManager.h
        src/trading/OrderManager.cpp
        src/trading/PositionManager.h
        src/trading/PositionManager.cpp
        src/trading/RiskController.h
        src/trading/RiskController.cpp
        src/trading/TradingService.h
        src/trading/TradingService.cpp
        src/trading/AccountSyncManager.h
        src/trading/AccountSyncManager.cpp
        src/trading/ConditionOrderEngine.h
        src/trading/ConditionOrderEngine.cpp
        src/trading/AlertManager.h
        src/trading/AlertManager.cpp
)

# 合并所有源文件
set(SOURCES
        main.cpp
        ${APP_SOURCES}
        ${CORE_BASE_SOURCES}
        ${CORE_CONFIG_SOURCES}
        ${CORE_CACHE_SOURCES}
        ${CORE_DATABASE_SOURCES}
        ${CORE_TASK_SOURCES}
        ${CORE_DI_SOURCES}
        ${CORE_NAVIGATION_SOURCES}
        ${CTP_API_SOURCES}
        ${CTP_CONFIG_SOURCES}
        ${CTP_SERVICE_SOURCES}
        ${AI_SERVICE_SOURCES}
        ${AI_PLUGIN_SOURCES}
        ${MARKET_SOURCES}
        ${MODEL_SOURCES}
        ${UTIL_SOURCES}
        ${NETWORK_SOURCES}
        ${PLUGIN_SOURCES}
        ${UI_COMPONENT_SOURCES}
        ${UI_ANIMATION_SOURCES}
        ${WIDGET_SOURCES}
        ${VIEW_SOURCES}
        ${TRADING_SOURCES}
)

# 创建可执行文件
add_executable(WealthPilot WIN32 MACOSX_BUNDLE ${SOURCES})

# 包含目录
target_include_directories(WealthPilot PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/src
)

# 链接Qt库
target_link_libraries(WealthPilot PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::WebSockets
        Qt6::Sql
        Qt6::TextToSpeech
        Qt6::Charts
        Qt6::Concurrent
        Qt6::Svg
        Qt6::StateMachine
)


# Windows 平台特定库
if (WIN32)
    target_link_libraries(WealthPilot PRIVATE
            crypt32
            Qt6::Core5Compat
    )

    # CTP 使用动态加载（解决 MinGW 与 MSVC DLL 名称修饰不兼容问题）
    # 不需要静态链接库，只需要确保 DLL 在运行时可用

    # 定义 CTP DLL 导入宏 (使用 DLL 需要 dllimport)
    target_compile_definitions(WealthPilot PRIVATE ISLIB)

    message(STATUS "CTP API will be loaded dynamically at runtime")
endif ()

# 添加资源文件
qt6_add_resources(WealthPilot_RESOURCES resources/resources.qrc)
target_sources(WealthPilot PRIVATE ${WealthPilot_RESOURCES})

# ==================== 编译选项 ====================

if (MSVC)
    target_compile_options(WealthPilot PRIVATE /W4 /WX-)
    target_compile_definitions(WealthPilot PRIVATE _CRT_SECURE_NO_WARNINGS)
else ()
    target_compile_options(WealthPilot PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wno-unused-parameter
            -Wno-unused-variable
    )
endif ()

# ==================== 构建后操作 ====================

# 复制资源文件到构建目录
add_custom_command(TARGET WealthPilot POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources
)

# 复制 CTP DLL 文件到构建目录
if (WIN32)
    # 复制 external/ctp 目录下的 DLL
    add_custom_command(TARGET WealthPilot POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/external/ctp/thostmduserapi_se.dll"
            "$<TARGET_FILE_DIR:WealthPilot>/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/external/ctp/thosttraderapi_se.dll"
            "$<TARGET_FILE_DIR:WealthPilot>/"
    )
    message(STATUS "CTP DLLs will be copied to output directory")
endif ()

# ==================== 安装配置 ====================

install(TARGETS WealthPilot
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
)

install(DIRECTORY resources DESTINATION .)

# ==================== 打包配置（Windows）====================

if (WIN32)
    set(CMAKE_WIN32_EXECUTABLE TRUE)

    # 查找windeployqt
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if (WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET WealthPilot POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --verbose 0
                --no-compiler-runtime
                --no-opengl-sw
                "$<TARGET_FILE:WealthPilot>"
        )
    endif ()
endif ()

# ==================== 输出信息 ====================

message(STATUS "WealthPilot Version: ${PROJECT_VERSION}")
message(STATUS "Qt6 Version: ${Qt6Core_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
