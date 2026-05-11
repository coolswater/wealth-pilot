# WealthPilot 模块依赖关系图

## 架构分层

```
┌─────────────────────────────────────────────────────────────┐
│                        UI 层                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  Views   │ │Components│ │  Charts  │ │  Dialogs │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                       业务层                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │ Analysis │ │  Quant   │ │  Social  │ │   AI     │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │ Backtest │ │  Risk    │ │Portfolio  │ │  Alert   │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                       服务层                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  Cache   │ │Database  │ │ Network  │ │   API    │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  Config  │ │  Task    │ │Security  │ │ Account  │       │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                       基础层                                 │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  Types   │ │  Base    │ │   DI     │ │Performance│      │
│  └──────────┘ └──────────┘ └──────────┘ └──────────┘       │
└─────────────────────────────────────────────────────────────┘
```

## 模块职责

### 基础层 (Foundation)

| 模块 | 职责 | 依赖 |
|------|------|------|
| `types` | 基础类型定义 | 无 |
| `base` | 基础工具类 | `types` |
| `di` | 依赖注入 | `base` |
| `performance` | 性能管理 | `base` |

### 服务层 (Services)

| 模块 | 职责 | 依赖 |
|------|------|------|
| `config` | 配置管理 | `base` |
| `cache` | 缓存服务 | `base`, `config` |
| `database` | 数据库服务 | `base`, `config` |
| `network` | 网络服务 | `base`, `config` |
| `api` | 数据API | `network`, `security` |
| `task` | 异步任务 | `base` |
| `security` | 权限管理 | `base` |
| `account` | 账户管理 | `base`, `database` |

### 业务层 (Business)

| 模块 | 职责 | 依赖 |
|------|------|------|
| `analysis` | 数据分析 | `cache`, `database` |
| `backtest` | 策略回测 | `analysis`, `database` |
| `quant` | 量化交易 | `analysis`, `network`, `security` |
| `risk` | 风险管理 | `analysis`, `account` |
| `social` | 社交交易 | `network`, `security` |
| `ai` | AI助手 | `network`, `analysis` |
| `portfolio` | 组合管理 | `analysis`, `account` |
| `alert` | 预警系统 | `analysis`, `network` |

### UI层 (Presentation)

| 模块 | 职责 | 依赖 |
|------|------|------|
| `views` | 页面视图 | `business.*`, `ui.components` |
| `components` | UI组件 | `business.*` |
| `charts` | 图表组件 | `analysis`, `backtest` |
| `dialogs` | 对话框 | `business.*` |

## 初始化顺序

```
1. 基础层
   ├── Types (无依赖)
   ├── Base (依赖 Types)
   ├── DI (依赖 Base)
   └── Performance (依赖 Base)

2. 服务层
   ├── Config (依赖 Base)
   ├── Cache (依赖 Config)
   ├── Database (依赖 Config)
   ├── Network (依赖 Config)
   ├── Task (依赖 Base)
   ├── Security (依赖 Base)
   ├── API (依赖 Network, Security)
   └── Account (依赖 Database)

3. 业务层
   ├── Analysis (依赖 Cache, Database)
   ├── Backtest (依赖 Analysis)
   ├── Quant (依赖 Analysis, Network)
   ├── Risk (依赖 Analysis)
   ├── Social (依赖 Network)
   ├── AI (依赖 Network)
   ├── Portfolio (依赖 Analysis)
   └── Alert (依赖 Analysis)

4. UI层
   ├── Components (依赖 Business)
   ├── Charts (依赖 Analysis)
   ├── Dialogs (依赖 Business)
   └── Views (依赖 All)
```

## 模块通信

### 同步通信
- 直接方法调用
- 信号槽机制

### 异步通信
- Task 异步任务
- 事件总线（可选）

## 扩展指南

### 添加新模块

1. 确定模块层级（基础/服务/业务/UI）
2. 定义模块接口
3. 实现模块功能
4. 注册到 ServiceLocator
5. 更新本文档

### 添加新功能

1. 确定功能所属模块
2. 检查依赖是否满足
3. 实现功能
4. 添加单元测试
5. 更新模块文档

---

**文档版本**: 1.0.0  
**最后更新**: 2026-05-11
