/**
 * @file DashboardPage.h
 * @brief 金融行情综合看板页面 - 使用 DataHub 数据中心
 *
 * @details 页面布局结构：
 * - 顶部区域（25%）：指数大盘走势 - 分时图展示
 * - 中部区域（60%）：六宫格排行榜
 *   - 左上：沪A涨跌榜
 *   - 左中：沪5分钟涨跌榜
 *   - 中上：深A涨跌榜
 *   - 中中：深5分钟涨跌榜
 *   - 右上：行业/概念板块热力图
 * - 底部区域（40%）：综合信息区
 *   - 左下：自选股列表
 *   - 中下：24小时滚动新闻
 *   - 右下：资金流向统计
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅指数、排行榜、自选股数据
 * - 自动生命周期管理
 * - 统一数据刷新策略
 *
 * 迁移说明 (v6.0.0)：
 * - startAutoRefresh() → registerToDataHub()
 * - 独立 QTimer → DataHub 统一调度
 * - 保留 fallback 兼容旧代码
 *
 * @author WealthPilot Team
 * @version 6.0.0
 */

#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <presentation/components/DataHubPageBase.h>
#include "shared/types/MarketTypes.h"
#include "shared/types/NewsTypes.h"

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::TimeShareData;
using WealthPilot::MarketSnapshot;
using WealthPilot::NewsItem;
#include <QTableView>
#include <QAbstractTableModel>
#include <memory>
#include "infrastructure/config/Tokens.h"
#include "data/market/StockDataSource.h"
#include "data/market/NewsDataSource.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QComboBox;
class QTabWidget;
class QSplitter;
class QGridLayout;
class QFrame;
class QListWidgetItem;
QT_END_NAMESPACE

// ============================================================================
// 数据结构定义
// ============================================================================

/**
 * @brief 指数数据结构
 */
struct IndexData {
    QString code;               ///< 指数代码
    QString name;               ///< 指数名称
    double current = 0.0;       ///< 当前点位
    double change = 0.0;        ///< 涨跌点数
    double changePercent = 0.0; ///< 涨跌幅百分比
    double volume = 0.0;        ///< 成交量
    double amount = 0.0;        ///< 成交额
    QVector<double> prices;     ///< 分时价格序列
    QVector<double> volumes;    ///< 分时成交量序列
};

/**
 * @brief 股票排行数据结构
 */
struct StockRankData {
    QString code;               ///< 股票代码
    QString name;               ///< 股票名称
    double price = 0.0;         ///< 现价
    double change = 0.0;        ///< 涨跌额
    double changePercent = 0.0; ///< 涨跌幅
    double volume = 0.0;        ///< 成交量
    double amount = 0.0;        ///< 成交额
    double turnover = 0.0;      ///< 换手率
    double pe = 0.0;            ///< 市盈率
    int rank = 0;               ///< 排名
};

/**
 * @brief 板块数据结构
 */
struct SectorData {
    QString code;               ///< 板块代码
    QString name;               ///< 板块名称
    double changePercent = 0.0; ///< 涨跌幅
    int upCount = 0;            ///< 上涨家数
    int downCount = 0;          ///< 下跌家数
    double amount = 0.0;        ///< 成交额
    double marketCap = 0.0;     ///< 总市值
    int rank = 0;               ///< 排名
};

/**
 * @brief 新闻数据结构
 */
struct NewsData {
    QString time;               ///< 发布时间
    QString category;           ///< 分类（要闻/研报/公告）
    QString title;              ///< 新闻标题
    QString source;             ///< 来源
    int importance = 0;         ///< 重要程度（1-3）
};

/**
 * @brief 资金流向数据结构
 */
struct MoneyFlowData {
    QString code;               ///< 股票代码
    QString name;               ///< 股票名称
    double netInflow = 0.0;     ///< 净流入金额
    double netInflowPercent = 0.0; ///< 净流入占比
    double day3Inflow = 0.0;    ///< 3日净流入
    double day5Inflow = 0.0;    ///< 5日净流入
    int rank = 0;               ///< 排名
};

// ============================================================================
// 表格模型定义
// ============================================================================

/**
 * @brief 股票排行表格模型 - 紧凑型
 */
class StockRankModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColRank = 0,        ///< 排名
        ColCode,            ///< 代码
        ColName,            ///< 名称
        ColPrice,           ///< 现价
        ColChange,          ///< 涨跌幅
        ColChangeAmount,    ///< 涨跌额
        ColCount
    };

    explicit StockRankModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setData(const QVector<StockRankData>& data);
    void updateRow(int row, const StockRankData& data);
    void clear();

private:
    QVector<StockRankData> m_data;
    QHash<QString, int> m_codeIndex; ///< 代码到行索引的映射

    static QString formatValue(double value);
};

/**
 * @brief 自选股表格模型 - 详细型
 */
class WatchlistModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColRank = 0,
        ColCode,
        ColName,
        ColPrice,
        ColChange,
        ColChangeAmount,
        ColVolume,
        ColAmount,
        ColTurnover,
        ColPE,
        ColCount
    };

    explicit WatchlistModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setData(const QVector<StockRankData>& data);
    void updateRow(int row, const StockRankData& data);
    void clear();

private:
    QVector<StockRankData> m_data;
    QHash<QString, int> m_codeIndex;
};

/**
 * @brief 板块热力图模型
 */
class SectorHeatmapModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit SectorHeatmapModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setData(const QVector<SectorData>& data);
    void clear();

private:
    QVector<SectorData> m_data;
};

/**
 * @brief 资金流向表格模型
 */
class MoneyFlowModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColRank = 0,
        ColCode,            ///< 代码
        ColName,
        ColNetInflow,
        ColNetInflowPercent,
        ColDay3,
        ColDay5,
        ColCount
    };

    explicit MoneyFlowModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setData(const QVector<MoneyFlowData>& data);
    void clear();

private:
    QVector<MoneyFlowData> m_data;
};

// ============================================================================
// DashboardPage 主类
// ============================================================================

/**
 * @brief 金融行情综合看板页面 - 六宫格布局
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 指数数据订阅（上证、深证、创业板）
 * - 排行榜数据订阅（涨跌榜）
 * - 自选股数据订阅
 * - 新闻数据订阅
 *
 * 页面销毁时自动取消所有订阅
 */
class DashboardPage : public WealthPilot::DataHubPageBase {
    Q_OBJECT

public:
    explicit DashboardPage(QWidget* parent = nullptr);
    ~DashboardPage() override;

    QString pageId() const override { return QStringLiteral("DashboardPage"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    /**
     * @brief 刷新数据
     *
     * @note 通过 DataHub 请求刷新，而非直接调用数据源
     */
    void refreshData();

signals:
    /**
     * @brief 导航到股票K线页面信号
     */
    void navigateToStockKLine(const QString& symbol, const QString& name);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    // ========== UI 交互槽函数 ==========

    void onSectorTabChanged(int index);
    void onWatchlistFilterChanged(int index);
    void onMoneyFlowPeriodChanged(int index);
    void onRowDoubleClicked(const QModelIndex& index);
    void onMoneyFlowRowDoubleClicked(const QModelIndex& index);
    void onSectorRowDoubleClicked(const QModelIndex& index);
    void onNewsItemClicked(QListWidgetItem* item);

    // ========== 定时更新槽函数 ==========

    void updateTimeDisplay();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupHeader();
    void setupIndexPanel();
    void setupRankGrid();           ///< 六宫格排行榜
    void setupInfoPanel();          ///< 底部信息区
    void setupWatchlistPanel();
    void setupNewsPanel();
    void setupMoneyFlowPanel();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅的数据：
     * 1. 指数数据：market:quote:sh000001, sh000300, sz399001, sz399006
     * 2. 排行榜数据：通过模式订阅 market:rank:*
     * 3. 自选股数据：market:watchlist:*
     * 4. 新闻数据：news:*
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载 ==========

    void loadDataWithFallback();    ///< 缓存->数据库->网络数据源
    bool loadFromCache();           ///< 从缓存加载数据
    bool loadFromDatabase();        ///< 从数据库加载数据
    void loadFromNetwork();         ///< 从网络加载数据
    void saveToCache();             ///< 保存数据到缓存
    void saveToDatabase();          ///< 保存数据到数据库

    void loadDemoData();
    void loadRealData();            ///< 加载真实数据
    void loadLocalData();           ///< 加载本地缓存数据
    void loadIndexData();
    void loadRankData();
    void loadWatchlistData();
    void loadNewsData();
    void loadMoneyFlowData();
    void loadSectorData();

    // ========== UI 更新 ==========

    void updateTheme();             ///< 主题切换更新
    void updateRealTimeData();      ///< 实时数据更新
    void updateIndexDisplay();
    void updateSectorHeatmap();

    // ========== 数据处理 ==========

    void processIndexQuotes(const QVector<StockQuote>& quotes);
    void processRankQuotes(const QVector<StockQuote>& quotes);
    void onIndexQuotesReceived(const QVector<StockQuote>& quotes);
    void onRankQuotesReceived(const QVector<StockQuote>& quotes);
    void onWatchlistQuotesReceived(const QVector<StockQuote>& quotes);
    QVector<StockRankData> filterTopGainers(const QVector<StockQuote>& quotes, int count);
    QVector<StockRankData> filterTopLosers(const QVector<StockQuote>& quotes, int count);

    // ========== 数据存储 ==========

    void saveIndexDataToDb(const QVector<StockQuote>& quotes);
    void saveQuoteCacheToDb(const QVector<StockQuote>& quotes);
    void saveNewsToDb(const QVector<NewsItem>& news);
    bool checkAndLoadLocalData();

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    QStringList m_indexSymbols;     ///< 已订阅的指数代码
    QStringList m_watchlistSymbols; ///< 已订阅的自选股代码
};

// 注册数据类型以支持 QVariant 序列化
Q_DECLARE_METATYPE(IndexData)
Q_DECLARE_METATYPE(StockRankData)
Q_DECLARE_METATYPE(SectorData)
Q_DECLARE_METATYPE(NewsData)
Q_DECLARE_METATYPE(MoneyFlowData)
Q_DECLARE_METATYPE(QVector<IndexData>)
Q_DECLARE_METATYPE(QVector<StockRankData>)
Q_DECLARE_METATYPE(QVector<SectorData>)
Q_DECLARE_METATYPE(QVector<NewsData>)
Q_DECLARE_METATYPE(QVector<MoneyFlowData>)

#endif // DASHBOARDPAGE_H