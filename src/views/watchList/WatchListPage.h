/**
 * @file WatchListPage.h
 * @brief 自选股页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 个人自选股管理
 * - 实时行情展示
 * - 添加/删除自选股
 * - 通过 DataHub 订阅数据（自动生命周期管理）
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef WATCHLISTPAGE_H
#define WATCHLISTPAGE_H

#include <QTableView>
#include <QSortFilterProxyModel>
#include <QAbstractTableModel>
#include <memory>
#include "ui/components/DataHubPageBase.h"
#include "market/StockDataSource.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
QT_END_NAMESPACE

namespace WealthPilot {

/**
 * @brief 自选股表格模型
 *
 * @details 提供自选股数据的表格展示：
 * - 支持排序（使用 Qt::UserRole 存储原始数值）
 * - 涨跌颜色显示
 * - 数据更新时自动刷新视图
 */
class WatchListModel : public QAbstractTableModel {
    Q_OBJECT
public:
    /**
     * @brief 列枚举
     */
    enum Column {
        ColCode = 0,      ///< 代码
        ColName,          ///< 名称
        ColPrice,         ///< 最新价
        ColChange,        ///< 涨跌幅
        ColChangeAmount,  ///< 涨跌额
        ColVolume,        ///< 成交量
        ColAmount,        ///< 成交额
        ColHigh,          ///< 最高价
        ColLow,           ///< 最低价
        ColCount          ///< 列数
    };

    explicit WatchListModel(QObject* parent = nullptr);

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
    void setData(const QVector<StockQuote>& quotes);

    /**
     * @brief 更新单只股票数据
     * @param symbol 股票代码
     * @param quote 新的行情数据
     */
    void updateQuote(const QString& symbol, const StockQuote& quote);

    /**
     * @brief 添加股票
     * @param symbol 股票代码
     */
    void addSymbol(const QString& symbol);

    /**
     * @brief 删除股票
     * @param row 行号
     */
    void removeSymbol(int row);

    /**
     * @brief 清空数据
     */
    void clear();

    /**
     * @brief 获取所有股票代码
     */
    QStringList symbols() const;

    /**
     * @brief 根据代码查找行索引
     * @return -1 表示未找到
     */
    int findRowBySymbol(const QString& symbol) const;

private:
    QVector<StockQuote> m_data;      ///< 数据存储
    QSet<QString> m_symbolSet;       ///< 代码集合（防止重复）
    QHash<QString, int> m_symbolIndex; ///< 代码到行索引的映射

    /**
     * @brief 格式化成交量
     */
    static QString formatVolume(qint64 volume);

    /**
     * @brief 格式化金额
     */
    static QString formatMoney(double value);
};

/**
 * @brief 自选股页面
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 页面初始化时订阅自选股行情数据
 * - 页面销毁时自动取消订阅
 * - 通过 DataHub 接收实时数据更新
 *
 * 使用方式：
 * 1. 继承 DataHubPageBase 而不是 BasePage
 * 2. 在 initializePage() 中调用 subscribeQuote() 订阅数据
 * 3. 在回调中更新 UI 显示
 */
class WatchListPage : public DataHubPageBase {
    Q_OBJECT

public:
    explicit WatchListPage(QWidget* parent = nullptr);
    ~WatchListPage() override;

    // ========== 页面信息 ==========

    QString pageId() const override;
    QString pageName() const override { return QStringLiteral("自选股"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 加载本地保存的自选股列表
     * 3. 订阅 DataHub 行情数据
     */
    void initializePage() override;

    /**
     * @brief 页面激活时调用
     */
    void onPageActivated(const QVariantMap& params = {}) override;

    /**
     * @brief 页面停用时调用
     */
    void onPageDeactivated() override;

public slots:
    /**
     * @brief 添加股票到自选
     * @param symbol 股票代码
     * @param name 股票名称（可选）
     */
    void addStock(const QString& symbol, const QString& name = QString());

signals:
    /**
     * @brief 导航到K线页面信号
     * @param symbol 股票代码
     * @param params 参数
     */
    void navigateToKLinePage(const QString& symbol, const QVariantMap& params);

private slots:
    // ========== UI 交互槽函数 ==========

    /**
     * @brief 搜索文本改变
     */
    void onSearchChanged(const QString& text);

    /**
     * @brief 刷新按钮点击
     */
    void onRefreshData();

    /**
     * @brief 添加股票按钮点击
     */
    void onAddStock();

    /**
     * @brief 删除股票按钮点击
     */
    void onRemoveStock();

    /**
     * @brief 表格行双击
     */
    void onRowDoubleClicked(const QModelIndex& index);

private:
    // ========== UI 初始化 ==========

    void setupUI();
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 使用 subscribeQuote() 订阅自选股
     * 2. 使用模式订阅监听所有 market:quote:* 更新
     * 3. 回调函数中更新模型数据
     * 4. 页面销毁时自动取消订阅
     */
    void setupDataHubSubscriptions();

    // ========== 数据管理 ==========

    /**
     * @brief 加载本地保存的自选股列表
     */
    void loadWatchList();

    /**
     * @brief 保存自选股列表到本地
     */
    void saveWatchList();

    /**
     * @brief 通过 DataHub 请求数据
     */
    void requestStockData();

    // ========== 私有实现类（PIMPL） ==========
    class Impl;
    std::unique_ptr<Impl> d;

    // ========== DataHub 相关 ==========

    /**
     * @brief 已订阅的股票列表
     */
    QStringList m_subscribedSymbols;
};

} // namespace WealthPilot

#endif // WATCHLISTPAGE_H