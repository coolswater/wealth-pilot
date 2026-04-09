cmake_minimum_required(VERSION 3.16)
project(WealthPilot VERSION 1.0.0 LANGUAGES CXX)

# 设置默认构建类型
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()
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
)
find_package(Qt6 REQUIRED COMPONENTS Core5Compat)

# ==================== 源文件分组 ====================

# Core 核心
set(CORE_SOURCES
    src/core/ThemeManager.h
    src/core/ThemeManager.cpp
    src/core/Singleton.h
    src/core/ConfigManager.h
    src/core/ConfigManager.cpp
    src/core/BasePage.h
    src/core/PageFactoryRegistry.h
    src/core/PageFactoryRegistry.cpp
    src/core/PageNavigatorManager.h
    src/core/PageNavigatorManager.cpp
    src/core/AnimationManager.h
    src/core/AnimationManager.cpp
    src/core/CtpDataBuffer.h
    src/core/CtpApiLoader.h
    src/core/CtpMarketSpi.h
    src/core/CtpMarketSpi.cpp
    src/core/CtpTradingSpi.h
    src/core/CtpTradingSpi.cpp
    src/core/QuoteDataManager.cpp
    src/core/QuoteDataManager.h
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
)

# Network 网络
set(NETWORK_SOURCES
    src/network/NetworkManager.cpp
    src/network/NetworkManager.h
)

# Services 服务
set(SERVICE_SOURCES
    src/services/CTPService.h
    src/services/CTPService.cpp
    src/services/AIService.cpp
    src/services/AIService.h
)

# Controllers 控制器
set(CONTROLLER_SOURCES
)

# Widgets 控件
set(WIDGET_SOURCES
    src/views/widgets/BaseWidget.h
    src/views/widgets/BaseWidget.cpp
    src/views/widgets/SidebarWidget.h
    src/views/widgets/SidebarWidget.cpp
    src/views/widgets/SvgIconEngine.h
    src/views/widgets/SvgIconEngine.cpp
    src/views/widgets/SvgIconWidget.h
    src/views/widgets/SvgIconWidget.cpp
    src/views/widgets/TitleBarWidget.h
    src/views/widgets/TitleBarWidget.cpp
    src/views/widgets/ThemeToggleButton.h
    src/views/widgets/ThemeToggleButton.cpp
    src/views/widgets/CardWidget.h
    src/views/widgets/CardWidget.cpp
    src/views/widgets/StatusBarWidget.h
    src/views/widgets/StatusBarWidget.cpp
    src/views/widgets/AssetCardWidget.h
    src/views/widgets/AssetCardWidget.cpp
    src/views/widgets/AssetPieChart.h
    src/views/widgets/AssetPieChart.cpp
    src/views/widgets/AIAssistantPanelWidget.cpp
    src/views/widgets/AIAssistantPanelWidget.h
    src/views/widgets/NetworkIndicator.cpp
    src/views/widgets/NetworkIndicator.h
    src/views/widgets/TreeMapWidget.cpp
    src/views/widgets/TreeMapWidget.h
)

# Views 视图
set(VIEW_SOURCES
    src/views/mainWindow/MainWindow.cpp
    src/views/mainWindow/MainWindow.h
    src/views/dashboard/DashboardPage.h
    src/views/dashboard/DashboardPage.cpp
    src/views/widgets/DividerWidget.h
    src/views/widgets/DividerWidget.cpp
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
    src/views/stock/StockQuotesPage.h
    src/views/stock/StockQuotesPage.cpp
    src/views/settings/SettingsPage.h
    src/views/settings/SettingsPage.cpp
    src/views/warning/WarningPage.h
    src/views/warning/WarningPage.cpp
    src/views/aboutus/AboutUSPage.h
    src/views/aboutus/AboutUSPage.cpp
)

# 合并所有源文件（只包含C++源文件和头文件，不包含资源文件）
set(SOURCES
    main.cpp
    ${CORE_SOURCES}
    ${MODEL_SOURCES}
    ${UTIL_SOURCES}
    ${NETWORK_SOURCES}
    ${SERVICE_SOURCES}
    ${CONTROLLER_SOURCES}
    ${WIDGET_SOURCES}
    ${VIEW_SOURCES}
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
if(WIN32)
    target_link_libraries(WealthPilot PRIVATE
        crypt32
        Qt6::Core5Compat
    )

    # CTP 使用动态加载（解决 MinGW 与 MSVC DLL 名称修饰不兼容问题）
    # 不需要静态链接库，只需要确保 DLL 在运行时可用

    # 定义 CTP DLL 导入宏 (使用 DLL 需要 dllimport)
    target_compile_definitions(WealthPilot PRIVATE ISLIB)

    message(STATUS "CTP API will be loaded dynamically at runtime")
endif()

# 添加资源文件
qt6_add_resources(WealthPilot_RESOURCES resources/resources.qrc)
target_sources(WealthPilot PRIVATE ${WealthPilot_RESOURCES})

# ==================== 编译选项 ====================

if(MSVC)
    target_compile_options(WealthPilot PRIVATE /W4 /WX-)
    target_compile_definitions(WealthPilot PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(WealthPilot PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-variable
    )
endif()

# ==================== 构建后操作 ====================

# 复制资源文件到构建目录
add_custom_command(TARGET WealthPilot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources
)

# 复制 CTP DLL 文件到构建目录
if(WIN32)
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
endif()

# ==================== 安装配置 ====================

install(TARGETS WealthPilot
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)

install(DIRECTORY resources DESTINATION .)

# ==================== 打包配置（Windows）====================

if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE TRUE)

    # 查找windeployqt
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET WealthPilot POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --verbose 0
            --no-compiler-runtime
            --no-opengl-sw
            "$<TARGET_FILE:WealthPilot>"
        )
    endif()
endif()

# ==================== 输出信息 ====================

message(STATUS "WealthPilot Version: ${PROJECT_VERSION}")
message(STATUS "Qt6 Version: ${Qt6Core_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
