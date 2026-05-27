/**
 * @file KLineChartEvents.cpp
 * @brief K线图事件处理实现
 *
 * @details 实现功能：
 * - 绘制事件（paintEvent）
 * - 大小调整事件（resizeEvent）
 * - 鼠标事件（点击、移动、释放）
 * - 滚轮事件（缩放）
 * - 键盘事件（快捷键）
 *
 * @author WealthPilot Team
 * @version 2.1.0
 */

#include "KLineChartImpl.h"
#include "infrastructure/config/Tokens.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QElapsedTimer>

#include "shared/utils/Logger.h"

// ========== 绘制事件 ==========

/**
 * @brief 绘制事件
 */
void KLineChart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QElapsedTimer timer;
    timer.start();
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false); // 性能优化：关闭抗锯齿
    
    // 绘制背景
    drawBackground(painter);
    
    // 绘制网格
    drawGrid(painter);
    
    // 绘制K线
    drawCandles(painter);
    
    // 绘制主图指标
    drawMainIndicators(painter);
    
    // 绘制成交量
    if (d->style.showVolume) {
        drawVolume(painter);
    }
    
    // 绘制副图指标
    drawSubIndicators(painter);
    
    // 绘制十字光标
    if (d->showCrosshair) {
        drawCrosshair(painter);
        // 绘制K线信息（右上角）
        drawKLineInfo(painter);
    }
    
    // 绘制坐标轴
    drawAxis(painter);
    
    LOG_DEBUG(QString("KLineChart painted in %1ms").arg(timer.elapsed()));
}

/**
 * @brief 调整大小事件
 */
void KLineChart::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // 更新图表区域布局
    // 布局结构：主图区域 + 成交量区域 + 副图指标区域
    int volumeHeight = height() * d->style.volumeHeightRatio;
    int subChartHeight = 80;  // 副图指标区域高度（固定80像素）
    int leftMargin = 5;       // 左侧边距（减少空白）
    int rightAxisWidth = 55;  // 右侧坐标轴宽度
    int topMargin = 25;       // 顶部边距
    int bottomMargin = 25;    // 底部边距
    
    // 主图区域（K线）
    d->chartRect = QRect(leftMargin, topMargin, 
                         width() - leftMargin - rightAxisWidth, 
                         height() - topMargin - bottomMargin - volumeHeight - subChartHeight);
    
    // 成交量区域
    d->volumeRect = QRect(leftMargin, 
                          d->chartRect.bottom() + 3, 
                          width() - leftMargin - rightAxisWidth, 
                          volumeHeight);
    
    // 副图指标区域
    d->subChartRect = QRect(leftMargin, 
                            d->volumeRect.bottom() + 3, 
                            width() - leftMargin - rightAxisWidth, 
                            subChartHeight);
    
    d->calculateVisibleRange();
}

// ========== 鼠标事件 ==========

/**
 * @brief 鼠标按下事件
 */
void KLineChart::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        d->isDragging = true;
        d->lastMouseX = event->pos().x();
        setCursor(Qt::ClosedHandCursor);
    }
}

/**
 * @brief 鼠标移动事件
 */
void KLineChart::mouseMoveEvent(QMouseEvent *event)
{
    if (d->isDragging) {
        int dx = event->pos().x() - d->lastMouseX;
        pan(dx);
        d->lastMouseX = event->pos().x();
    } else {
        // 显示十字光标
        d->showCrosshair = true;
        d->crosshairX = event->pos().x();
        d->crosshairY = event->pos().y();
        
        // 计算当前K线索引
        int index = d->xToIndex(d->crosshairX);
        d->crosshairIndex = index;
        
        update();
        
        // 发送信号
        if (index >= 0 && index < d->data.size()) {
            double price = d->yToPrice(d->crosshairY);
            emit crosshairMoved(d->data[index].time, price);
            // 发送K线信息信号
            emit klineInfoChanged(d->data[index], index);
        }
    }
}

/**
 * @brief 鼠标释放事件
 */
void KLineChart::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        d->isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

// ========== 滚轮事件 ==========

/**
 * @brief 滚轮事件（缩放）
 */
void KLineChart::wheelEvent(QWheelEvent *event)
{
    double factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    zoom(factor);
}

// ========== 键盘事件 ==========

/**
 * @brief 键盘按下事件
 */
void KLineChart::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Left:
            pan(-20);
            break;
        case Qt::Key_Right:
            pan(20);
            break;
        case Qt::Key_Up:
            zoom(1.1);
            break;
        case Qt::Key_Down:
            zoom(0.9);
            break;
        case Qt::Key_Home:
            resetView();
            break;
        case Qt::Key_End:
            showLatest();
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}