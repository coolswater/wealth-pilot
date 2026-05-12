/**
 * @file StockQuotesPage.h
 * @brief 股票行情页面
 * @details 显示股票实时行情列表，支持搜索、筛选和排序
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef STOCKQUOTESPAGE_H
#define STOCKQUOTESPAGE_H

#include "ui/components/BasePage.h"
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
 */
struct StockQuoteData
{
    QString symbol; ///< 股票代码（如 sh600000）
    QString name; ///< 股票名称
    double price = 0.0; ///< 最新价
    double change = 0.0; ///< 涨跌额
    double changePercent = 0.0; ///< 涨跌幅
    qint64 volume = 0; ///< 成交量
    double turnover = 0.0; ///< 成交额
    double high = 0.0; ///< 最高价
    double low = 0.0; ///< 最低价
    double open = 0.0; ///< 开盘价
    double prevClose = 0.0; ///< 昨收价
};

/**
 * @brief 股票行情表格模型
 * @details 提供股票数据的表格展示，支持排序和涨跌颜色显示
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
        ColCode = 0, ///< 代码
        ColName, ///< 名称
        ColPrice, ///< 最新价
        ColChange, ///< 涨跌额
        ColChangePercent, ///< 涨跌幅
        ColVolume, ///< 成交量
        ColTurnover, ///< 成交额
        ColHigh, ///< 最高价
        ColLow, ///< 最低价
        ColCount ///< 列数
    };

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit StockQuoteModel(QObject* parent = nullptr);

    /**
     * @brief 获取行数
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief 获取列数
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief 获取数据
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /**
     * @brief 获取表头数据
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief 设置数据
     * @param quotes 股票行情数据列表
     */
    void setData(const QVector<StockQuoteData>& quotes);

    /**
     * @brief 清空数据
     */
    void clear();

    /**
     * @brief 获取指定行的股票数据
     */
    StockQuoteData getQuote(int row) const;

private:
    QVector<StockQuoteData> m_data; ///< 数据存储
};

/**
 * @brief 股票行情页面
 * @details 显示股票实时行情列表，支持搜索、筛选和排序功能
 */
class StockQuotesPage : public BasePage {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit StockQuotesPage(QWidget* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~StockQuotesPage() override;

    /**
     * @brief 获取页面ID
     */
    QString pageId() const override { return QStringLiteral("stock-quotes"); }
    
    /**
     * @brief 获取页面名称
     */
    QString pageName() const override { return QStringLiteral("股票行情"); }
    
    /**
     * @brief 初始化页面
     */
    void initializePage() override;

signals :
/**
     * @brief 导航到K线页面信号
     * @param symbol 股票代码
     * @param name 股票名称
     */
    void navigateToKLinePage(const QString& symbol, const QString& name);

private slots:
    /**
     * @brief 搜索文本改变槽函数
     */
    void onSearchChanged(const QString& text);
    
    /**
     * @brief 筛选条件改变槽函数
     */
    void onFilterChanged(int index);
    
    /**
     * @brief 刷新数据槽函数
     */
    void onRefreshData();
    
    /**
     * @brief 行双击槽函数
     */
    void onRowDoubleClicked(const QModelIndex& index);

private:
    /**
     * @brief 初始化UI
     */
    void setupUI();

    /**
     * @brief 初始化连接
     */
    void setupConnections();
    
    /**
     * @brief 加载演示数据
     */
    void loadDemoData();

    /**
     * @brief 应用筛选
     */
    void applyFilter();

    // UI 组件
    QLineEdit* m_searchEdit = nullptr;       ///< 搜索框
    QComboBox* m_filterCombo = nullptr;      ///< 筛选下拉框
    QPushButton* m_refreshBtn = nullptr;     ///< 刷新按钮
    QTableView* m_tableView = nullptr;       ///< 表格视图
    StockQuoteModel* m_model = nullptr; ///< 数据模型
    QSortFilterProxyModel* m_proxyModel = nullptr; ///< 代理模型（用于排序和筛选）
    QLabel* m_statusLabel = nullptr;         ///< 状态标签

    // 数据
    QVector<StockQuoteData> m_allData; ///< 所有数据
};

} // namespace WealthPilot

#endif // STOCKQUOTESPAGE_H
