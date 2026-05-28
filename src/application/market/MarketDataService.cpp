/**
 * @file MarketDataService.cpp
 * @brief 市场数据应用服务实现
 */

#include "MarketDataService.h"
#include "data/datahub/DataHub.h"
#include "shared/utils/Logger.h"
#include <QTimer>

namespace WealthPilot {
namespace Application {

struct MarketDataService::Impl {
    QStringList activeSubscriptions;
};

MarketDataService::MarketDataService()
    : d(std::make_unique<Impl>())
{
    LOG_INFO("MarketDataService initialized");
}

MarketDataService::~MarketDataService() = default;

void MarketDataService::getQuote(const QString& symbol,
                                  std::function<void(const QVariant&)> callback)
{
    auto& hub = DataHub::DataHub::instance();

    // 订阅并等待数据（DataHub 会在有缓存时立即回调）
    QString topic = QString("market:quote:%1").arg(symbol);
    hub.subscribe(this, topic, [this, symbol, callback](const QVariant& data) {
        emit quoteUpdated(symbol, data);
        if (callback) callback(data);
    });

    d->activeSubscriptions.append(symbol);
}

void MarketDataService::getKLine(const QString& symbol, int period, int count,
                                  std::function<void(const QVariant&)> callback)
{
    Q_UNUSED(period)
    Q_UNUSED(count)
    // TODO: 实现 K 线数据获取
    Q_UNUSED(callback)
    LOG_DEBUG(QString("getKLine: %1").arg(symbol));
}

QString MarketDataService::subscribeQuote(const QString& symbol,
                                           std::function<void(const QVariant&)> callback)
{
    auto& hub = DataHub::DataHub::instance();
    QString topic = QString("market:quote:%1").arg(symbol);

    hub.subscribe(this, topic, [this, symbol, callback](const QVariant& data) {
        emit quoteUpdated(symbol, data);
        if (callback) callback(data);
    });

    d->activeSubscriptions.append(symbol);
    return symbol;
}

void MarketDataService::unsubscribe(const QString& subscriptionId)
{
    auto& hub = DataHub::DataHub::instance();
    QString topic = QString("market:quote:%1").arg(subscriptionId);
    hub.unsubscribe(this);
    d->activeSubscriptions.removeOne(subscriptionId);
}

void MarketDataService::preloadData(const QStringList& symbols)
{
    LOG_INFO(QString("Preloading %1 symbols").arg(symbols.size()));
    for (const QString& symbol : symbols) {
        subscribeQuote(symbol, nullptr);
    }
}

void MarketDataService::refresh()
{
    // DataHub 自动管理刷新，无需手动请求
    LOG_DEBUG(QString("Refresh requested for %1 subscriptions").arg(d->activeSubscriptions.size()));
}

} // namespace Application
} // namespace WealthPilot
