/**
 * @file BacktestChartWidget.cpp
 * @brief 回测结果可视化图表组件实现
 */

#include "BacktestChartWidget.h"
#include "shared/utils/Logger.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <algorithm>

namespace WealthPilot {

struct BacktestChartWidget::Impl {
    QVector<BacktestDataPoint> equityCurve;
    QVector<TradeMarker> trades;
    
    QDateTime displayStart;
    QDateTime displayEnd;
    
    double minEquity = 0;
    double maxEquity = 0;
    double maxDrawdown = 0;
    
    bool showTrades = true;
    bool showDrawdown = true;
    
    // 鼠标交互
    int mouseX = -1;
    int mouseY = -1;
    bool isDragging = false;
    int dragStartX = 0;
    
    // 图表区域
    QRect chartRect;
    int marginLeft = 60;
    int marginRight = 20;
    int marginTop = 40;
    int marginBottom = 50;
};

BacktestChartWidget::BacktestChartWidget(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setMouseTracking(true);
    setMinimumSize(400, 300);
    setBackgroundRole(QPalette::Base);
}

BacktestChartWidget::~BacktestChartWidget() = default;

void BacktestChartWidget::setData(const QVector<BacktestDataPoint>& equityCurve,
                                   const QVector<TradeMarker>& trades)
{
    d->equityCurve = equityCurve;
    d->trades = trades;
    
    if (!equityCurve.isEmpty()) {
        d->displayStart = equityCurve.first().date;
        d->displayEnd = equityCurve.last().date;
    }
    
    calculateRange();
    update();
}

void BacktestChartWidget::clearData()
{
    d->equityCurve.clear();
    d->trades.clear();
    d->minEquity = 0;
    d->maxEquity = 0;
    d->maxDrawdown = 0;
    update();
}

void BacktestChartWidget::setDisplayRange(const QDateTime& start, const QDateTime& end)
{
    d->displayStart = start;
    d->displayEnd = end;
    calculateRange();
    update();
}

void BacktestChartWidget::setShowTrades(bool show)
{
    d->showTrades = show;
    update();
}

void BacktestChartWidget::setShowDrawdown(bool show)
{
    d->showDrawdown = show;
    update();
}

double BacktestChartWidget::currentEquity() const
{
    if (d->equityCurve.isEmpty()) {
        return 0;
    }
    return d->equityCurve.last().equity;
}

void BacktestChartWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 计算 chart 区域
    d->chartRect = QRect(
        d->marginLeft,
        d->marginTop,
        width() - d->marginLeft - d->marginRight,
        height() - d->marginTop - d->marginBottom
    );
    
    // 绘制背景
    painter.fillRect(d->chartRect, QColor(20, 25, 30));
    
    if (d->equityCurve.isEmpty()) {
        painter.setPen(QColor(100, 100, 100));
        painter.drawText(d->chartRect, Qt::AlignCenter, "无回测数据");
        return;
    }
    
    drawGrid(painter);
    drawEquityCurve(painter);
    
    if (d->showDrawdown) {
        drawDrawdownCurve(painter);
    }
    
    if (d->showTrades) {
        drawTradeMarkers(painter);
    }
    
    drawAxis(painter);
    drawTooltip(painter);
}

void BacktestChartWidget::drawGrid(QPainter& painter)
{
    painter.setPen(QColor(50, 55, 60));
    
    // 横线（价格网格）
    int yGridCount = 5;
    for (int i = 0; i <= yGridCount; ++i) {
        int y = d->chartRect.top() + i * d->chartRect.height() / yGridCount;
        painter.drawLine(d->chartRect.left(), y, d->chartRect.right(), y);
    }
    
    // 竖线（时间网格）
    int xGridCount = 6;
    for (int i = 0; i <= xGridCount; ++i) {
        int x = d->chartRect.left() + i * d->chartRect.width() / xGridCount;
        painter.drawLine(x, d->chartRect.top(), x, d->chartRect.bottom());
    }
}

void BacktestChartWidget::drawEquityCurve(QPainter& painter)
{
    if (d->equityCurve.size() < 2) {
        return;
    }
    
    QPen pen(QColor(0, 150, 255));
    pen.setWidth(2);
    painter.setPen(pen);
    
    QPainterPath path;
    bool first = true;
    
    for (const auto& point : d->equityCurve) {
        int x = dateToX(point.date);
        int y = equityToY(point.equity);
        
        if (first) {
            path.moveTo(x, y);
            first = false;
        } else {
            path.lineTo(x, y);
        }
    }
    
    painter.drawPath(path);
    
    // 绘制填充区域
    QPainterPath fillPath = path;
    fillPath.lineTo(d->chartRect.right(), d->chartRect.bottom());
    fillPath.lineTo(d->chartRect.left(), d->chartRect.bottom());
    fillPath.closeSubpath();
    
    QColor fillColor(0, 150, 255, 30);
    painter.fillPath(fillPath, fillColor);
}

void BacktestChartWidget::drawDrawdownCurve(QPainter& painter)
{
    if (d->equityCurve.size() < 2) {
        return;
    }
    
    QPen pen(QColor(255, 80, 80));
    pen.setWidth(1);
    painter.setPen(pen);
    
    // 绘制回撤曲线
    for (int i = 1; i < d->equityCurve.size(); ++i) {
        const auto& prev = d->equityCurve[i - 1];
        const auto& curr = d->equityCurve[i];
        
        int x1 = dateToX(prev.date);
        int y1 = equityToY(prev.equity - prev.drawdown * prev.equity / 100);
        int x2 = dateToX(curr.date);
        int y2 = equityToY(curr.equity - curr.drawdown * curr.equity / 100);
        
        painter.drawLine(x1, y1, x2, y2);
    }
}

void BacktestChartWidget::drawTradeMarkers(QPainter& painter)
{
    for (const auto& trade : d->trades) {
        int x = dateToX(trade.date);
        int y = equityToY(trade.price * trade.quantity);
        
        QColor color = trade.action == "buy" 
            ? QColor(0, 200, 100) 
            : QColor(255, 100, 100);
        
        // 绘制标记点
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(x - 4, y - 4, 8, 8);
        
        // 绘制标记线
        painter.setPen(QPen(color, 1));
        painter.drawLine(x, y, x, d->chartRect.bottom());
    }
}

void BacktestChartWidget::drawAxis(QPainter& painter)
{
    painter.setPen(QColor(150, 150, 150));
    QFont font("Arial", 9);
    painter.setFont(font);
    
    // Y轴标签（权益）
    int yGridCount = 5;
    for (int i = 0; i <= yGridCount; ++i) {
        double equity = d->maxEquity - i * (d->maxEquity - d->minEquity) / yGridCount;
        int y = d->chartRect.top() + i * d->chartRect.height() / yGridCount;
        
        QString label = QString::number(equity / 10000, 'f', 1) + "万";
        painter.drawText(d->marginLeft - 50, y + 5, label);
    }
    
    // X轴标签（日期）
    int xGridCount = 6;
    for (int i = 0; i <= xGridCount; ++i) {
        int x = d->chartRect.left() + i * d->chartRect.width() / xGridCount;
        
        QDateTime date = xToDate(x);
        QString label = date.toString("MM-dd");
        painter.drawText(x - 20, height() - 20, label);
    }
    
    // 标题
    QFont titleFont("Arial", 12, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(10, 20, "策略回测资金曲线");
}

void BacktestChartWidget::drawTooltip(QPainter& painter)
{
    if (d->mouseX < d->chartRect.left() || d->mouseX > d->chartRect.right()) {
        return;
    }
    
    // 找到最近的数据点
    QDateTime mouseDate = xToDate(d->mouseX);
    BacktestDataPoint nearestPoint;
    int minDiff = INT_MAX;
    
    for (const auto& point : d->equityCurve) {
        int diff = qAbs(point.date.daysTo(mouseDate));
        if (diff < minDiff) {
            minDiff = diff;
            nearestPoint = point;
        }
    }
    
    // 绘制tooltip背景
    QString tooltip = QString("日期: %1\n权益: %2万\n收益: %3%\n回撤: %4%")
        .arg(nearestPoint.date.toString("yyyy-MM-dd"))
        .arg(nearestPoint.equity / 10000, 'f', 2)
        .arg(nearestPoint.returnRate, 'f', 2)
        .arg(nearestPoint.drawdown, 'f', 2);
    
    QFontMetrics fm(painter.font());
    QRect tooltipRect = fm.boundingRect(QRect(0, 0, 150, 100), Qt::TextWordWrap, tooltip);
    tooltipRect.moveTo(d->mouseX + 10, d->mouseY - tooltipRect.height() - 10);
    
    painter.fillRect(tooltipRect, QColor(40, 45, 50, 200));
    painter.setPen(QColor(200, 200, 200));
    painter.drawRect(tooltipRect);
    painter.drawText(tooltipRect.adjusted(5, 5, -5, -5), Qt::TextWordWrap, tooltip);
    
    // 绘制竖线指示器
    painter.setPen(QPen(QColor(100, 100, 100), 1, Qt::DashLine));
    int x = dateToX(nearestPoint.date);
    painter.drawLine(x, d->chartRect.top(), x, d->chartRect.bottom());
}

void BacktestChartWidget::calculateRange()
{
    if (d->equityCurve.isEmpty()) {
        return;
    }
    
    d->minEquity = d->equityCurve.first().equity;
    d->maxEquity = d->equityCurve.first().equity;
    d->maxDrawdown = 0;
    
    for (const auto& point : d->equityCurve) {
        d->minEquity = qMin(d->minEquity, point.equity);
        d->maxEquity = qMax(d->maxEquity, point.equity);
        d->maxDrawdown = qMax(d->maxDrawdown, point.drawdown);
    }
    
    // 增加10%的上下边距
    double range = d->maxEquity - d->minEquity;
    d->minEquity -= range * 0.1;
    d->maxEquity += range * 0.1;
}

int BacktestChartWidget::dateToX(const QDateTime& date) const
{
    if (d->displayStart == d->displayEnd) {
        return d->chartRect.left();
    }
    
    qint64 totalDays = d->displayStart.daysTo(d->displayEnd);
    qint64 daysFromStart = d->displayStart.daysTo(date);
    
    double ratio = daysFromStart / static_cast<double>(totalDays);
    return d->chartRect.left() + static_cast<int>(ratio * d->chartRect.width());
}

double BacktestChartWidget::equityToY(double equity) const
{
    if (d->maxEquity == d->minEquity) {
        return d->chartRect.center().y();
    }
    
    double ratio = (d->maxEquity - equity) / (d->maxEquity - d->minEquity);
    return d->chartRect.top() + ratio * d->chartRect.height();
}

QDateTime BacktestChartWidget::xToDate(int x) const
{
    if (d->chartRect.width() == 0) {
        return d->displayStart;
    }
    
    double ratio = (x - d->chartRect.left()) / static_cast<double>(d->chartRect.width());
    qint64 totalDays = d->displayStart.daysTo(d->displayEnd);
    qint64 daysFromStart = static_cast<qint64>(ratio * totalDays);
    
    return d->displayStart.addDays(daysFromStart);
}

void BacktestChartWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    calculateRange();
}

void BacktestChartWidget::mouseMoveEvent(QMouseEvent* event)
{
    d->mouseX = static_cast<int>(event->position().x());
    d->mouseY = static_cast<int>(event->position().y());
    update();
}

void BacktestChartWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        d->isDragging = true;
        d->dragStartX = static_cast<int>(event->position().x());
    }
}

void BacktestChartWidget::wheelEvent(QWheelEvent* event)
{
    // 缩放功能
    qint64 totalDays = d->displayStart.daysTo(d->displayEnd);
    qint64 zoomDays = totalDays / 4;
    
    if (event->angleDelta().y() > 0) {
        // 放大
        zoomDays = qMax(7, zoomDays);  // 最少显示7天
        qint64 centerDays = d->displayStart.daysTo(xToDate(d->mouseX));
        qint64 newStartDays = qMax(0, centerDays - zoomDays / 2);
        qint64 newEndDays = qMin(totalDays, centerDays + zoomDays / 2);
        
        d->displayStart = d->displayStart.addDays(newStartDays);
        d->displayEnd = d->displayStart.addDays(newEndDays);
    } else {
        // 缩小
        zoomDays = qMin(totalDays * 2, zoomDays);
        d->displayStart = d->displayStart.addDays(-zoomDays);
        d->displayEnd = d->displayEnd.addDays(zoomDays);
    }
    
    calculateRange();
    update();
}

} // namespace WealthPilot