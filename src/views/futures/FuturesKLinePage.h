/**
 * @file FuturesKLinePage.h
 * @brief 期货K线页面 - 专业级K线图表和技术分析
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
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef FUTURES_KLINE_PAGE_H
#define FUTURES_KLINE_PAGE_H

#include "core/base/BasePage.h"
#include "core/types/MarketTypes.h"  // 使用统一的类型定义
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
 * @details 专业级K线图表页面，提供：
 * - 多周期K线图表（分时、1分、5分、15分、30分、60分、日线、周线、月线）
 * - 技术指标叠加（MA、MACD、RSI、KDJ、BOLL等）
 * - 实时盘口深度
 * - 分笔成交记录
 * - 十字光标交互
 * - CTP实时行情对接
 *
 * @example
 * @code
 * FuturesKLinePage* page = new FuturesKLinePage();
 * page->setInstrument("IF2501", "沪深300指数期货");
 * page->setPeriod(KLinePeriod::Minute15);
 * page->setIndicatorEnabled("MA5", true);
 * @endcode
 */
class FuturesKLinePage : public BasePage
{
    Q_OBJECT

public:
    // ========== 构造与析构 ==========

    /**
     * @brief 构造函数
     * @param parent 父控件
     */
    explicit FuturesKLinePage(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~FuturesKLinePage() override;

    // ========== BasePage 接口实现 ==========

    QString pageId() const override { return "FuturesKLine"; }
    void initializePage() override;

    // ========== 公共接口 ==========

    /**
     * @brief 设置合约
     * @param instrumentId 合约代码
     * @param instrumentName 合约名称
     */
    void setInstrument(const QString& instrumentId, const QString& instrumentName = QString());

    /**
     * @brief 获取当前合约代码
     * @return 合约代码
     */
    QString instrument() const;

    /**
     * @brief 设置K线周期
     * @param period K线周期
     */
    void setPeriod(KLinePeriod period);

    /**
     * @brief 获取当前K线周期
     * @return K线周期
     */
    KLinePeriod period() const;

    /**
     * @brief 设置指标启用状态
     * @param indicator 指标名称
     * @param enabled 是否启用
     */
    void setIndicatorEnabled(const QString& indicator, bool enabled);

    /**
     * @brief 获取指标启用状态
     * @param indicator 指标名称
     * @return 是否启用
     */
    bool isIndicatorEnabled(const QString& indicator) const;

    /**
     * @brief 刷新页面数据
     */
    void refresh();

    // ========== 页面生命周期 ==========

    void onPageActivated(const QVariantMap& params) override;

signals:
    /**
     * @brief 交易请求信号
     * @param instrumentId 合约代码
     * @param direction 方向（"buy" 或 "sell"）
     * @param price 价格
     * @param volume 数量
     */
    void tradeRequested(const QString& instrumentId,
                       const QString& direction,
                       double price,
                       int volume);

    /**
     * @brief 页面标题变化信号
     * @param title 新标题
     */
    void pageTitleChanged(const QString& title);

    /**
     * @brief 十字光标移动信号
     * @param time 时间
     * @param price 价格
     * @param info 附加信息
     */
    void crosshairMoved(const QDateTime& time, double price, const QString& info);

protected:
    /**
     * @brief 大小改变事件
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // ========== CTP 数据槽 ==========

    /**
     * @brief CTP行情数据接收槽
     * @param data 行情数据
     */
    void onCtpMarketDataReceived(const CTP::MarketData& data);

    /**
     * @brief 插件行情数据接收槽
     * @param data 行情数据
     */
    void onMarketDataUpdated(const MarketData& data);

    /**
     * @brief K线数据接收槽
     * @param data K线数据
     */
    void onKLineDataReceived(const QVector<KLineData>& data);

    /**
     * @brief 分笔成交接收槽
     * @param time 时间
     * @param price 价格
     * @param volume 成交量
     * @param flag 方向
     */
    void onTickReceived(const QString& time, double price, int volume, const QString& flag);

    // ========== 工具栏槽 ==========

    void onPeriodChanged(KLinePeriod period);
    void onAdjustmentChanged(AdjustmentType type);
    void onIndicatorToggled(const QString& indicator, bool enabled);
    void onDrawToolSelected(const QString& tool);
    void onChartTypeChanged(const QString& type);

    // ========== 图表交互槽 ==========

    void onCrosshairMoved(const QDateTime& time, double price);

private:
    // ========== 初始化方法 ==========

    void setupUI();
    void setupConnections();
    void setupServices();

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

    // ========== PIMPL ==========

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FUTURES_KLINE_PAGE_H
