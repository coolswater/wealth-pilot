/**
 * @file CTPPlugin.h
 * @brief CTP插件实现 - 基于插件接口的CTP服务
 *
 * @details 功能：
 * - 实现ICTPPlugin接口
 * - 集成ServiceLocator依赖注入
 * - 集成EnvironmentConfig配置管理
 * - 集成CacheManager缓存系统
 * - 高性能行情数据处理
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef CTPPLUGIN_H
#define CTPPLUGIN_H

#include "../plugins/ICTPPlugin.h"
#include "../core/ServiceLocator.h"
#include "../core/EnvironmentConfig.h"
#include "../core/CacheManager.h"
#include <QTimer>
#include <QQueue>
#include <memory>

// 前向声明
struct CThostFtdcDepthMarketDataField;
struct CThostFtdcOrderField;
struct CThostFtdcTradeField;

namespace CTP {

/**
 * @brief CTP插件实现类
 * @note 不使用 Q_PLUGIN_METADATA，作为普通服务类使用
 */
class CTPPlugin : public ICTPPlugin
{
    Q_OBJECT
    Q_INTERFACES(ICTPPlugin)

public:
    explicit CTPPlugin();
    ~CTPPlugin() override;

    // ========== IPlugin接口实现 ==========

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

    // ========== ICTPPlugin接口实现 ==========

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
    // IPlugin信号
    void stateChanged(PluginState newState);
    void errorOccurred(const QString& error);
    void configurationChanged();

    // ICTPPlugin信号
    void connected();
    void disconnected();
    void marketDataUpdated(const MarketData& data);
    void orderUpdated(const OrderData& order);
    void accountUpdated(const AccountData& account);

private:
    // 内部实现
    class Impl;
    std::unique_ptr<Impl> d;
    
    // 状态管理
    PluginState m_state;
    QJsonObject m_config;
    
    // 性能优化：行情数据缓存
    QMap<QString, MarketData> m_marketDataCache;
    mutable QMutex m_cacheMutex;
    
    // 批量订阅缓冲（性能优化）
    QQueue<QString> m_subscribeBuffer;
    QTimer* m_subscribeBufferTimer;
    void flushSubscribeBuffer();
    
    // 集成CacheManager
    void updateMarketDataCache(const QString& instrumentId, const MarketData& data);
    MarketData getCachedMarketData(const QString& instrumentId) const;
    
    // 集成EnvironmentConfig
    void loadEnvironmentConfig();
    
    // 辅助方法
    void setState(PluginState newState);
    QString generateOrderRef();
};

} // namespace CTP

#endif // CTPPLUGIN_H
