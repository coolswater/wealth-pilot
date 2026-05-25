/**
 * @file DataHubPageBase.h
 * @brief DataHub 页面基类 - 简化页面订阅操作
 *
 * @details 功能：
 * - 封装 DataHub 订阅逻辑
 * - 自动生命周期管理
 * - 提供便捷的订阅方法
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAHUBPAGEBASE_H
#define DATAHUBPAGEBASE_H

#include "presentation/components/BasePage.h"
#include "core/datahub/DataHub.h"
#include "shared/types/MarketTypes.h"  // 直接包含类型定义
#include <QVariant>
#include <optional>

namespace WealthPilot {
namespace UI {

// 前向声明
using WealthPilot::StockQuote;
using WealthPilot::MarketSnapshot;
using WealthPilot::KLineData;
using WealthPilot::TimeShareData;

/**
 * @brief DataHub 页面基类
 * 
 * 继承此类的页面可以方便地订阅 DataHub 数据
 * 页面销毁时自动取消所有订阅
 */
class DataHubPageBase : public BasePage
{
    Q_OBJECT

public:
    explicit DataHubPageBase(QWidget* parent = nullptr);
    virtual ~DataHubPageBase() override;

protected:
    /**
     * @brief 获取 DataHub 实例
     */
    DataHub::DataHub& dataHub() const { return DataHub::DataHub::instance(); }

    /**
     * @brief 订阅 Topic
     * @param topic Topic 名称
     * @param slot 数据回调
     * @return 连接对象
     * 
     * @note 页面销毁时自动取消订阅
     */
    QMetaObject::Connection subscribe(
        const QString& topic,
        std::function<void(const QVariant&)> slot);

    /**
     * @brief 订阅 Topic（泛型版本）
     * @tparam T 数据类型
     */
    template<typename T>
    QMetaObject::Connection subscribe(
        const QString& topic,
        std::function<void(const T&)> slot);

    /**
     * @brief 订阅股票行情
     * @param symbol 股票代码
     * @param slot 回调函数
     */
    QMetaObject::Connection subscribeQuote(
        const QString& symbol,
        std::function<void(const StockQuote&)> slot);

    /**
     * @brief 订阅行情快照
     * @param symbol 合约代码
     * @param slot 回调函数
     */
    QMetaObject::Connection subscribeSnapshot(
        const QString& symbol,
        std::function<void(const MarketSnapshot&)> slot);

    /**
     * @brief 订阅 K 线数据
     * @param symbol 代码
     * @param period 周期
     * @param slot 回调函数
     */
    QMetaObject::Connection subscribeKLine(
        const QString& symbol,
        const QString& period,
        std::function<void(const QVector<KLineData>&)> slot);

    /**
     * @brief 请求刷新数据
     * @param topic Topic 名称
     * @param force 是否强制刷新
     */
    void requestData(const QString& topic, bool force = false);

    /**
     * @brief 批量请求刷新
     */
    void requestData(const QStringList& topics, bool force = false);

    /**
     * @brief 获取缓存的股票行情
     */
    std::optional<StockQuote> getCachedQuote(const QString& symbol) const;

    /**
     * @brief 获取缓存的行情快照
     */
    std::optional<MarketSnapshot> getCachedSnapshot(const QString& symbol) const;

private:
    // 存储订阅的 Topic 列表（用于调试）
    QStringList m_subscribedTopics;
};

// ========== 模板实现 ==========

template<typename T>
QMetaObject::Connection DataHubPageBase::subscribe(
    const QString& topic,
    std::function<void(const T&)> slot)
{
    m_subscribedTopics.append(topic);
    return dataHub().subscribe<T>(this, topic, slot);
}

} // namespace UI
} // namespace WealthPilot

// ========== 命名空间别名 ==========
// 为方便使用，在 WealthPilot 命名空间中添加别名
namespace WealthPilot {
    // 使用 DataHubPageBase 时无需指定 UI 命名空间
    using DataHubPageBase = UI::DataHubPageBase;
}

#endif // DATAHUBPAGEBASE_H