# WealthPilot 测试套件

## 测试结构

```
tests/
├── core/                   # 核心模块测试
│   ├── DataHubTest.cpp     # 数据中心测试
│   ├── ConfigManagerTest.cpp
│   └── CacheManagerTest.cpp
├── trading/                # 交易模块测试
│   ├── OrderManagerTest.cpp
│   ├── TradingServiceTest.cpp
│   └── RiskControllerTest.cpp
├── market/                 # 行情模块测试
│   ├── QuoteDataManagerTest.cpp
│   └── DataSourceTest.cpp
└── ui/                     # UI 模块测试
    ├── ThemeManagerTest.cpp
    └── PageTest.cpp
```

## 运行测试

### 运行所有测试

```bash
cd build
ctest --output-on-failure
```

### 运行单个测试

```bash
./tests/core/DataHubTest
./tests/trading/OrderManagerTest
```

### 生成测试覆盖率报告

```bash
cmake -DENABLE_COVERAGE=ON ..
make
make test
gcovr -r .. --html --html-details -o coverage.html
```

## 测试覆盖率目标

| 模块           | 当前覆盖率  | 目标覆盖率   |
|--------------|--------|---------|
| core/datahub | 0%     | 80%     |
| trading      | 0%     | 75%     |
| market       | 0%     | 70%     |
| ui           | 0%     | 60%     |
| **总计**       | **3%** | **70%** |

## 测试原则

1. **独立性**: 每个测试用例独立运行，不依赖其他测试
2. **可重复性**: 测试结果稳定可重复
3. **快速性**: 单元测试应在毫秒级完成
4. **完整性**: 覆盖正常流程、边界条件和异常情况

## Mock 对象

对于依赖外部系统的模块（如 CTP、网络），使用 Mock 对象：

```cpp
class MockCTPService : public CTP::CTPService {
public:
    MOCK_METHOD(void, connect, (), (override));
    MOCK_METHOD(void, subscribe, (const QString&), (override));
};
```
