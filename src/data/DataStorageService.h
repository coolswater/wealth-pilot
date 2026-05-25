/**
 * @file DataStorageService.h
 * @brief 数据存储服务 - 管理本地数据存储
 *
 * @details 功能:
 * - 指数数据存储与读取
 * - 自选股数据管理(增删改查)
 * - 新闻数据存储与读取
 * - K线数据存储与读取
 * - 分时数据存储与读取
 * - 离线数据支持
 */

#ifndef DATASTORAGESERVICE_H
#define DATASTORAGESERVICE_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QDateTime>
#include <memory>
#include "infrastructure/database/DatabaseManager.h"
#include "shared/types/NewsTypes.h"     // NewsItem 定义
#include "shared/types/MarketTypes.h"   // KLineData 定义

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::NewsItem;
using WealthPilot::KLineData;
using WealthPilot::TimeShareData;
using WealthPilot::StockQuote;
using WealthPilot::MarketSnapshot;

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 指数历史数据
 */
struct IndexHistoryData {
    QString code;               ///< 指数代码
    QString name;               ///< 指数名称
    double closePrice = 0.0;    ///< 收盘价
    double changePercent = 0.0; ///< 涨跌幅
    double volume = 0.0;        ///< 成交量
    double amount = 0.0;        ///< 成交额
    QDate date;                 ///< 日期
};

/**
 * @brief 自选股数据
 */
struct WatchlistItem {
    QString symbol;             ///< 股票代码(如 sh600000)
    QString name;               ///< 股票名称
    int sort_order = 0;         ///< 排序
    QString group_name;         ///< 分组名称
    QDateTime created_at;       ///< 添加时间
    QDateTime updated_at;       ///< 更新时间
};

/**
 * @brief 缓存的行情数据
 */
struct CachedQuoteData {
    QString symbol;             ///< 股票代码
    QString name;               ///< 股票名称
    double lastPrice = 0.0;     ///< 最新价
    double changePercent = 0.0; ///< 涨跌幅
    double changeAmount = 0.0;  ///< 涨跌额
    double volume = 0.0;        ///< 成交量
    double amount = 0.0;        ///< 成交额
    QDateTime update_time;      ///< 更新时间
};

/**
 * @brief 数据存储服务
 */
class DataStorageService : public QObject
{
    Q_OBJECT

public:
    static DataStorageService* instance();

    /**
     * @brief 初始化服务
     */
    bool initialize(const QString& dbPath = QString());

    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

    // ========== 指数数据 ==========

    /**
     * @brief 保存指数数据(收盘后调用)
     */
    bool saveIndexData(const QString& code, const QString& name,
                       double closePrice, double changePercent,
                       double volume, double amount, const QDate& date);

    /**
     * @brief 批量保存指数数据
     */
    bool saveIndexDataBatch(const QVector<IndexHistoryData>& data);

    /**
     * @brief 获取最新的指数数据
     */
    QVector<IndexHistoryData> getLatestIndexData();

    /**
     * @brief 获取指定日期的指数数据
     */
    QVector<IndexHistoryData> getIndexDataByDate(const QDate& date);

    /**
     * @brief 获取指数历史数据
     */
    QVector<IndexHistoryData> getIndexHistory(const QString& code, int days = 30);

    // ========== 自选股数据 ==========

    /**
     * @brief 添加自选股
     */
    bool addWatchlistItem(const QString& symbol, const QString& name,
                          const QString& groupName = QString());

    /**
     * @brief 删除自选股
     */
    bool removeWatchlistItem(const QString& symbol);

    /**
     * @brief 更新自选股
     */
    bool updateWatchlistItem(const WatchlistItem& item);

    /**
     * @brief 获取所有自选股
     */
    QVector<WatchlistItem> getAllWatchlistItems();

    /**
     * @brief 检查是否在自选股中
     */
    bool isInWatchlist(const QString& symbol);

    /**
     * @brief 获取自选股代码列表
     */
    QStringList getWatchlistSymbols();

    /**
     * @brief 更新自选股排序
     */
    bool updateWatchlistOrder(const QString& symbol, int order);

    // ========== 新闻数据 ==========

    /**
     * @brief 保存新闻
     */
    bool saveNews(const NewsItem& news);

    /**
     * @brief 批量保存新闻
     */
    bool saveNewsBatch(const QVector<NewsItem>& newsList);

    /**
     * @brief 获取最新新闻
     */
    QVector<NewsItem> getLatestNews(int count = 50);

    /**
     * @brief 检查新闻是否存在
     */
    bool newsExists(const QString& id);

    /**
     * @brief 清理过期新闻
     */
    void cleanOldNews(int daysToKeep = 30);

    // ========== 行情缓存 ==========

    /**
     * @brief 保存行情缓存
     */
    bool saveQuoteCache(const QString& symbol, const CachedQuoteData& data);

    /**
     * @brief 批量保存行情缓存
     */
    bool saveQuoteCacheBatch(const QVector<CachedQuoteData>& dataList);

    /**
     * @brief 获取行情缓存
     */
    CachedQuoteData getQuoteCache(const QString& symbol);

    /**
     * @brief 获取所有行情缓存
     */
    QVector<CachedQuoteData> getAllQuoteCache();

    /**
     * @brief 清理行情缓存
     */
    void clearQuoteCache();

    // ========== K线数据 ==========

    /**
     * @brief 保存K线数据
     * @param symbol 股票代码
     * @param period K线周期
     * @param data K线数据
     */
    bool saveKLineData(const QString& symbol, int period, const QVector<KLineData>& data);

    /**
     * @brief 获取K线数据
     * @param symbol 股票代码
     * @param period K线周期
     * @param count 数据条数（0表示全部）
     */
    QVector<KLineData> getKLineData(const QString& symbol, int period, int count = 0);

    /**
     * @brief 获取K线数据条数
     */
    int getKLineDataCount(const QString& symbol, int period);

    /**
     * @brief 清理K线数据
     * @param symbol 股票代码（空表示清理全部）
     * @param daysToKeep 保留天数
     */
    void cleanKLineData(const QString& symbol = QString(), int daysToKeep = 365);

    // ========== 分时数据 ==========

    /**
     * @brief 分时数据点
     */
    struct TimeSharePoint {
        QDateTime time;     ///< 时间
        double price = 0.0; ///< 价格
        qint64 volume = 0;  ///< 成交量
    };

    /**
     * @brief 保存分时数据
     * @param symbol 股票代码
     * @param data 分时数据
     * @param basePrice 昨收价
     */
    bool saveTimeShareData(const QString& symbol, const QVector<TimeSharePoint>& data, double basePrice);

    /**
     * @brief 获取分时数据
     * @param symbol 股票代码
     * @param date 日期（默认今天）
     */
    QVector<TimeSharePoint> getTimeShareData(const QString& symbol, const QDate& date = QDate());

    /**
     * @brief 获取分时数据基准价
     */
    double getTimeShareBasePrice(const QString& symbol, const QDate& date = QDate());

    /**
     * @brief 清理分时数据
     */
    void cleanTimeShareData(int daysToKeep = 7);

    // ========== 通用方法 ==========

    /**
     * @brief 检查是否有本地数据
     */
    bool hasLocalData();

    /**
     * @brief 获取最后更新时间
     */
    QDateTime getLastUpdateTime(const QString& dataType);

    /**
     * @brief 设置最后更新时间
     */
    void setLastUpdateTime(const QString& dataType, const QDateTime& time);

signals:
    void errorOccurred(const QString& error);
    void watchlistChanged();

private:
    DataStorageService();
    ~DataStorageService();
    
    void createTables();
    
    bool m_initialized = false;
    QString m_dbPath;
    
    static DataStorageService* s_instance;
};

#endif // DATASTORAGESERVICE_H
