# WealthPilot API 文档

## 核心模块 API

### ServiceLocator - 依赖注入容器

#### 方法

##### registerSingleton
```cpp
template<typename TInterface, typename TImplementation>
void registerSingleton();
```

注册单例服务。服务在整个应用生命周期内只创建一次。

**示例**:
```cpp
ServiceLocator::instance().registerSingleton<IAIService, AIService>();
```

---

##### registerTransient
```cpp
template<typename TInterface, typename TImplementation>
void registerTransient();
```

注册瞬态服务。每次请求都创建新实例。

**示例**:
```cpp
ServiceLocator::instance().registerTransient<ICTPService, CTPService>();
```

---

##### resolve
```cpp
template<typename TInterface>
TInterface* resolve();
```

解析服务。如果服务未注册，抛出异常。

**示例**:
```cpp
IAIService* aiService = ServiceLocator::instance().resolve<IAIService>();
```

---

##### tryResolve
```cpp
template<typename TInterface>
TInterface* tryResolve();
```

尝试解析服务。如果服务未注册，返回nullptr。

**示例**:
```cpp
IAIService* aiService = ServiceLocator::instance().tryResolve<IAIService>();
if (aiService) {
    // 使用服务
}
```

---

### CacheManager - 缓存管理器

#### 方法

##### set
```cpp
void set(const QString& key, 
        const QVariant& value,
        int ttlSeconds = 300,
        CacheLevel level = CacheLevel::L1_Memory);
```

设置缓存。

**参数**:
- `key`: 缓存键
- `value`: 缓存值
- `ttlSeconds`: 过期时间（秒）
- `level`: 缓存级别（L1_Memory, L2_Disk, L3_Database）

**示例**:
```cpp
CacheManager::instance().set("market_data", marketData, 60, CacheLevel::L1_Memory);
```

---

##### get
```cpp
QVariant get(const QString& key, const QVariant& defaultValue = QVariant());
```

获取缓存。

**示例**:
```cpp
QVariant data = CacheManager::instance().get("market_data");
if (data.isValid()) {
    // 使用缓存数据
}
```

---

##### setBatch
```cpp
void setBatch(const QMap<QString, QVariant>& data, int ttlSeconds = 300);
```

批量设置缓存（性能优化）。

**示例**:
```cpp
QMap<QString, QVariant> data;
data["key1"] = "value1";
data["key2"] = "value2";
CacheManager::instance().setBatch(data, 60);
```

---

##### statistics
```cpp
CacheStats statistics() const;
```

获取缓存统计信息。

**示例**:
```cpp
CacheStats stats = CacheManager::instance().statistics();
qDebug() << "Hit rate:" << stats.hitRate;
qDebug() << "Memory usage:" << stats.memoryUsage / 1024 / 1024 << "MB";
```

---

### EnvironmentConfig - 环境配置管理

#### 方法

##### currentSettings
```cpp
const EnvironmentSettings& currentSettings() const;
```

获取当前环境配置。

**示例**:
```cpp
auto settings = EnvironmentConfig::instance().currentSettings();
QString apiUrl = settings.apiUrl;
int timeout = settings.requestTimeout;
```

---

##### setCurrentEnvironment
```cpp
void setCurrentEnvironment(Environment env);
```

切换环境。

**示例**:
```cpp
EnvironmentConfig::instance().setCurrentEnvironment(Environment::Production);
```

---

### PluginLoader - 插件加载器

#### 方法

##### loadPlugin
```cpp
bool loadPlugin(const QString& pluginName);
```

加载指定插件。

**示例**:
```cpp
PluginLoader::instance().loadPlugin("CTPPlugin");
```

---

##### getPlugin
```cpp
template<typename T>
T* getPlugin(const QString& pluginName);
```

获取插件实例。

**示例**:
```cpp
ICTPPlugin* ctpPlugin = PluginLoader::instance().getPlugin<ICTPPlugin>("CTPPlugin");
```

---

##### reloadPlugin
```cpp
bool reloadPlugin(const QString& pluginName);
```

热重载插件。

**示例**:
```cpp
PluginLoader::instance().reloadPlugin("CTPPlugin");
```

---

## 插件接口 API

### ICTPPlugin - CTP插件接口

#### 方法

##### connect
```cpp
bool connect(const QString& brokerId, 
            const QString& userId, 
            const QString& password,
            const QString& marketFront,
            const QString& tradeFront);
```

连接到CTP服务器。

**示例**:
```cpp
ctpPlugin->connect("9999", "user001", "password", 
                  "tcp://180.168.146.187:10131",
                  "tcp://180.168.146.187:10130");
```

---

##### subscribeMarketData
```cpp
bool subscribeMarketData(const QStringList& instruments);
```

订阅行情。

**示例**:
```cpp
ctpPlugin->subscribeMarketData({"cu2505", "ag2506", "au2506"});
```

---

##### sendOrder
```cpp
QString sendOrder(const QString& instrumentId,
                 const QString& direction,
                 const QString& offsetFlag,
                 double price,
                 int volume);
```

下单。

**示例**:
```cpp
QString orderId = ctpPlugin->sendOrder("cu2505", "buy", "open", 75000.0, 1);
```

---

### IAIPlugin - AI插件接口

#### 方法

##### sendMessage
```cpp
QString sendMessage(const QString& message, 
                   const QJsonObject& context = QJsonObject());
```

发送消息。

**示例**:
```cpp
QString response = aiPlugin->sendMessage("分析一下铜期货的走势", context);
```

---

##### analyzeMarket
```cpp
AIAnalysisResult analyzeMarket(const QString& instrumentId,
                              const QMap<QString, double>& data);
```

分析市场行情。

**示例**:
```cpp
QMap<QString, double> data;
data["close"] = 75000.0;
data["volume"] = 100000;
data["openInterest"] = 50000;

AIAnalysisResult result = aiPlugin->analyzeMarket("cu2505", data);
qDebug() << result.summary;
qDebug() << "Confidence:" << result.confidence;
```

---

##### predictPrice
```cpp
MarketPrediction predictPrice(const QString& instrumentId,
                             const QMap<QString, double>& historicalData);
```

预测价格。

**示例**:
```cpp
MarketPrediction prediction = aiPlugin->predictPrice("cu2505", historicalData);
qDebug() << "Predicted price:" << prediction.predictedPrice;
qDebug() << "Trend:" << prediction.trend;
```

---

## 技术指标 API

### TechnicalIndicators - 技术指标计算

#### 方法

##### SMA
```cpp
static QVector<double> SMA(const QVector<double>& data, int period);
```

简单移动平均。

**示例**:
```cpp
QVector<double> prices = {100, 101, 102, 103, 104, 105};
QVector<double> sma = TechnicalIndicators::SMA(prices, 5);
```

---

##### EMA
```cpp
static QVector<double> EMA(const QVector<double>& data, int period);
```

指数移动平均。

**示例**:
```cpp
QVector<double> ema = TechnicalIndicators::EMA(prices, 20);
```

---

##### MACD
```cpp
static IndicatorResult MACD(const QVector<double>& data, 
                           int fastPeriod = 12, 
                           int slowPeriod = 26, 
                           int signalPeriod = 9);
```

MACD指标。

**示例**:
```cpp
IndicatorResult macd = TechnicalIndicators::MACD(prices, 12, 26, 9);
QVector<double> dif = macd.values["DIF"];
QVector<double> dea = macd.values["DEA"];
QVector<double> macdBar = macd.values["MACD"];
```

---

##### RSI
```cpp
static QVector<double> RSI(const QVector<double>& data, int period = 14);
```

RSI相对强弱指标。

**示例**:
```cpp
QVector<double> rsi = TechnicalIndicators::RSI(prices, 14);
```

---

##### KDJ
```cpp
static IndicatorResult KDJ(const QVector<double>& high,
                          const QVector<double>& low,
                          const QVector<double>& close,
                          int n = 9, int m1 = 3, int m2 = 3);
```

KDJ随机指标。

**示例**:
```cpp
IndicatorResult kdj = TechnicalIndicators::KDJ(high, low, close, 9, 3, 3);
QVector<double> k = kdj.values["K"];
QVector<double> d = kdj.values["D"];
QVector<double> j = kdj.values["J"];
```

---

##### BollingerBands
```cpp
static IndicatorResult BollingerBands(const QVector<double>& data,
                                     int period = 20,
                                     double stdDev = 2.0);
```

布林带。

**示例**:
```cpp
IndicatorResult bb = TechnicalIndicators::BollingerBands(prices, 20, 2.0);
QVector<double> upper = bb.values["Upper"];
QVector<double> middle = bb.values["Middle"];
QVector<double> lower = bb.values["Lower"];
```

---

## 异步任务 API

### AsyncTaskManager - 异步任务管理器

#### 方法

##### submitTask
```cpp
template<typename T>
QString submitTask(const QString& taskId,
                  typename AsyncTask<T>::TaskFunction func,
                  TaskPriority priority = TaskPriority::Normal,
                  typename AsyncTask<T>::CallbackFunction callback = nullptr);
```

提交异步任务。

**示例**:
```cpp
QString taskId = AsyncTaskManager::instance().submitTask<double>(
    "calculate_indicator",
    []() {
        // 执行计算
        return calculateMACD();
    },
    TaskPriority::High,
    [](const TaskResult<double>& result) {
        if (result.success) {
            qDebug() << "Result:" << result.value;
        }
    }
);
```

---

##### submitTaskWithTimeout
```cpp
template<typename T>
QString submitTaskWithTimeout(const QString& taskId,
                             typename AsyncTask<T>::TaskFunction func,
                             int timeoutMs,
                             TaskPriority priority = TaskPriority::Normal,
                             typename AsyncTask<T>::CallbackFunction callback = nullptr);
```

提交带超时的异步任务。

**示例**:
```cpp
AsyncTaskManager::instance().submitTaskWithTimeout<QString>(
    "fetch_data",
    []() { return fetchData(); },
    5000,  // 5秒超时
    TaskPriority::Normal,
    [](const TaskResult<QString>& result) {
        if (result.state == TaskState::Timeout) {
            qDebug() << "Task timeout";
        }
    }
);
```

---

## 主题引擎 API

### ThemeEngine - 主题引擎

#### 方法

##### setCurrentTheme
```cpp
void setCurrentTheme(const QString& themeName);
```

设置当前主题。

**示例**:
```cpp
ThemeEngine::instance().setCurrentTheme("dark");  // 深色主题
ThemeEngine::instance().setCurrentTheme("light"); // 浅色主题
ThemeEngine::instance().setCurrentTheme("eyecare"); // 护眼主题
```

---

##### compiledStyleSheet
```cpp
QString compiledStyleSheet() const;
```

获取预编译样式表。

**示例**:
```cpp
QString styleSheet = ThemeEngine::instance().compiledStyleSheet();
widget->setStyleSheet(styleSheet);
```

---

##### themeConfig
```cpp
const UIComponents::ThemeConfig& themeConfig() const;
```

获取当前主题配置。

**示例**:
```cpp
auto config = ThemeEngine::instance().themeConfig();
QColor primaryColor = config.primaryColor;
QColor bgColor = config.backgroundColor;
```

---

## 完整使用示例

### 应用初始化

```cpp
#include "core/ApplicationInitializer.h"
#include "core/ServiceLocator.h"
#include "plugins/PluginLoader.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 初始化应用
    if (!ApplicationInitializer::instance().initialize()) {
        return -1;
    }
    
    // 创建主窗口
    MainWindow window;
    window.show();
    
    return app.exec();
}
```

### 使用CTP服务

```cpp
// 获取CTP插件
auto ctpPlugin = PluginLoader::instance().getPlugin<ICTPPlugin>("CTPPlugin");

// 连接
ctpPlugin->connect("9999", "user001", "password", 
                  "tcp://180.168.146.187:10131",
                  "tcp://180.168.146.187:10130");

// 订阅行情
ctpPlugin->subscribeMarketData({"cu2505", "ag2506"});

// 获取行情
MarketData data = ctpPlugin->getMarketData("cu2505");
qDebug() << "Last price:" << data.lastPrice;

// 下单
QString orderId = ctpPlugin->sendOrder("cu2505", "buy", "open", 75000.0, 1);
```

### 使用AI服务

```cpp
// 获取AI插件
auto aiPlugin = ServiceLocator::instance().resolve<IAIPlugin>();

// 发送消息
QString response = aiPlugin->sendMessage("分析一下铜期货的走势");

// 分析市场
QMap<QString, double> data;
data["close"] = 75000.0;
data["volume"] = 100000;

AIAnalysisResult result = aiPlugin->analyzeMarket("cu2505", data);
qDebug() << result.summary;
qDebug() << "Confidence:" << result.confidence;

// 预测价格
MarketPrediction prediction = aiPlugin->predictPrice("cu2505", historicalData);
qDebug() << "Predicted:" << prediction.predictedPrice;
```

### 使用缓存

```cpp
// 设置缓存
CacheManager::instance().set("market_data", marketData, 60, CacheLevel::L1_Memory);

// 获取缓存
QVariant data = CacheManager::instance().get("market_data");

// 批量操作
QMap<QString, QVariant> batchData;
batchData["key1"] = "value1";
batchData["key2"] = "value2";
CacheManager::instance().setBatch(batchData, 300);

// 查看统计
CacheStats stats = CacheManager::instance().statistics();
qDebug() << "Hit rate:" << stats.hitRate * 100 << "%";
```

### 使用技术指标

```cpp
// 准备数据
QVector<double> prices = getHistoricalPrices();

// 计算MA
QVector<double> ma20 = TechnicalIndicators::SMA(prices, 20);
QVector<double> ma60 = TechnicalIndicators::SMA(prices, 60);

// 计算MACD
IndicatorResult macd = TechnicalIndicators::MACD(prices, 12, 26, 9);

// 计算RSI
QVector<double> rsi = TechnicalIndicators::RSI(prices, 14);

// 计算KDJ
IndicatorResult kdj = TechnicalIndicators::KDJ(high, low, close, 9, 3, 3);

// 计算布林带
IndicatorResult bb = TechnicalIndicators::BollingerBands(prices, 20, 2.0);
```

---

## 版本信息

- **版本**: 2.0.0
- **日期**: 2026-04-14
- **作者**: WealthPilot Team
