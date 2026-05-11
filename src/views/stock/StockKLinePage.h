/**
 * @file StockKLinePage.h
 * @brief 股票K线图页面 - 支持 Widgets/QML 混合渲染
 */

#ifndef STOCKKLINEPAGE_H
#define STOCKKLINEPAGE_H

#include "ui/components/BasePage.h"
#include "ui/components/KLineChart.h"
#include "ui/components/StockInfoPanel.h"
#include "market/StockDataSource.h"
#include "core/types/MarketTypes.h"
#include "analysis/chanlun/ChanLunIntegration.h"
#include <QWidget>
#include <QVector>
#include <QDateTime>
#include <memory>

class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QTabWidget;
class QSplitter;
class QmlKLineWidget;

/**
 * @brief K线周期枚举
 */
enum class StockKLinePeriod {
    Min1 = 0,
    Min5,
    Min15,
    Min30,
    Min60,
    Day,
    Week,
    Month
};

/**
 * @brief 图表类型
 */
enum class ChartType {
    KLine,      // K线图
    TimeShare   // 分时图
};

/**
 * @brief 渲染引擎类型
 */
enum class RenderEngine {
    Widgets,    // 传统 Widgets 渲染
    QML         // QML GPU 加速渲染
};

/**
 * @brief 性能统计
 */
struct PerformanceStats {
    qint64 renderTimeMs = 0;        // 渲染耗时
    qint64 dataLoadTimeMs = 0;      // 数据加载耗时
    int frameRate = 0;              // 帧率
    double memoryMB = 0;            // 内存占用
    QString engineName;             // 引擎名称
};

/**
 * @brief 股票K线图页面
 */
class StockKLinePage : public WealthPilot::BasePage
{
    Q_OBJECT

public:
    explicit StockKLinePage(QWidget* parent = nullptr);
    ~StockKLinePage() override;

    QString pageId() const override { return "stock-kline"; }
    QString pageName() const override { return QStringLiteral("股票K线"); }

    void setStock(const QString& stockCode, const QString& stockName = QString());
    QString stockCode() const { return m_stockCode; }
    QString stockName() const { return m_stockName; }
    
    void setPeriod(StockKLinePeriod period);
    void setChartType(ChartType type);
    void setRenderEngine(RenderEngine engine);
    
    // 性能统计
    PerformanceStats lastPerformanceStats() const { return m_lastStats; }
    void runPerformanceBenchmark(int iterations = 10);

signals:
    void stockChanged(const QString& stockCode);
    void periodChanged(int period);
    void renderEngineChanged(int engine);
    void performanceStatsChanged(const PerformanceStats& stats);

private slots:
    void onChartTypeChanged(int index);
    void onPeriodChanged(int index);
    void onMainIndicatorChanged(int index);
    void onSubIndicatorChanged(int index);
    void onRenderEngineChanged(int index);
    void onRefresh();
    void onCrosshairMoved(const QDateTime& time, double price);
    void onKLineInfoChanged(const KLineData& kline, int index);
    
    // 数据源回调
    void onKLineReceived(const QString& symbol, const QVector<KLineData>& data);
    void onTimeShareReceived(const QString& symbol, const QVector<TimeShareData>& data);
    void onRealtimeQuoteReceived(const QString& symbol, const StockQuote& quote);
    void onRealtimeKLineUpdate(const QString& symbol, const RealtimeKLineUpdate& update);
    
    // 实时更新控制
    void startRealtimeUpdate();
    void stopRealtimeUpdate();

private:
    void setupUI();
    void setupConnections();
    
    // 数据加载流程：缓存 → 数据库 → 网络数据源
    void loadDataWithFallback();
    bool loadFromCache();
    bool loadFromDatabase();
    void loadFromNetwork();
    void saveToCache();
    void saveToDatabase();
    
    // 分时图数据加载
    void loadTimeShareWithFallback();
    bool loadTimeShareFromCache();
    bool loadTimeShareFromDatabase();
    void loadTimeShareFromNetwork();
    void generateDemoTimeShareData();
    void saveTimeShareToCache();
    void saveTimeShareToDatabase();
    
    // 辅助方法
    QString cacheKey() const;
    QString timeShareCacheKey() const;
    void updateInfoLabel(const KLineData& kline);
    KLinePeriod toKLinePeriod(StockKLinePeriod period) const;
    
    // 性能测量
    void measureRenderPerformance();
    void updatePerformanceDisplay();

    QString m_stockCode;
    QString m_stockName;
    StockKLinePeriod m_period = StockKLinePeriod::Day;
    ChartType m_chartType = ChartType::KLine;
    RenderEngine m_renderEngine = RenderEngine::Widgets;
    
    // UI组件
    QLabel* m_stockNameLabel = nullptr;
    QTabWidget* m_chartTypeTab = nullptr;
    QComboBox* m_periodCombo = nullptr;
    QComboBox* m_mainIndicatorCombo = nullptr;
    QComboBox* m_subIndicatorCombo = nullptr;
    QComboBox* m_renderEngineCombo = nullptr;
    QPushButton* m_refreshBtn = nullptr;
    QPushButton* m_benchmarkBtn = nullptr;
    QLabel* m_infoLabel = nullptr;
    QLabel* m_performanceLabel = nullptr;
    
    // 图表组件 - Widgets
    KLineChart* m_klineChart = nullptr;
    QWidget* m_timeShareWidget = nullptr;
    
    // 图表组件 - QML
    QmlKLineWidget* m_qmlKLineChart = nullptr;
    
    // 信息面板
    StockInfoPanel* m_infoPanel = nullptr;
    
    // 数据源
    StockDataSource* m_dataSource = nullptr;
    
    // 缠论分析
    WealthPilot::ChanLun::ChanLunIntegration* m_chanLun = nullptr;
    
    // 性能统计
    PerformanceStats m_lastStats;
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STOCKKLINEPAGE_H
