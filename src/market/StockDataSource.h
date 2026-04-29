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
#include "core/types/MarketTypes.h"

/**
 * @brief 股票行情数据
 */
struct StockQuote {
    QString symbol;             ///< 股票代码（如 sh600000）
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

    bool isValid() const { return !symbol.isEmpty() && lastPrice > 0; }
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
     * @brief 请求股票列表
     * @param market 市场类型（sh/sz）
     */
    void requestStockList(const QString &market = QString());

    /**
     * @brief 获取缓存行情
     */
    StockQuote getCachedQuote(const QString &symbol) const;

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
    void quotesReceived(const QVector<StockQuote> &quotes);
    void kLineReceived(const QString &symbol, const QVector<KLineData> &data);
    void stockListReceived(const QStringList &symbols);
    void errorOccurred(const QString &error);

private slots:
    void onNetworkReply(QNetworkReply *reply);
    void onRefreshTimer();

private:
    // 解析函数
    void parseSinaQuotes(const QByteArray &data);
    void parseTencentQuotes(const QByteArray &data);
    void parseEastMoneyQuotes(const QByteArray &data);
    void parseSinaKLine(const QByteArray &data, const QString &symbol);
    void parseSinaStockList(const QByteArray &data);

    // URL构建
    QString buildQuotesUrl(const QStringList &symbols) const;
    QString buildKLineUrl(const QString &symbol, KLinePeriod period, int count) const;

    // 辅助函数
    static QString normalizeSymbol(const QString &symbol);
    static QString toSinaSymbol(const QString &symbol);

    Source m_source;
    QNetworkAccessManager *m_networkManager;
    QTimer *m_refreshTimer;

    // 缓存
    QHash<QString, StockQuote> m_quoteCache;
    QStringList m_subscribedSymbols;

    // 请求追踪
    QHash<QNetworkReply*, QString> m_pendingRequests;
    enum class RequestType { Quotes, KLine, StockList };
    QHash<QNetworkReply*, QPair<RequestType, QString>> m_requestTypes;
};

#endif // STOCKDATASOURCE_H
