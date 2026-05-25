/**
 * @file MarketDataProducer.h
 * @brief 行情数据生产者 - 实现 IDataProducer 接口
 *
 * @details 功能：
 * - 实现 DataHub 的 IDataProducer 接口
 * - 统一管理股票、期货、外汇等行情数据获取
 * - 支持批量刷新和速率限制
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef MARKETDATAPRODUCER_H
#define MARKETDATAPRODUCER_H

#include "data/datahub/DataHub.h"
#include "shared/types/MarketTypes.h"  // 使用统一的类型定义
#include <QObject>
#include <QHash>
#include <QSet>
#include <QTimer>

// 前向声明
class StockDataSource;
class FuturesQuoteModel;

namespace WealthPilot {
namespace Producers {

/**
 * @brief 行情数据生产者
 * 
 * 负责的 Topic 模式：
 * - market:quote:*        股票行情
 * - market:futures:*      期货行情
 * - market:kline:*        K线数据
 * - market:timeshare:*    分时数据
 */
class MarketDataProducer : public DataHub::IDataProducer
{
    Q_OBJECT

public:
    explicit MarketDataProducer(QObject* parent = nullptr);
    ~MarketDataProducer() override;

    // ========== IDataProducer 接口实现 ==========

    /**
     * @brief 返回支持的 topic 模式
     */
    QStringList topicPatterns() const override;

    /**
     * @brief 刷新指定的 topics
     */
    void refresh(const QStringList& topics) override;

    /**
     * @brief 最大请求速率（每秒）
     * @note 新浪财经限制约 10 次/秒
     */
    int maxRequestsPerSecond() const override { return 10; }

    /**
     * @brief Topic 变为空闲时调用
     */
    void onTopicIdle(const QString& topic) override;

    /**
     * @brief Topic 变为活跃时调用
     */
    void onTopicActive(const QString& topic) override;

    // ========== 初始化 ==========

    /**
     * @brief 设置股票数据源
     */
    void setStockDataSource(StockDataSource* source);

    /**
     * @brief 设置期货数据模型
     */
    void setFuturesModel(FuturesQuoteModel* model);

signals:
    /**
     * @brief 请求行情信号
     */
    void quotesRequested(const QStringList& symbols);

    /**
     * @brief 请求 K 线信号
     */
    void kLineRequested(const QString& symbol, KLinePeriod period);

private slots:
    void onStockDataReceived(const QVariantList& quotesData);
    void onFuturesQuotesUpdated();

private:
    // Topic 解析
    struct ParsedTopic {
        QString domain;      // market
        QString type;        // quote, futures, kline, timeshare
        QString symbol;      // 代码
        QString modifier;    // 修饰符（如 kline 周期）
        bool isValid() const { return !domain.isEmpty() && !type.isEmpty(); }
    };
    
    ParsedTopic parseTopic(const QString& topic) const;
    void refreshQuote(const QString& symbol);
    void refreshFutures(const QString& symbol);
    void refreshKLine(const QString& symbol, const QString& period);
    void refreshTimeShare(const QString& symbol);

    // 数据源
    StockDataSource* m_stockSource = nullptr;
    FuturesQuoteModel* m_futuresModel = nullptr;

    // 订阅状态
    QSet<QString> m_activeStockSymbols;
    QSet<QString> m_activeFuturesSymbols;
    QSet<QString> m_activeKLineTopics;

    // 批量请求定时器
    QTimer* m_batchTimer;
    QStringList m_pendingStockRequests;
    QStringList m_pendingFuturesRequests;
};

} // namespace Producers
} // namespace WealthPilot

// 注册元类型
Q_DECLARE_METATYPE(WealthPilot::Producers::MarketDataProducer)

#endif // MARKETDATAPRODUCER_H