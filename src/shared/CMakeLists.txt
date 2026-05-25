# ==================== Shared 模块 CMakeLists.txt ====================
# 共享基础模块：基础类、类型定义、工具类

add_library(Shared STATIC)

# 创建 ALIAS 目标，供其他模块使用 WealthPilot::Shared 名称
add_library(WealthPilot::Shared ALIAS Shared)

# ==================== Base 基础类 ====================
target_sources(Shared PRIVATE
        base/Singleton.h
        base/ErrorCode.h
        base/NamespaceGuide.h
        base/Result.h
)

# ==================== Types 类型定义 ====================
target_sources(Shared PRIVATE
        types/NewsTypes.h
        types/NewsTypes.cpp
        types/MarketTypes.h
)

# ==================== Utils 工具类 ====================
target_sources(Shared PRIVATE
        utils/EventBus.h
        utils/ErrorHandler.h
        utils/ErrorHandler.cpp
        utils/DataExporter.h
        utils/DataExporter.cpp
        utils/Logger.cpp
        utils/Logger.h
        utils/FuturesMockDataGenerator.h
        utils/FuturesMockDataGenerator.cpp
        utils/TechnicalIndicators.h
        utils/TechnicalIndicators.cpp
        utils/StressTest.h
        utils/StressTest.cpp
        utils/ExcelExporter.h
        utils/ExcelExporter.cpp
        utils/ReportGenerator.h
        utils/ReportGenerator.cpp
)

# ==================== 包含目录 ====================
target_include_directories(Shared PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# ==================== 链接库 ====================
target_link_libraries(Shared PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Sql
        Qt6::Network
        Qt6::Concurrent
        Qt6::Core5Compat
)

# ==================== 编译选项 ====================
if (MSVC)
    target_compile_options(Shared PRIVATE /W4)
else ()
    target_compile_options(Shared PRIVATE -Wall -Wextra -Wpedantic)
endif ()
