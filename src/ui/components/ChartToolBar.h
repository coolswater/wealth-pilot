/**
 * @file ChartToolBar.h
 * @brief 图表工具栏组件 - K线周期、复权、指标、画线工具
 *
 * @details 功能：
 * - 周期切换（分时、1分、5分、15分、30分、60分、日线、周线、月线）
 * - 复权类型选择（不复权、前复权、后复权）
 * - 技术指标开关（MA、MACD、RSI、KDJ、BOLL等）
 * - 画线工具（趋势线、水平线、平行线等）
 * - 图表类型切换（K线图、分时图）
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef CHARTTOOLBAR_H
#define CHARTTOOLBAR_H

#include <QWidget>
#include <QFrame>
#include <memory>
#include "core/types/MarketTypes.h"  // 使用统一的类型定义

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::KLinePeriod;
using WealthPilot::AdjustmentType;

// 前向声明
class QComboBox;
class QToolButton;
class QButtonGroup;
class QLabel;

/**
 * @brief 图表工具栏组件
 *
 * @details 提供K线图表的所有控制功能，包括：
 * - 周期选择（下拉框 + 快捷按钮）
 * - 复权类型（下拉菜单）
 * - 技术指标（多选菜单）
 * - 画线工具（单选菜单）
 * - 图表类型切换
 *
 * @example
 * @code
 * ChartToolBar* toolBar = new ChartToolBar(this);
 * connect(toolBar, &ChartToolBar::periodChanged, this, &MyClass::onPeriodChanged);
 * toolBar->setCurrentPeriod(KLinePeriod::Minute15);
 * @endcode
 */
class ChartToolBar : public QWidget
{
    Q_OBJECT

public:
    // ========== 构造与析构 ==========

    /**
     * @brief 构造函数
     * @param parent 父控件
     */
    explicit ChartToolBar(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ChartToolBar() override;

    // ========== 状态设置 ==========

    /**
     * @brief 设置当前周期
     * @param period K线周期
     */
    void setCurrentPeriod(KLinePeriod period);

    /**
     * @brief 获取当前周期
     * @return 当前K线周期
     */
    KLinePeriod currentPeriod() const;

    /**
     * @brief 设置当前复权类型
     * @param type 复权类型
     */
    void setCurrentAdjustment(AdjustmentType type);

    /**
     * @brief 获取当前复权类型
     * @return 当前复权类型
     */
    AdjustmentType currentAdjustment() const;

    /**
     * @brief 设置当前主图指标
     * @param indicator 主图指标名称
     */
    void setCurrentMainIndicator(const QString& indicator);

    /**
     * @brief 获取当前主图指标
     * @return 当前主图指标名称
     */
    QString currentMainIndicator() const;

    /**
     * @brief 设置当前副图指标
     * @param indicator 副图指标名称
     */
    void setCurrentSubIndicator(const QString& indicator);

    /**
     * @brief 获取当前副图指标
     * @return 当前副图指标名称
     */
    QString currentSubIndicator() const;

signals:
    // ========== 信号 ==========

    /**
     * @brief 周期改变信号
     * @param period 新的K线周期
     */
    void periodChanged(KLinePeriod period);

    /**
     * @brief 复权类型改变信号
     * @param type 新的复权类型
     */
    void adjustmentChanged(AdjustmentType type);

    /**
     * @brief 主图指标切换信号（单选）
     * @param indicator 主图指标名称
     */
    void mainIndicatorChanged(const QString& indicator);

    /**
     * @brief 副图指标切换信号（单选）
     * @param indicator 副图指标名称
     */
    void subIndicatorChanged(const QString& indicator);

    /**
     * @brief 画线工具选择信号
     * @param tool 工具名称
     */
    void drawToolSelected(const QString& tool);

    /**
     * @brief 图表类型改变信号
     * @param type 图表类型（"kline" 或 "timeline"）
     */
    void chartTypeChanged(const QString& type);

private slots:
    // ========== 私有槽函数 ==========

    void onPeriodComboChanged(int index);
    void onAdjustmentMenuTriggered(QAction* action);
    void onMainIndicatorMenuTriggered(QAction* action);
    void onSubIndicatorMenuTriggered(QAction* action);
    void onDrawToolMenuTriggered(QAction* action);
    void onChartTypeMenuTriggered(QAction* action);

private:
    // ========== 私有方法 ==========

    void setupUI();
    void setupPeriodSelector();
    void setupToolButtons();
    void setupMenus();
    QFrame* createSeparator();

    // ========== PIMPL ==========

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // CHARTTOOLBAR_H
