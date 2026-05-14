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
        connect(m_stockSource, &StockDataSource::quotesReceived,
                this, &MarketDataProducer::onStockQuotesReceived);
    }
}

void MarketDataProducer::setFuturesModel(FuturesQuoteModel* model)
{
    if (m_futuresModel) {
        disconnect(m_futuresModel, nullptr, this, nullptr);
    }

    m_futuresModel = model;

    if (m_futuresModel) {
        connect(m_futuresModel, &FuturesQuoteModel::dataUpdated,
                this, &MarketDataProducer::onFuturesQuotesUpdated);
    }
}

void MarketDataProducer::onStockQuotesReceived(const QVector<StockQuote>& quotes)
{
    auto& hub = DataHub::DataHub::instance();

    for (const auto& quote : quotes) {
        if (!quote.isValid()) continue;

        // 发布到 DataHub
        QString topic = QString("market:quote:%1").arg(quote.symbol);
        hub.publish(topic, QVariant::fromValue(quote));

        // 同时发布为 MarketSnapshot 格式（兼容）
        MarketSnapshot snapshot;
        snapshot.instrumentId = quote.symbol;
        snapshot.instrumentName = quote.name;
        snapshot.lastPrice = quote.lastPrice;
        snapshot.preClose = quote.preClose;
        snapshot.openPrice = quote.openPrice;
        snapshot.highestPrice = quote.highPrice;
        snapshot.lowestPrice = quote.lowPrice;
        snapshot.volume = quote.volume;
        snapshot.turnover = quote.turnover;
        snapshot.upperLimit = quote.limitUp;
        snapshot.lowerLimit = quote.limitDown;
        snapshot.updateTime = quote.updateTime;

        // 复制五档盘口
        for (int i = 0; i < 5; ++i) {
            snapshot.bidPrice[i] = quote.bidPrice[i];
            snapshot.bidVolume[i] = quote.bidVolume[i];
            snapshot.askPrice[i] = quote.askPrice[i];
            snapshot.askVolume[i] = quote.askVolume[i];
        }

        hub.publish(QString("market:snapshot:%1").arg(quote.symbol), 
                    QVariant::fromValue(snapshot));
    }

    qDebug() << "[MarketDataProducer] Published" << quotes.size() << "stock quotes";
}

void MarketDataProducer::onFuturesQuotesUpdated()
{
    // TODO: 从 FuturesQuoteModel 获取数据并发布
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
    // TODO: 实现期货行情刷新
    Q_UNUSED(symbol)
}

void MarketDataProducer::refreshKLine(const QString& symbol, const QString& period)
{
    if (!m_stockSource) return;

    KLinePeriod klinePeriod = KLinePeriod::Day1;

    if (period == "1m" || period == "minute1") {
        klinePeriod = KLinePeriod::Minute1;
    } else if (period == "5m" || period == "minute5") {
        klinePeriod = KLinePeriod::Minute5;
    } else if (period == "15m" || period == "minute15") {
        klinePeriod = KLinePeriod::Minute15;
    } else if (period == "30m" || period == "minute30") {
        klinePeriod = KLinePeriod::Minute30;
    } else if (period == "60m" || period == "hour1") {
        klinePeriod = KLinePeriod::Hour1;
    } else if (period == "day" || period == "day1") {
        klinePeriod = KLinePeriod::Day1;
    } else if (period == "week" || period == "week1") {
        klinePeriod = KLinePeriod::Week1;
    } else if (period == "month" || period == "month1") {
        klinePeriod = KLinePeriod::Month1;
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