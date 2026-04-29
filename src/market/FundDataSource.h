/**
 * @file FundDataSource.h
 * @brief 基金数据源 - 接入第三方基金净值API
 *
 * @details 支持数据源：
 * - 天天基金（免费）
 * - 东方财富基金（免费）
 * - 蛋卷基金（免费）
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FUNDDATASOURCE_H
#define FUNDDATASOURCE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QHash>
#include <QVector>
#include <functional>

/**
 * @brief 基金类型
 */
enum class FundType {
    ETF,        ///< 交易所交易基金
    LOF,        ///< 上市开放式基金
    OpenEnd,    ///< 开放式基金
    ClosedEnd,  ///< 封闭式基金
    Money,      ///< 货币基金
    Bond,       ///< 债券基金
    Mixed,      ///< 混合基金
    Stock,      ///< 股票基金
    Index,      ///< 指数基金
    QDII,       ///< QDII基金
    Unknown     ///< 未知类型
};

/**
 * @brief 基金行情数据
 */
struct FundQuote {
    QString code;               ///< 基金代码
    QString name;               ///< 基金名称
    FundType type = FundType::Unknown;  ///< 基金类型
    
    double nav = 0.0;           ///< 单位净值
    double accNav = 0.0;        ///< 累计净值
    double lastPrice = 0.0;     ///< 最新价格（ETF/LOF）
    double changeAmount = 0.0;  ///< 涨跌额
    double changePercent = 0.0; ///< 涨跌幅
    
    qint64 volume = 0;          ///< 成交量
    double turnover = 0.0;      ///< 成交额
    
    QString manager;            ///< 基金经理
    QString company;            ///< 基金公司
    double scale = 0.0;         ///< 基金规模（亿元）
    
    QDateTime navDate;          ///< 净值日期
    QDateTime updateTime;       ///< 更新时间

    bool isValid() const { return !code.isEmpty() && nav > 0; }
};

/**
 * @brief 基金数据源
 */
class FundDataSource : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 数据源类型
     */
    enum class DataSource {
        EastMoney,  ///< 东方财富（天天基金）
        Danjuan,    ///< 蛋卷基金
        Sina        ///< 新浪基金
    };

    static FundDataSource* instance();

    /**
     * @brief 设置数据源
     */
    void setDataSource(DataSource source);

    /**
     * @brief 请求单个基金行情
     * @param code 基金代码
     * @param callback 回调函数
     */
    void requestQuote(const QString& code,
                      std::function<void(const FundQuote&)> callback);

    /**
     * @brief 批量请求多个基金行情
     * @param codes 基金代码列表
     * @param callback 回调函数
     */
    void requestQuotes(const QStringList& codes,
                       std::function<void(const QVector<FundQuote>&)> callback);

    /**
     * @brief 搜索基金
     * @param keyword 关键词（代码或名称）
     * @param callback 回调函数
     */
    void searchFund(const QString& keyword,
                    std::function<void(const QVector<FundQuote>&)> callback);

    /**
     * @brief 请求基金列表
     * @param type 基金类型
     * @param sortField 排序字段（nav, changePercent, scale）
     * @param limit 数量限制
     * @param callback 回调函数
     */
    void requestFundList(FundType type, const QString& sortField, int limit,
                         std::function<void(const QVector<FundQuote>&)> callback);

    /**
     * @brief 获取缓存的行情
     */
    FundQuote cachedQuote(const QString& code) const;

    /**
     * @brief 启动自动刷新
     * @param intervalMs 刷新间隔（毫秒）
     */
    void startAutoRefresh(int intervalMs = 60000);

    /**
     * @brief 停止自动刷新
     */
    void stopAutoRefresh();

signals:
    /**
     * @brief 行情更新信号
     */
    void quoteUpdated(const FundQuote& quote);

    /**
     * @brief 批量更新信号
     */
    void quotesUpdated(const QVector<FundQuote>& quotes);

    /**
     * @brief 错误信号
     */
    void error(const QString& errorMessage);

private:
    FundDataSource(QObject* parent = nullptr);
    ~FundDataSource() override;

    // 禁止拷贝
    FundDataSource(const FundDataSource&) = delete;
    FundDataSource& operator=(const FundDataSource&) = delete;

    // 数据解析
    FundQuote parseEastMoneyResponse(const QString& code, const QByteArray& data);
    FundQuote parseDanjuanResponse(const QString& code, const QByteArray& data);
    FundQuote parseSinaResponse(const QString& code, const QByteArray& data);
    QVector<FundQuote> parseBatchResponse(const QByteArray& data);
    QVector<FundQuote> parseSearchResponse(const QByteArray& data);
    QVector<FundQuote> parseListResponse(const QByteArray& data);
    FundType detectFundType(const QString& code);

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FUNDDATASOURCE_H
