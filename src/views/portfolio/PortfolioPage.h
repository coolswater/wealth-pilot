/**
 * @file PortfolioPage.h
 * @brief 持仓页面 - 专业级投资组合管理
 *
 * @details 功能：
 * - 账户总览（总资产、今日盈亏、持仓收益、风险指标）
 * - 资产配置饼图 + 占比明细
 * - 净值走势图（支持时间范围切换）
 * - 持仓明细表格（股票/期货/基金分类）
 *
 * @author WealthPilot Team
 * @version 3.0.0
 */

#ifndef PORTFOLIOPAGE_H
#define PORTFOLIOPAGE_H

#include <ui/components/BasePage.h>
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
class QTimer;
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

/**
 * @brief 持仓数据结构
 */
struct PositionData {
    QString instrumentId;       ///< 合约代码
    QString instrumentName;     ///< 合约名称
    QString exchangeId;         ///< 交易所
    QString type;               ///< 类型：股�?期货/基金
    int volume = 0;             ///< 持仓数量
    double avgPrice = 0.0;      ///< 持仓均价
    double currentPrice = 0.0;  ///< 当前价格
    double marketValue = 0.0;   ///< 市�?
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
 */
class PositionTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
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

    // QAbstractTableModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 数据操作
    void setData(const QVector<PositionData>& data);
    void updatePrice(const QString& instrumentId, double price);
    void clear();

    // 汇总计算
    AccountSummary calculateSummary() const;

private:
    QVector<PositionData> m_data;
    QHash<QString, int> m_indexMap;  ///< 快速索引
};

/**
 * @brief 持仓页面 - 高性能优化版本
 */
class PortfolioPage : public WealthPilot::BasePage {
    Q_OBJECT

public:
    explicit PortfolioPage(QWidget* parent = nullptr);
    ~PortfolioPage() override;

    QString pageId() const override { return QStringLiteral("PortfolioPage"); }
    void initializePage() override;

    // 公共接口
    void refreshData();
    void setAccountSummary(const AccountSummary& summary);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onSearch(const QString& text);
    void onTimeRangeChanged(int index);
    void onTabChanged(int index);
    void onRowDoubleClicked(const QModelIndex& index);
    void updateRealTimeData();

private:
    void setupUI();
    void setupHeader();
    void setupSummaryCards();
    void setupMainContent();
    void setupAssetAllocation();
    void setupNetValueChart();
    void setupPositionTable();
    void setupConnections();
    void loadDemoData();
    void updateSummaryDisplay();
    void updateAssetAllocation();
    void updateNetValueChart(int days);

    struct Impl;
    std::unique_ptr<Impl> d;
};



 // PORTFOLIOPAGE_H

#endif
