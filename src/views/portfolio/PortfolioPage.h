/**
 * @file PortfolioPage.h
 * @brief 持仓页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 账户总览（总资产、今日盈亏、持仓收益、风险指标）
 * - 资产配置饼图 + 占比明细
 * - 净值走势图（支持时间范围切换）
 * - 持仓明细表格（股票/期货/基金分类）
 * - 成交记录
 * - 条件单
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅持仓行情数据
 * - 自动生命周期管理
 * - 实时价格更新
 *
 * @author WealthPilot Team
 * @version 4.0.0
 */

#ifndef PORTFOLIOPAGE_H
#define PORTFOLIOPAGE_H

#include <ui/components/DataHubPageBase.h>
#include <QTableView>
#include <QAbstractTableModel>
#include <QChartView>
#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
class QTabWidget;
class QSplitter;
class QProgressBar;
class QPieSeries;
class QLineSeries;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE
namespace QtCharts {
    class QChartView;
    class QPieSeries;
    class QLineSeries;
    class QChart;
}
QT_END_NAMESPACE

namespace WealthPilot
{
    class TradeHistoryPage;
    class ConditionOrderPage;
}

/**
 * @brief 持仓数据结构
 */
struct PositionData {
    QString instrumentId;       ///< 合约代码
    QString instrumentName;     ///< 合约名称
    QString exchangeId;         ///< 交易所
    QString type;               ///< 类型：股票/期货/基金
    int volume = 0;             ///< 持仓数量
    double avgPrice = 0.0;      ///< 持仓均价
    double currentPrice = 0.0;  ///< 当前价格
    double marketValue = 0.0;   ///< 市值
    double profitLoss = 0.0;    ///< 盈亏
    double profitLossPercent = 0.0; ///< 盈亏比例
    double cost = 0.0;          ///< 成本
};

/**
 * @brief 账户汇总数据
 */
struct AccountSummary {
    double totalAssets = 0.0;       ///< 总资产
    double available = 0.0;         ///< 可用资金
    double marketValue = 0.0;       ///< 持仓市值
    double margin = 0.0;            ///< 占用保证金
    double dailyPnL = 0.0;          ///< 今日盈亏
    double totalPnL = 0.0;          ///< 总盈亏
    double returnRate = 0.0;        ///< 收益率
    double riskLevel = 0.0;         ///< 风险度 (0-100)
};

/**
 * @brief 资产配置项
 */
struct AssetAllocation {
    QString name;       ///< 名称
    double value = 0.0; ///< 金额
    double percent = 0.0; ///< 占比
    QColor color;       ///< 颜色
};

/**
 * @brief 持仓表格模型 - 高性能优化
 *
 * @details 特性：
 * - 使用 QHash 快速索引
 * - 支持增量更新
 * - 自动计算汇总
 */
class PositionTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    /**
     * @brief 列枚举
     */
    enum Column {
        ColInstrument = 0,  ///< 合约
        ColVolume,          ///< 持仓
        ColAvgPrice,        ///< 均价
        ColCurrentPrice,    ///< 现价
        ColMarketValue,     ///< 市值
        ColPnL,             ///< 盈亏
        ColPnLPercent,      ///< 盈亏%
        ColCount
    };

    explicit PositionTableModel(QObject* parent = nullptr);
    ~PositionTableModel() override = default;

    // ========== QAbstractTableModel 接口 ==========

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // ========== 数据操作 ==========

    /**
     * @brief 设置数据（批量更新）
     */
    void setData(const QVector<PositionData>& data);

    /**
     * @brief 更新单个合约价格
     * @param instrumentId 合约ID
     * @param price 新价格
     */
    void updatePrice(const QString& instrumentId, double price);

    /**
     * @brief 更新持仓数据
     */
    void updatePosition(const PositionData& pos);

    /**
     * @brief 清空数据
     */
    void clear();

    /**
     * @brief 计算汇总
     */
    AccountSummary calculateSummary() const;
    
    /**
     * @brief 获取所有合约ID列表
     */
    QStringList instrumentIds() const;

private:
    QVector<PositionData> m_data;      ///< 数据存储
    QHash<QString, int> m_indexMap;    ///< 快速索引
};

/**
 * @brief 持仓页面 - DataHub 集成版本
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅持仓合约的实时行情
 * - 自动更新盈亏计算
 * - 页面销毁时自动取消订阅
 */
class PortfolioPage : public WealthPilot::DataHubPageBase {
    Q_OBJECT

public:
    explicit PortfolioPage(QWidget* parent = nullptr);
    ~PortfolioPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override { return QStringLiteral("PortfolioPage"); }
    QString pageName() const override { return QStringLiteral("账户"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 持仓数据
     * 3. 加载初始数据
     */
    void initializePage() override;

    /**
     * @brief 刷新数据
     */
    void refreshData();

    /**
     * @brief 更新实时数据
     */
    void updateRealTimeData();

    /**
     * @brief 设置账户汇总
     */
    void setAccountSummary(const AccountSummary& summary);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    // ========== UI 交互槽函数 ==========

    void onSearch(const QString& text);
    void onTimeRangeChanged(int index);
    void onTabChanged(int index);
    void onRowDoubleClicked(const QModelIndex& index);
    void updateTheme();

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupHeader();
    void setupSummaryCards();
    void setupMainContent();
    void setupAssetAllocation();
    void setupNetValueChart();
    QFrame* setupPositionTable();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 获取持仓列表中的所有合约ID
     * 2. 使用 subscribeQuote() 订阅行情
     * 3. 价格更新时自动计算盈亏
     */
    void setupDataHubSubscriptions();

    // ========== 数据加载 ==========

    void loadDemoData();
    void loadRealData();

    // ========== UI 更新 ==========

    void updateSummaryDisplay();
    void updateAssetAllocation();
    void updateNetValueChart(int days);

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的合约列表
     */
    QStringList m_subscribedInstruments;
};

#endif // PORTFOLIOPAGE_H