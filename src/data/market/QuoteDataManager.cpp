#include "QuoteDataManager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QDateTime>
#include <QTimer>
#include <QDebug>
#include <iterator>

#include "shared/utils/Logger.h"

/**
 * @brief 构造函数
 * 初始化网络管理器和定时器，设置默认数据源
 */
QuoteDataManager::QuoteDataManager(QObject* parent)
    : QObject(parent)
      , m_networkManager(new QNetworkAccessManager(this))
      , m_updateTimer(new QTimer(this))
      , m_updateInterval(8000) // 默认8秒更新一次
      , m_source(Mock) // 默认使用模拟数据
{
    // 连接网络请求完成信号
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &QuoteDataManager::onNetworkReply);

    // 连接定时器信号
    connect(m_updateTimer, &QTimer::timeout, this, &QuoteDataManager::onUpdateTimer);
}

/**
 * @brief 请求实时行情（批量）
 * 构造API请求URL并发送网络请求
 * @param codes 股票代码列表，格式如 ["sh000001", "sz000001"]
 */
void QuoteDataManager::requestRealTimeQuotes(const QStringList& codes)
{
    if (codes.isEmpty()) return;

    m_currentCodes = codes;

    // 根据数据源构造不同格式的请求
    switch (m_source)
    {
    case Sina:
        {
            // 新浪行情API：http://hq.sinajs.cn/list=sh000001,sz000001
            const QString url = "http://hq.sinajs.cn/list=" + codes.join(",");
            QNetworkRequest request{QUrl (url)};
            request.setHeader(QNetworkRequest::UserAgentHeader,
                              "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
            m_networkManager->get(request);
            break;
        }
    case EastMoney:
        {
            // 东方财富API（需要构造特定格式）
            QString codesStr;
            for (const QString& code : codes)
            {
                if (code.startsWith("sh"))
                {
                    codesStr += code.mid(2) + ","; // 去掉sh前缀
                }
                else if (code.startsWith("sz"))
                {
                    codesStr += code.mid(2) + ",";
                }
                else
                {
                    codesStr += code + ",";
                }
            }
            codesStr.chop(1); // 去掉末尾逗号

            QString url = QString("http://push2.eastmoney.com/api/qt/stock/get?"
                    "secid=0.%1&fields=f43,f44,f45,f46,f47,f48,f57,f58,f60,f170")
                .arg(codesStr);
            m_networkManager->get(QNetworkRequest(QUrl(url)));
            break;
        }
    case Tushare:
        {
            // Tushare Pro需要Token认证，这里预留接口
            LOG_DEBUG("Tushare API需要配置Token");
            break;
        }
    case Mock:
        // 模拟数据模式，直接生成不发送网络请求
        generateMockData(codes.size());
        break;
    }
}

/**
 * @brief 请求市场列表数据
 * 获取某个市场的全部股票列表（如沪深300成分股）
 * @param market 市场标识符：sh-上证, sz-深证, hs300-沪深300等
 */
void QuoteDataManager::requestMarketList(const QString& market)
{
    // 实际生产环境中，这里应该调用API获取成分股列表
    // 这里使用模拟数据演示

    int count = 100;
    if (market == "hs300") count = 300;
    else if (market == "zz500") count = 500;

    generateMockData(count);
}

/**
 * @brief 请求历史数据
 * 用于复盘功能，获取某股票的历史行情序列
 * @param code 股票代码
 * @param start 开始时间
 * @param end 结束时间
 */
void QuoteDataManager::requestHistoryData(const QString& code,
                                          const QDateTime& start,
                                          const QDateTime& end)
{
    // 东方财富历史数据API示例
    QString url = QString("http://push2.eastmoney.com/api/qt/stock/kline/get?"
                      "secid=0.%1&fields1=f1,f2,f3,f4,f5,f6,f7,f8,f9,f10,f11,f12,f13&"
                      "fields2=f51,f52,f53,f54,f55,f56,f57,f58,f59,f60,f61&"
                      "klt=101&fqt=0&beg=%2&end=%3")
                  .arg(code)
                  .arg(start.toString("yyyyMMdd"))
                  .arg(end.toString("yyyyMMdd"));

    QNetworkReply* reply = m_networkManager->get(QNetworkRequest(QUrl(url)));
    reply->setProperty("isHistory", true); // 标记为历史数据请求
    reply->setProperty("code", code);
}

/**
 * @brief 生成模拟股票数据
 * 用于开发和演示，生成随机的股票行情数据
 * @param count 生成数量
 */
void QuoteDataManager::generateMockData(int count)
{
    m_itemMap.clear();
    m_currentCodes.clear();

    // 行业列表，用于随机分配
    QStringList industries = {
        "银行", "证券", "保险", "医药", "半导体", "新能源",
        "白酒", "房地产", "钢铁", "煤炭", "电力", "汽车"
    };

    // 股票名称前缀池
    QStringList namePrefixes = {
        "中国", "平安", "招商", "工商", "建设", "农业",
        "中信", "光大", "海通", "广发", "兴业", "浦发"
    };

    QRandomGenerator* rng = QRandomGenerator::global();

    for (int i = 0; i < count; ++i)
    {
        StockQuoteItem item;

        // 生成代码：6位数字，根据索引分配市场
        if (i < count / 3)
        {
            item.code = QString("6%1").arg(rng->bounded(100000, 999999));
            item.isFutures = false;
        }
        else if (i < count * 2 / 3)
        {
            item.code = QString("0%1").arg(rng->bounded(100000, 999999));
            item.isFutures = false;
        }
        else
        {
            // 生成期货代码
            QStringList futuresCodes = {
                "IF2503", "IC2506", "IH2509", "T2506",
                "TF2503", "TA2505", "MA2501", "rb2505"
            };
            item.code = futuresCodes[rng->bounded(futuresCodes.size())];
            item.isFutures = true;
        }

        // 生成名称
        if (item.isFutures)
        {
            item.name = item.code;
        }
        else
        {
            item.name = namePrefixes[rng->bounded(namePrefixes.size())] +
                industries[rng->bounded(industries.size())];
        }

        // 随机行业
        item.industry = industries[rng->bounded(industries.size())];

        // 生成价格数据
        item.preClose = rng->bounded(1000, 50000) / 100.0; // 10.00-500.00

        // 生成涨跌幅（正态分布模拟）
        double changePercent = (rng->bounded(-1000, 1000) / 100.0); // -10% 到 +10%
        if (rng->bounded(100) < 5) changePercent = 0; // 5%概率平盘

        item.changePercent = changePercent;
        item.price = item.preClose * (1 + changePercent / 100.0);
        item.change = item.price - item.preClose;

        // 生成市值（亿元）
        if (item.isFutures)
        {
            item.marketCap = rng->bounded(100, 2000); // 期货市值小
        }
        else
        {
            item.marketCap = rng->bounded(50, 20000); // 股票市值范围大
        }

        // 成交量和成交额
        item.volume = rng->bounded(10000, 10000000) / 10000.0; // 万手
        item.turnover = item.volume * item.price * 10000; // 万元

        // 期货特有数据
        if (item.isFutures)
        {
            item.openInterest = rng->bounded(10000, 500000);
            item.settlement = item.price * (1 + rng->bounded(-50, 50) / 10000.0);
        }

        // 保存到映射表
        m_itemMap[item.code] = item;
        m_currentCodes.append(item.code);
    }

    // 发送数据更新信号
    emit dataUpdated(getAllItems());
    qDebug() << "Generated" << count << "mock items";
}

/**
 * @brief 生成模拟期货数据
 * 专门生成期货合约的模拟数据
 */
void QuoteDataManager::generateMockFuturesData()
{
    m_itemMap.clear();
    m_currentCodes.clear();

    // 主要期货品种列表
    struct FuturesInfo
    {
        QString code;
        QString name;
        double basePrice;
        double volatility; // 波动率
    };

    QVector<FuturesInfo> futuresList = {
        {"IF2503", "沪深300股指", 3500.0, 1.5}, // 股指期货
        {"IC2503", "中证500股指", 5500.0, 1.8},
        {"IH2503", "上证50股指", 2400.0, 1.2},
        {"T2506", "10年期国债", 105.5, 0.3}, // 国债期货
        {"TF2506", "5年期国债", 104.2, 0.25},
        {"TA2505", "PTA化工", 4800.0, 2.0}, // 商品期货
        {"MA2505", "甲醇化工", 2200.0, 2.5},
        {"rb2505", "螺纹钢", 3200.0, 3.0},
        {"cu2505", "沪铜", 68000.0, 2.2},
        {"au2506", "沪金", 480.0, 1.0},
        {"ag2506", "沪银", 5800.0, 2.8},
        {"sc2505", "原油", 550.0, 3.5}
    };

    QRandomGenerator* rng = QRandomGenerator::global();

    for (const auto& info : futuresList)
    {
        StockQuoteItem item;
        item.code = info.code;
        item.name = info.name;
        item.industry = "期货";
        item.isFutures = true;

        // 根据波动率生成价格变化
        item.preClose = info.basePrice;
        double change = (rng->bounded(-100, 100) / 100.0) * info.volatility;
        item.changePercent = change;
        item.price = item.preClose * (1 + change / 100.0);
        item.change = item.price - item.preClose;

        // 期货市值通常较小（保证金交易）
        item.marketCap = rng->bounded(200, 1500);
        item.volume = rng->bounded(5000, 500000);
        item.turnover = item.volume * item.price * rng->bounded(5, 20); // 杠杆效应

        item.openInterest = rng->bounded(10000, 200000);
        item.settlement = item.price * (1 + rng->bounded(-30, 30) / 10000.0);

        m_itemMap[item.code] = item;
        m_currentCodes.append(item.code);
    }

    emit dataUpdated(getAllItems());
    qDebug() << "Generated" << futuresList.size() << "futures items";
}

/**
 * @brief 获取所有项目
 * 从映射表中提取所有数据项
 * @return 数据项列表
 */
QVector<StockQuoteItem> QuoteDataManager::getAllItems() const
{
    QVector<StockQuoteItem> items;
    items.reserve(m_itemMap.size());

    for (auto it = m_itemMap.begin(); it != m_itemMap.end(); ++it)
    {
        items.append(it.value());
    }

    return items;
}

/**
 * @brief 根据市场获取项目
 * 筛选特定市场的股票（上证/深证等）
 * @param market 市场标识符
 * @return 筛选后的数据项列表
 */
QVector<StockQuoteItem> QuoteDataManager::getItemsByMarket(const QString& market) const
{
    QVector<StockQuoteItem> items;

    for (auto it = m_itemMap.begin(); it != m_itemMap.end(); ++it)
    {
        const StockQuoteItem& item = it.value();

        bool include = false;
        if (market == "sh" && item.code.startsWith("6"))
        {
            include = true; // 上证主板
        }
        else if (market == "sz" && (item.code.startsWith("0") || item.code.startsWith("3")))
        {
            include = true; // 深证主板/中小板/创业板
        }
        else if (market == "cyb" && item.code.startsWith("3"))
        {
            include = true; // 创业板
        }
        else if (market == "kcb" && item.code.startsWith("68"))
        {
            include = true; // 科创板
        }
        else if (market == "futures" && item.isFutures)
        {
            include = true; // 期货
        }

        if (include)
        {
            items.append(item);
        }
    }

    return items;
}

/**
 * @brief 获取单个项目
 * 根据代码查询特定股票的详细信息
 * @param code 股票代码
 * @return 对应的StockQuoteItem，未找到返回默认构造
 */
StockQuoteItem QuoteDataManager::getItem(const QString& code) const
{
    auto it = m_itemMap.find(code);
    if (it != m_itemMap.end())
    {
        return it.value();
    }
    return StockQuoteItem(); // 返回空对象
}

/**
 * @brief 获取市场分类列表
 * 返回支持的市场筛选选项
 * @return 市场名称列表
 */
QStringList QuoteDataManager::getMarketCategories()
{
    return QStringList{
        "全部", "上证", "深证", "沪深300", "中证500",
        "创业板", "科创板", "期货"
    };
}

/**
 * @brief 获取期货分类列表
 * @return 期货类别名称列表
 */
QStringList QuoteDataManager::getFuturesCategories()
{
    return QStringList{"股指期货", "国债期货", "商品期货", "贵金属", "能源化工"};
}

/**
 * @brief 启动自动更新
 * 开启定时器，按设定间隔自动刷新行情
 * @param intervalMs 更新间隔（毫秒），默认8000（8秒）
 */
void QuoteDataManager::startAutoUpdate(int intervalMs)
{
    m_updateInterval = intervalMs;
    m_updateTimer->start(intervalMs);
    qDebug() << "Auto update started, interval:" << intervalMs << "ms";
}

/**
 * @brief 停止自动更新
 */
void QuoteDataManager::stopAutoUpdate()
{
    m_updateTimer->stop();
    qDebug() << "Auto update stopped";
}

/**
 * @brief 手动刷新
 * 立即执行一次数据更新
 */
void QuoteDataManager::refresh()
{
    onUpdateTimer();
}

/**
 * @brief 网络请求完成回调
 * 解析不同数据源的返回数据
 * @param reply 网络响应对象
 */
void QuoteDataManager::onNetworkReply(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        emit error(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    bool isHistory = reply->property("isHistory").toBool();
    QString code = reply->property("code").toString();

    if (isHistory)
    {
        // 解析历史数据（复盘功能）
        parseHistoryData(data, code);
    }
    else
    {
        // 解析实时数据
        switch (m_source)
        {
        case Sina:
            parseSinaQuote(QString::fromLocal8Bit(data));
            break;
        case EastMoney:
            parseEastMoneyQuote(data);
            break;
        default:
            break;
        }
    }

    reply->deleteLater();
}

/**
 * @brief 定时器回调
 * 自动更新触发时执行，模拟数据模式下模拟价格波动
 */
void QuoteDataManager::onUpdateTimer()
{
    if (m_source == Mock)
    {
        // 模拟模式下，随机更新部分数据的价格
        QRandomGenerator* rng = QRandomGenerator::global();

        // 更新约30%的数据，模拟真实市场变化
        int updateCount = qMax(1, m_itemMap.size() / 3);

        for (int i = 0; i < updateCount; ++i)
        {
            // 随机选择一个项目更新
            int index = rng->bounded(m_itemMap.size());
            auto it = std::next(m_itemMap.begin(), index);
            StockQuoteItem& item = it.value();

            // 小幅随机波动
            double change = (rng->bounded(-100, 100) / 10000.0); // ±1%
            item.price *= (1 + change);
            item.change = item.price - item.preClose;
            item.changePercent = (item.change / item.preClose) * 100.0;

            // 更新成交量
            item.volume += rng->bounded(100, 5000);
            item.turnover = item.volume * item.price;

            // 发送单个项目更新信号
            emit itemUpdated(item);
        }

        // 批量更新也发送一次，确保UI刷新
        emit dataUpdated(getAllItems());
    }
    else
    {
        // 真实数据源，重新请求
        requestRealTimeQuotes(m_currentCodes);
    }
}

/**
 * @brief 解析新浪行情数据
 * 新浪返回格式：var hq_str_sh600000="浦发银行,10.50,10.48..."
 * @param response 原始响应字符串
 */
void QuoteDataManager::parseSinaQuote(const QString& response)
{
    // 新浪数据格式示例：
    // var hq_str_sh600000="浦发银行,10.50,10.48,10.52,10.60,10.40,10000,20000..."
    // 字段顺序：名称,今日开盘价,昨日收盘价,当前价,今日最高价,今日最低价,
    //         竞买价,竞卖价,成交股数,成交金额,买一档量,买一档价...

    QStringList lines = response.split(";");

    for (const QString& line : lines)
    {
        if (line.trimmed().isEmpty()) continue;

        // 提取代码部分
        int varStart = line.indexOf("var hq_str_") + 11;
        int eqPos = line.indexOf("=");
        if (varStart < 0 || eqPos < 0) continue;

        QString code = line.mid(varStart, eqPos - varStart);

        // 提取引号内的数据
        int quoteStart = line.indexOf("\"", eqPos) + 1;
        int quoteEnd = line.lastIndexOf("\"");
        if (quoteStart < 1 || quoteEnd < quoteStart) continue;

        QString data = line.mid(quoteStart, quoteEnd - quoteStart);
        QStringList fields = data.split(",");

        if (fields.size() < 10) continue; // 数据不完整

        StockQuoteItem item;
        item.code = code;
        item.name = fields[0];
        item.preClose = fields[2].toDouble();
        item.price = fields[3].toDouble();
        item.change = item.price - item.preClose;
        item.changePercent = (item.change / item.preClose) * 100.0;
        item.volume = fields[8].toDouble() / 10000.0; // 转换为万手
        item.turnover = fields[9].toDouble() / 10000.0; // 转换为万元

        // 计算市值（这里需要外部数据源，暂时用模拟）
        item.marketCap = item.price * 10; // 简化计算

        m_itemMap[code] = item;
    }

    emit dataUpdated(getAllItems());
}

/**
 * @brief 解析东方财富数据
 * 东方财富返回JSON格式
 * @param data 原始JSON字节数组
 */
void QuoteDataManager::parseEastMoneyQuote(const QByteArray& data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject root = doc.object();
    QJsonObject dataObj = root.value("data").toObject();

    if (dataObj.isEmpty()) return;

    StockQuoteItem item;
    item.code = dataObj.value("f57").toString(); // 代码
    item.name = dataObj.value("f58").toString(); // 名称
    item.price = dataObj.value("f43").toDouble() / 100.0; // 当前价（分转元）
    item.preClose = dataObj.value("f60").toDouble() / 100.0; // 昨收
    item.changePercent = dataObj.value("f170").toDouble() / 100.0; // 涨跌幅%
    item.change = item.price - item.preClose;
    item.volume = dataObj.value("f47").toDouble() / 10000.0; // 成交量（手转万手）
    item.turnover = dataObj.value("f48").toDouble() / 10000.0; // 成交额

    // 市值（万元转亿元）
    item.marketCap = dataObj.value("f20").toDouble() / 10000.0;

    m_itemMap[item.code] = item;

    emit dataUpdated(getAllItems());
}

/**
 * @brief 解析历史数据
 * 用于复盘功能，解析K线序列数据
 * @param data 原始JSON数据
 * @param code 对应的股票代码
 */
void QuoteDataManager::parseHistoryData(const QByteArray& data, const QString& code)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return;

    const QJsonObject root = doc.object();
    const QJsonObject dataObj = root.value("data").toObject();
    QJsonArray klines = dataObj.value("klines").toArray();

    // 解析K线数据，存储到历史数据缓存
    // 格式：日期,开盘价,收盘价,最高价,最低价,成交量,成交额,振幅,涨跌幅,涨跌额,换手率
    QVector<StockQuoteItem> history;

    for (const QJsonValue& val : klines)
    {
        QString line = val.toString();
        QStringList fields = line.split(",");

        if (fields.size() < 9) continue;

        StockQuoteItem item;
        item.code = code;
        item.preClose = fields[1].toDouble(); // 开盘价作为昨收参考
        item.price = fields[2].toDouble(); // 收盘价作为当前价
        item.changePercent = fields[8].toDouble(); // 涨跌幅
        item.change = fields[9].toDouble(); // 涨跌额
        item.volume = fields[5].toDouble();
        item.turnover = fields[6].toDouble();

        history.append(item);
    }

    // 这里可以存储到历史数据缓存，供复盘使用
    qDebug() << "Parsed" << history.size() << "history items for" << code;
}
