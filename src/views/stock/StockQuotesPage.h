/**
 * @file StockQuotesPage.h
 * @brief 股票行情页面 - 使用 Controller 模式
 * @details 显示股票实时行情列表，支持搜索、筛选和排序
 * @author WealthPilot Team
 * @version 2.0.0 - MVVM 重构
 */

#ifndef STOCKQUOTESPAGE_H
#define STOCKQUOTESPAGE_H

#include "ui/components/BasePage.h"
#include "controllers/StockQuotesController.h"
#include <QTableView>
#include <memory>

// 前向声明
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;

namespace WealthPilot {

/**
 * @brief 股票行情页面
 * @details 显示股票实时行情列表，支持搜索、筛选和排序功能
 * 
 * @details 架构说明：
 * - View (本类): 只负责 UI 渲染和用户交互
 * - Controller: 处理业务逻辑（数据获取、筛选、导出）
 * - Model: 数据存储和展示
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

    /**
     * @brief 获取 Controller
     */
    StockQuotesController* controller() const { return m_controller; }

    signals:
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

    // ========== Controller 信号处理 ==========

    /**
     * @brief 数据刷新完成
     */
    void onDataRefreshed(int count);

    /**
     * @brief 数据筛选完成
     */
    void onDataFiltered(int visibleCount, int totalCount);

    /**
     * @brief 搜索完成
     */
    void onSearchCompleted(int resultCount, const QString& keyword);

    /**
     * @brief 数据加载状态变化
     */
    void onDataLoading(bool loading);

    /**
     * @brief 导出完成
     */
    void onExportCompleted(const QString& filePath);

    /**
     * @brief 错误发生
     */
    void onErrorOccurred(const QString& error);

private:
    /**
     * @brief 初始化UI
     */
    void setupUI();

    /**
     * @brief 初始化 Controller
     */
    void setupController();

    /**
     * @brief 初始化连接
     */
    void setupConnections();
    
    /**
     * @brief 更新状态显示
     */
    void updateStatus();

    // ========== UI 组件 ==========
    QLineEdit* m_searchEdit = nullptr;       ///< 搜索框
    QComboBox* m_filterCombo = nullptr;      ///< 筛选下拉框
    QPushButton* m_refreshBtn = nullptr;     ///< 刷新按钮
    QPushButton* m_exportBtn = nullptr; ///< 导出按钮
    QTableView* m_tableView = nullptr;       ///< 表格视图
    QLabel* m_statusLabel = nullptr;         ///< 状态标签
    QLabel* m_countLabel = nullptr; ///< 数量标签

    // ========== Controller ==========
    StockQuotesController* m_controller = nullptr; ///< 业务逻辑控制器
};

} // namespace WealthPilot

#endif // STOCKQUOTESPAGE_H