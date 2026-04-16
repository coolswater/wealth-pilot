/**
 * @file FuturesKLinePage.h
 * @brief 期货K线详情页 - 高性能K线图和技术分�? *
 * @details 功能�? * - 实时K线图表（支持多周期）
 * - 技术指标分析（MA、MACD、RSI、KDJ等）
 * - 成交量分�? * - 实时行情数据
 * - 交易操作面板
 * - 智能分析建议
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef FUTURES_KLINE_PAGE_H
#define FUTURES_KLINE_PAGE_H

#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QSplitter>

#include "core/BasePage.h"
#include "plugins/ICTPPlugin.h"
#include "ui/components/KLineChart.h"

// 前向声明
class TechnicalIndicatorPanel;
class TradingPanel;
class RealtimeQuoteWidget;

/**
 * @brief K线周期枚�? */
enum class KLinePeriod {
    Minute1,        // 1分钟
    Minute5,        // 5分钟
    Minute15,       // 15分钟
    Minute30,       // 30分钟
    Hour1,          // 1小时
    Hour4,          // 4小时
    Day1,           // 日线
    Week1,          // 周线
    Month1          // 月线
};

/**
 * @brief 期货K线详情页
 */
class FuturesKLinePage : public BasePage
{
    Q_OBJECT

public:
    /**
     * @brief 构造函�?     * @param parent 父控�?     */
    explicit FuturesKLinePage(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~FuturesKLinePage() override;

    /**
     * @brief 页面ID
     */
    QString pageId() const override { return "FuturesKLine"; }

    /**
     * @brief 初始化页�?     */
    void initializePage() override;

    /**
     * @brief 设置合约
     * @param instrumentId 合约代码
     */
    void setInstrument(const QString& instrumentId);

    /**
     * @brief 获取合约代码
     */
    QString instrument() const;

    /**
     * @brief 设置K线周�?     */
    void setPeriod(KLinePeriod period);

    /**
     * @brief 刷新数据
     */
    void refresh();

    /**
     * @brief 页面激活（重写�?     */
    void onPageActivated(const QVariantMap& params) override;

signals:
    /**
     * @brief 交易信号
     */
    void tradeRequested(const QString& instrumentId, 
                       const QString& direction,
                       double price, 
                       int volume);

protected:
    /**
     * @brief 初始化UI
     */
    void setupUI();

    /**
     * @brief 连接信号
     */
    void connectSignals();

private slots:
    /**
     * @brief 周期切换
     */
    void onPeriodChanged(int index);

    /**
     * @brief 技术指标切�?     */
    void onIndicatorToggled(const QString& indicator, bool enabled);

    /**
     * @brief K线数据更�?     */
    void onKLineDataReceived(const QVector<KLineData>& data);

    /**
     * @brief 实时行情更新
     */
    void onQuoteUpdated(const MarketData& quote);

    /**
     * @brief 买入按钮点击
     */
    void onBuyClicked();

    /**
     * @brief 卖出按钮点击
     */
    void onSellClicked();

    /**
     * @brief AI分析请求
     */
    void onAIAnalysisRequested();

private:
    /**
     * @brief 创建顶部工具�?     */
    QWidget* createToolbar();

    /**
     * @brief 创建K线图表区�?     */
    QWidget* createChartArea();

    /**
     * @brief 创建右侧面板
     */
    QWidget* createRightPanel();

    /**
     * @brief 创建底部面板
     */
    QWidget* createBottomPanel();

    /**
     * @brief 加载K线数�?     */
    void loadKLineData();

    /**
     * @brief 计算技术指�?     */
    void calculateIndicators();

    /**
     * @brief 更新行情显示
     */
    void updateQuoteDisplay(const MarketData& quote);

    /**
     * @brief 获取周期文本
     */
    QString periodText(KLinePeriod period) const;

    /**
     * @brief 获取周期分钟�?     */
    int periodMinutes(KLinePeriod period) const;

    // PIMPL实现
    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief 技术指标面�? */
class TechnicalIndicatorPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TechnicalIndicatorPanel(QWidget *parent = nullptr);
    ~TechnicalIndicatorPanel();
    
    /**
     * @brief 设置指标数据
     */
    void setIndicatorData(const QString& name, const QMap<QString, double>& data);
    
    /**
     * @brief 清空数据
     */
    void clearData();

signals:
    void indicatorToggled(const QString& indicator, bool enabled);

private:
    void setupUI();
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief 交易面板
 */
class TradingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TradingPanel(QWidget *parent = nullptr);
    ~TradingPanel();
    
    /**
     * @brief 设置合约信息
     */
    void setInstrument(const QString& instrumentId);
    
    /**
     * @brief 设置价格
     */
    void setPrice(double price);
    
    /**
     * @brief 设置可用资金
     */
    void setAvailable(double available);

signals:
    void buyClicked();
    void sellClicked();

private:
    void setupUI();
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief 实时行情组件
 */
class RealtimeQuoteWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RealtimeQuoteWidget(QWidget *parent = nullptr);
    ~RealtimeQuoteWidget();
    
    /**
     * @brief 更新行情
     */
    void updateQuote(const MarketData& quote);
    
    /**
     * @brief 设置合约
     */
    void setInstrument(const QString& instrumentId);

private:
    void setupUI();
    void updateStyle();
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FUTURES_KLINE_PAGE_H
