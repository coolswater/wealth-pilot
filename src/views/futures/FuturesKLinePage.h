/**
 * @file FuturesKLinePage.h
 * @brief 期货K线页面 - 使用 DataHub 数据中心
 *
 * @details 布局结构：
 * +-----------------------------------------------------------------------------+
 * | ChartToolBar (周期切换、复权、指标、画线、图表类型)                          |
 * +-----------------------------------------------------------------------------+
 * | +--------------------------------+---------------------------------------+ |
 * | |                                | MarketDepthWidget (盘口信息)           | |
 * | |     KLineChart                 | - 最新价、涨跌幅                       | |
 * | |     (K线/分时图 + 均线/指标)    | - 买卖五档                             | |
 * | |                                +---------------------------------------+ |
 * | |                                | TickTableView (分笔成交)               | |
 * | |                                | - 时间、价格、成交量、方向              | |
 * | +--------------------------------+---------------------------------------+ |
 * | ChartStatusBar (账户信息、连接状态、坐标数值)                              |
 * +-----------------------------------------------------------------------------+
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅K线数据
 * - 自动生命周期管理
 * - CTP 实时行情对接
 *
 * @author WealthPilot Team
 * @version 3.0.0
 */

#ifndef FUTURES_KLINE_PAGE_H
#define FUTURES_KLINE_PAGE_H

#include "ui/components/DataHubPageBase.h"
#include "shared/types/MarketTypes.h"
#include "ui/components/KLineChart.h"
#include "ui/components/ChartToolBar.h"
#include "ui/components/MarketDepthWidget.h"
#include "ui/components/TickTableView.h"
#include "ui/components/ChartStatusBar.h"
#include "plugins/ICTPPlugin.h"

#include <QSplitter>
#include <QDateTime>
#include <memory>

// 前向声明
namespace CTP {
    struct MarketData;
    class CTPService;
}

class ICTPPlugin;
class IAIPlugin;

/**
 * @brief 期货K线页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅K线数据（market:kline:{instrument}:{period}）
 * - 订阅实时行情（market:quote:{instrument}）
 * - 订阅盘口深度（market:depth:{instrument}）
 * - 页面销毁时自动取消订阅
 */
class FuturesKLinePage : public WealthPilot::DataHubPageBase
{
    Q_OBJECT

public:
    // ========== 构造与析构 ==========

    explicit FuturesKLinePage(QWidget *parent = nullptr);
    ~FuturesKLinePage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return "FuturesKLine"; }
    QString pageName() const override { return QStringLiteral("期货K线"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub K线数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    // ========== 公共接口 ==========

    void setInstrument(const QString& instrumentId, const QString& instrumentName = QString());
    QString instrument() const;
    void setPeriod(KLinePeriod period);
    KLinePeriod period() const;
    void setIndicatorEnabled(const QString& indicator, bool enabled);
    bool isIndicatorEnabled(const QString& indicator) const;
    void refresh();

    // ========== 页面生命周期 ==========

    void onPageActivated(const QVariantMap& params) override;

signals:
    void tradeRequested(const QString& instrumentId,
                       const QString& direction,
                       double price,
                       int volume);
    void pageTitleChanged(const QString& title);
    void crosshairMoved(const QDateTime& time, double price, const QString& info);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // ========== CTP 数据槽 ==========

    void onCtpMarketDataReceived(const CTP::MarketData& data);
    void onMarketDataUpdated(const MarketData& data);
    void onKLineDataReceived(const QVector<WealthPilot::KLineData>& data);
    void onTickReceived(const QString& time, double price, int volume, const QString& flag);

    // ========== 工具栏槽 ==========

    void onPeriodChanged(WealthPilot::KLinePeriod period);
    void onAdjustmentChanged(WealthPilot::AdjustmentType type);
    void onMainIndicatorChanged(const QString& indicator);
    void onSubIndicatorChanged(const QString& indicator);
    void onDrawToolSelected(const QString& tool);
    void onChartTypeChanged(const QString& type);

    // ========== 图表交互槽 ==========

    void onCrosshairMoved(const QDateTime& time, double price);

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupConnections();
    void setupServices();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅K线数据（market:kline:{instrument}:{period}）
     * 2. 订阅实时行情（market:quote:{instrument}）
     * 3. 订阅盘口深度（market:depth:{instrument}）
     * 4. 回调函数中更新图表
     */
    void setupDataHubSubscriptions();

    // ========== 数据处理方法 ==========

    void subscribeMarketData();
    void requestKLineFromCache();
    void updateKLineFromTick(const CTP::MarketData& tick);
    QDateTime calculateBarTime(const QDateTime& tickTime, KLinePeriod period);

    // ========== 指标计算 ==========

    void calculateIndicators();

    // ========== 显示更新 ==========

    void updateQuoteDisplay(const MarketData& quote);
    void updateQuoteDisplayFromCtp(const CTP::MarketData& quote);
    void updateStatusBar();
    void updateWindowTitle();

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 当前订阅的合约ID
     */
    QString m_currentInstrument;
};

#endif // FUTURES_KLINE_PAGE_H