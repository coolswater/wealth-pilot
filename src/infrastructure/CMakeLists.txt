# ==================== Infrastructure 模块 CMakeLists.txt ====================
# 基础设施模块：配置、数据库、网络、CTP、AI、插件系统

add_library(Infrastructure STATIC)

# ==================== Config 配置管理 ====================
target_sources(Infrastructure PRIVATE
        config/ConfigManager.h
        config/ConfigManager.cpp
        config/EnvironmentConfig.h
        config/EnvironmentConfig.cpp
        config/Tokens.h
)

# ==================== Database 数据库 ====================
target_sources(Infrastructure PRIVATE
        database/DatabaseManager.h
        database/DatabaseManager.cpp
)

# ==================== Network 网络 ====================
target_sources(Infrastructure PRIVATE
        network/NetworkManager.cpp
        network/NetworkManager.h
        network/NetworkCache.h
        network/NetworkCache.cpp
        network/core/WebSocketManager.h
        network/core/WebSocketManager.cpp
)

# ==================== API 数据API ====================
target_sources(Infrastructure PRIVATE
        api/DataAPIManager.h
        api/DataAPIManager.cpp
)

# ==================== CTP 交易接口 ====================

# CTP API
target_sources(Infrastructure PRIVATE
        ctp/api/CtpApiLoader.h
        ctp/api/CtpDataBuffer.h
        ctp/api/CtpMarketSpi.h
        ctp/api/CtpMarketSpi.cpp
        ctp/api/CtpTradingSpi.h
        ctp/api/CtpTradingSpi.cpp
)

# CTP Config
target_sources(Infrastructure PRIVATE
        ctp/config/CTPConfigManager.h
        ctp/config/CTPConfigManager.cpp
)

# CTP Service
target_sources(Infrastructure PRIVATE
        ctp/service/CTPService.h
        ctp/service/CTPService.cpp
)

# ==================== AI 智能助手 ====================

# AI Core
target_sources(Infrastructure PRIVATE
        ai/core/AIAssistant.h
        ai/core/AIAssistant.cpp
)

# AI Service
target_sources(Infrastructure PRIVATE
        ai/service/AIService.h
        ai/service/AIService.cpp
        ai/service/AIConversationManager.h
        ai/service/AIConversationManager.cpp
        ai/service/RoboAdvisor.h
        ai/service/RoboAdvisor.cpp
        ai/service/AIStockPicker.h
        ai/service/AIStockPicker.cpp
        ai/service/SentimentAnalysisService.h
        ai/service/SentimentAnalysisService.cpp
        ai/service/SmartReportService.h
        ai/service/SmartReportService.cpp
)

# AI Plugin
target_sources(Infrastructure PRIVATE
        ai/plugin/AIPlugin.h
        ai/plugin/AIPlugin.cpp
)

# ==================== Plugins 插件系统 ====================

# 插件框架
target_sources(Infrastructure PRIVATE
        plugins/IPlugin.h
        plugins/PluginLoader.h
        plugins/PluginLoader.cpp
        plugins/ICTPPlugin.h
        plugins/IAIPlugin.h
        plugins/CTPPlugin.h
        plugins/CTPPlugin.cpp
)

# 插件核心
target_sources(Infrastructure PRIVATE
        plugins/core/PluginMarketManager.h
        plugins/core/PluginMarketManager.cpp
        plugins/core/PluginSystem.h
        plugins/core/PluginSystem.cpp
)

# ==================== 包含目录 ====================
target_include_directories(Infrastructure PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/external
)

# ==================== 链接库 ====================
target_link_libraries(Infrastructure PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        Qt6::WebSockets
        Qt6::Concurrent
        WealthPilot::Shared
        WealthPilot::Core
)

# ==================== 编译选项 ====================
if (MSVC)
    target_compile_options(Infrastructure PRIVATE /W4)
else ()
    target_compile_options(Infrastructure PRIVATE -Wall -Wextra -Wpedantic)
endif ()

# 添加别名，允许其他模块使用 WealthPilot::Infrastructure
add_library(WealthPilot::Infrastructure ALIAS Infrastructure)
