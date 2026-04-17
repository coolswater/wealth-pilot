/**
 * @file DashboardPage.h
 * @brief 金融行情综合看板页面 - 专业级六宫格布局
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
 * @author WealthPilot Team
 * @version 4.0.0
 */

#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <core/base/BasePage.h>
#include <QTableView>
#include <QAbstractTableModel>
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
class QTabWidget;
class QSplitter;
class QTimer;
class QGridLayout;
class QFrame;
QT_END_NAMESPACE

// ============================================================================
// 颜色常量 - 金融软件专业配色
// ============================================================================

namespace DashboardColors {
    // 背景色
    constexpr const char* BgMain = "#0d1117";       ///< 主背景（极深灰）
    constexpr const char* BgCard = "#161b22";       ///< 卡片背景
    constexpr const char* BgHeader = "#21262d";     ///< 表头背景
    constexpr const char* BgHover = "#30363d";      ///< 悬停背景
    constexpr const char* BgBorder = "#30363d";     ///< 边框颜色
    
    // 文字色
    constexpr const char* TextPrimary = "#c9d1d9";   ///< 主文字（浅灰白）
    constexpr const char* TextSecondary = "#8b949e"; ///< 次要文字（中灰）
    constexpr const char* TextTertiary = "#6e7681";  ///< 三级文字
    
    // 涨跌色（金融标准：红涨绿跌）
    constexpr const char* Up = "#ff4d4f";      ///< 涨 - 正红
    constexpr const char* UpBg = "#ff4d4f20";  ///< 涨背景（透明红）
    constexpr const char* Down = "#00b578";    ///< 跌 - 正绿
    constexpr const char* DownBg = "#00b57820"; ///< 跌背景（透明绿）
    constexpr const char* Flat = "#8b949e";    ///< 平 - 灰色
    
    // 特殊高亮
    constexpr const char* LimitUp = "#ff4d4f40";   ///< 涨停背景
    constexpr const char* LimitDown = "#00b57840"; ///< 跌停背景
    
    // 主色调
    constexpr const char* Primary = "#3b82f6";  ///< 主蓝色
    constexpr const char* Warning = "#f97316";  ///< 警告橙色
    constexpr const char* Info = "#58a6ff";     ///< 信息蓝色
}

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
    void clear();

private:
    QVector<StockRankData> m_data;
    
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
    void clear();

private:
    QVector<StockRankData> m_data;
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
 */
class DashboardPage : public BasePage {
    Q_OBJECT

public:
    explicit DashboardPage(QWidget* parent = nullptr);
    ~DashboardPage() override;

    QString pageId() const override { return QStringLiteral("DashboardPage"); }
    void initializePage() override;

    // 公共接口
    void refreshData();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onSearchChanged(const QString& text);
    void onMarketChanged(int index);
    void onSectorTabChanged(int index);
    void onWatchlistFilterChanged(int index);
    void onMoneyFlowPeriodChanged(int index);
    void onRowDoubleClicked(const QModelIndex& index);
    void updateRealTimeData();
    void updateTimeDisplay();

private:
    void setupUI();
    void setupHeader();
    void setupIndexPanel();
    void setupRankGrid();           ///< 六宫格排行榜
    void setupInfoPanel();          ///< 底部信息区
    void setupWatchlistPanel();
    void setupNewsPanel();
    void setupMoneyFlowPanel();
    void setupConnections();
    
    // 数据加载
    void loadDemoData();
    void loadIndexData();
    void loadRankData();
    void loadWatchlistData();
    void loadNewsData();
    void loadMoneyFlowData();
    void loadSectorData();
    
    // UI更新
    void updateIndexDisplay();
    void updateSectorHeatmap();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // DASHBOARDPAGE_H
