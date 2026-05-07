# 快速测试指南 - K线数据源

## 🎯 测试目标

验证K线页面是否能正确获取和显示真实行情数据。

## 📋 测试步骤

### 1. 编译运行程序

```bash
# 编译
cd D:\C++\wealth-pilot\cmake-build-debug
cmake --build . --target WealthPilot

# 运行
./WealthPilot.exe
```

### 2. 测试K线页面

#### 方法A: 通过界面测试

1. 启动WealthPilot程序
2. 导航到"股票K线"页面
3. 在搜索框输入股票代码，例如：
   - `sh600000` (浦发银行)
   - `sh600519` (贵州茅台)
   - `sz000001` (平安银行)
4. 观察K线图是否正确显示

#### 方法B: 通过代码测试

在 `MainWindow.cpp` 的初始化代码中添加：

```cpp
// 自动加载测试股票
auto* klinePage = findChild<StockKLinePage*>("stock-kline");
if (klinePage) {
    klinePage->setStock("sh600000", "浦发银行");
}
```

### 3. 检查日志输出

程序运行时会在日志中输出以下信息：

**成功加载的日志：**
```
[INFO] Loading KLine data with fallback for sh600000
[INFO] Requesting KLine: sh600000, period: 0
[DEBUG] Parsed 500 KLines for sh600000
[INFO] KLine data received: 500 items for sh600000
[INFO] 已加载 500 条K线数据
```

**失败情况的日志：**
```
[ERROR] 数据源错误: Network error
[INFO] 未获取到数据
```

### 4. 验证数据显示

检查以下内容：

✅ **K线图显示**
- K线蜡烛图是否正确绘制
- 红涨绿跌的颜色是否正确
- 时间轴是否显示日期

✅ **指标显示**
- MA均线是否正确计算和显示
- MACD指标是否正确显示
- 成交量柱状图是否正确

✅ **交互功能**
- 鼠标移动时十字光标是否跟随
- 信息面板是否显示当前K线详情
- 缩放和滚动功能是否正常

## 🔍 问题排查

### 问题1: 无法获取数据

**可能原因：**
- 网络连接问题
- API服务器不可用
- 股票代码格式错误

**解决方法：**
```cpp
// 检查网络连接
QNetworkConfigurationManager manager;
if (!manager.isOnline()) {
    qDebug() << "网络未连接";
}

// 检查股票代码格式
// 正确格式: sh600000 或 sz000001
// 错误格式: 600000 (缺少市场前缀)
```

### 问题2: 数据格式错误

**症状：**
- K线数据为空
- 价格数据异常（如0或负数）

**解决方法：**
检查 `StockDataSource::parseSinaKLine` 的解析逻辑：
```cpp
// 添加数据验证
if (kline.close <= 0 || kline.volume < 0) {
    LOG_WARN("Invalid KLine data detected");
    continue;
}
```

### 问题3: 数据更新慢

**原因：**
- 新浪API有频率限制
- 缓存机制导致延迟

**解决方法：**
```cpp
// 清除缓存，强制刷新
CacheManager::instance()->remove(cacheKey());
loadFromNetwork();
```

## 📊 性能测试

### 测试数据加载速度

```cpp
// 在StockKLinePage::onKLineReceived中添加计时
QElapsedTimer timer;
timer.start();

// 数据处理...
m_klineChart->setData(data);

qDebug() << "数据处理耗时:" << timer.elapsed() << "ms";
```

**预期性能：**
- 网络请求: 500-2000ms
- 数据解析: 10-50ms
- 图表渲染: 50-200ms
- 总耗时: <3000ms

### 测试内存占用

```cpp
// 监控内存使用
qDebug() << "K线数据大小:" << data.size() << "条";
qDebug() << "内存占用:" << sizeof(KLineData) * data.size() << "bytes";
```

**预期内存：**
- 500条K线: ~40KB
- 1000条K线: ~80KB

## 🧪 单元测试

创建单元测试验证数据解析：

```cpp
// tests/StockDataSourceTest.cpp

void testParseSinaKLine()
{
    // 模拟新浪API返回数据
    QByteArray testData = "2024-01-01,10.50,10.60,10.40,10.55,1000000\n"
                         "2024-01-02,10.55,10.70,10.50,10.65,1200000\n";

    StockDataSource source;
    // 调用parseSinaKLine（需要修改为public或添加测试接口）

    // 验证解析结果
    QCOMPARE(klines.size(), 2);
    QCOMPARE(klines[0].open, 10.50);
    QCOMPARE(klines[0].close, 10.55);
}
```

## 📝 测试报告模板

```markdown
# K线数据源测试报告

## 测试环境
- 操作系统: Windows 10/11
- Qt版本: 6.10.2
- 编译器: MinGW 13.1.0
- 测试时间: YYYY-MM-DD HH:MM

## 测试结果

### 功能测试
| 测试项 | 结果 | 备注 |
|--------|------|------|
| 实时行情获取 | ✅/❌ | |
| K线数据获取 | ✅/❌ | |
| 数据解析正确 | ✅/❌ | |
| 图表显示正确 | ✅/❌ | |
| 指标计算正确 | ✅/❌ | |

### 性能测试
| 测试项 | 实际值 | 预期值 | 结果 |
|--------|--------|--------|------|
| 网络请求耗时 | XXms | <2000ms | ✅/❌ |
| 数据解析耗时 | XXms | <50ms | ✅/❌ |
| 图表渲染耗时 | XXms | <200ms | ✅/❌ |
| 内存占用 | XXKB | <100KB | ✅/❌ |

### 问题记录
1. [问题描述]
   - 影响: [严重程度]
   - 原因: [分析结果]
   - 解决: [解决方案]

## 结论
[总体评价和建议]
```

## 🚀 下一步测试

1. **多股票测试**: 测试同时加载多个股票的数据
2. **长时间运行**: 测试程序长时间运行的稳定性
3. **异常情况**: 测试网络断开、数据异常等情况
4. **并发测试**: 测试多个页面同时请求数据
5. **边界测试**: 测试极端数据（如停牌股票、新股等）

## 📞 联系支持

如果遇到问题，请查看：
- 日志文件: `logs/WealthPilot.log`
- 文档: `docs/data-source-integration.md`
- API文档: 新浪财经API官方文档