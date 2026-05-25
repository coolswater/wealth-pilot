/**
 * @file StockKLinePage.h
 * @brief 股票K线图页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 支持 Widgets/QML 混合渲染
 * - 多周期切换（1分、5分、日线等）
 * - 技术指标叠加
 * - 缠论分析
 * - 实时行情更新
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅K线数据
 * - 自动生命周期管理
 * - 实时K线更新
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef STOCKKLINEPAGE_H
#define STOCKKLINEPAGE_H

#include "presentation/components/DataHubPageBase.h"
#include "presentation/components/KLineChart.h"
#include "presentation/components/StockInfoPanel.h"
#include "data/market/StockDataSource.h"
#include "shared/types/MarketTypes.h"
#include "core/domain/analysis/legacy/chanlun/ChanLunIntegration.h"
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
    KLine,      ///< K线图
    TimeShare   ///< 分时图
};

/**
 * @brief 渲染引擎类型
 */
enum class RenderEngine {
    Widgets,    ///< 传统 Widgets 渲染
    QML         ///< QML GPU 加速渲染
};

/**
 * @brief K线图性能统计
 */
struct KLinePerformanceStats {
    qint64 renderTimeMs = 0;        ///< 渲染耗时
    qint64 dataLoadTimeMs = 0;      ///< 数据加载耗时
    int frameRate = 0;              ///< 帧率
    double memoryMB = 0;            ///< 内存占用
    QString engineName;             ///< 引擎名称
};

/**
 * @brief 股票K线图页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅K线数据（market:kline:{symbol}:{period}）
 * - 订阅实时行情（market:quote:{symbol}）
 * - 页面销毁时自动取消订阅
 */
class StockKLinePage : public WealthPilot::DataHubPageBase
{
    Q_OBJECT

public:
    explicit StockKLinePage(QWidget* parent = nullptr);
    ~StockKLinePage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return "stock-kline"; }
    QString pageName() const override { return QStringLiteral("股票K线"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub K线数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    // ========== 股票设置 ==========

    void setStock(const QString& stockCode, const QString& stockName = QString());
    QString stockCode() const { return m_stockCode; }
    QString stockName() const { return m_stockName; }

    // ========== 周期和类型设置 ==========

    void setPeriod(StockKLinePeriod period);
    void setChartType(ChartType type);
    void setRenderEngine(RenderEngine engine);

    // ========== 性能统计 ==========

    KLinePerformanceStats lastPerformanceStats() const { return m_lastStats; }
    void runPerformanceBenchmark(int iterations = 10);

signals:
    void stockChanged(const QString& stockCode);
    void periodChanged(int period);
    void renderEngineChanged(int engine);
    void performanceStatsChanged(const KLinePerformanceStats& stats);

private slots:
    // ========== UI 交互槽函数 ==========

    void onChartTypeChanged(int index);
    void onPeriodChanged(int index);
    void onMainIndicatorChanged(int index);
    void onSubIndicatorChanged(int index);
    void onRenderEngineChanged(int index);
    void onRefresh();
    void onCrosshairMoved(const QDateTime& time, double price);
    void onKLineInfoChanged(const KLineData& kline, int index);

    // ========== 实时更新控制 ==========

    void startRealtimeUpdate();
    void stopRealtimeUpdate();
    
    // ========== 数据接收槽函数 ==========
    
    void onKLineReceived(const QString& symbol, const QVector<KLineData>& data);
    void onTimeShareReceived(const QString& symbol, const QVector<TimeShareData>& data);
    void onRealtimeQuoteReceived(const QString& symbol, const StockQuote& quote);
    void onRealtimeKLineUpdate(const QString& symbol, const RealtimeKLineUpdate& update);

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅K线数据（market:kline:{symbol}:{period}）
     * 2. 订阅实时行情（market:quote:{symbol}）
     * 3. 回调函数中更新图表
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载流程 ==========

    void loadDataWithFallback();
    bool loadFromCache();
    bool loadFromDatabase();
    void loadFromNetwork();
    void saveToCache();
    void saveToDatabase();

    // ========== 分时图数据加载 ==========

    void loadTimeShareWithFallback();
    bool loadTimeShareFromCache();
    bool loadTimeShareFromDatabase();
    void loadTimeShareFromNetwork();
    void generateDemoTimeShareData();
    void saveTimeShareToCache();
    void saveTimeShareToDatabase();

    // ========== 辅助方法 ==========

    QString cacheKey() const;
    QString timeShareCacheKey() const;
    void updateInfoLabel(const KLineData& kline);
    KLinePeriod toKLinePeriod(StockKLinePeriod period) const;

    // ========== 性能测量 ==========

    void measureRenderPerformance();
    void updatePerformanceDisplay();

    // ========== 成员变量 ==========

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
    KLinePerformanceStats m_lastStats;

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STOCKKLINEPAGE_H