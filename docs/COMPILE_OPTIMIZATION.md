# 编译性能优化报告

## 问题分析

### 1. 编译产物过大
- 源文件: 348 个 (2.73 MB)
- 编译产物: 163 个 .obj 文件 (302 MB)
- 平均每个 .obj: ~1.85 MB

### 2. 缺少预编译头 (PCH)
- **当前状态**: 未配置
- **影响**: 每个源文件独立编译 Qt 头文件
- **预计优化**: 减少 40-60% 编译时间

### 3. 头文件包含过多
- `FeatureIntegration.h`: 18 个包含
- `OrderDialog.h`: 18 个包含
- `DatabaseManager.h`: 12 个包含

### 4. 单体编译单元
- 所有源文件编译到单个可执行文件
- 无动态库分离
- 修改任何文件都需要重新链接

---

## 优化方案

### 方案 1: 添加预编译头 (推荐)

**效果**: 减少 40-60% 编译时间

```cmake
# CMakeLists.txt
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

# 创建预编译头
set(PCH_HEADER
    <QObject>
    <QString>
    <QVector>
    <QHash>
    <QVariant>
    <QDateTime>
    <QMainWindow>
    <QWidget>
)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(WealthPilot PRIVATE ${PCH_HEADER})
endif()
```

### 方案 2: 减少头文件包含

**原则**: 头文件中尽量使用前向声明

```cpp
// 不好的做法
#include <QMainWindow>
#include <QString>
#include <QVector>

// 好的做法
class QMainWindow;
class QString;
// 在 .cpp 中包含完整头文件
```

### 方案 3: 模块化编译

**效果**: 减少链接时间，支持增量编译

```
WealthPilot
├── WealthPilotCore (动态库)
│   ├── base
│   ├── config
│   ├── cache
│   └── database
├── WealthPilotBusiness (动态库)
│   ├── analysis
│   ├── backtest
│   └── quant
└── WealthPilot (可执行文件)
    └── ui
```

### 方案 4: 并行编译优化

**当前**: `-j 10`
**建议**: 根据 CPU 核心数调整

```cmake
# 检测 CPU 核心数
include(ProcessorCount)
ProcessorCount(N)
if(N EQUAL 0)
    set(N 4)
endif()
set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
```

---

## 推荐实施顺序

1. **立即**: 添加预编译头 (最大收益)
2. **短期**: 优化 FeatureIntegration.h 包含
3. **中期**: 模块化编译
4. **长期**: 使用 C++20 模块

---

## 预期效果

| 优化项 | 当前时间 | 优化后 | 提升 |
|--------|----------|--------|------|
| 预编译头 | ~5min | ~2min | 60% |
| 头文件优化 | ~2min | ~1.5min | 25% |
| 模块化 | ~1.5min | ~30s | 67% |
| **总计** | **~5min** | **~30s** | **90%** |

---

**报告日期**: 2026-05-11
