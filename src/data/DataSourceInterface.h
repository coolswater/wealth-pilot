/**
 * @file DataSourceInterface.h
 * @brief 统一数据源接口 - 多数据源抽象层
 *
 * @details 提供统一的数据源接口，支持多种数据提供商（AkShare、Tushare、东方财富等）。
 * 参考 FinceptTerminal 的多数据源连接器设计。
 *
 * @example
 * // 获取数据源
 * auto& registry = DataSourceRegistry::instance();
 * auto akshare = registry.getSource("akshare");
 *
 * // 获取行情
 * auto result = akshare->getQuote("sh600000");
 * if (result.isOk()) {
 *     qDebug() << "Price:" << result.value().lastPrice;
 * }
 */

#ifndef WEALTHPILOT_DATA_SOURCE_INTERFACE_H
#define WEALTHPILOT_DATA_SOURCE_INTERFACE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QMap>
#include <memory>
#include "shared/base/Result.h"

namespace WealthPilot {

/**
 * @brief 行情数据
 */
struct Quote {
    QString symbol;           // 代码
    QString name;             // 名称
    double lastPrice = 0.0;   // 最新价
    double openPrice = 0.0;   // 开盘价
    double highPrice = 0.0;   // 最高价
    double lowPrice = 0.0;    // 最低价
    double preClose = 0.0;    // 昨收
    double change = 0.0;      // 涨跌额
    double changePercent = 0.0; // 涨跌幅
    double volume = 0.0;      // 成交量
    double amount = 0.0;      // 成交额
    double turnover = 0.0;    // 换手率
    double pe = 0.0;          // 市盈率
    double pb = 0.0;          // 市净率
    double marketCap = 0.0;   // 总市值
    QDateTime time;           // 时间
};

/**
 * @brief K线数据
 */
struct Bar {
    QDateTime time;           // 时间
    double open = 0.0;        // 开盘价
    double high = 0.0;        // 最高价
    double low = 0.0;         // 最低价
    double close = 0.0;       // 收盘价
    double volume = 0.0;      // 成交量
    double amount = 0.0;      // 成交额
};

/**
 * @brief 分时数据
 */
struct Tick {
    QDateTime time;           // 时间
    double price = 0.0;       // 价格
    double volume = 0.0;      // 成交量
    double amount = 0.0;      // 成交额
    int direction = 0;        // 方向 (1: 买入, -1: 卖出, 0: 中性)
};

/**
 * @brief 板块数据
 */
struct Sector {
    QString code;             // 板块代码
    QString name;             // 板块名称
    double changePercent = 0.0; // 涨跌幅
    int upCount = 0;          // 上涨家数
    int downCount = 0;        // 下跌家数
    double amount = 0.0;      // 成交额
    double marketCap = 0.0;   // 总市值
};

/**
 * @brief 新闻数据
 */
struct News {
    QString id;               // 新闻ID
    QString title;            // 标题
    QString content;          // 内容
    QString source;           // 来源
    QString category;         // 分类
    QDateTime time;           // 时间
    int importance = 0;       // 重要性 (1-3)
    QStringList relatedStocks; // 相关股票
};

/**
 * @brief 数据源能力标志
 */
struct DataSourceCapabilities {
    bool supportsStocks = false;      // 支持股票
    bool supportsFutures = false;     // 支持期货
    bool supportsForex = false;       // 支持外汇
    bool supportsCrypto = false;      // 支持加密货币
    bool supportsFund = false;        // 支持基金
    bool supportsRealtime = false;    // 支持实时数据
    bool supportsHistory = false;     // 支持历史数据
    bool supportsNews = false;        // 支持新闻
    bool supportsSectors = false;     // 支持板块
    bool supportsFinancials = false;  // 支持财务数据
};

/**
 * @brief 数据源接口 - 抽象基类
 */
class DataSourceInterface {
public:
    virtual ~DataSourceInterface() = default;

    // ========== 基本信息 ==========
    
    /**
     * @brief 获取数据源名称
     */
    virtual QString name() const = 0;
    
    /**
     * @brief 获取数据源ID
     */
    virtual QString id() const = 0;
    
    /**
     * @brief 获取数据源描述
     */
    virtual QString description() const = 0;
    
    /**
     * @brief 获取数据源能力
     */
    virtual DataSourceCapabilities capabilities() const = 0;

    // ========== 连接管理 ==========
    
    /**
     * @brief 初始化数据源
     */
    virtual Result<void> initialize() = 0;
    
    /**
     * @brief 关闭数据源
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief 检查是否已初始化
     */
    virtual bool isInitialized() const = 0;
    
    /**
     * @brief 检查连接状态
     */
    virtual bool isConnected() const = 0;

    // ========== 行情数据 ==========
    
    /**
     * @brief 获取单个股票行情
     */
    virtual Result<Quote> getQuote(const QString& symbol) = 0;
    
    /**
     * @brief 获取多个股票行情
     */
    virtual Result<QVector<Quote>> getQuotes(const QStringList& symbols) = 0;
    
    /**
     * @brief 获取实时行情（订阅）
     */
    virtual void subscribeQuotes(const QStringList& symbols) = 0;
    
    /**
     * @brief 取消订阅
     */
    virtual void unsubscribeQuotes(const QStringList& symbols) = 0;

    // ========== K线数据 ==========
    
    /**
     * @brief 获取K线数据
     * @param symbol 股票代码
     * @param period 周期 (1m, 5m, 15m, 30m, 60m, D, W, M)
     * @param start 开始时间
     * @param end 结束时间
     */
    virtual Result<QVector<Bar>> getBars(
        const QString& symbol,
        const QString& period,
        const QDateTime& start,
        const QDateTime& end) = 0;
    
    /**
     * @brief 获取最近N根K线
     */
    virtual Result<QVector<Bar>> getRecentBars(
        const QString& symbol,
        const QString& period,
        int count) = 0;

    // ========== 分时数据 ==========
    
    /**
     * @brief 获取分时数据
     */
    virtual Result<QVector<Tick>> getTicks(
        const QString& symbol,
        const QDateTime& start,
        const QDateTime& end) = 0;

    // ========== 板块数据 ==========
    
    /**
     * @brief 获取板块列表
     */
    virtual Result<QVector<Sector>> getSectors() = 0;
    
    /**
     * @brief 获取板块成分股
     */
    virtual Result<QStringList>> getSectorStocks(const QString& sectorCode) = 0;

    // ========== 新闻数据 ==========
    
    /**
     * @brief 获取新闻列表
     */
    virtual Result<QVector<News>> getNews(
        int count = 20,
        const QString& category = QString()) = 0;
    
    /**
     * @brief 获取股票相关新闻
     */
    virtual Result<QVector<News>> getStockNews(const QString& symbol, int count = 10) = 0;

    // ========== 排行数据 ==========
    
    /**
     * @brief 获取涨幅榜
     */
    virtual Result<QVector<Quote>> getTopGainers(int count = 10) = 0;
    
    /**
     * @brief 获取跌幅榜
     */
    virtual Result<QVector<Quote>> getTopLosers(int count = 10) = 0;
    
    /**
     * @brief 获取成交额榜
     */
    virtual Result<QVector<Quote>> getTopVolume(int count = 10) = 0;

signals:
    // 需要在实现类中声明
    // void quoteReceived(const Quote& quote);
    // void errorOccurred(const QString& error);
};

/**
 * @brief 数据源注册表 - 管理所有数据源
 */
class DataSourceRegistry : public QObject {
    Q_OBJECT

public:
    static DataSourceRegistry& instance() {
        static DataSourceRegistry instance;
        return instance;
    }

    /**
     * @brief 注册数据源
     */
    void registerSource(std::shared_ptr<DataSourceInterface> source) {
        if (source) {
            m_sources[source->id()] = source;
        }
    }

    /**
     * @brief 获取数据源
     */
    std::shared_ptr<DataSourceInterface> getSource(const QString& id) const {
        return m_sources.value(id);
    }

    /**
     * @brief 获取所有数据源
     */
    QList<std::shared_ptr<DataSourceInterface>> getAllSources() const {
        return m_sources.values();
    }

    /**
     * @brief 获取支持特定能力的数据源
     */
    QList<std::shared_ptr<DataSourceInterface>> getSourcesByCapability(
        const std::function<bool(const DataSourceCapabilities&)>& filter) const {
        QList<std::shared_ptr<DataSourceInterface>> result;
        for (const auto& source : m_sources) {
            if (filter(source->capabilities())) {
                result.append(source);
            }
        }
        return result;
    }

    /**
     * @brief 初始化所有数据源
     */
    Result<void> initializeAll() {
        for (auto& source : m_sources) {
            auto result = source->initialize();
            if (result.isError()) {
                return Result<void>::error(
                    QString("Failed to initialize %1: %2")
                        .arg(source->name(), result.errorMessage()));
            }
        }
        return Result<void>::ok();
    }

    /**
     * @brief 关闭所有数据源
     */
    void shutdownAll() {
        for (auto& source : m_sources) {
            source->shutdown();
        }
    }

    /**
     * @brief 获取默认数据源
     */
    std::shared_ptr<DataSourceInterface> getDefaultSource() const {
        return m_defaultSource;
    }

    /**
     * @brief 设置默认数据源
     */
    void setDefaultSource(const QString& id) {
        m_defaultSource = m_sources.value(id);
    }

private:
    DataSourceRegistry() = default;
    DataSourceRegistry(const DataSourceRegistry&) = delete;
    DataSourceRegistry& operator=(const DataSourceRegistry&) = delete;

    QMap<QString, std::shared_ptr<DataSourceInterface>> m_sources;
    std::shared_ptr<DataSourceInterface> m_defaultSource;
};

} // namespace WealthPilot

#endif // WEALTHPILOT_DATA_SOURCE_INTERFACE_H
