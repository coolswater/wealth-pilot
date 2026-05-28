# WealthPilot 架构重构报告

## 重构目标

根据架构分析建议，将 Core 层拆分为 **Domain** 和 **Services** 两个独立层，并引入 **Application Service** 层协调 Domain 和
Infrastructure。

## 新架构概览

```
src/
├── application/          # [新增] 应用服务层 - 协调 Domain 和 Infrastructure
│   ├── market/          # 市场数据服务
│   ├── trading/         # 交易执行服务
│   ├── portfolio/       # 组合管理服务
│   ├── analysis/        # 分析服务
│   ├── risk/            # 风控服务
│   └── usecases/        # 用例编排
│
├── domain/              # [新增] 业务领域层 - 纯业务逻辑
│   ├── analysis/        # 技术分析（缠论、波浪、道氏等）
│   ├── backtest/        # 回测引擎
│   ├── portfolio/       # 组合优化
│   ├── quant/           # 量化交易
│   ├── recommendation/  # 智能推荐
│   ├── risk/             # 风险管理
│   └── trading/          # 交易模型
│
├── services/            # [新增] 基础服务层 - 通用服务能力
│   ├── account/         # 多账户管理
│   ├── alert/           # 预警系统
│   ├── cache/           # 缓存服务
│   ├── di/              # 依赖注入
│   ├── feedback/        # 用户反馈
│   ├── lifecycle/       # 生命周期
│   ├── monitoring/      # 性能监控
│   ├── navigation/      # 页面导航
│   ├── performance/     # 性能管理
│   ├── security/        # 权限管理
│   ├── social/          # 社交交易
│   ├── task/            # 异步任务
│   └── trading/         # 交易服务
│
├── data/                # 数据层（保持不变）
├── infrastructure/      # 基础设施层（保持不变）
├── presentation/        # 表示层（保持不变）
└── shared/              # 共享模块（保持不变）
```

## 模块职责

### 1. Application 层（新增）

**职责**：协调 Domain 和 Infrastructure，编排用例

- 统一对外接口，隐藏内部复杂性
- 管理事务边界和用例生命周期
- 协调多个 Domain 服务完成复杂业务流程

**核心类**：

- `MarketDataService` - 市场数据获取、订阅管理
- `TradingExecutionService` - 交易执行、订单管理
- `AnalysisAppService` - 分析流程编排
- `UseCaseBase` - 用例基类，定义执行模式

### 2. Domain 层（从 Core 拆分）

**职责**：纯业务逻辑，无基础设施依赖

- 分析算法（缠论、波浪、道氏、量价）
- 交易策略（量化、回测）
- 风险模型（风控、预警）
- 组合优化

**特点**：

- 不依赖 Qt Network、Qt WebSockets 等基础设施
- 可独立测试和复用
- 包含核心业务规则和计算逻辑

### 3. Services 层（从 Core 拆分）

**职责**：提供通用服务能力

- 依赖注入（DI 容器）
- 缓存管理
- 导航服务
- 异步任务调度
- 性能监控
- 权限管理

**特点**：

- 依赖 Qt Core、Qt Sql
- 提供跨 Domain 的通用能力
- 可被多个 Application Service 复用

## 依赖关系

```
┌─────────────────────────────────────────────────────────────┐
│                     Presentation                            │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                     Application                             │
│  MarketDataService, TradingExecutionService, UseCases       │
└─────────────────────────────────────────────────────────────┘
                              │
                    ┌────────┴────────┐
                    ▼                 ▼
┌───────────────────────┐   ┌─────────────────────────────────┐
│       Domain          │   │          Services                │
│ Analysis, Trading,    │   │ DI, Cache, Navigation, Task      │
│ Portfolio, Risk       │   │                                  │
└───────────────────────┘   └─────────────────────────────────┘
                    │                 │
                    └────────┬────────┘
                             ▼
┌─────────────────────────────────────────────────────────────┐
│                     Infrastructure + Data                   │
│ Database, Network, CTP, AI, Configuration                  │
└─────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────┐
│                       Shared                                │
│ Types, Utils, Base Classes                                 │
└─────────────────────────────────────────────────────────────┘
```

## 重构完成项

| 项目                        | 状态 | 说明                                         |
|---------------------------|----|--------------------------------------------|
| 创建 Application 模块         | ✅  | src/application/                           |
| 创建 Domain 模块              | ✅  | src/domain/                                |
| 创建 Services 模块            | ✅  | src/services/                              |
| 编写 Application CMakeLists | ✅  | 新增 3 个 CMakeLists.txt                      |
| 创建核心应用服务                  | ✅  | MarketDataService, TradingExecutionService |
| 创建用例基类                    | ✅  | UseCaseBase + 4 个具体用例                      |
| 迁移 Domain 文件              | ✅  | 从 core/domain 复制                           |
| 迁移 Services 文件            | ✅  | 从 core/services 复制                         |
| 修复 include 路径             | ✅  | 50+ 文件批量替换                                 |
| 更新依赖关系                    | ✅  | Infrastructure, Presentation, tests        |

## 待完成项

| 项目          | 状态 | 说明                |
|-------------|----|-------------------|
| 编译验证        | ⏳  | MinGW 环境问题        |
| 删除旧 core 目录 | ⏳  | 已重命名为 core_backup |
| 完善应用服务实现    | 📋 | 当前为骨架代码           |
| 补充用例编排逻辑    | 📋 | 需要具体业务实现          |
| 更新文档        | 📋 | 架构图、开发指南          |

## 已创建文件清单

### Application 层

```
src/application/
├── CMakeLists.txt
├── ApplicationInitializer.h/cpp  (已迁移)
├── FeatureIntegration.h/cpp      (已迁移)
├── market/
│   └── MarketDataService.h/cpp   (新增)
├── trading/
│   └── TradingExecutionService.h/cpp (新增)
├── portfolio/
│   └── PortfolioAppService.h/cpp (新增)
├── analysis/
│   └── AnalysisAppService.h/cpp  (新增)
├── risk/
│   └── RiskAppService.h/cpp      (新增)
└── usecases/
    ├── UseCaseBase.h/cpp         (新增)
    ├── StockAnalysisUseCase.h/cpp (新增)
    ├── PortfolioOptimizeUseCase.h/cpp (新增)
    ├── BacktestStrategyUseCase.h/cpp (新增)
    └── TradeExecutionUseCase.h/cpp (新增)
```

### Services 层

```
src/services/
├── CMakeLists.txt
└── trading/
    └── TradingService.h/cpp      (新增)
```

## 架构优势

1. **职责分离**：Domain 纯业务逻辑，Services 通用能力，Application 编排协调
2. **依赖清晰**：单向依赖，避免循环引用
3. **易于测试**：Domain 层可独立单元测试
4. **易于扩展**：新增功能只需在对应层添加
5. **符合 DDD**：领域驱动设计，业务逻辑集中在 Domain

## 下一步建议

1. **验证编译**：在 CLion 中重新配置 CMake，确保编译通过
2. **完善实现**：补充 Application Service 的具体业务逻辑
3. **补充测试**：为 Domain 层添加单元测试
4. **更新文档**：更新开发指南和架构文档

---
*生成时间：2026-05-28*
*架构师：WealthPilot Team*
