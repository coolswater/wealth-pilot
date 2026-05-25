/**
 * @file StockDataSource.h
 * @brief 股票数据源 - 接入第三方股票行情API
 *
 * @details 支持数据源：
 * - 新浪财经（免费，无需API Key）
 * - 腾讯财经（免费）
 * - 东方财富（免费）
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef STOCKDATASOURCE_H
#define STOCKDATASOURCE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QHash>
#include <QVector>
#include "shared/types/MarketTypes.h"

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::StockQuote;
using WealthPilot::KLinePeriod;
using WealthPilot::KLineData;
using WealthPilot::TimeShareData;

/**
 * @brief 旧版股票行情数据（兼容适配器）
 * @note 用于解析第三方 API 数据，然后转换为 WealthPilot::StockQuote
 */
struct StockQuoteApiData {
    QString symbol;             ///< 股票代码
    QString name;               ///< 股票名称
    double lastPrice = 0.0;     ///< 最新价
    double openPrice = 0.0;     ///< 开盘价
    double highPrice = 0.0;     ///< 最高价
    double lowPrice = 0.0;      ///< 最低价
    double preClose = 0.0;      ///< 昨收价
    double closePrice = 0.0;    ///< 收盘价
    qint64 volume = 0;          ///< 成交量
    double turnover = 0.0;      ///< 成交额
    double changePercent = 0.0; ///< 涨跌幅
    double changeAmount = 0.0;  ///< 涨跌额
    QDateTime updateTime;       ///< 更新时间

    // 五档盘口
    double bidPrice[5] = {0};   ///< 买价1-5
    qint64 bidVolume[5] = {0};  ///< 买量1-5
    double askPrice[5] = {0};   ///< 卖价1-5
    qint64 askVolume[5] = {0};  ///< 卖量1-5

    // 委比委差
    double orderRatio = 0.0;    ///< 委比
    qint64 orderDiff = 0;       ///< 委差

    // 其他数据
    double avgPrice = 0.0;      ///< 均价
    double turnoverRate = 0.0; ///< 换手率
    double volumeRatio = 0.0;  ///< 量比
    double limitUp = 0.0;      ///< 涨停价
    double limitDown = 0.0;    ///< 跌停价
    qint64 outerVolume = 0;    ///< 外盘
    qint64 innerVolume = 0;    ///< 内盘

    bool isValid() const { return !symbol.isEmpty() && lastPrice > 0; }

    /**
     * @brief 转换为 WealthPilot::StockQuote
     */
    WealthPilot::StockQuote toStockQuote() const {
        WealthPilot::StockQuote quote;
        quote.symbol = symbol;
        quote.name = name;
        quote.price = lastPrice;
        quote.open = openPrice;
        quote.high = highPrice;
        quote.low = lowPrice;
        quote.prevClose = preClose;
        quote.volume = volume;
        quote.amount = turnover;
        quote.change = changeAmount;
        quote.changePercent = changePercent;
        quote.updateTime = updateTime;
        quote.upperLimit = limitUp;
        quote.lowerLimit = limitDown;
        for (int i = 0; i < 5; ++i) {
            quote.bidPrice[i] = bidPrice[i];
            quote.bidVolume[i] = bidVolume[i];
            quote.askPrice[i] = askPrice[i];
            quote.askVolume[i] = askVolume[i];
        }
        return quote;
    }
};

// TimeShareData 已在 core/types/MarketTypes.h 中定义

/**
 * @brief 实时K线更新数据
 */
struct RealtimeKLineUpdate {
    QString symbol;
    double lastPrice = 0.0;     ///< 最新价（实时更新收盘价）
    double highPrice = 0.0;     ///< 最高价
    double lowPrice = 0.0;      ///< 最低价
    qint64 volume = 0;          ///< 成交量
    QDateTime updateTime;       ///< 更新时间
    bool isTrading = false;     ///< 是否交易中
};

/**
 * @brief 股票数据源
 */
class StockDataSource : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 数据源类型
     */
    enum class Source {
        Sina,           ///< 新浪财经
        Tencent,        ///< 腾讯财经
        EastMoney       ///< 东方财富
    };

    explicit StockDataSource(Source source = Source::Sina, QObject *parent = nullptr);
    ~StockDataSource();

    /**
     * @brief 请求实时行情
     * @param symbols 股票代码列表（如 sh600000,sz000001）
     */
    void requestQuotes(const QStringList &symbols);

    /**
     * @brief 请求K线数据
     * @param symbol 股票代码
     * @param period K线周期
     * @param count 数据条数
     */
    void requestKLine(const QString &symbol, KLinePeriod period, int count = 500);

    /**
     * @brief 请求分时数据
     * @param symbol 股票代码
     */
    void requestTimeShare(const QString &symbol);

    /**
     * @brief 请求股票列表
     * @param market 市场类型（sh/sz）
     */
    void requestStockList(const QString &market = QString());

    /**
     * @brief 获取缓存行情
     */
    WealthPilot::StockQuote getCachedQuote(const QString &symbol) const;

    /**
     * @brief 启动实时行情推送
     * @param symbol 股票代码
     * @param intervalMs 刷新间隔（默认3秒）
     */
    void startRealtimeQuotes(const QString &symbol, int intervalMs = 3000);

    /**
     * @brief 停止实时行情推送
     */
    void stopRealtimeQuotes();

    /**
     * @brief 启动自动刷新
     * @param intervalMs 刷新间隔
     */
    void startAutoRefresh(int intervalMs = 5000);

    /**
     * @brief 停止自动刷新
     */
    void stopAutoRefresh();

signals:
    void quotesReceived(const QVector<WealthPilot::StockQuote> &quotes);
    void kLineReceived(const QString &symbol, const QVector<WealthPilot::KLineData> &data);
    void timeShareReceived(const QString &symbol, const QVector<WealthPilot::TimeShareData> &data);
    void realtimeQuoteReceived(const QString &symbol, const WealthPilot::StockQuote &quote);
    void realtimeKLineUpdate(const QString &symbol, const RealtimeKLineUpdate &update);
    void stockListReceived(const QStringList &symbols);
    void errorOccurred(const QString &error);

private slots:
    void onNetworkReply(QNetworkReply *reply);
    void onRefreshTimer();
    void onRealtimeQuoteTimer();

private:
    // 解析函数
    void parseSinaQuotes(const QByteArray &data);
    void parseTencentQuotes(const QByteArray &data);
    void parseEastMoneyQuotes(const QByteArray &data);
    void parseSinaKLine(const QByteArray &data, const QString &symbol);
    void parseSinaTimeShare(const QByteArray &data, const QString &symbol);
    void parseSinaStockList(const QByteArray &data);

    // URL构建
    QString buildQuotesUrl(const QStringList &symbols) const;
    QString buildKLineUrl(const QString &symbol, KLinePeriod period, int count) const;
    QString buildTimeShareUrl(const QString &symbol) const;

    // 辅助函数
    static QString normalizeSymbol(const QString &symbol);
    static QString toSinaSymbol(const QString &symbol);

    Source m_source;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_refreshTimer;
    QTimer *m_realtimeQuoteTimer;

    // 缓存
    QHash<QString, StockQuote> m_quoteCache;
    QStringList m_subscribedSymbols;
    QString m_realtimeSymbol;

    // 请求追踪
    QHash<QNetworkReply*, QString> m_pendingRequests;
    enum class RequestType { Quotes, KLine, TimeShare, StockList, RealtimeQuote };
    QHash<QNetworkReply*, QPair<RequestType, QString>> m_requestTypes;
};

#endif // STOCKDATASOURCE_H
