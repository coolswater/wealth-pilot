# 技术分析系统使用指南

## 概述

WealthPilot 技术分析系统整合了四大经典分析理论：

- **波浪理论** (Elliott Wave)
- **缠论** (ChanLun)
- **道氏理论** (Dow Theory)
- **量价形态** (Volume Pattern)

通过多层过滤机制，产生高质量的综合交易信号。

## 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                    AnalysisManager                       │
│                   (统一管理入口)                          │
└────────────────────┬────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
┌───────▼────────┐        ┌──────▼──────┐
│  SignalService │        │ SignalFilter│
│  (实时服务)     │        │ (多层过滤)   │
└───────┬────────┘        └─────────────┘
        │
   ┌────┴────┬─────────┬─────────┬─────────┐
   │         │         │         │         │
┌──▼──┐  ┌──▼──┐  ┌──▼──┐  ┌──▼──┐  ┌──▼──┐
│波浪  │  │缠论  │  │道氏  │  │量价  │  │...  │
│理论  │  │      │  │理论  │  │形态  │  │     │
└─────┘  └─────┘  └─────┘  └─────┘  └─────┘
```

## 快速开始

### 1. 初始化

```cpp
#include "analysis/AnalysisManager.h"

using namespace WealthPilot::Analysis;

// 获取单例实例
auto* manager = AnalysisManager::instance();

// 初始化（只需调用一次）
manager->initialize();
```

### 2. 基本使用

```cpp
// 准备K线数据
QVector<KLine> klines;
// ... 填充K线数据 ...

// 分析标的
QString symbol = "IF2501";
CompositeSignal signal = manager->analyze(symbol, klines);

// 查看结果
qDebug() << "方向:" << signal.directionText();
qDebug() << "置信度:" << signal.confidence;
qDebug() << "支持理论数:" << signal.theoryCount;
qDebug() << "描述:" << signal.description;

if (signal.isStrongSignal()) {
    qDebug() << "发现强信号！";
}
```

### 3. 获取详细分析结果

```cpp
// 获取分析摘要
auto summary = manager->getAnalysisSummary(symbol);
qDebug() << "分析摘要:" << summary;

// 获取特定理论的分析器
auto* elliottWave = manager->getAnalyzer(TheoryType::ElliottWave);
auto* chanLun = manager->getAnalyzer(TheoryType::ChanLun);
```

## 各理论模块

### 波浪理论 (Elliott Wave)

**核心功能：**

- 波浪识别与计数
- 波浪级别判断
- 斐波那契比例验证
- 波浪规则验证

**信号类型：**

- 浪2、浪4调整结束信号
- 浪3强势上涨信号
- 浪5顶部风险信号
- C浪底部反转信号

```cpp
#include "analysis/elliottwave/ElliottWaveAnalyzer.h"

auto* analyzer = new ElliottWave::ElliottWaveAnalyzer();
auto result = analyzer->analyze(klines);

// 获取波浪计数
auto* waveCount = analyzer->currentWaveCount();
if (waveCount) {
    qDebug() << "当前波浪:" << waveCount->currentWave()->numberString();
    qDebug() << "预期下一波浪:" << waveCount->nextExpectedWave;
}
```

### 缠论 (ChanLun)

**核心功能：**

- K线包含处理
- 分型识别
- 笔划分
- 线段划分
- 中枢识别
- 背驰判断
- 买卖点识别

**信号类型：**

- 一买/一卖信号
- 二买/二卖信号
- 三买/三卖信号
- 背驰信号

```cpp
#include "analysis/chanlun/ChanLunAnalyzer.h"

auto* analyzer = new ChanLun::ChanLunAnalyzer();
auto result = analyzer->analyze(klines);

// 获取分析结果
const auto& chanLunResult = analyzer->result();
for (const auto& signal : chanLunResult.signals) {
    qDebug() << "信号:" << signal.description;
}
```

### 道氏理论 (Dow Theory)

**核心功能：**

- 趋势识别（主要、次要、小趋势）
- 高低点分析
- 趋势线绘制
- 趋势反转确认

**信号类型：**

- 趋势方向信号
- 趋势反转信号

```cpp
#include "analysis/dowtheory/DowTheoryAnalyzer.h"

auto* analyzer = new DowTheory::DowTheoryAnalyzer();
auto result = analyzer->analyze(klines);

// 获取趋势信息
const auto& dowResult = analyzer->dowResult();
qDebug() << "主要趋势:" << dowResult.primaryTrend.direction;
qDebug() << "趋势强度:" << dowResult.primaryTrend.strength;
```

### 量价形态 (Volume Pattern)

**核心功能：**

- 成交量形态识别
- 价格形态识别
- 量价组合分析
- OBV指标计算
- 背离检测

**信号类型：**

- 价涨量增（健康上涨）
- 价涨量缩（上涨乏力）
- 价跌量增（可能见底）
- 价跌量缩（下跌动能减弱）
- 放量突破
- 顶部/底部背离

```cpp
#include "analysis/volumepattern/VolumePatternAnalyzer.h"

auto* analyzer = new VolumePattern::VolumePatternAnalyzer();
auto result = analyzer->analyze(klines);

// 获取量价分析结果
const auto& volumeResult = analyzer->volumeResult();
qDebug() << "平均成交量:" << volumeResult.avgVolume;
qDebug() << "是否有背离:" << volumeResult.hasDivergence;
qDebug() << "是否有突破:" << volumeResult.hasBreakout;
```

## 多层过滤信号系统

### 过滤策略

1. **单理论信号强度过滤**
    - 最小信号强度要求
    - 最小置信度要求

2. **多理论一致性过滤**
    - 检查信号方向一致性
    - 计算综合置信度

3. **理论权重配置**
   ```cpp
   SignalFilterConfig config;
   config.theoryWeights[TheoryType::ChanLun] = 1.2;      // 缠论权重较高
   config.theoryWeights[TheoryType::ElliottWave] = 1.0;  // 波浪理论
   config.theoryWeights[TheoryType::DowTheory] = 0.9;    // 道氏理论
   config.theoryWeights[TheoryType::VolumePattern] = 0.8; // 量价形态

   manager->signalFilter()->setConfig(config);
   ```

4. **风险收益比评估**
    - 基于信号强度和置信度
    - 考虑理论支持数量

### 综合信号评分

```cpp
CompositeSignal signal;
double score = signal.score(); // 0-100分

// 评分公式：
// score = theoryCount * 25 + confidence * 0.5
```

## 实时信号服务

### 订阅信号

```cpp
// 添加订阅
SignalSubscription sub;
sub.symbol = "IF2501";
sub.minConfidence = 70.0;
sub.enableAlert = true;
sub.alertChannel = "default";

manager->signalService()->addSubscription(sub);
```

### 预警配置

```cpp
AlertConfig alertConfig;
alertConfig.enabled = true;
alertConfig.minConfidence = 75.0;
alertConfig.minTheoryCount = 3;
alertConfig.alertOnStrongSignals = true;

manager->signalService()->setAlertConfig(alertConfig);
```

### K线图集成

```cpp
// 获取指标数据
auto indicatorData = manager->signalService()->getIndicatorData(symbol);

// 获取叠加配置
auto overlayConfig = manager->signalService()->getOverlayConfig(symbol);

// 在K线图上显示信号
// overlayConfig["marker_color"] - 信号标记颜色
// overlayConfig["marker_shape"] - 信号标记形状
// overlayConfig["signal_direction"] - 信号方向
```

## 信号类型说明

### SignalDirection (信号方向)

- `Bullish` - 看涨
- `Bearish` - 看跌
- `Neutral` - 中性

### SignalStrength (信号强度)

- `Weak` (1) - 弱信号
- `Moderate` (2) - 中等信号
- `Strong` (3) - 强信号
- `VeryStrong` (4) - 非常强信号

### CompositeSignal (综合信号)

```cpp
struct CompositeSignal {
    QString symbol;             // 标的代码
    QDateTime time;             // 信号时间
    double price;               // 信号价格
    SignalDirection direction;  // 综合方向
    double confidence;          // 综合置信度 (0-100)
    int theoryCount;            // 支持理论数量
    QString description;        // 综合描述

    bool isStrongSignal() const; // 是否为强信号
    double score() const;        // 综合得分
};
```

## 最佳实践

### 1. 信号确认

```cpp
// 等待多个理论确认
if (signal.theoryCount >= 3 && signal.confidence >= 70) {
    // 高质量信号，可以考虑入场
}
```

### 2. 风险控制

```cpp
// 结合风险收益比
double riskReward = signal.metadata["riskRewardRatio"].toDouble();
if (riskReward >= 2.0) {
    // 风险收益比合理
}
```

### 3. 趋势确认

```cpp
// 确保信号与主要趋势一致
auto* dowAnalyzer = manager->getAnalyzer(TheoryType::DowTheory);
auto trend = dowAnalyzer->currentTrend();

if (signal.direction == SignalDirection::Bullish &&
    trend == TrendDirection::Upward) {
    // 信号与趋势一致，更可靠
}
```

## 性能优化

### 1. 增量更新

```cpp
// 使用增量更新而非全量分析
analyzer->update(newKlines);
```

### 2. 批量分析

```cpp
// 批量分析多个标的
QMap<QString, QVector<KLine>> data;
data["IF2501"] = klines1;
data["IC2501"] = klines2;

manager->signalService()->analyzeBatch(data);
```

### 3. 定时更新

```cpp
// SignalService 内置定时更新机制
// 默认每分钟更新一次
```

## 注意事项

1. **数据质量**：确保K线数据完整、准确
2. **参数调优**：根据不同品种调整分析参数
3. **组合使用**：建议多理论组合使用，提高信号可靠性
4. **风险提示**：技术分析仅供参考，不构成投资建议

## 更新日志

### v1.0.0 (2026-05-07)

- 实现波浪理论分析模块
- 实现道氏理论分析模块
- 实现量价形态分析模块
- 实现多层过滤信号系统
- 实现实时行情信号服务
- 集成到K线图和UI

---

**作者：** WealthPilot Team
**版本：** 1.0.0
**最后更新：** 2026-05-07
