# ==================== WealthPilot 根目录 CMakeLists.txt ====================
# 模块化构建配置 - 主入口

cmake_minimum_required(VERSION 3.16)
project(WealthPilot VERSION 1.0.0 LANGUAGES CXX)

# ==================== 编译器版本检验 (C++20要求) ====================
if (MSVC AND MSVC_VERSION LESS 1930)
    message(FATAL_ERROR
            "MSVC 19.30 (Visual Studio 2022 17.0) or newer required for C++20 support.\n"
            "Detected MSVC_VERSION: ${MSVC_VERSION}\n"
            "Install VS 2022 from https://visualstudio.microsoft.com/downloads/")
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
    message(FATAL_ERROR
            "GCC 10.0 or newer required for C++20 support. Detected: ${CMAKE_CXX_COMPILER_VERSION}")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.0)
    message(FATAL_ERROR
            "Clang 13.0 or newer required for C++20 support. Detected: ${CMAKE_CXX_COMPILER_VERSION}")
endif ()

# ==================== 构建选项 ====================
option(WEALTHPILOT_ENABLE_LTO "Enable Link Time Optimization" OFF)
option(WEALTHPILOT_ENABLE_UNITY_BUILD "Enable Unity Build for faster compilation" ON)
option(WEALTHPILOT_ENABLE_CCACHE "Enable ccache/sccache for compiler caching" ON)
option(WEALTHPILOT_ENABLE_DEBUG_OUTPUT "Enable debug output" OFF)
option(BUILD_TESTS "Build unit tests" ON)

# 设置并行编译数（默认使用所有CPU核心）
if (NOT DEFINED CMAKE_BUILD_PARALLEL_LEVEL)
    include(ProcessorCount)
    ProcessorCount(N)
    if (NOT N EQUAL 0)
        set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
        message(STATUS "Parallel build level: ${N}")
    endif ()
endif ()

# ==================== 编译缓存 (增强PCH支持) ====================
if (WEALTHPILOT_ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM NAMES ccache sccache)
    if (CCACHE_PROGRAM)
        # 设置ccache环境变量以支持PCH
        if (WIN32)
            set(CCACHE_DIR "$ENV{LOCALAPPDATA}/ccache")
        elseif (APPLE)
            set(CCACHE_DIR "$ENV{HOME}/Library/Caches/ccache")
        else ()
            set(CCACHE_DIR "$ENV{HOME}/.ccache")
        endif ()

        # 写入ccache配置文件
        if (EXISTS "${CCACHE_DIR}" AND IS_DIRECTORY "${CCACHE_DIR}")
            file(WRITE "${CCACHE_DIR}/ccache.conf"
                    "sloppiness = pch_defines,time_macros,include_file_mtime,include_file_ctime,locale\n"
                    "pch_external_checksum = true\n"
                    "depend_mode = true\n"
                    "compiler_check = content\n"
                    "max_size = 5G\n")
            message(STATUS "Wrote ccache config to ${CCACHE_DIR}/ccache.conf")
        endif ()

        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        message(STATUS "Compiler cache enabled: ${CCACHE_PROGRAM} (PCH-aware)")
    endif ()
endif ()

# ==================== Unity Build ====================
if (WEALTHPILOT_ENABLE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 14)
    message(STATUS "Unity Build enabled with batch size 14")
endif ()

# 设置Qt6路径（如果未设置）
if (NOT CMAKE_PREFIX_PATH)
    if (WIN32)
        set(CMAKE_PREFIX_PATH "C:/Qt/6.10.2/mingw_64")
    elseif (APPLE)
        set(CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.10.2/macos")
    else ()
        set(CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.10.2/gcc_64")
    endif ()
endif ()

# 设置默认构建类型
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# ==================== C++ 标准 ====================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)  # 使用标准C++，不使用编译器扩展
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 导出 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==================== 查找Qt6组件 ====================
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
        Quick
        QuickWidgets
        Qml
)

# ==================== 全局包含目录 ====================
set(WEALTHPILOT_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/src
)

# ==================== 添加子目录 ====================
add_subdirectory(src/shared)
add_subdirectory(src/core)
add_subdirectory(src/data)
add_subdirectory(src/infrastructure)
add_subdirectory(src/presentation)
add_subdirectory(src/app)

# ==================== 主可执行文件 ====================
add_executable(WealthPilot WIN32 MACOSX_BUNDLE
        main.cpp
)

# 链接所有模块
target_link_libraries(WealthPilot PRIVATE
        WealthPilot::App
        WealthPilot::Core
        WealthPilot::Data
        WealthPilot::Infrastructure
        WealthPilot::Presentation
        WealthPilot::Shared
)

# 预编译头 (CMake 3.16+)
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(WealthPilot PRIVATE
        src/pch.h
    )
    message(STATUS "Precompiled headers enabled")
endif()

# 包含目录
target_include_directories(WealthPilot PRIVATE
        ${WEALTHPILOT_INCLUDE_DIRS}
)

# QML 模块导入路径
if(WIN32)
    set(QT_QML_IMPORT_PATH "C:/Qt/6.10.2/mingw_64/qml")
    set_target_properties(WealthPilot PROPERTIES
        QT_QML_MODULE_IMPORT_PATH "${QT_QML_IMPORT_PATH}"
    )
endif()

# Windows 平台特定库
if (WIN32)
    target_link_libraries(WealthPilot PRIVATE
            crypt32
            Qt6::Core5Compat
    )
    target_compile_definitions(WealthPilot PRIVATE ISLIB)
    message(STATUS "CTP API will be loaded dynamically at runtime")
endif ()

# 添加资源文件
qt6_add_resources(WealthPilot_RESOURCES resources/resources.qrc)
target_sources(WealthPilot PRIVATE ${WealthPilot_RESOURCES})

# QML 资源
target_sources(WealthPilot PRIVATE qml/qml.qrc)

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
    # MinGW Debug 模式需要 -Wa,-mbig-obj 避免大文件编译错误
    if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
        target_compile_options(WealthPilot PRIVATE -Wa,-mbig-obj)
    endif ()
endif ()

# ==================== 构建后操作 ====================

# 复制资源文件到构建目录
add_custom_command(TARGET WealthPilot POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources ${CMAKE_BINARY_DIR}/resources
)

# 复制 CTP DLL 文件到构建目录
if (WIN32)
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

# ==================== 测试配置 ====================
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
    message(STATUS "Tests enabled")
endif()
