# WealthPilot 单元测试 CMake 配置
cmake_minimum_required(VERSION 3.16)

# 查找 Qt Test 模块
find_package(Qt6 REQUIRED COMPONENTS Test)

# ========== 核心模块测试 ==========

add_executable(CacheManagerTest core/CacheManagerTest.cpp)
target_include_directories(CacheManagerTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(CacheManagerTest
        PRIVATE
        Qt6::Test
        WealthPilot::Services
)
add_test(NAME CacheManagerTest COMMAND CacheManagerTest)

add_executable(DataHubTest core/DataHubTest.cpp)
target_include_directories(DataHubTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(DataHubTest
        PRIVATE
        Qt6::Test
        WealthPilot::Data
)
add_test(NAME DataHubTest COMMAND DataHubTest)

# ========== UI 模块测试 ==========

add_executable(ThemeManagerTest presentation/ThemeManagerTest.cpp)
target_include_directories(ThemeManagerTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(ThemeManagerTest
        PRIVATE
        Qt6::Test
        WealthPilot::Presentation
)
add_test(NAME ThemeManagerTest COMMAND ThemeManagerTest)

# ========== 测试配置 ==========

# 设置测试属性
set_tests_properties(
        CacheManagerTest
        DataHubTest
        ThemeManagerTest
        PROPERTIES
        TIMEOUT 60
        LABELS "unit"
)

# 添加自定义测试目标
add_custom_target(test_all
        COMMAND ctest --output-on-failure
        DEPENDS
        CacheManagerTest
        DataHubTest
        ThemeManagerTest
)

# 内存检查（Linux）
if (UNIX)
    add_custom_target(test_memcheck
            COMMAND valgrind --leak-check=full --error-exitcode=1 ctest
            DEPENDS test_all
    )
endif ()

# 代码覆盖率（可选）
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if (ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
endif ()
