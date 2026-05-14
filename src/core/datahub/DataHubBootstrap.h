/**
 * @file DataHubBootstrap.h
 * @brief DataHub 启动引导 - 初始化数据中心
 *
 * @details 功能：
 * - 初始化 DataHub 单例
 * - 注册所有 Producer
 * - 配置 Topic 策略
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAHUBBOOTSTRAP_H
#define DATAHUBBOOTSTRAP_H

#include <QObject>
#include <memory>

namespace WealthPilot {

// 前向声明
namespace DataHub { class DataHub; }
namespace Producers { class MarketDataProducer; }

/**
 * @brief DataHub 启动引导器
 * 
 * 在应用启动时调用，完成数据中心的初始化
 */
class DataHubBootstrap : public QObject
{
    Q_OBJECT

public:
    explicit DataHubBootstrap(QObject* parent = nullptr);
    ~DataHubBootstrap() override;

    /**
     * @brief 初始化 DataHub
     * @return true 初始化成功
     */
    bool initialize();

    /**
     * @brief 关闭 DataHub
     */
    void shutdown();

    /**
     * @brief 获取行情 Producer
     */
    Producers::MarketDataProducer* marketProducer() const { return m_marketProducer; }

private:
    void setupDefaultPolicies();
    void registerProducers();

    Producers::MarketDataProducer* m_marketProducer = nullptr;
    bool m_initialized = false;
};

} // namespace WealthPilot

#endif // DATAHUBBOOTSTRAP_H