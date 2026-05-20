/**
 * @file MarketDataProducer.cpp
 * @brief 行情数据生产者实现
 */

#include "MarketDataProducer.h"
#include "market/StockDataSource.h"
#include "models/FuturesQuoteModel.h"
#include <QDebug>
#include <QRegularExpression>

namespace WealthPilot {
namespace Producers {

MarketDataProducer::MarketDataProducer(QObject* parent)
    : IDataProducer(parent)
    , m_batchTimer(new QTimer(this))
{
    // 批量请求定时器，每 500ms 合并一次请求
    m_batchTimer->setInterval(500);
    m_batchTimer->setSingleShot(true);
    connect(m_batchTimer, &QTimer::timeout, this, [this]() {
        if (!m_pendingStockRequests.isEmpty()) {
            if (m_stockSource) {
                m_stockSource->requestQuotes(m_pendingStockRequests);
            }
            m_pendingStockRequests.clear();
        }
    });
}

MarketDataProducer::~MarketDataProducer()
{
    m_batchTimer->stop();
}

QStringList MarketDataProducer::topicPatterns() const
{
    return {
        "market:quote:*",
        "market:futures:*",
        "market:kline:*",
        "market:timeshare:*"
    };
}

void MarketDataProducer::refresh(const QStringList& topics)
{
    for (const auto& topic : topics) {
        auto parsed = parseTopic(topic);
        if (!parsed.isValid()) {
            qWarning() << "[MarketDataProducer] Invalid topic:" << topic;
            continue;
        }

        if (parsed.type == "quote") {
            // 股票行情
            m_pendingStockRequests.append(parsed.symbol);
        } else if (parsed.type == "futures") {
            // 期货行情
            refreshFutures(parsed.symbol);
        } else if (parsed.type == "kline") {
            // K线数据
            refreshKLine(parsed.symbol, parsed.modifier);
        } else if (parsed.type == "timeshare") {
            // 分时数据
            refreshTimeShare(parsed.symbol);
        }
    }

    // 启动批量请求定时器
    if (!m_pendingStockRequests.isEmpty() && !m_batchTimer->isActive()) {
        m_batchTimer->start();
    }
}

void MarketDataProducer::onTopicIdle(const QString& topic)
{
    auto parsed = parseTopic(topic);
    if (!parsed.isValid()) return;

    if (parsed.type == "quote") {
        m_activeStockSymbols.remove(parsed.symbol);
        qDebug() << "[MarketDataProducer] Topic idle:" << topic
                 << "remaining stock symbols:" << m_activeStockSymbols.size();
    } else if (parsed.type == "futures") {
        m_activeFuturesSymbols.remove(parsed.symbol);
    } else if (parsed.type == "kline") {
        m_activeKLineTopics.remove(topic);
    }
}

void MarketDataProducer::onTopicActive(const QString& topic)
{
    auto parsed = parseTopic(topic);
    if (!parsed.isValid()) return;

    if (parsed.type == "quote") {
        m_activeStockSymbols.insert(parsed.symbol);
        qDebug() << "[MarketDataProducer] Topic active:" << topic
                 << "total stock symbols:" << m_activeStockSymbols.size();
    } else if (parsed.type == "futures") {
        m_activeFuturesSymbols.insert(parsed.symbol);
    } else if (parsed.type == "kline") {
        m_activeKLineTopics.insert(topic);
    }
}

void MarketDataProducer::setStockDataSource(StockDataSource* source)
{
    if (m_stockSource) {
        disconnect(m_stockSource, nullptr, this, nullptr);
    }

    m_stockSource = source;

    if (m_stockSource) {
        // 使用 lambda 适配信号
        connect(m_stockSource, &StockDataSource::quotesReceived,
                this, [this](const QVector<WealthPilot::StockQuote>& quotes) {
                    QVariantList dataList;
                    for (const auto& q : quotes) {
                        dataList.append(QVariant::fromValue(q));
                    }
                    onStockDataReceived(dataList);
                });
    }
}

void MarketDataProducer::setFuturesModel(FuturesQuoteModel* model)
{
    if (m_futuresModel) {
        disconnect(m_futuresModel, nullptr, this, nullptr);
    }

    m_futuresModel = model;

    // FuturesQuoteModel 数据更新通过定时检查或外部触发
    // 模型本身会自动更新，DataHub 通过 request() 触发刷新
}

void MarketDataProducer::onStockDataReceived(const QVariantList& quotesData)
{
    auto& hub = DataHub::DataHub::instance();
    int count = 0;

    for (const auto& quoteVar : quotesData) {
        auto quote = quoteVar.value<WealthPilot::StockQuote>();
        if (!quote.isValid()) continue;

        // 发布到 DataHub
        QString topic = QString("market:quote:%1").arg(quote.symbol);
        hub.publish(topic, QVariant::fromValue(quote));

        // 同时发布为 MarketSnapshot 格式（兼容）
        WealthPilot::MarketSnapshot snapshot;
        snapshot.instrumentId = quote.symbol;
        snapshot.instrumentName = quote.name;
        snapshot.lastPrice = quote.price;
        snapshot.preClose = quote.prevClose;
        snapshot.openPrice = quote.open;
        snapshot.highestPrice = quote.high;
        snapshot.lowestPrice = quote.low;
        snapshot.volume = quote.volume;
        snapshot.turnover = quote.amount;
        snapshot.upperLimit = quote.upperLimit;
        snapshot.lowerLimit = quote.lowerLimit;
        snapshot.updateTime = quote.updateTime;

        // 复制五档盘口
        for (int i = 0; i < 5; ++i) {
            snapshot.bidPrice[i] = quote.bidPrice[i];
            snapshot.bidVolume[i] = static_cast<int>(quote.bidVolume[i]);
            snapshot.askPrice[i] = quote.askPrice[i];
            snapshot.askVolume[i] = static_cast<int>(quote.askVolume[i]);
        }

        hub.publish(QString("market:snapshot:%1").arg(quote.symbol), 
                    QVariant::fromValue(snapshot));
        count++;
    }

    qDebug() << "[MarketDataProducer] Published" << count << "stock quotes";
}

void MarketDataProducer::onFuturesQuotesUpdated()
{
    // 从 FuturesQuoteModel 获取数据并发布到 DataHub
    if (!m_futuresModel) return;
    
    auto& hub = DataHub::DataHub::instance();
    
    // 遍历模型中的所有期货行情
    for (int row = 0; row < m_futuresModel->rowCount(); ++row) {
        auto index = m_futuresModel->index(row, 0);
        QString instrumentId = m_futuresModel->data(index, Qt::UserRole).toString();
        
        // 构造 MarketSnapshot 并发布
        WealthPilot::MarketSnapshot snapshot;
        snapshot.instrumentId = instrumentId;
        // 从模型获取其他字段...
        
        QString topic = QString("market:futures:%1").arg(instrumentId);
        hub.publish(topic, QVariant::fromValue(snapshot));
    }
}

MarketDataProducer::ParsedTopic MarketDataProducer::parseTopic(const QString& topic) const
{
    ParsedTopic result;

    // Topic 格式: domain:type:symbol[:modifier]
    // 示例: market:quote:sh600000
    //       market:kline:sh600000:day1
    //       market:futures:IF2501

    auto parts = topic.split(':');
    if (parts.size() < 3) {
        return result;
    }

    result.domain = parts[0];
    result.type = parts[1];
    result.symbol = parts[2];

    if (parts.size() > 3) {
        result.modifier = parts[3];
    }

    return result;
}

void MarketDataProducer::refreshQuote(const QString& symbol)
{
    if (m_stockSource) {
        m_stockSource->requestQuotes({symbol});
    }
}

void MarketDataProducer::refreshFutures(const QString& symbol)
{
    // 期货行情刷新
    // 通过 CTP 或其他期货数据源获取
    if (m_futuresModel) {
        // 触发模型刷新请求
        // FuturesQuoteModel 内部会处理数据获取和更新
        qDebug() << "[MarketDataProducer] Futures refresh requested:" << symbol;
    }
}

void MarketDataProducer::refreshKLine(const QString& symbol, const QString& period)
{
    if (!m_stockSource) return;

    WealthPilot::KLinePeriod klinePeriod = WealthPilot::KLinePeriod::Day1;

    if (period == "1m" || period == "minute1") {
        klinePeriod = WealthPilot::KLinePeriod::Minute1;
    } else if (period == "5m" || period == "minute5") {
        klinePeriod = WealthPilot::KLinePeriod::Minute5;
    } else if (period == "15m" || period == "minute15") {
        klinePeriod = WealthPilot::KLinePeriod::Minute15;
    } else if (period == "30m" || period == "minute30") {
        klinePeriod = WealthPilot::KLinePeriod::Minute30;
    } else if (period == "60m" || period == "hour1") {
        klinePeriod = WealthPilot::KLinePeriod::Hour1;
    } else if (period == "day" || period == "day1") {
        klinePeriod = WealthPilot::KLinePeriod::Day1;
    } else if (period == "week" || period == "week1") {
        klinePeriod = WealthPilot::KLinePeriod::Week1;
    } else if (period == "month" || period == "month1") {
        klinePeriod = WealthPilot::KLinePeriod::Month1;
    }

    m_stockSource->requestKLine(symbol, klinePeriod);
}

void MarketDataProducer::refreshTimeShare(const QString& symbol)
{
    if (m_stockSource) {
        m_stockSource->requestTimeShare(symbol);
    }
}

} // namespace Producers
} // namespace WealthPilot