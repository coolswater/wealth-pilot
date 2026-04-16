/**
 * @file CTPPlugin.h
 * @brief CTP Plugin Implementation - Based on plugin interface
 *
 * @details Features:
 * - Implements ICTPPlugin interface
 * - Integrates ServiceLocator dependency injection
 * - Integrates EnvironmentConfig configuration management
 * - Integrates CacheManager caching system
 * - High-performance market data processing
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef CTPPLUGIN_H
#define CTPPLUGIN_H

#include "ICTPPlugin.h"
#include "../core/di/ServiceLocator.h"
#include "../core/config/EnvironmentConfig.h"
#include "../core/cache/CacheManager.h"
#include <QTimer>
#include <QQueue>
#include <memory>

// Forward declarations
struct CThostFtdcDepthMarketDataField;
struct CThostFtdcOrderField;
struct CThostFtdcTradeField;

namespace CTP {

/**
 * @brief CTP Plugin Implementation Class
 */
class CTPPlugin : public ICTPPlugin
{
    Q_OBJECT
    Q_INTERFACES(ICTPPlugin)

public:
    explicit CTPPlugin();
    ~CTPPlugin() override;

    // ========== IPlugin Interface Implementation ==========

    PluginMetaData metaData() const override;
    PluginState state() const override;

    bool load() override;
    bool initialize(const QJsonObject& config = QJsonObject()) override;
    bool start() override;
    void stop() override;
    void unload() override;

    QJsonObject configuration() const override;
    void setConfiguration(const QJsonObject& config) override;

    bool checkDependencies() const override;
    QStringList dependencies() const override;

    // ========== ICTPPlugin Interface Implementation ==========

    bool connect(const QString& brokerId,
                const QString& userId,
                const QString& password,
                const QString& marketFront,
                const QString& tradeFront) override;
    
    void disconnect() override;
    bool isConnected() const override;

    bool subscribeMarketData(const QStringList& instruments) override;
    void unsubscribeMarketData(const QStringList& instruments) override;
    
    ::MarketData getMarketData(const QString& instrumentId) const override;
    QMap<QString, ::MarketData> getAllMarketData() const override;

    QString sendOrder(const QString& instrumentId,
                     const QString& direction,
                     const QString& offsetFlag,
                     double price,
                     int volume) override;
    
    bool cancelOrder(const QString& orderId) override;
    QList<OrderData> queryOrders() override;
    QList<OrderData> queryTrades() override;

    AccountData queryAccount() override;
    QList<AccountData> queryPositions() override;

signals:
    // IPlugin signals
    void stateChanged(PluginState newState);
    void errorOccurred(const QString& error);
    void configurationChanged();

    // ICTPPlugin signals
    void connected();
    void disconnected();
    void marketDataUpdated(const MarketData& data);
    void orderUpdated(const OrderData& order);
    void accountUpdated(const AccountData& account);

private:
    // Internal implementation
    class Impl;
    std::unique_ptr<Impl> d;
    
    // State management
    PluginState m_state;
    QJsonObject m_config;
    
    // Performance optimization: market data cache
    QMap<QString, MarketData> m_marketDataCache;
    mutable QMutex m_cacheMutex;
    
    // Batch subscription buffer (performance optimization)
    QQueue<QString> m_subscribeBuffer;
    QTimer* m_subscribeBufferTimer;
    void flushSubscribeBuffer();
    
    // Integrate CacheManager
    void updateMarketDataCache(const QString& instrumentId, const MarketData& data);
    MarketData getCachedMarketData(const QString& instrumentId) const;
    
    // Integrate EnvironmentConfig
    void loadEnvironmentConfig();
    
    // Helper methods
    void setState(PluginState newState);
    QString generateOrderRef();
};

} // namespace CTP

#endif // CTPPLUGIN_H
