# ==================== App 模块 CMakeLists.txt ====================
# 应用层模块：应用初始化、功能集成

add_library(App STATIC)

# ==================== 应用初始化 ====================
target_sources(App PRIVATE
        ApplicationInitializer.h
        ApplicationInitializer.cpp
        FeatureIntegration.h
        FeatureIntegration.cpp
)

# ==================== 包含目录 ====================
target_include_directories(App PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# ==================== 链接库 ====================
target_link_libraries(App PUBLIC
        Qt6::Core
        Qt6::Widgets
        WealthPilot::Shared
        WealthPilot::Core
        WealthPilot::Data
        WealthPilot::Infrastructure
        WealthPilot::Presentation
)

# ==================== 编译选项 ====================
if (MSVC)
    target_compile_options(App PRIVATE /W4)
else ()
    target_compile_options(App PRIVATE -Wall -Wextra -Wpedantic)
endif ()

# 添加别名，允许其他模块使用 WealthPilot::App
add_library(WealthPilot::App ALIAS App)
