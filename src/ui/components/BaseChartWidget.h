/**
 * @file BaseChartWidget.h
 * @brief 图表组件基类 - 提供通用图表功能
 *
 * @details 功能：
 * - 统一的深色主题样式
 * - 通用的绘制工具方法
 * - 性能优化基类
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef BASECHARTWIDGET_H
#define BASECHARTWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <memory>

#include "ThemeColors.h"

/**
 * @brief 图表组件基类
 *
 * @details 提供图表组件的通用功能：
 * - 深色主题背景
 * - 双缓冲绘制
 * - 鼠标交互基础
 * - 性能优化
 *
 * @example
 * @code
 * class MyChart : public BaseChartWidget {
 * protected:
 *     void paintContent(QPainter& painter) override {
 *         // 绘制内容
 *     }
 * };
 * @endcode
 */
class BaseChartWidget : public QWidget
{
    Q_OBJECT

public:
    // ========== 构造与析构 ==========

    explicit BaseChartWidget(QWidget *parent = nullptr);
    ~BaseChartWidget() override = default;

    // ========== 公共接口 ==========

    /**
     * @brief 设置是否显示网格
     */
    void setShowGrid(bool show) { m_showGrid = show; update(); }

    /**
     * @brief 是否显示网格
     */
    bool showGrid() const { return m_showGrid; }

    /**
     * @brief 设置是否显示十字光标
     */
    void setShowCrosshair(bool show) { m_showCrosshair = show; update(); }

    /**
     * @brief 是否显示十字光标
     */
    bool showCrosshair() const { return m_showCrosshair; }

    /**
     * @brief 设置背景颜色
     */
    void setBackgroundColor(const QColor& color) { m_backgroundColor = color; update(); }

    /**
     * @brief 获取背景颜色
     */
    QColor backgroundColor() const { return m_backgroundColor; }

    /**
     * @brief 设置网格颜色
     */
    void setGridColor(const QColor& color) { m_gridColor = color; update(); }

    /**
     * @brief 获取网格颜色
     */
    QColor gridColor() const { return m_gridColor; }

    /**
     * @brief 启用/禁用性能优化模式
     * @param enabled 是否启用
     * @details 启用后会降低刷新频率，适合大量数据场景
     */
    void setPerformanceMode(bool enabled);

    /**
     * @brief 是否处于性能优化模式
     */
    bool isPerformanceMode() const { return m_performanceMode; }

signals:
    /**
     * @brief 视图范围改变信号
     */
    void viewRangeChanged();

    /**
     * @brief 鼠标位置改变信号
     */
    void mousePositionChanged(const QPoint& pos);

protected:
    // ========== 事件处理 ==========

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;

    // ========== 绘制方法（子类重写） ==========

    /**
     * @brief 绘制内容（子类实现）
     */
    virtual void paintContent(QPainter& painter) { Q_UNUSED(painter); }

    /**
     * @brief 绘制背景
     */
    virtual void paintBackground(QPainter& painter);

    /**
     * @brief 绘制网格
     */
    virtual void paintGrid(QPainter& painter);

    /**
     * @brief 绘制十字光标
     */
    virtual void paintCrosshair(QPainter& painter);

    /**
     * @brief 绘制边框
     */
    virtual void paintBorder(QPainter& painter);

    // ========== 工具方法 ==========

    /**
     * @brief 请求延迟更新（性能优化）
     */
    void requestDelayedUpdate(int delayMs = 16);

    /**
     * @brief 取消延迟更新
     */
    void cancelDelayedUpdate();

    /**
     * @brief 获取图表区域（排除边距）
     */
    QRect chartRect() const { return m_chartRect; }

    /**
     * @brief 设置图表边距
     */
    void setChartMargins(const QMargins& margins);

    /**
     * @brief 获取图表边距
     */
    QMargins chartMargins() const { return m_margins; }

    /**
     * @brief 更新图表区域
     */
    void updateChartRect();

    // ========== 鼠标状态 ==========

    bool isMousePressed() const { return m_mousePressed; }
    QPoint lastMousePos() const { return m_lastMousePos; }
    QPoint currentMousePos() const { return m_currentMousePos; }

private:
    // ========== 私有成员 ==========

    bool m_showGrid = true;
    bool m_showCrosshair = false;
    bool m_performanceMode = false;
    bool m_mousePressed = false;

    QColor m_backgroundColor = ThemeColors::backgroundPrimary();
    QColor m_gridColor = QColor("#2A2A3E");
    QColor m_crosshairColor = QColor("#4B5563");

    QRect m_chartRect;
    QMargins m_margins{60, 30, 20, 30};

    QPoint m_lastMousePos;
    QPoint m_currentMousePos;

    QTimer* m_updateTimer = nullptr;
};

#endif // BASECHARTWIDGET_H
