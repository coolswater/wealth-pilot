# WealthPilot API 文档

## 核心模块 API

### ServiceLocator (依赖注入)

#### 注册单例服务

```cpp
template<typename TInterface, typename TImplementation>
void registerSingleton();
```

**示例:**

```cpp
ServiceLocator::instance().registerSingleton<IDataService, DataServiceImpl>();
```

#### 注册瞬态服务

```cpp
template<typename TInterface, typename TImplementation>
void registerTransient();
```

#### 解析服务

```cpp
template<typename T>
T* resolve();
```

**示例:**

```cpp
auto dataService = ServiceLocator::instance().resolve<IDataService>();
```

#### 检查是否已注册

```cpp
template<typename T>
bool isRegistered();
```

---

### DataHub (数据中心)

#### 订阅数据

```cpp
QMetaObject::Connection subscribe(
    QObject* owner,
    const QString& topic,
    std::function<void(const QVariant&)> slot
);
```

**参数:**

- `owner`: 订阅者对象，用于自动清理
- `topic`: 主题名称，支持通配符 (如 `market.*`)
- `slot`: 数据处理回调函数

**示例:**

```cpp
DataHub::instance().subscribe(this, "market.AAPL", [](const QVariant& data) {
    auto quote = data.value<QuoteData>();
    // 处理行情数据
});
```

#### 发布数据

```cpp
void publish(const QString& topic, const QVariant& data);
```

**示例:**

```cpp
DataHub::instance().publish("market.AAPL", QVariant::fromValue(quoteData));
```

#### 取消订阅

```cpp
void unsubscribe(QObject* owner);
void unsubscribe(QObject* owner, const QString& topic);
```

---

### CacheManager (缓存管理)

#### 初始化

```cpp
bool initialize(
    qint64 maxMemorySize = 10 * 1024 * 1024,  // 默认 10MB
    qint64 maxDiskSize = 100 * 1024 * 1024    // 默认 100MB
);
```

#### 设置缓存

```cpp
void set(const QString& key, const QByteArray& data, qint64 ttlMs = 0);
```

**参数:**

- `key`: 缓存键
- `data`: 缓存数据
- `ttlMs`: 过期时间（毫秒），0 表示永不过期

**示例:**

```cpp
CacheManager::instance()->set("quote:AAPL", quoteData, 60000); // 60秒过期
```

#### 获取缓存

```cpp
template<typename T>
std::optional<T> get(const QString& key);
```

**示例:**

```cpp
auto data = CacheManager::instance()->get<QByteArray>("quote:AAPL");
if (data) {
    // 使用缓存数据
}
```

#### 清除缓存

```cpp
void remove(const QString& key);
void clearAll();
```

---

### AsyncTaskManager (异步任务)

#### 执行异步任务

```cpp
template<typename Func>
AsyncTask<Func> run(Func task);
```

**示例:**

```cpp
AsyncTaskManager::instance().run([]() {
    // 后台任务
    return fetchData();
}).then([](const Data& result) {
    // 主线程回调
    updateUI(result);
});
```

---

### Logger (日志系统)

#### 日志宏

```cpp
LOG_DEBUG(message);
LOG_INFO(message);
LOG_WARNING(message);
LOG_ERROR(message);
```

**示例:**

```cpp
LOG_INFO(QString("User logged in: %1").arg(username));
LOG_ERROR("Connection failed");
```

---

## 插件 API

### IPlugin (插件接口)

#### 元数据

```cpp
virtual PluginMetaData metaData() const = 0;
```

**PluginMetaData 结构:**

```cpp
struct PluginMetaData {
    QString name;           // 插件名称
    QString version;        // 版本号
    QString description;    // 描述
    QString author;         // 作者
    QString license;        // 许可证
    QString website;        // 网站
    QStringList dependencies; // 依赖
    int priority;           // 优先级
    bool enableHotReload;   // 支持热重载
};
```

#### 状态管理

```cpp
virtual PluginState state() const = 0;
```

**PluginState 枚举:**

```cpp
enum class PluginState {
    Unloaded,   // 未加载
    Loading,    // 加载中
    Loaded,     // 已加载
    Initializing, // 初始化中
    Initialized, // 已初始化
    Starting,   // 启动中
    Running,    // 运行中
    Stopping,   // 停止中
    Stopped,    // 已停止
    Unloading,  // 卸载中
    Error       // 错误
};
```

#### 生命周期方法

```cpp
virtual bool load() = 0;
virtual bool initialize(const QJsonObject& config) = 0;
virtual bool start() = 0;
virtual void stop() = 0;
virtual void unload() = 0;
```

---

## 配置 API

### DataSourceConfig

#### 加载配置

```cpp
bool loadFromFile(const QString& filePath);
```

#### 获取数据源

```cpp
QVector<DataSourceConfigItem> getDataSources(const QString& type) const;
DataSourceConfigItem getBestDataSource(const QString& type) const;
```

---

### PluginConfigLoader

#### 加载 CTP 配置

```cpp
bool loadCTPConfig(const QString& filePath);
CTPConfig getCTPConfig() const;
```

#### 加载 AI 配置

```cpp
bool loadAIConfig(const QString& filePath);
AIConfig getAIConfig() const;
```

---

## UI API

### ThemeManager

#### 设置主题

```cpp
void setTheme(ThemeType theme);
```

**ThemeType 枚举:**

```cpp
enum class ThemeType {
    Light,  // 浅色主题
    Dark    // 深色主题
};
```

#### 获取当前主题

```cpp
ThemeType currentTheme() const;
```

#### 主题变化信号

```cpp
signals:
    void themeChanged(ThemeType newTheme);
```

---

### PageNavigation

#### 导航到页面

```cpp
void navigateTo(const QString& pageId);
```

#### 注册页面

```cpp
void registerPage(const QString& id, QWidget* page);
```

---

## 数据模型

### QuoteData (行情数据)

```cpp
struct QuoteData {
    QString symbol;         // 代码
    QString name;           // 名称
    double price;           // 当前价
    double open;            // 开盘价
    double high;            // 最高价
    double low;             // 最低价
    double prevClose;       // 昨收价
    double volume;          // 成交量
    double turnover;        // 成交额
    double change;          // 涨跌额
    double changePercent;   // 涨跌幅
    QDateTime updateTime;   // 更新时间
};
```

### KLineData (K线数据)

```cpp
struct KLineData {
    QDateTime time;         // 时间
    double open;            // 开盘价
    double high;            // 最高价
    double low;             // 最低价
    double close;           // 收盘价
    double volume;          // 成交量
    double turnover;        // 成交额
};
```

---

*API 文档最后更新: 2026-05-15*