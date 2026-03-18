cmake_minimum_required(VERSION 3.16)
project(WealthPilot VERSION 1.1.0 LANGUAGES CXX)

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
)

# ==================== 源文件分组 ====================

# Core 核心
set(CORE_SOURCES
    src/core/ThemeManager.h
    src/core/ThemeManager.cpp
    src/core/Singleton.h
    src/core/Tokens.h
    src/core/ConfigManager.h
    src/core/ConfigManager.cpp
)

# Models 数据模型
set(MODEL_SOURCES
)

# Utils 工具
set(UTIL_SOURCES
    src/utils/Logger.cpp
    src/utils/Logger.h
)

# Network 网络
set(NETWORK_SOURCES
)

# Services 服务
set(SERVICE_SOURCES
)

# Controllers 控制器
set(CONTROLLER_SOURCES
)

# Widgets 控件
set(WIDGET_SOURCES
)

# Views 视图
set(VIEW_SOURCES
    src/views/mainWindow/MainWindow.cpp
    src/views/mainWindow/MainWindow.h
)

# Resources 资源
set(RESOURCE_SOURCES
    resources/style/theme_dark.qss
    resources/style/theme_light.qss
    resources/style/theme_eyecare.qss
)

# 合并所有源文件
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
    ${RESOURCE_SOURCES}
)

# 创建可执行文件
add_executable(WealthPilot WIN32 MACOSX_BUNDLE ${SOURCES})

# 包含目录
target_include_directories(WealthPilot PRIVATE
    ${CMAKE_SOURCE_DIR}
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
)

# Windows 平台特定库
if(WIN32)
    target_link_libraries(WealthPilot PRIVATE
        crypt32
    )
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
