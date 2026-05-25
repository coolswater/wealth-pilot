/**
 * @file StockQuotesController.h
 * @brief 股票行情控制器 - MVP 模式示例
 * 
 * @details 用于 Widget 页面的业务逻辑处理：
 * - 数据刷新
 * - 搜索筛选
 * - 导出功能
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef STOCKQUOTESCONTROLLER_H
#define STOCKQUOTESCONTROLLER_H

#include "ControllerBase.h"
#include "data/models/QuoteModelBase.h"
#include "data/models/StockQuoteModel.h"
#include "data/market/StockDataSource.h"
#include <QSortFilterProxyModel>

// 前向声明
class StockDataSource;
class StockQuoteModel;

namespace WealthPilot
{
    /**
 * @brief 股票行情控制器
 *
 * @details 职责：
 * - 管理行情数据获取
 * - 处理搜索和筛选逻辑
 * - 提供导出功能
 * - 管理数据状态
 *
 * @example
 * @code
 * // StockQuotesPage.cpp
 * void StockQuotesPage::setupUI() {
 *     m_controller = new StockQuotesController(this);
 *
 *     // 连接信号
 *     connect(m_refreshBtn, &QPushButton::clicked,
 *             m_controller, &StockQuotesController::refreshData);
 *     connect(m_searchEdit, &QLineEdit::textChanged,
 *             m_controller, &StockQuotesController::searchData);
 *     connect(m_controller, &StockQuotesController::dataRefreshed,
 *             this, [this](int count) {
 *                 m_statusLabel->setText(QString("已加载 %1 条").arg(count));
 *             });
 *
 *     // 设置模型
 *     m_tableView->setModel(m_controller->proxyModel());
 * }
 * @endcode
 */
    class StockQuotesController : public ControllerBase
    {
        Q_OBJECT

    public:
        explicit StockQuotesController(QObject* parent = nullptr);
        ~StockQuotesController() override;

        // ========== 初始化 ==========

        void initialize() override;
        void cleanup() override;

        // ========== 模型访问 ==========

        /**
     * @brief 获取代理模型（用于 View 绑定）
     */
        QSortFilterProxyModel* proxyModel() const { return m_proxyModel; }

        /**
     * @brief 获取源模型
     */
        StockQuoteModel* sourceModel() const { return m_model; }

        // ========== 数据操作 ==========

        /**
     * @brief 刷新数据
     */
    Q_INVOKABLE void refreshData();

        /**
     * @brief 搜索数据
     * @param keyword 搜索关键词
     */
    Q_INVOKABLE void searchData(const QString& keyword);

        /**
     * @brief 按市场筛选
     * @param market 市场类型（"all", "sh", "sz", "cyb", "kcb"）
     */
    Q_INVOKABLE void filterByMarket(const QString& market);

        /**
     * @brief 按涨跌筛选
     * @param filter 涨跌筛选（"all", "up", "down", "flat"）
     */
    Q_INVOKABLE void filterByChange(const QString& filter);

        /**
     * @brief 排序
     * @param column 列索引
     * @param order 排序顺序
     */
    Q_INVOKABLE void sortByColumn(int column, Qt::SortOrder order = Qt::AscendingOrder);

        /**
     * @brief 清空数据
     */
    Q_INVOKABLE void clearData();

        // ========== 导出功能 ==========

        /**
     * @brief 导出为 CSV
     * @param filePath 文件路径
     */
    Q_INVOKABLE void exportToCSV(const QString& filePath);

        /**
     * @brief 导出为 Excel
     * @param filePath 文件路径
     */
    Q_INVOKABLE void exportToExcel(const QString& filePath);

        /**
     * @brief 复制选中行到剪贴板
     */
    Q_INVOKABLE void copyToClipboard(const QModelIndexList& indices);

        // ========== 统计信息 ==========

        /**
     * @brief 获取总数量
     */
    Q_INVOKABLE int totalCount() const;

        /**
     * @brief 获取筛选后数量
     */
    Q_INVOKABLE int filteredCount() const;

        /**
     * @brief 获取涨跌统计
     */
    Q_INVOKABLE QVariantMap getChangeStatistics() const;

        signals :
        /**
     * @brief 数据刷新完成
     */

        void dataRefreshed(int count);

        /**
     * @brief 数据筛选完成
     */
        void dataFiltered(int visibleCount, int totalCount);

        /**
     * @brief 搜索完成
     */
        void searchCompleted(int resultCount, const QString& keyword);

        /**
     * @brief 导出完成
     */
        void exportCompleted(const QString& filePath);

        /**
     * @brief 导出失败
     */
        void exportFailed(const QString& error);

        /**
     * @brief 数据加载中
     */
        void dataLoading(bool loading);

    private
        slots :
        // 数据源回调

        void onDataReceived(const QVector<StockQuote>& quotes);
        void onDataError(const QString& error);

    private:
        // 应用筛选
        void applyFilter();

        // ========== 成员变量 ==========

        StockDataSource* m_dataSource = nullptr;
        StockQuoteModel* m_model = nullptr;
        QSortFilterProxyModel* m_proxyModel = nullptr;

        // 筛选状态
        QString m_searchKeyword;
        QString m_marketFilter = "all";
        QString m_changeFilter = "all";

        // 缓存数据
        QVector<StockQuote> m_allQuotes;
    };
} // namespace WealthPilot

#endif // STOCKQUOTESCONTROLLER_H
