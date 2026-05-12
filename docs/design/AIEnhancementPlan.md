# AI 功能增强开发计划

**项目**: WealthPilot 智能投资助手
**版本**: 1.0.0
**日期**: 2026-05-12
**目标**: 对标华泰证券涨乐财富通 AI 功能

---

## 一、现状分析

### 1.1 当前 AI 功能

| 功能    | 状态    | 完成度 |
|-------|-------|-----|
| AI 对话 | ✅ 已实现 | 80% |
| 投资建议  | ⚠️ 基础 | 50% |
| 组合分析  | ⚠️ 基础 | 40% |
| 股票分析  | ⚠️ 基础 | 40% |
| 智能选股  | ❌ 未实现 | 0%  |
| 语音交互  | ❌ 未实现 | 0%  |
| 智能预警  | ❌ 未实现 | 10% |
| 情绪分析  | ❌ 未实现 | 10% |
| 知识图谱  | ❌ 未实现 | 0%  |
| 智能报告  | ❌ 未实现 | 0%  |

### 1.2 与竞品差距

**华泰证券涨乐财富通 AI 功能:**

1. ✅ 智能投顾 - 个性化投资建议
2. ✅ 智能选股 - AI 驱动选股
3. ✅ 智能客服 - 7x24 服务
4. ✅ 语音交互 - 语音下单
5. ✅ 智能预警 - 风险/机会提醒
6. ✅ 情绪分析 - 市场情绪
7. ✅ 知识图谱 - 关联分析
8. ✅ 智能报告 - 自动生成

**WealthPilot 差距:**

- 智能投顾: 差距大
- 智能选股: 未实现
- 语音交互: 未实现
- 情绪分析: 未实现
- 知识图谱: 未实现
- 智能报告: 未实现

---

## 二、开发计划

### 阶段一：基础增强 (2周)

**目标**: 完善 AI 对话功能

#### 任务清单

| 任务        | 描述                    | 优先级 | 工作量 | 负责 |
|-----------|-----------------------|-----|-----|----|
| AI 对话界面优化 | 美化对话界面，添加 Markdown 渲染 | P0  | 2天  | -  |
| 对话历史管理    | 保存/加载对话历史             | P0  | 1天  | -  |
| 多轮对话上下文   | 实现上下文记忆               | P0  | 2天  | -  |
| 快捷问题模板    | 常见问题快捷入口              | P1  | 1天  | -  |
| 对话导出功能    | 导出对话记录                | P2  | 1天  | -  |

#### 技术方案

```cpp
// AI 对话历史管理
class AIConversationManager {
public:
    void saveConversation(const QString& id, const QList<AIMessage>& messages);
    QList<AIMessage> loadConversation(const QString& id);
    void deleteConversation(const QString& id);
    QList<QString> listConversations();
    
private:
    QString m_storagePath;
    int m_maxConversations = 100;
};
```

#### 验收标准

- [ ] 对话界面美观，支持 Markdown
- [ ] 对话历史可保存/加载
- [ ] 多轮对话上下文正确
- [ ] 快捷问题模板可用
- [ ] 对话可导出为文本

---

### 阶段二：智能投顾 (3周)

**目标**: 实现个性化投资建议

#### 任务清单

| 任务     | 描述            | 优先级 | 工作量 | 负责 |
|--------|---------------|-----|-----|----|
| 用户画像系统 | 收集用户偏好、风险承受能力 | P0  | 3天  | -  |
| 风险评估问卷 | 风险承受能力评估      | P0  | 2天  | -  |
| 投资组合建议 | 基于用户画像推荐组合    | P0  | 4天  | -  |
| 资产配置建议 | 资产配置方案        | P1  | 3天  | -  |
| 定投计划生成 | 智能定投建议        | P2  | 2天  | -  |

#### 技术方案

```cpp
// 用户画像
struct UserProfile {
    QString userId;
    int riskTolerance;        // 风险承受能力 1-10
    int investmentHorizon;    // 投资期限（月）
    double totalAssets;       // 总资产
    QStringList preferredSectors; // 偏好板块
    QStringList excludedStocks;   // 排除股票
    InvestmentGoal goal;      // 投资目标
};

// 智能投顾服务
class RoboAdvisor {
public:
    // 风险评估
    RiskAssessmentResult assessRisk(const UserProfile& profile);
    
    // 投资建议
    InvestmentAdvice getAdvice(const UserProfile& profile);
    
    // 资产配置
    AssetAllocation allocateAssets(const UserProfile& profile);
    
    // 定投计划
    SIPPlan generateSIPPlan(const UserProfile& profile);
};
```

#### 验收标准

- [ ] 风险评估问卷完整
- [ ] 用户画像可保存
- [ ] 投资建议个性化
- [ ] 资产配置合理
- [ ] 定投计划可执行

---

### 阶段三：智能选股 (3周)

**目标**: AI 驱动的选股功能

#### 任务清单

| 任务      | 描述      | 优先级 | 工作量 | 负责 |
|---------|---------|-----|-----|----|
| 选股策略引擎  | 多策略选股框架 | P0  | 4天  | -  |
| 因子选股模型  | 多因子选股   | P0  | 3天  | -  |
| AI 选股模型 | AI 辅助选股 | P1  | 4天  | -  |
| 回测验证系统  | 选股策略回测  | P1  | 3天  | -  |
| 选股结果展示  | 选股结果 UI | P0  | 2天  | -  |

#### 技术方案

```cpp
// 选股因子
enum class StockFactor {
    PE,           // 市盈率
    PB,           // 市净率
    ROE,          // 净资产收益率
    RevenueGrowth, // 营收增长
    ProfitGrowth,  // 利润增长
    DebtRatio,    // 负债率
    TurnoverRate, // 换手率
    Momentum,     // 动量
    Volatility,   // 波动率
    Volume        // 成交量
};

// 选股策略
class StockScreener {
public:
    // 因子选股
    QList<Stock> screenByFactors(const QList<StockFactor>& factors,
                                  const QMap<StockFactor, double>& minValues,
                                  const QMap<StockFactor, double>& maxValues);
    
    // AI 选股
    QList<Stock> screenByAI(const QString& description);
    
    // 综合选股
    QList<Stock> screen(const ScreeningCriteria& criteria);
};

// AI 选股服务
class AIStockPicker {
public:
    // 自然语言选股
    QList<Stock> pickFromDescription(const QString& description);
    
    // 相似股票推荐
    QList<Stock> findSimilar(const QString& stockCode);
    
    // 热点概念选股
    QList<Stock> pickByConcept(const QString& concept);
};
```

#### 验收标准

- [ ] 因子选股可用
- [ ] AI 选股准确率 > 60%
- [ ] 选股结果可回测
- [ ] 结果展示清晰
- [ ] 支持自定义策略

---

### 阶段四：语音交互 (2周)

**目标**: 语音输入输出功能

#### 任务清单

| 任务     | 描述         | 优先级 | 工作量 | 负责 |
|--------|------------|-----|-----|----|
| 语音识别集成 | 集成语音识别 API | P0  | 3天  | -  |
| 语音合成集成 | 集成语音合成 API | P0  | 2天  | -  |
| 语音命令解析 | 解析语音指令     | P1  | 3天  | -  |
| 语音下单功能 | 语音下单       | P2  | 2天  | -  |

#### 技术方案

```cpp
// 语音服务
class VoiceService {
public:
    // 语音识别
    void startRecognition();
    void stopRecognition();
    
signals:
    void recognitionResult(const QString& text);
    void recognitionError(const QString& error);

private:
    // 可选：Azure Speech / 讯飞语音
    VoiceRecognizer* m_recognizer;
    VoiceSynthesizer* m_synthesizer;
};

// 语音命令解析
class VoiceCommandParser {
public:
    struct Command {
        QString action;      // buy, sell, query, etc.
        QString target;      // stock code
        QVariantMap params;  // parameters
    };
    
    Command parse(const QString& voiceText);
};
```

#### 验收标准

- [ ] 语音识别准确率 > 90%
- [ ] 语音合成自然流畅
- [ ] 命令解析正确
- [ ] 语音下单可用

---

### 阶段五：智能预警 (2周)

**目标**: AI 驱动的预警系统

#### 任务清单

| 任务     | 描述         | 优先级 | 工作量 | 负责 |
|--------|------------|-----|-----|----|
| 异常检测算法 | 价格/成交量异常检测 | P0  | 3天  | -  |
| 机会识别算法 | 投资机会识别     | P1  | 3天  | -  |
| 预警推送系统 | 预警消息推送     | P0  | 2天  | -  |
| 预警历史管理 | 预警记录管理     | P2  | 1天  | -  |

#### 技术方案

```cpp
// 智能预警服务
class SmartAlertService {
public:
    // 异常检测
    QList<Alert> detectAnomalies(const QString& stockCode);
    
    // 机会识别
    QList<Alert> identifyOpportunities(const QString& stockCode);
    
    // 设置预警规则
    void setAlertRule(const AlertRule& rule);
    
signals:
    void alertTriggered(const Alert& alert);
};

// 预警类型
enum class AlertType {
    PriceSurge,      // 价格暴涨
    PricePlunge,     // 价格暴跌
    VolumeSurge,     // 成交量放大
    Breakout,        // 突破
    Breakdown,       // 跌破
    GoldenCross,     // 金叉
    DeathCross,      // 死叉
    SupportTest,     // 支撑测试
    ResistanceTest,  // 阻力测试
    AIRecommendation // AI 推荐
};
```

#### 验收标准

- [ ] 异常检测准确率 > 80%
- [ ] 机会识别有效
- [ ] 预警推送及时
- [ ] 历史可查询

---

### 阶段六：情绪分析 (2周)

**目标**: 市场情绪分析

#### 任务清单

| 任务     | 描述       | 优先级 | 工作量 | 负责 |
|--------|----------|-----|-----|----|
| 新闻情绪分析 | 新闻情感分析   | P0  | 3天  | -  |
| 社交媒体情绪 | 社交媒体情感分析 | P1  | 3天  | -  |
| 情绪指标计算 | 情绪指标计算   | P0  | 2天  | -  |
| 情绪可视化  | 情绪图表展示   | P1  | 2天  | -  |

#### 技术方案

```cpp
// 情绪分析服务
class SentimentAnalysisService {
public:
    // 分析新闻情绪
    SentimentResult analyzeNews(const QList<NewsItem>& news);
    
    // 分析社交媒体情绪
    SentimentResult analyzeSocialMedia(const QList<SocialPost>& posts);
    
    // 综合情绪指标
    SentimentIndex calculateIndex(const QString& stockCode);
    
    // 情绪趋势
    QList<SentimentPoint> getTrend(const QString& stockCode, int days);
};

// 情绪结果
struct SentimentResult {
    double positive;    // 正面情绪占比
    double negative;    // 负面情绪占比
    double neutral;     // 中性情绪占比
    double score;       // 情绪得分 -1 到 1
    QStringList keywords; // 关键词
};
```

#### 验收标准

- [ ] 新闻情绪分析准确
- [ ] 社交媒体情绪可用
- [ ] 情绪指标有意义
- [ ] 可视化清晰

---

### 阶段七：知识图谱 (3周)

**目标**: 股票关联分析

#### 任务清单

| 任务     | 描述       | 优先级 | 工作量 | 负责 |
|--------|----------|-----|-----|----|
| 知识图谱构建 | 构建股票知识图谱 | P0  | 4天  | -  |
| 关联关系挖掘 | 挖掘股票关联   | P0  | 3天  | -  |
| 图谱可视化  | 图谱展示     | P1  | 3天  | -  |
| 关联推荐   | 基于关联推荐   | P2  | 2天  | -  |

#### 技术方案

```cpp
// 知识图谱节点
struct GraphNode {
    QString id;          // 节点 ID
    QString type;        // stock, sector, concept, etc.
    QString name;        // 名称
    QVariantMap properties; // 属性
};

// 知识图谱边
struct GraphEdge {
    QString source;      // 源节点
    QString target;      // 目标节点
    QString relation;    // 关系类型
    double weight;       // 权重
};

// 知识图谱服务
class KnowledgeGraphService {
public:
    // 获取关联股票
    QList<GraphNode> getRelatedStocks(const QString& stockCode, int depth = 2);
    
    // 获取概念股票
    QList<GraphNode> getConceptStocks(const QString& concept);
    
    // 获取产业链
    QList<GraphEdge> getSupplyChain(const QString& stockCode);
    
    // 相似度计算
    double calculateSimilarity(const QString& stock1, const QString& stock2);
};
```

#### 验收标准

- [ ] 知识图谱构建完成
- [ ] 关联关系准确
- [ ] 图谱可视化美观
- [ ] 推荐有效

---

### 阶段八：智能报告 (2周)

**目标**: 自动生成分析报告

#### 任务清单

| 任务     | 描述          | 优先级 | 工作量 | 负责 |
|--------|-------------|-----|-----|----|
| 报告模板系统 | 报告模板管理      | P0  | 2天  | -  |
| 数据分析引擎 | 自动数据分析      | P0  | 3天  | -  |
| 报告生成器  | 报告自动生成      | P0  | 3天  | -  |
| 报告导出功能 | 导出 PDF/Word | P1  | 1天  | -  |

#### 技术方案

```cpp
// 报告类型
enum class ReportType {
    DailyBrief,       // 日报
    WeeklySummary,    // 周报
    MonthlyReview,    // 月报
    StockAnalysis,    // 个股分析
    PortfolioReview,  // 组合回顾
    MarketOutlook     // 市场展望
};

// 智能报告服务
class SmartReportService {
public:
    // 生成报告
    Report generateReport(ReportType type, const ReportParams& params);
    
    // 获取报告模板
    QList<ReportTemplate> getTemplates();
    
    // 自定义模板
    void saveTemplate(const ReportTemplate& tmpl);
    
    // 导出报告
    void exportReport(const Report& report, const QString& format, const QString& path);
};
```

#### 验收标准

- [ ] 报告模板完整
- [ ] 数据分析准确
- [ ] 报告生成快速
- [ ] 导出格式正确

---

## 三、技术选型

### 3.1 AI 服务

| 服务    | 推荐方案                  | 说明    |
|-------|-----------------------|-------|
| 对话 AI | OpenAI GPT-4 / Claude | 已集成   |
| 语音识别  | Azure Speech / 讯飞     | 中文支持好 |
| 语音合成  | Azure Speech / 讯飞     | 中文支持好 |
| 情绪分析  | 自研 + LLM              | 结合使用  |

### 3.2 数据存储

| 数据类型 | 推荐方案          | 说明   |
|------|---------------|------|
| 对话历史 | SQLite        | 本地存储 |
| 用户画像 | SQLite + JSON | 本地存储 |
| 知识图谱 | Neo4j         | 图数据库 |
| 情绪数据 | SQLite        | 时序数据 |

### 3.3 性能优化

| 优化点    | 方案      | 说明   |
|--------|---------|------|
| API 调用 | 异步 + 缓存 | 减少延迟 |
| 大数据处理  | 分片 + 增量 | 提高效率 |
| UI 响应  | 多线程     | 避免卡顿 |

---

## 四、资源需求

### 4.1 人力需求

| 角色     | 人数 | 周期 |
|--------|----|----|
| 架构师    | 1  | 全程 |
| 后端开发   | 2  | 全程 |
| 前端开发   | 1  | 全程 |
| AI 工程师 | 1  | 全程 |
| 测试工程师  | 1  | 全程 |

### 4.2 时间规划

| 阶段   | 周期 | 开始时间       | 结束时间       |
|------|----|------------|------------|
| 基础增强 | 2周 | 2026-05-13 | 2026-05-26 |
| 智能投顾 | 3周 | 2026-05-27 | 2026-06-16 |
| 智能选股 | 3周 | 2026-06-17 | 2026-07-07 |
| 语音交互 | 2周 | 2026-07-08 | 2026-07-21 |
| 智能预警 | 2周 | 2026-07-22 | 2026-08-04 |
| 情绪分析 | 2周 | 2026-08-05 | 2026-08-18 |
| 知识图谱 | 3周 | 2026-08-19 | 2026-09-08 |
| 智能报告 | 2周 | 2026-09-09 | 2026-09-22 |

**总周期**: 约 4.5 个月

---

## 五、风险评估

### 5.1 技术风险

| 风险       | 概率 | 影响 | 应对措施        |
|----------|----|----|-------------|
| API 调用限制 | 中  | 高  | 本地缓存 + 多服务商 |
| 语音识别准确率  | 中  | 中  | 多模型对比       |
| AI 选股准确率 | 高  | 高  | 回测验证 + 人工审核 |
| 性能问题     | 中  | 中  | 优化 + 异步     |

### 5.2 业务风险

| 风险    | 概率 | 影响 | 应对措施      |
|-------|----|----|-----------|
| 用户接受度 | 中  | 高  | 用户测试 + 迭代 |
| 合规问题  | 低  | 高  | 法律审核      |
| 竞品压力  | 高  | 中  | 差异化功能     |

---

## 六、验收标准

### 6.1 功能验收

- [ ] 所有功能模块完成
- [ ] 功能测试通过率 > 95%
- [ ] 性能测试通过
- [ ] 安全测试通过

### 6.2 质量验收

- [ ] 代码覆盖率 > 70%
- [ ] 无严重 Bug
- [ ] 文档完整
- [ ] 用户满意度 > 80%

---

**文档版本**: 1.0.0
**最后更新**: 2026-05-12