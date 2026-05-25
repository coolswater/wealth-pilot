# ==================== Tests 模块 CMakeLists.txt ====================
# 单元测试配置

# 查找 Qt Test 模块
find_package(Qt6 REQUIRED COMPONENTS Test)

# ==================== 测试通用源文件 ====================
set(TEST_COMMON_SOURCES
        ${CMAKE_SOURCE_DIR}/src/core/services/cache/CacheManager.cpp
        ${CMAKE_SOURCE_DIR}/src/core/services/di/ServiceLocator.cpp
        ${CMAKE_SOURCE_DIR}/src/shared/utils/Logger.cpp
)

# ==================== 独立测试可执行文件 ====================

# 1. ServiceLocator 测试
add_executable(TestServiceLocator TestServiceLocator.cpp ${TEST_COMMON_SOURCES})
target_link_libraries(TestServiceLocator PRIVATE Qt6::Core Qt6::Test)
target_include_directories(TestServiceLocator PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME ServiceLocatorTest COMMAND TestServiceLocator)

# 2. CacheManager 测试
add_executable(TestCacheManager TestCacheManager.cpp ${TEST_COMMON_SOURCES})
target_link_libraries(TestCacheManager PRIVATE Qt6::Core Qt6::Test Qt6::Sql Qt6::Network)
target_include_directories(TestCacheManager PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME CacheManagerTest COMMAND TestCacheManager)

# 3. PluginLoader 测试
add_executable(TestPluginLoader TestPluginLoader.cpp)
target_link_libraries(TestPluginLoader PRIVATE Qt6::Core Qt6::Test)
target_include_directories(TestPluginLoader PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME PluginLoaderTest COMMAND TestPluginLoader)

# 4. ApplicationInitializer 测试
add_executable(TestApplicationInitializer TestApplicationInitializer.cpp)
target_link_libraries(TestApplicationInitializer PRIVATE Qt6::Core Qt6::Test)
target_include_directories(TestApplicationInitializer PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME ApplicationInitializerTest COMMAND TestApplicationInitializer)

# 5. Performance 测试
add_executable(PerformanceTest PerformanceTest.cpp)
target_link_libraries(PerformanceTest PRIVATE Qt6::Core Qt6::Test)
target_include_directories(PerformanceTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME PerformanceTest COMMAND PerformanceTest)

# 6. Analysis 测试
add_executable(AnalysisTest AnalysisTest.cpp)
target_link_libraries(AnalysisTest PRIVATE Qt6::Core Qt6::Test)
target_include_directories(AnalysisTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME AnalysisTest COMMAND AnalysisTest)

# 7. UserFeedbackManager 测试
add_executable(UserFeedbackManagerTest UserFeedbackManagerTest.cpp)
target_link_libraries(UserFeedbackManagerTest PRIVATE Qt6::Core Qt6::Test Qt6::Widgets)
target_include_directories(UserFeedbackManagerTest PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME UserFeedbackManagerTest COMMAND UserFeedbackManagerTest)
