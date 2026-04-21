#include "DividerWidget.h"
#include <QDebug>

// 构造函数1：指定方向
DividerWidget::DividerWidget(Qt::Orientation orientation, QWidget *parent)
    : QWidget(parent), m_orientation(orientation)
{
    updateSizePolicy();
    setAttribute(Qt::WA_StaticContents);
}

// 构造函数2：默认水平方向
DividerWidget::DividerWidget(QWidget *parent)
    : DividerWidget(Qt::Horizontal, parent)
{
}

// 根据方向更新尺寸策略
void DividerWidget::updateSizePolicy()
{
    if (m_orientation == Qt::Horizontal) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(m_lineWidth);
    } else {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        setFixedWidth(m_lineWidth);
    }
    update();
}

QColor DividerWidget::lineColor() const
{
    return m_lineColor;
}

void DividerWidget::setLineColor(const QColor &color)
{
    if (m_lineColor != color) {
        m_lineColor = color;
        emit lineColorChanged(color);
        update();
    }
}

int DividerWidget::lineWidth() const
{
    return m_lineWidth;
}

void DividerWidget::setLineWidth(int width)
{
    if (width < 1) width = 1;
    if (m_lineWidth != width) {
        m_lineWidth = width;
        emit lineWidthChanged(width);
        updateSizePolicy();  // 更新固定尺寸
    }
}

Qt::Orientation DividerWidget::orientation() const
{
    return m_orientation;
}

void DividerWidget::setOrientation(Qt::Orientation orientation)
{
    if (m_orientation != orientation) {
        m_orientation = orientation;
        emit orientationChanged(orientation);
        updateSizePolicy();
    }
}

int DividerWidget::margin() const
{
    return m_margin;
}

void DividerWidget::setMargin(int margin)
{
    if (margin < 0) margin = 0;
    if (m_margin != margin) {
        m_margin = margin;
        emit marginChanged(margin);
        update();
    }
}

// 重写绘制事件
void DividerWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);  // 分割线不需要抗锯齿

    // 启用样式表支持（如果在Designer中使用或设置了QSS）
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    // 设置画笔
    QPen pen(m_lineColor);
    pen.setWidth(m_lineWidth);
    pen.setCapStyle(Qt::FlatCap);  // 平头线
    painter.setPen(pen);

    // 计算绘制区域（考虑边距）
    const QRect rect = this->rect();
    int x1, y1, x2, y2;

    if (m_orientation == Qt::Horizontal) {
        // 水平分割线：居中绘制，考虑左右边距
        int y = rect.height() / 2;
        x1 = m_margin;
        x2 = rect.width() - m_margin;
        y1 = y2 = y;

        // 如果设置了线宽大于1，需要调整Y坐标确保居中
        if (m_lineWidth > 1) {
            y1 = y2 = m_lineWidth / 2;
        }
    } else {
        // 垂直分割线：居中绘制，考虑上下边距
        int x = rect.width() / 2;
        x1 = x2 = x;
        y1 = m_margin;
        y2 = rect.height() - m_margin;

        // 如果设置了线宽大于1，需要调整X坐标确保居中
        if (m_lineWidth > 1) {
            x1 = x2 = m_lineWidth / 2;
        }
    }

    painter.drawLine(x1, y1, x2, y2);
}

QSize DividerWidget::sizeHint() const
{
    if (m_orientation == Qt::Horizontal) {
        return QSize(100, m_lineWidth);  // 水平线默认长度100，高度为线宽
    } else {
        return QSize(m_lineWidth, 100);  // 垂直线默认宽度为线宽，高度100
    }
}

QSize DividerWidget::minimumSizeHint() const
{
    if (m_orientation == Qt::Horizontal) {
        return QSize(m_margin * 2 + 10, m_lineWidth);  // 最小长度要容纳边距
    } else {
        return QSize(m_lineWidth, m_margin * 2 + 10);
    }
}

// 便捷工厂方法
DividerWidget* DividerWidget::createHorizontal(QWidget *parent,
                                   const QColor &color,
                                   int width,
                                   int margin)
{
    auto *divider = new DividerWidget(Qt::Horizontal, parent);
    divider->setLineColor(color);
    divider->setLineWidth(width);
    divider->setMargin(margin);
    return divider;
}

DividerWidget* DividerWidget::createVertical(QWidget *parent,
                                 const QColor &color,
                                 int width,
                                 int margin)
{
    auto *divider = new DividerWidget(Qt::Vertical, parent);
    divider->setLineColor(color);
    divider->setLineWidth(width);
    divider->setMargin(margin);
    return divider;
}
