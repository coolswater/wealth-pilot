# Legacy 代码迁移计划

## 现状分析

Legacy 目录包含 25 个文件，核心功能：

### 仍在使用的模块

| 模块                    | 文件数 | 代码行数  | 使用情况                                             |
|-----------------------|-----|-------|--------------------------------------------------|
| 缠论分析 (chanlun)        | 7   | ~2500 | **活跃使用** - KLineChartWithSignals, StockKLinePage |
| 信号服务 (signal)         | 4   | ~500  | **活跃使用** - SignalDetailPanel, SignalMarker       |
| 道氏理论 (dowtheory)      | 3   | ~800  | 待评估                                              |
| 波浪理论 (elliottwave)    | 3   | ~600  | 待评估                                              |
| 成交量形态 (volumepattern) | 3   | ~400  | 待评估                                              |

### 引用关系

```
KLineChartWithSignals.h -> legacy/AnalysisTypes.h
SignalDetailPanel.h -> legacy/AnalysisTypes.h
SignalMarker.h -> legacy/AnalysisTypes.h
StockKLinePage.h -> legacy/chanlun/ChanLunIntegration.h
```

## 迁移策略

### 策略 A：保持现状（推荐）

**理由**：

- 缠论、信号服务是核心功能，运行稳定
- 迁移风险高，收益低
- Legacy 标记仅为警告，不影响功能

**行动**：

- 重命名目录：`legacy` -> `advanced` 或 `theories`
- 更新注释，移除 "legacy" 字样
- 保持代码不变

### 策略 B：渐进迁移

**步骤**：

1. 将 `AnalysisTypes.h` 提升到 `src/shared/types/`
2. 创建新接口 `ITheoryAnalyzer` 替代 `IAnalyzer`
3. 逐个模块迁移，保持向后兼容

**风险**：

- 破坏现有功能
- 需要大量测试
- 时间成本高

## 建议

**采用策略 A**：重命名目录，保持代码不变。

```bash
# 重命名
mv src/core/domain/analysis/legacy src/core/domain/analysis/theories

# 更新 include 路径
# 使用 IDE 批量替换
```

## 待办事项

- [ ] 评估道氏理论使用情况
- [ ] 评估波浪理论使用情况
- [ ] 评估成交量形态使用情况
- [ ] 决定是否重命名目录
