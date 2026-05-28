/**
 * @file DataHubBootstrap.cpp
 * @brief DataHub 启动引导实现
 */

#include "DataHubBootstrap.h"
#include "DataHub.h"
#include "MarketDataProducer.h"
#include <QDebug>

namespace WealthPilot {

DataHubBootstrap::DataHubBootstrap(QObject* parent)
    : QObject(parent)
{
}

DataHubBootstrap::~DataHubBootstrap()
{
    shutdown();
}

bool DataHubBootstrap::initialize()
{
    if (m_initialized) {
        return true;
    }

    LOG_DEBUG("[DataHubBootstrap] Initializing DataHub...");

    // 获取 DataHub 实例（确保初始化）
    [[maybe_unused]] auto& hub = DataHub::DataHub::instance();

    // 创建 Producer
    m_marketProducer = new Producers::MarketDataProducer(this);

    // 设置默认策略
    setupDefaultPolicies();

    // 注册 Producer
    registerProducers();

    m_initialized = true;
    LOG_DEBUG("[DataHubBootstrap] DataHub initialized successfully");

    return true;
}

void DataHubBootstrap::shutdown()
{
    if (!m_initialized) {
        return;
    }

    LOG_DEBUG("[DataHubBootstrap] Shutting down DataHub...");

    // 1. 先注销 Producer（停止数据生产）
    auto& hub = DataHub::DataHub::instance();
    if (m_marketProducer) {
        hub.unregisterProducer(m_marketProducer);
        m_marketProducer->deleteLater();  // 安全删除
        m_marketProducer = nullptr;
    }

    // 2. 调用 DataHub 的显式 shutdown（关键！）
    hub.shutdown();

    m_initialized = false;
    LOG_DEBUG("[DataHubBootstrap] DataHub shutdown complete");
}

void DataHubBootstrap::setupDefaultPolicies()
{
    auto& hub = DataHub::DataHub::instance();

    // 股票行情策略：30秒缓存，5秒最小间隔
    DataHub::TopicPolicy quotePolicy;
    quotePolicy.ttlMs = 30000;
    quotePolicy.minIntervalMs = 5000;
    quotePolicy.pushOnly = false;
    quotePolicy.priority = 10;
    hub.setPolicyPattern("market:quote:*", quotePolicy);

    // 期货行情策略：10秒缓存，3秒最小间隔（更实时）
    DataHub::TopicPolicy futuresPolicy;
    futuresPolicy.ttlMs = 10000;
    futuresPolicy.minIntervalMs = 3000;
    futuresPolicy.pushOnly = false;
    futuresPolicy.priority = 20;  // 更高优先级
    hub.setPolicyPattern("market:futures:*", futuresPolicy);

    // K线数据策略：5分钟缓存，30秒最小间隔
    DataHub::TopicPolicy klinePolicy;
    klinePolicy.ttlMs = 300000;
    klinePolicy.minIntervalMs = 30000;
    klinePolicy.pushOnly = false;
    klinePolicy.priority = 5;
    hub.setPolicyPattern("market:kline:*", klinePolicy);

    // 分时数据策略：1分钟缓存，10秒最小间隔
    DataHub::TopicPolicy timesharePolicy;
    timesharePolicy.ttlMs = 60000;
    timesharePolicy.minIntervalMs = 10000;
    timesharePolicy.pushOnly = false;
    timesharePolicy.priority = 8;
    hub.setPolicyPattern("market:timeshare:*", timesharePolicy);

    // 行情快照策略
    DataHub::TopicPolicy snapshotPolicy;
    snapshotPolicy.ttlMs = 30000;
    snapshotPolicy.minIntervalMs = 5000;
    snapshotPolicy.pushOnly = false;
    snapshotPolicy.priority = 10;
    hub.setPolicyPattern("market:snapshot:*", snapshotPolicy);

    LOG_DEBUG("[DataHubBootstrap] Default policies configured");
}

void DataHubBootstrap::registerProducers()
{
    auto& hub = DataHub::DataHub::instance();

    // 注册行情 Producer
    hub.registerProducer(m_marketProducer);

    LOG_DEBUG("[DataHubBootstrap] Producers registered");
}

} // namespace WealthPilot