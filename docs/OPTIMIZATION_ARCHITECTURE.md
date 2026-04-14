# WealthPilot 项目优化架构文档

## 概述

本文档详细描述了WealthPilot项目的优化架构，包括UI组件库、依赖注入、配置管理、插件系统、缓存系统等核心模块的设计和实现。

## 架构层次

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层 (Application)                     │
│  MainWindow, Pages, Controllers                              │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                      插件层 (Plugins)                         │
│  CTP Plugin, AI Plugin, Strategy Plugin                      │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                      服务层 (Services)                        │
│  Service Locator, DI Container                               │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                      核心层 (Core)                            │
│  Config Manager, Cache Manager, Theme Engine                 │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                      UI组件库 (UI Components)                 │
│  BaseWidget, Button, Input, Table, Chart                     │
└─────────────────────────────────────────────────────────────┘
```

## 第一阶段：基础架构优化

### 1. UI组件库 (UI Components)

**位置**: `src/ui/components/`

**核心组件**:
- **ThemeEngine**: 主题引擎，管理应用主题和样式
  - 支持深色、浅色、护眼三种主题
  - 预编译样式表，减少运行时计算
  - 样式属性缓存，快速访问
  - 性能优化：使用查找表避免字符串比较

- **UIComponents**: 统一组件管理
  - 组件注册和获取
  - 主题配置管理
  - 响应式断点配置

**性能优化**:
- 预编译样式表
- 样式属性缓存
- 查找表优化
- 延迟加载

### 2. 依赖注入容器 (Service Locator)

**位置**: `src/core/ServiceLocator.h`

**功能**:
- 服务注册和解析
- 生命周期管理（单例、瞬态、作用域）
- 延迟初始化
- 线程安全
- 性能优化：使用type_index作为key，避免字符串比较

**使用示例**:
```cpp
// 注册服务
ServiceLocator::instance().registerSingleton<IAIService, AIService>();
ServiceLocator::instance().registerTransient<ICTPService, CTPService>();

// 解析服务
IAIService* aiService = ServiceLocator::instance().resolve<IAIService>();
```

### 3. 多环境配置管理 (Environment Config)

**位置**: `src/core/EnvironmentConfig.h`

**功能**:
- 多环境配置管理（开发/测试/预发布/生产）
- 环境切换和验证
- 配置继承和覆盖
- 热更新支持
- 性能优化：配置缓存

**环境配置**:
```cpp
// 开发环境
apiUrl: "http://localhost:8080/api"
enableDebugLog: true
cacheExpireTime: 60s

// 测试环境
apiUrl: "https://test-api.wealthpilot.com/api"
enableDebugLog: true
cacheExpireTime: 300s

// 生产环境
apiUrl: "https://api.wealthpilot.com/api"
enableDebugLog: false
cacheExpireTime: 600s
```

## 第二阶段：服务插件化

### 1. 插件接口 (IPlugin)

**位置**: `src/plugins/IPlugin.h`

**核心接口**:
- 生命周期管理：load, initialize, start, stop, unload
- 插件元数据：名称、版本、描述、依赖
- 配置管理：获取和设置配置
- 依赖管理：检查和获取依赖

### 2. 插件加载器 (PluginLoader)

**位置**: `src/plugins/PluginLoader.h`

**功能**:
- 动态加载插件（DLL/SO）
- 插件生命周期管理
- 插件依赖解析
- 热插拔支持
- 性能优化：延迟加载、插件缓存

**使用示例**:
```cpp
// 初始化插件系统
PluginLoader::instance().initialize("plugins");

// 加载所有插件
PluginLoader::instance().loadAll();

// 获取插件
ICTPPlugin* ctpPlugin = PluginLoader::instance().getPlugin<ICTPPlugin>("CTPPlugin");

// 热重载插件
PluginLoader::instance().reloadPlugin("CTPPlugin");
```

### 3. CTP插件接口 (ICTPPlugin)

**位置**: `src/plugins/ICTPPlugin.h`

**功能**:
- 连接管理：connect, disconnect
- 行情接口：订阅、取消订阅、获取行情
- 交易接口：下单、撤单、查询订单
- 账户接口：查询账户、查询持仓

### 4. AI插件接口 (IAIPlugin)

**位置**: `src/plugins/IAIPlugin.h`

**功能**:
- 对话接口：发送消息、异步发送、清除历史
- 分析接口：市场分析、技术指标分析、情绪分析
- 预测接口：价格预测、趋势预测
- 投顾接口：投资建议、策略生成、组合优化
- 风险评估：风险评估、VaR计算

## 第三阶段：数据层优化

### 1. 缓存管理器 (CacheManager)

**位置**: `src/core/CacheManager.h`

**功能**:
- 三级缓存：L1（内存）-> L2（磁盘）-> L3（数据库）
- 缓存策略：LRU、LFU、TTL、FIFO
- 自动过期清理
- 线程安全
- 性能优化：预分配内存、批量操作

**缓存级别**:
- **L1_Memory**: 内存缓存，最快，适合热点数据
- **L2_Disk**: 磁盘缓存，较快，适合大数据
- **L3_Database**: 数据库缓存，较慢，适合持久化数据

**使用示例**:
```cpp
// 设置缓存
CacheManager::instance().set("market_data", marketData, 300, CacheLevel::L1_Memory);

// 获取缓存
QVariant data = CacheManager::instance().get("market_data");

// 批量操作
CacheManager::instance().setBatch(dataMap, 300);
QMap<QString, QVariant> result = CacheManager::instance().getBatch(keys);

// 预热缓存
CacheManager::instance().warmup(initialData);
```

### 2. 网络请求缓存 (NetworkCache)

**位置**: `src/network/NetworkCache.h`

**功能**:
- HTTP请求缓存
- 请求去重
- 批量请求合并
- 失败重试
- 离线支持

## 第四阶段：功能扩展（待实现）

### 1. 高级图表分析

**计划功能**:
- K线图组件
- 技术指标计算（MA、MACD、RSI、KDJ等）
- 市场情绪分析
- 风险评估模型

### 2. AI增强功能

**计划功能**:
- 智能投顾建议
- 市场预测算法
- 自动交易策略
- 投资组合优化

### 3. 交易社区

**计划功能**:
- 策略分享平台
- 专家跟随功能
- 社区讨论
- 排行榜

## 第五阶段：性能优化（待实现）

### 1. 数据库优化

**计划功能**:
- SQLite性能调优
- 索引优化
- 查询优化
- 连接池

### 2. 异步处理框架

**计划功能**:
- 任务队列
- 线程池
- 异步IO
- 事件循环

### 3. 内存管理优化

**计划功能**:
- 对象池
- 内存监控
- 内存泄漏检测
- 智能指针优化

## 性能指标

### 缓存性能
- L1缓存命中时间：< 1ms
- L2缓存命中时间：< 10ms
- 缓存命中率目标：> 80%

### 插件加载性能
- 插件加载时间：< 100ms
- 插件热重载时间：< 200ms
- 插件解析时间：< 1ms

### UI渲染性能
- 主题切换时间：< 50ms
- 组件创建时间：< 10ms
- 样式应用时间：< 5ms

### 网络请求性能
- 缓存命中响应时间：< 5ms
- 请求去重率：> 30%
- 失败重试成功率：> 95%

## 使用指南

### 1. 初始化系统

```cpp
// 初始化配置管理
EnvironmentConfig::instance().initialize();

// 初始化缓存管理器
CacheManager::instance().initialize(100 * 1024 * 1024, 1024 * 1024 * 1024);

// 初始化主题引擎
ThemeEngine::instance().initialize();

// 初始化插件系统
PluginLoader::instance().initialize("plugins");
PluginLoader::instance().loadAll();

// 注册服务
ServiceLocator::instance().registerSingleton<IAIService, AIService>();
ServiceLocator::instance().registerSingleton<ICTPService, CTPService>();
```

### 2. 使用服务

```cpp
// 通过Service Locator获取服务
IAIService* aiService = ServiceLocator::instance().resolve<IAIService>();

// 通过插件系统获取插件
ICTPPlugin* ctpPlugin = PluginLoader::instance().getPlugin<ICTPPlugin>("CTPPlugin");
```

### 3. 使用缓存

```cpp
// 设置缓存
CacheManager::instance().set("key", data, 300, CacheLevel::L1_Memory);

// 获取缓存
QVariant data = CacheManager::instance().get("key");

// 查看统计
CacheStats stats = CacheManager::instance().statistics();
qDebug() << "Hit rate:" << stats.hitRate;
```

## 目录结构

```
src/
├── core/                   # 核心模块
│   ├── ServiceLocator.h    # 服务定位器
│   ├── EnvironmentConfig.h # 环境配置
│   ├── CacheManager.h      # 缓存管理器
│   ├── ConfigManager.h     # 配置管理器
│   └── ThemeManager.h      # 主题管理器
│
├── plugins/                # 插件系统
│   ├── IPlugin.h          # 插件接口
│   ├── PluginLoader.h     # 插件加载器
│   ├── ICTPPlugin.h       # CTP插件接口
│   └── IAIPlugin.h        # AI插件接口
│
├── ui/                     # UI组件库
│   └── components/
│       ├── UIComponents.h  # 组件管理
│       └── ThemeEngine.h   # 主题引擎
│
├── network/                # 网络模块
│   └── NetworkCache.h     # 网络缓存
│
├── services/               # 服务层
│   ├── AIService.h        # AI服务
│   └── CTPService.h       # CTP服务
│
└── views/                  # 视图层
    └── widgets/            # UI组件
```

## 下一步计划

1. 实现第四阶段功能扩展
   - 开发K线图组件
   - 实现技术指标计算
   - 开发AI分析功能

2. 实现第五阶段性能优化
   - 优化SQLite性能
   - 实现异步处理框架
   - 优化内存管理

3. 编写单元测试
   - 核心模块测试
   - 插件系统测试
   - 缓存系统测试

4. 性能测试和优化
   - 压力测试
   - 内存泄漏检测
   - 性能调优

## 版本历史

- v2.0.0 (2026-04-14)
  - 实现UI组件库和主题系统
  - 实现Service Locator依赖注入
  - 实现多环境配置管理
  - 实现插件系统架构
  - 实现多级缓存系统
  - 实现网络请求缓存

- v1.0.0
  - 初始版本
  - 基础UI组件
  - 基本配置管理
  - CTP和AI服务基础实现
