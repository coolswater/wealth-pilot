# WealthPilot 新功能使用指南

## 快速开始

### 1. 初始化所有功能

在 `main.cpp` 或 `ApplicationInitializer` 中添加：

```cpp
#include "FeatureIntegration.h"

// 初始化所有新功能
WealthPilot::FeatureIntegration::instance()->initialize(mainWindow);
```

---

## 短期规划功能

### WebSocket 断线重连

```cpp
#include "core/network/WebSocketManager.h"

// 连接到服务器
auto* ws = WebSocketManager::instance();
ws->setReconnectConfig(-1, 3000);  // 无限重试，间隔3秒
ws->setHeartbeat(30000, "{}");     // 30秒心跳
ws->connectToServer("wss://quote.example.com/ws");

// 发送消息
ws->sendMessage("{\"action\":\"subscribe\",\"symbol\":\"600519\"}");

// 接收消息
connect(ws, &WebSocketManager::messageReceived, [](const QString& msg) {
    qDebug() << "Received:" << msg;
});
```

### 快捷键系统

```cpp
#include "core/ui/ShortcutManager.h"

auto* shortcutMgr = ShortcutManager::instance();

// 注册快捷键
shortcutMgr->registerShortcut(
    "custom.action",
    "自定义动作",
    QKeySequence("Ctrl+Alt+D"),
    []() {
        qDebug() << "快捷键触发";
    },
    "自定义",
    "执行自定义动作"
);

// 修改快捷键
shortcutMgr->setShortcut("custom.action", QKeySequence("Ctrl+Shift+D"));
```

### 窗口布局保存

```cpp
#include "core/ui/LayoutManager.h"

auto* layoutMgr = WindowLayoutManager::instance();

// 保存当前布局
layoutMgr->saveLayout("my_layout");

// 恢复布局
layoutMgr->restoreLayout("my_layout");

// 注册分割器（自动保存状态）
layoutMgr->registerSplitter("main_splitter", ui->splitter);
```

### 股票筛选器

```cpp
#include "core/analysis/StockScreener.h"

auto* screener = StockScreener::instance();

// 使用预设策略
screener->setupBreakoutStrategy();  // 突破策略

// 添加自定义条件
screener->addCondition("pe", "<", 20, 1.0);  // PE < 20
screener->addCondition("volume", ">", "avgVolume5", 1.5);  // 放量

// 设置筛选范围
screener->setScope({"600519", "000858", "000001"});

// 执行筛选
screener->execute();

// 获取结果
auto results = screener->getResults();
```

### 策略回测

```cpp
#include "core/backtest/BacktestEngine.h"
#include "core/analysis/ExampleStrategies.h"

auto* backtest = BacktestEngine::instance();

// 设置策略
auto strategy = std::make_shared<DoubleMAStrategy>(5, 20);
backtest->setStrategy(strategy);

// 设置参数
backtest->setParameters(1000000, 0.0003, 0.001);  // 100万，手续费0.03%，滑点0.1%

// 设置数据
QVector<QVariantMap> klineData = loadKLineData("600519");
backtest->setData("600519", klineData);

// 运行回测
backtest->run();

// 获取结果
auto stats = backtest->getStats();
qDebug() << "总收益率:" << stats.totalReturn * 100 << "%";
qDebug() << "最大回撤:" << stats.maxDrawdown * 100 << "%";
qDebug() << "夏普比率:" << stats.sharpeRatio;
```

### 风险分析

```cpp
#include "core/analysis/RiskAnalyzer.h"

auto* riskAnalyzer = RiskAnalyzer::instance();

// 设置持仓
QVector<RiskPositionInfo> positions;
positions.append({"600519", "贵州茅台", 100, 1000.0, 1100.0, 110000.0, 10000.0, 0.1});
riskAnalyzer->setPositions(positions);

// 计算风险指标
auto metrics = riskAnalyzer->calculateRiskMetrics();
qDebug() << "VaR(95%):" << metrics.var95 * 100 << "%";

// 检查风险预警
auto alerts = riskAnalyzer->checkRiskAlerts();
for (const auto& alert : alerts) {
    qDebug() << alert.message;
}
```

---

## 中期规划功能

### 策略分享

```cpp
#include "core/social/StrategyShareManager.h"

auto* shareMgr = StrategyShareManager::instance();

// 发布策略
SharedStrategy strategy;
strategy.name = "双均线策略";
strategy.description = "MA5上穿MA20买入";
strategy.strategyData = serializeStrategy();
shareMgr->publishStrategy(strategy);

// 订阅策略
shareMgr->subscribeStrategy("strategy_id", true, 0.5);  // 自动跟单，50%比例

// 评分策略
shareMgr->rateStrategy("strategy_id", 4.5, "策略表现不错");
```

### 画线工具

```cpp
#include "core/chart/DrawingToolManager.h"

auto* drawingMgr = ChartDrawingToolManager::instance();

// 设置工具
drawingMgr->setCurrentTool(DrawingToolType::TrendLine);

// 开始绘制
drawingMgr->startDrawing(QPointF(100, 100));
drawingMgr->updateDrawing(QPointF(200, 150));
auto drawing = drawingMgr->finishDrawing(QPointF(300, 200));

// 计算斐波那契回调
auto fibLevels = drawingMgr->calculateFibonacciLevels(100.0, 80.0);
for (const auto& level : fibLevels) {
    qDebug() << level.label << ":" << level.price;
}
```

### 量化交易引擎

```cpp
#include "core/quant/QuantTradingEngine.h"

auto* quantEngine = QuantTradingEngine::instance();

// 添加策略
auto strategy = std::make_shared<RSIStrategy>(14, 30, 70);
quantEngine->addStrategy(strategy, "rsi_strategy");

// 订阅标的
quantEngine->subscribeSymbols("rsi_strategy", {"600519", "000858"});

// 添加风控规则
QuantRiskRule rule;
rule.id = "loss_limit";
rule.type = "loss_limit";
rule.threshold = 50000;  // 最大亏损5万
rule.action = "stop";
quantEngine->addRiskRule(rule);

// 启动策略
quantEngine->startStrategy("rsi_strategy");

// 设置交易模式
quantEngine->setTradingMode(TradingMode::Simulation);  // 模拟盘
```

---

## 长期规划功能

### AI 智能助手

```cpp
#include "core/ai/AIAssistant.h"

auto* ai = AIAssistant::instance();

// 处理用户输入
auto response = ai->processInput("查询贵州茅台的行情");
qDebug() << response.text;

// 获取推荐
auto recommendations = ai->getPersonalizedRecommendations();
for (const auto& rec : recommendations) {
    qDebug() << rec.symbol << rec.name << rec.reason;
}
```

### 多账户管理

```cpp
#include "core/account/MultiAccountManager.h"

auto* accountMgr = MultiAccountManager::instance();

// 添加账户
MultiAccountInfo account;
account.name = "股票账户";
account.type = AccountType::Stock;
account.broker = "华泰证券";
account.accountNumber = "12345678";
accountMgr->addAccount(account);

// 切换账户
accountMgr->setCurrentAccount(account.id);

// 获取统计
auto stats = accountMgr->getStats();
qDebug() << "总资产:" << stats.totalAsset;
```

### 权限管理

```cpp
#include "core/security/PermissionManager.h"

auto* permMgr = PermissionManager::instance();

// 检查权限
if (permMgr->hasPermission(Permission::PlaceOrder)) {
    // 允许下单
}

// 添加用户
UserInfo user;
user.username = "trader1";
user.roleId = "trader";  // 交易员角色
permMgr->addUser(user);
```

### 数据 API

```cpp
#include "core/api/DataAPIManager.h"

auto* apiMgr = DataAPIManager::instance();

// 创建 API 密钥
auto apiKey = apiMgr->createKey("user1", "My API Key", 100);

// 处理 API 请求
QVariantMap params;
params["symbol"] = "600519";
auto response = apiMgr->handleRequest(
    apiKey.key,
    "/api/v1/market/quote",
    "GET",
    params
);

// 生成 API 文档
apiMgr->exportDocumentation("api_docs.md");
```

### 插件市场

```cpp
#include "core/plugin/PluginMarketManager.h"

auto* pluginMgr = PluginMarketManager::instance();

// 搜索插件
auto plugins = pluginMgr->searchPlugins("量化");

// 安装插件
pluginMgr->installPlugin(plugins[0].id);

// 启用插件
pluginMgr->enablePlugin(plugins[0].id);
```

---

## 性能监控

```cpp
#include "core/performance/PerformanceManager.h"

// 使用自动计时器
{
    PERF_TIMER(my_operation);
    // ... 执行操作
}  // 自动结束计时

// 手动计时
PERF_BEGIN(custom_operation);
// ... 执行操作
PERF_END(custom_operation);

// 生成报告
auto report = PerformanceManager::instance()->generateReport();
qDebug() << report;
```

---

## 完整示例

```cpp
#include "FeatureIntegration.h"
#include <QApplication>
#include <QMainWindow>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QMainWindow mainWindow;
    mainWindow.show();

    // 初始化所有功能
    WealthPilot::FeatureIntegration::instance()->initialize(&mainWindow);

    // 获取状态报告
    qDebug() << WealthPilot::FeatureIntegration::instance()->getStatusReport();

    return app.exec();
}
```

---

**文档版本**: 1.0.0  
**最后更新**: 2026-05-11
