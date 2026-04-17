/**
 * @file BaseChartWidget.cpp
 * @brief 图表组件基类实现
 */

#include "BaseChartWidget.h"
#include <QElapsedTimer>

// ============================================================================
// 构造函数
// ============================================================================

BaseChartWidget::BaseChartWidget(QWidget *parent)
    : QWidget(parent)
{
    // 启用鼠标追踪
    setMouseTracking(true);

    // 设置最小尺寸
    setMinimumSize(400, 300);

    // 创建更新定时器（性能优化用）
    m_updateTimer = new QTimer(this);
    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&QWidget::update));

    // 设置背景
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, m_backgroundColor);
    setPalette(pal);
}

// ============================================================================
// 公共接口
// ============================================================================

void BaseChartWidget::setPerformanceMode(bool enabled)
{
    m_performanceMode = enabled;
    if (!enabled) {
        cancelDelayedUpdate();
    }
}

// ============================================================================
// 事件处理
// ============================================================================

void BaseChartWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    // 关闭抗锯齿以提升性能
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 绘制背景
    paintBackground(painter);

    // 绘制网格
    if (m_showGrid) {
        paintGrid(painter);
    }

    // 绘制内容（子类实现）
    paintContent(painter);

    // 绘制十字光标
    if (m_showCrosshair && underMouse()) {
        paintCrosshair(painter);
    }

    // 绘制边框
    paintBorder(painter);
}

void BaseChartWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateChartRect();
}

void BaseChartWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void BaseChartWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_currentMousePos = event->pos();
    emit mousePositionChanged(m_currentMousePos);

    if (m_showCrosshair) {
        update();
    }

    if (m_mousePressed) {
        m_lastMousePos = event->pos();
    }
}

void BaseChartWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_mousePressed = false;
        setCursor(Qt::ArrowCursor);
    }
}

void BaseChartWidget::wheelEvent(QWheelEvent *event)
{
    Q_UNUSED(event);
    // 子类实现缩放逻辑
}

void BaseChartWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_showCrosshair) {
        update();
    }
}

// ============================================================================
// 绘制方法
// ============================================================================

void BaseChartWidget::paintBackground(QPainter& painter)
{
    painter.fillRect(rect(), m_backgroundColor);
}

void BaseChartWidget::paintGrid(QPainter& painter)
{
    painter.setPen(QPen(m_gridColor, 1, Qt::DashLine));

    // 水平网格线
    int hLines = 5;
    for (int i = 0; i <= hLines; ++i) {
        int y = m_chartRect.top() + i * m_chartRect.height() / hLines;
        painter.drawLine(m_chartRect.left(), y, m_chartRect.right(), y);
    }

    // 垂直网格线
    int vLines = 6;
    for (int i = 0; i <= vLines; ++i) {
        int x = m_chartRect.left() + i * m_chartRect.width() / vLines;
        painter.drawLine(x, m_chartRect.top(), x, m_chartRect.bottom());
    }
}

void BaseChartWidget::paintCrosshair(QPainter& painter)
{
    if (!m_chartRect.contains(m_currentMousePos)) {
        return;
    }

    painter.setPen(QPen(m_crosshairColor, 1, Qt::DashLine));

    // 水平线
    painter.drawLine(m_chartRect.left(), m_currentMousePos.y(),
                    m_chartRect.right(), m_currentMousePos.y());

    // 垂直线
    painter.drawLine(m_currentMousePos.x(), m_chartRect.top(),
                    m_currentMousePos.x(), m_chartRect.bottom());
}

void BaseChartWidget::paintBorder(QPainter& painter)
{
    painter.setPen(QPen(ThemeColors::border(), 1));
    painter.drawRect(m_chartRect);
}

// ============================================================================
// 工具方法
// ============================================================================

void BaseChartWidget::requestDelayedUpdate(int delayMs)
{
    if (m_performanceMode) {
        m_updateTimer->start(delayMs);
    } else {
        update();
    }
}

void BaseChartWidget::cancelDelayedUpdate()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
    }
}

void BaseChartWidget::setChartMargins(const QMargins& margins)
{
    m_margins = margins;
    updateChartRect();
}

void BaseChartWidget::updateChartRect()
{
    m_chartRect = QRect(
        m_margins.left(),
        m_margins.top(),
        width() - m_margins.left() - m_margins.right(),
        height() - m_margins.top() - m_margins.bottom()
    );
}
