# 单元测试规划

## 1. 测试框架

使用 Qt Test 框架，支持：

- 单元测试
- 集成测试
- 性能基准测试
- GUI 测试

## 2. 测试目录结构

```
tests/
├── CMakeLists.txt
├── core/
│   ├── CacheManagerTest.cpp
│   ├── DataHubTest.cpp
│   └── ServiceLifecycleTest.cpp
├── data/
│   ├── DataStorageServiceTest.cpp
│   └── StockDataSourceTest.cpp
├── presentation/
│   ├── ThemeManagerTest.cpp
│   ├── KLineRendererTest.cpp
│   └── AnimationTest.cpp
└── utils/
    └── LoggerTest.cpp
```

## 3. 关键模块测试覆盖

### 3.1 核心模块（高优先级）

- CacheManager：缓存读写、TTL、淘汰策略
- DataHub：发布订阅、数据分发、生命周期
- ServiceLifecycle：服务启动顺序、依赖检查、优雅关闭

### 3.2 数据模块（中优先级）

- DataStorageService：CRUD 操作、事务、并发
- StockDataSource：数据获取、解析、错误处理

### 3.3 UI 模块（低优先级）

- ThemeManager：主题切换、样式缓存、监听器管理
- KLineRenderer：渲染输出、坐标转换
- Animation：动画状态、时间控制

## 4. 测试目标

| 模块            | 目标覆盖率 | 当前状态 |
|---------------|-------|------|
| CacheManager  | 80%   | 待实现  |
| DataHub       | 70%   | 待实现  |
| ThemeManager  | 60%   | 待实现  |
| KLineRenderer | 50%   | 待实现  |

## 5. 运行测试

```bash
# 构建测试
cmake -DBUILD_TESTS=ON ..
make

# 运行所有测试
ctest --output-on-failure

# 运行特定测试
./tests/core/CacheManagerTest
```
