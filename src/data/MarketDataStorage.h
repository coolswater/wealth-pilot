/**
 * @file MarketDataStorage.h
 * @brief 市场数据存储管理器 - 统一管理各类市场数据的本地存储
 *
 * @details 功能：
 * - 股票行情数据存储
 * - 期货行情数据存储
 * - 数字货币行情数据存储
 * - 基金净值数据存储
 * - 外汇汇率数据存储
 * - 缓存优先读取策略
 * - 数据过期自动更新
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef MARKETDATASTORAGE_H
#define MARKETDATASTORAGE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QHash>
#include <QMutex>
#include <QDateTime>
#include <memory>
#include <functional>
#include "shared/types/MarketTypes.h"  // 使用完整的类型定义

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::StockQuote;
using WealthPilot::ForexQuote;
using WealthPilot::CryptoQuote;
using WealthPilot::FundQuote;

/**
 * @brief 数据类型枚举
 */
enum class MarketDataType {
    Stock,      ///< 股票
    Futures,    ///< 期货
    Crypto,     ///< 数字货币
    Fund,       ///< 基金
    Forex       ///< 外汇
};

/**
 * @brief 数据新鲜度
 */
struct DataFreshness {
    QDateTime lastUpdate;       ///< 最后更新时间
    int ttlSeconds = 60;        ///< 数据有效期（秒）
    bool isExpired() const {
        return lastUpdate.secsTo(QDateTime::currentDateTime()) > ttlSeconds;
    }
};

/**
 * @brief 市场数据存储管理器
 */
class MarketDataStorage : public QObject
{
    Q_OBJECT

public:
    static MarketDataStorage* instance();

    /**
     * @brief 初始化存储
     * @param dbPath 数据库路径
     */
    bool initialize(const QString& dbPath = QString());

    /**
     * @brief 关闭存储
     */
    void shutdown();

    // ========== 股票数据 ==========

    /**
     * @brief 保存股票行情
     */
    bool saveStockQuote(const QString& code, const StockQuote& quote);

    /**
     * @brief 批量保存股票行情
     */
    bool saveStockQuotes(const QVector<QPair<QString, StockQuote>>& quotes);

    /**
     * @brief 获取股票行情
     * @param code 股票代码
     * @param outQuote 输出行情
     * @return 是否存在有效数据
     */
    bool getStockQuote(const QString& code, StockQuote& outQuote);

    /**
     * @brief 批量获取股票行情
     */
    QVector<StockQuote> getStockQuotes(const QStringList& codes);

    /**
     * @brief 获取所有股票行情
     */
    QVector<StockQuote> getAllStockQuotes();

    // ========== 期货数据 ==========

    /**
     * @brief 保存期货行情
     */
    bool saveFuturesQuote(const QString& instrumentId, const QVariantMap& quote);

    /**
     * @brief 获取期货行情
     */
    bool getFuturesQuote(const QString& instrumentId, QVariantMap& outQuote);

    /**
     * @brief 获取所有期货行情
     */
    QVector<QVariantMap> getAllFuturesQuotes();

    // ========== 数字货币数据 ==========

    /**
     * @brief 保存数字货币行情
     */
    bool saveCryptoQuote(const QString& symbol, const CryptoQuote& quote);

    /**
     * @brief 批量保存数字货币行情
     */
    bool saveCryptoQuotes(const QVector<CryptoQuote>& quotes);

    /**
     * @brief 获取数字货币行情
     */
    bool getCryptoQuote(const QString& symbol, CryptoQuote& outQuote);

    /**
     * @brief 获取所有数字货币行情
     */
    QVector<CryptoQuote> getAllCryptoQuotes();

    // ========== 基金数据 ==========

    /**
     * @brief 保存基金净值
     */
    bool saveFundQuote(const QString& code, const FundQuote& quote);

    /**
     * @brief 批量保存基金净值
     */
    bool saveFundQuotes(const QVector<FundQuote>& quotes);

    /**
     * @brief 获取基金净值
     */
    bool getFundQuote(const QString& code, FundQuote& outQuote);

    /**
     * @brief 获取所有基金净值
     */
    QVector<FundQuote> getAllFundQuotes();

    // ========== 外汇数据 ==========

    /**
     * @brief 保存外汇汇率
     */
    bool saveForexQuote(const QString& pair, const ForexQuote& quote);

    /**
     * @brief 批量保存外汇汇率
     */
    bool saveForexQuotes(const QVector<ForexQuote>& quotes);

    /**
     * @brief 获取外汇汇率
     */
    bool getForexQuote(const QString& pair, ForexQuote& outQuote);

    /**
     * @brief 获取所有外汇汇率
     */
    QVector<ForexQuote> getAllForexQuotes();

    // ========== 数据新鲜度管理 ==========

    /**
     * @brief 检查数据是否新鲜
     */
    bool isDataFresh(MarketDataType type, const QString& code);

    /**
     * @brief 获取数据新鲜度信息
     */
    DataFreshness getDataFreshness(MarketDataType type, const QString& code);

    /**
     * @brief 设置数据有效期
     */
    void setDataTTL(MarketDataType type, int ttlSeconds);

    // ========== 数据维护 ==========

    /**
     * @brief 清理过期数据
     */
    void cleanExpiredData();

    /**
     * @brief 清理所有数据
     */
    void clearAllData();

    /**
     * @brief 获取数据库大小
     */
    qint64 getDatabaseSize() const;

    /**
     * @brief 优化数据库
     */
    void optimize();

signals:
    /**
     * @brief 数据更新信号
     */
    void dataUpdated(MarketDataType type, const QString& code);

    /**
     * @brief 数据过期信号
     */
    void dataExpired(MarketDataType type, const QString& code);

    /**
     * @brief 错误信号
     */
    void errorOccurred(const QString& error);

private:
    MarketDataStorage(QObject* parent = nullptr);
    ~MarketDataStorage() override;

    // 禁止拷贝
    MarketDataStorage(const MarketDataStorage&) = delete;
    MarketDataStorage& operator=(const MarketDataStorage&) = delete;

    bool createTables();
    QString tableName(MarketDataType type) const;

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // MARKETDATASTORAGE_H
