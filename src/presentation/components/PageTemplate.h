/**
 * @file PageTemplate.h
 * @brief 页面模板系统 - 统一页面布局和样式
 *
 * @details 功能：
 * - 提供标准页面布局模板
 * - 统一卡片、表格、图表样式
 * - 减少页面重复代码
 * - 支持自定义扩展
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PAGETEMPLATE_H
#define PAGETEMPLATE_H

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTableView>
#include <QSplitter>
#include <QTabWidget>
#include <QGridLayout>
#include <QScrollArea>
#include <functional>

namespace WealthPilot {
namespace UI {

/**
 * @brief 卡片样式配置
 */
struct CardStyle {
    QString backgroundColor = QStringLiteral("#2C2D33");
    QString borderColor = QStringLiteral("#3A3B42");
    int borderRadius = 8;
    int padding = 16;
    QString titleColor = QStringLiteral("#FFFFFF");
    QString valueColor = QStringLiteral("#FFFFFF");
    QString detailColor = QStringLiteral("#8E8E93");
};

/**
 * @brief 表格样式配置
 */
struct TableStyle {
    QString headerBackground = QStringLiteral("#1E1F24");
    QString headerTextColor = QStringLiteral("#8E8E93");
    QString rowBackground = QStringLiteral("#2C2D33");
    QString rowTextColor = QStringLiteral("#FFFFFF");
    QString gridLineColor = QStringLiteral("#3A3B42");
    int rowHeight = 40;
    int headerHeight = 36;
};

/**
 * @brief 页面模板工厂
 *
 * @details 提供创建标准 UI 组件的静态方法
 */
class PageTemplate
{
public:
    // ========== 页面头部 ==========

    /**
     * @brief 创建标准页面头部
     * @param parent 父组件
     * @param title 页面标题
     * @param showSearch 是否显示搜索框
     * @param showRefresh 是否显示刷新按钮
     * @return 创建的头部 Frame
     */
    static QFrame* createPageHeader(QWidget* parent,
                                     const QString& title,
                                     bool showSearch = true,
                                     bool showRefresh = true);

    /**
     * @brief 创建带导航的页面头部
     */
    static QFrame* createPageHeaderWithNav(QWidget* parent,
                                            const QString& title,
                                            const QStringList& navItems);

    // ========== 汇总卡片 ==========

    /**
     * @brief 创建单个汇总卡片
     * @param parent 父组件
     * @param title 标题
     * @param value 初始值
     * @param detail 详情（可选）
     * @return 创建的卡片 Frame
     */
    static QFrame* createSummaryCard(QWidget* parent,
                                      const QString& title,
                                      const QString& value,
                                      const QString& detail = QString());

    /**
     * @brief 创建汇总卡片组（水平排列）
     * @param parent 父组件
     * @param cards 卡片配置列表
     * @return 包含所有卡片的 Frame
     */
    static QFrame* createSummaryCardRow(QWidget* parent,
                                          const QVector<QPair<QString, QString>>& cards);

    /**
     * @brief 创建汇总卡片网格
     * @param parent 父组件
     * @param cards 卡片配置列表
     * @param columns 列数
     * @return 包含所有卡片的 Frame
     */
    static QFrame* createSummaryCardGrid(QWidget* parent,
                                          const QVector<QPair<QString, QString>>& cards,
                                          int columns = 4);

    // ========== 表格 ==========

    /**
     * @brief 创建标准表格
     * @param parent 父组件
     * @param headers 表头列表
     * @return 创建的表格视图
     */
    static QTableView* createStandardTable(QWidget* parent,
                                            const QStringList& headers);

    /**
     * @brief 创建带搜索的表格面板
     * @param parent 父组件
     * @param headers 表头列表
     * @param placeholder 搜索框占位文本
     * @return 包含搜索框和表格的 Frame
     */
    static QFrame* createTableWithSearch(QWidget* parent,
                                          const QStringList& headers,
                                          const QString& placeholder = QStringLiteral("搜索..."));

    // ========== 分割器 ==========

    /**
     * @brief 创建标准垂直分割器
     */
    static QSplitter* createVerticalSplitter(QWidget* parent);

    /**
     * @brief 创建标准水平分割器
     */
    static QSplitter* createHorizontalSplitter(QWidget* parent);

    // ========== 标签页 ==========

    /**
     * @brief 创建标准标签页组件
     * @param parent 父组件
     * @param tabs 标签页配置（名称, 图标可选）
     * @return 创建的 TabWidget
     */
    static QTabWidget* createStandardTabs(QWidget* parent,
                                           const QStringList& tabs);

    // ========== 图表容器 ==========

    /**
     * @brief 创建图表容器
     * @param parent 父组件
     * @param title 图表标题
     * @return 包含标题和图表区域的 Frame
     */
    static QFrame* createChartContainer(QWidget* parent,
                                         const QString& title);

    // ========== 滚动区域 ==========

    /**
     * @brief 创建标准滚动区域
     */
    static QScrollArea* createScrollArea(QWidget* parent);

    // ========== 按钮组 ==========

    /**
     * @brief 创建按钮组（水平排列）
     * @param parent 父组件
     * @param buttons 按钮配置（文本, 点击回调）
     * @return 包含按钮的 Frame
     */
    static QFrame* createButtonRow(QWidget* parent,
                                    const QVector<QPair<QString, std::function<void()>>>& buttons);

    // ========== 空状态 ==========

    /**
     * @brief 创建空状态提示
     * @param parent 父组件
     * @param message 提示消息
     * @param icon 图标（可选）
     * @return 空状态组件
     */
    static QFrame* createEmptyState(QWidget* parent,
                                     const QString& message,
                                     const QString& icon = QString());

    // ========== 加载状态 ==========

    /**
     * @brief 创建加载状态组件
     */
    static QFrame* createLoadingState(QWidget* parent,
                                       const QString& message = QStringLiteral("加载中..."));

    // ========== 样式应用 ==========

    /**
     * @brief 应用卡片样式
     */
    static void applyCardStyle(QFrame* card, const CardStyle& style = CardStyle());

    /**
     * @brief 应用表格样式
     */
    static void applyTableStyle(QTableView* table, const TableStyle& style = TableStyle());

    /**
     * @brief 获取标准卡片样式表
     */
    static QString standardCardStyleSheet();

    /**
     * @brief 获取标准表格样式表
     */
    static QString standardTableStyleSheet();

    /**
     * @brief 获取标准按钮样式表
     */
    static QString standardButtonStyleSheet();

private:
    // 私有构造，纯静态类
    PageTemplate() = delete;
};

} // namespace UI
} // namespace WealthPilot

#endif // PAGETEMPLATE_H