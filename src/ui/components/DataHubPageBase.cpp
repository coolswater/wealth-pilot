/**
 * @file DataHubPageBase.cpp
 * @brief DataHub 页面基类实现
 */

#include "DataHubPageBase.h"
#include "market/StockDataSource.h"
#include <QDebug>

namespace WealthPilot {
namespace UI {

DataHubPageBase::DataHubPageBase(QWidget* parent)
    : BasePage(parent)
{
}

DataHubPageBase::~DataHubPageBase()
{
    // DataHub 会自动清理 this 的所有订阅
    qDebug() << "[DataHubPageBase] Destroyed, auto-unsubscribed topics:" << m_subscribedTopics.size();
}

QMetaObject::Connection DataHubPageBase::subscribe(
    const QString& topic,
    std::function<void(const QVariant&)> slot)
{
    m_subscribedTopics.append(topic);
    return dataHub().subscribe(this, topic, slot);
}

QMetaObject::Connection DataHubPageBase::subscribeQuote(
    const QString& symbol,
    std::function<void(const StockQuote&)> slot)
{
    QString topic = QString("market:quote:%1").arg(symbol);
    m_subscribedTopics.append(topic);
    return dataHub().subscribe<StockQuote>(this, topic, slot);
}

QMetaObject::Connection DataHubPageBase::subscribeSnapshot(
    const QString& symbol,
    std::function<void(const MarketSnapshot&)> slot)
{
    QString topic = QString("market:snapshot:%1").arg(symbol);
    m_subscribedTopics.append(topic);
    return dataHub().subscribe<MarketSnapshot>(this, topic, slot);
}

QMetaObject::Connection DataHubPageBase::subscribeKLine(
    const QString& symbol,
    const QString& period,
    std::function<void(const QVector<KLineData>&)> slot)
{
    QString topic = QString("market:kline:%1:%2").arg(symbol, period);
    m_subscribedTopics.append(topic);
    return dataHub().subscribe<QVector<KLineData>>(this, topic, slot);
}

void DataHubPageBase::requestData(const QString& topic, bool force)
{
    dataHub().request(topic, force);
}

void DataHubPageBase::requestData(const QStringList& topics, bool force)
{
    dataHub().request(topics, force);
}

std::optional<StockQuote> DataHubPageBase::getCachedQuote(const QString& symbol) const
{
    QString topic = QString("market:quote:%1").arg(symbol);
    QVariant value = dataHub().peek(topic);
    
    if (value.isValid() && value.canConvert<StockQuote>()) {
        return value.value<StockQuote>();
    }
    
    return std::nullopt;
}

std::optional<MarketSnapshot> DataHubPageBase::getCachedSnapshot(const QString& symbol) const
{
    QString topic = QString("market:snapshot:%1").arg(symbol);
    QVariant value = dataHub().peek(topic);
    
    if (value.isValid() && value.canConvert<MarketSnapshot>()) {
        return value.value<MarketSnapshot>();
    }
    
    return std::nullopt;
}

} // namespace UI
} // namespace WealthPilot