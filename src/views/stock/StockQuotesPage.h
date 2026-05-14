/**
 * @file StockQuotesPage.h
 * @brief 股票行情页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 显示股票实时行情列表
 * - 支持搜索、筛选和排序
 * - 通过 DataHub 订阅行情数据（自动生命周期管理）
 * - 无独立 QTimer，由 DataHub 统一调度刷新
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef STOCKQUOTESPAGE_H
#define STOCKQUOTESPAGE_H

#include "ui/components/DataHubPageBase.h"
#include <QTableView>
#include <QAbstractTableModel>
#include <memory>

// 前向声明
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QSortFilterProxyModel;

namespace WealthPilot {

/**
 * @brief 股票行情数据结构
 * 
 * 用于表格展示的股票行情数据
 */
struct StockQuoteData
{
    QString symbol;         ///< 股票代码（如 sh600000）
    QString name;           ///< 股票名称
    double price = 0.0;     ///< 最新价
    double change = 0.0;    ///< 涨跌额
    double changePercent = 0.0; ///< 涨跌幅
    qint64 volume = 0;      ///< 成交量
    double turnover = 0.0;  ///< 成交额
    double high = 0.0;      ///< 最高价
    double low = 0.0;       ///< 最低价
    double open = 0.0;      ///< 开盘价
    double prevClose = 0.0; ///< 昨收价
};

/**
 * @brief 股票行情表格模型
 * 
 * @details 提供股票数据的表格展示：
 * - 支持排序（使用 Qt::UserRole 存储原始数值）
 * - 涨跌颜色显示（通过委托实现）
 * - 数据更新时自动刷新视图
 */
class StockQuoteModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /**
     * @brief 列枚举
     */
    enum Column
    {
        ColCode = 0,         ///< 代码
        ColName,             ///< 名称
        ColPrice,            ///< 最新价
        ColChange,           ///< 涨跌额
        ColChangePercent,    ///< 涨跌幅
        ColVolume,           ///< 成交量
        ColTurnover,         ///< 成交额
        ColHigh,             ///< 最高价
        ColLow,              ///< 最低价
        ColCount             ///< 列数
    };

    explicit StockQuoteModel(QObject* parent = nullptr);

    // ========== QAbstractTableModel 接口 ==========
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // ========== 数据操作 ==========
    
    /**
     * @brief 设置数据（批量更新）
     * @param quotes 股票行情数据列表
     */
    void setData(const QVector<StockQuoteData>& quotes);

    /**
     * @brief 更新单只股票数据
     * @param symbol 股票代码
     * @param quote 新的行情数据
     */
    void updateQuote(const QString& symbol, const StockQuoteData& quote);

    /**
     * @brief 清空数据
     */
    void clear();

    /**
     * @brief 获取指定行的股票数据
     */
    StockQuoteData getQuote(int row) const;

    /**
     * @brief 根据代码查找行索引
     * @return -1 表示未找到
     */
    int findRowBySymbol(const QString& symbol) const;

private:
    QVector<StockQuoteData> m_data; ///< 数据存储
    QHash<QString, int> m_symbolIndex; ///< 代码到行索引的映射（加速查找）

    /**
     * @brief 格式化成交量/成交额
     */
    static QString formatVolume(qint64 volume);
};

/**
 * @brief 股票行情页面
 * 
 * @details 继承 DataHubPageBase，自动管理数据订阅生命周期：
 * - 页面初始化时订阅行情数据
 * - 页面销毁时自动取消订阅
 * - 通过 DataHub 接收实时数据更新
 * 
 * 使用方式：
 * 1. 继承 DataHubPageBase 而不是 BasePage
 * 2. 在 initializePage() 中调用 subscribeQuote() 订阅数据
 * 3. 在回调中更新 UI 显示
 */
class StockQuotesPage : public DataHubPageBase {
    Q_OBJECT

public:
    explicit StockQuotesPage(QWidget* parent = nullptr);
    ~StockQuotesPage() override;

    // ========== 页面信息 ==========
    QString pageId() const override { return QStringLiteral("stock-quotes"); }
    QString pageName() const override { return QStringLiteral("股票行情"); }
    
    /**
     * @brief 初始化页面
     * 
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 行情数据
     * 3. 加载初始数据
     */
    void initializePage() override;

signals:
    /**
     * @brief 导航到K线页面信号
     * @param symbol 股票代码
     * @param name 股票名称
     */
    void navigateToKLinePage(const QString& symbol, const QString& name);

private slots:
    /**
     * @brief 搜索文本改变
     */
    void onSearchChanged(const QString& text);
    
    /**
     * @brief 筛选条件改变
     */
    void onFilterChanged(int index);
    
    /**
     * @brief 刷新按钮点击
     */
    void onRefreshData();
    
    /**
     * @brief 表格行双击
     */
    void onRowDoubleClicked(const QModelIndex& index);

private:
    // ========== UI 初始化 ==========
    void setupUI();
    void setupConnections();
    
    // ========== 数据订阅 ==========
    
    /**
     * @brief 设置 DataHub 数据订阅
     * 
     * @details 订阅流程：
     * 1. 使用 subscribeQuote() 订阅关注的股票
     * 2. 回调函数中更新模型数据
     * 3. 页面销毁时自动取消订阅
     */
    void setupDataHubSubscriptions();
    
    /**
     * @brief 加载演示数据
     */
    void loadDemoData();

    /**
     * @brief 应用筛选
     */
    void applyFilter();

    // ========== UI 组件 ==========
    QLineEdit* m_searchEdit = nullptr;       ///< 搜索框
    QComboBox* m_filterCombo = nullptr;      ///< 筛选下拉框
    QPushButton* m_refreshBtn = nullptr;     ///< 刷新按钮
    QTableView* m_tableView = nullptr;       ///< 表格视图
    StockQuoteModel* m_model = nullptr;      ///< 数据模型
    QSortFilterProxyModel* m_proxyModel = nullptr; ///< 代理模型（排序筛选）
    QLabel* m_statusLabel = nullptr;         ///< 状态标签

    // ========== 数据 ==========
    QVector<StockQuoteData> m_allData;       ///< 所有数据
    QStringList m_subscribedSymbols;         ///< 已订阅的股票代码列表
};

} // namespace WealthPilot

#endif // STOCKQUOTESPAGE_H