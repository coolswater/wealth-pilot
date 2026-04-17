/**
 * @file FuturesKLinePage.h
 * @brief 期货K线页面 - 专业级K线图表和技术分析
 *
 * @details 布局结构：
 * +-----------------------------------------------------------------------------+
 * | MenuBar / ToolBar (周期切换、复权、画线、指标、显示模式等)                  |
 * +-----------------------------------------------------------------------------+
 * | +--------------------------------+---------------------------------------+ |
 * | |                                | 盘口信息（最新价、买卖价、涨跌幅）     | |
 * | |     中央区域                   | 统计信息（成交量、持仓量、资金等）     | |
 * | |     ChartWidget                +---------------------------------------+ |
 * | |     (K线/分时图 + 均线/指标)    | 分笔成交列表 (QTableView)              | |
 * | |                                |                                       | |
 * | +--------------------------------+---------------------------------------+ |
 * | | 状态栏 (账户信息、连接状态、坐标数值等)                              | |
 * +-----------------------------------------------------------------------------+
 */

#ifndef FUTURES_KLINE_PAGE_H
#define FUTURES_KLINE_PAGE_H

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QSplitter>
#include <QButtonGroup>
#include <QDateTime>

#include "core/base/BasePage.h"
#include "plugins/ICTPPlugin.h"
#include "ui/components/KLineChart.h"

// 前向声明 CTP 命名空间
namespace CTP {
    struct MarketData;
    class CTPService;
}

// Forward declarations
class MarketDepthWidget;
class TickTableView;
class ChartToolBar;
class ChartStatusBar;

/**
 * @brief K线周期枚举
 */
enum class KLinePeriod {
    Timeline,       // 分时图
    Minute1,        // 1分钟
    Minute5,        // 5分钟
    Minute15,       // 15分钟
    Minute30,       // 30分钟
    Hour1,          // 60分钟
    Day1,           // 日线
    Week1,          // 周线
    Month1,         // 月线
    Custom          // 自定义周期
};

/**
 * @brief 复权类型
 */
enum class AdjustmentType {
    None,           // 不复权
    Front,          // 前复权
    Back            // 后复权
};

/**
 * @brief 期货K线页面
 */
class FuturesKLinePage : public BasePage
{
    Q_OBJECT

public:
    explicit FuturesKLinePage(QWidget *parent = nullptr);
    ~FuturesKLinePage() override;

    QString pageId() const override { return "FuturesKLine"; }
    void initializePage() override;
    void refresh();

    // 合约设置
    void setInstrument(const QString& instrumentId, const QString& instrumentName = QString());
    QString instrument() const;

    // 周期设置
    void setPeriod(KLinePeriod period);
    KLinePeriod period() const;

    // 指标控制
    void setIndicatorEnabled(const QString& indicator, bool enabled);
    bool isIndicatorEnabled(const QString& indicator) const;

    // 页面激活回调
    void onPageActivated(const QVariantMap& params) override;

signals:
    // 交易信号
    void tradeRequested(const QString& instrumentId,
                       const QString& direction,
                       double price,
                       int volume);

    // 页面标题变化
    void pageTitleChanged(const QString& title);

    // 十字光标移动
    void crosshairMoved(const QDateTime& time, double price, const QString& info);

protected:
    void setupUI();
    void setupConnections();
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // CTP 行情数据槽（使用 CTP::MarketData）
    void onCtpMarketDataReceived(const CTP::MarketData& data);

    // 插件行情数据槽（使用 MarketData）
    void onMarketDataUpdated(const MarketData& data);
    void onKLineDataReceived(const QVector<KLineData>& data);
    void onTickReceived(const QString& time, double price, int volume, const QString& flag);

    // 工具栏槽
    void onPeriodChanged(KLinePeriod period);
    void onAdjustmentChanged(AdjustmentType type);
    void onIndicatorToggled(const QString& indicator, bool enabled);
    void onDrawToolSelected(const QString& tool);
    void onChartTypeChanged(const QString& type);

    // K线图交互槽
    void onCrosshairMoved(const QDateTime& time, double price);

private:
    // CTP 数据处理
    void subscribeMarketData();
    void requestKLineFromCache();
    void updateKLineFromTick(const CTP::MarketData& tick);
    QDateTime calculateBarTime(const QDateTime& tickTime, KLinePeriod period);

    // 指标计算
    void calculateIndicators();

    // 显示更新
    void updateQuoteDisplay(const MarketData& quote);
    void updateQuoteDisplayFromCtp(const CTP::MarketData& quote);
    void updateStatusBar();
    void updateWindowTitle();

    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief 图表工具栏
 */
class ChartToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit ChartToolBar(QWidget *parent = nullptr);
    ~ChartToolBar();

    void setCurrentPeriod(KLinePeriod period);
    void setCurrentAdjustment(AdjustmentType type);

signals:
    void periodChanged(KLinePeriod period);
    void adjustmentChanged(AdjustmentType type);
    void indicatorToggled(const QString& indicator, bool enabled);
    void drawToolSelected(const QString& tool);
    void chartTypeChanged(const QString& type);

private:
    void setupUI();
    void setupPeriodButtons();
    void setupToolButtons();
    QFrame* createSeparator();

    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief 盘口信息组件
 */
class MarketDepthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MarketDepthWidget(QWidget *parent = nullptr);
    ~MarketDepthWidget();

    void updateQuote(const MarketData& quote);
    void setInstrument(const QString& instrumentId, const QString& instrumentName);
    void clear();

private:
    void setupUI();
    void updatePriceColor(QLabel* label, double change);

    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief 分笔成交表格
 */
class TickTableView : public QTableWidget
{
    Q_OBJECT

public:
    explicit TickTableView(QWidget *parent = nullptr);
    ~TickTableView();

    void addTick(const QString& time, double price, int volume, const QString& flag);
    void clearTicks();
    void setMaxRows(int max);

private:
    void setupUI();
    void autoScrollToBottom();

    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief 图表状态栏
 */
class ChartStatusBar : public QWidget
{
    Q_OBJECT

public:
    explicit ChartStatusBar(QWidget *parent = nullptr);
    ~ChartStatusBar();

    void setAccountInfo(const QString& account, double available, double margin);
    void setConnectionStatus(const QString& status, const QColor& color);
    void setCoordinateInfo(const QString& info);
    void setCrosshairInfo(const QDateTime& time, double price, double volume);

private:
    void setupUI();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FUTURES_KLINE_PAGE_H
