/**
 * @file EquityCurveWidget.cpp
 * @brief 资金曲线组件实现
 */

#include "EquityCurveWidget.h"
#include "core/config/Tokens.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

// ========== PIMPL 实现 ==========

struct EquityCurveWidget::Impl
{
    QVector<EquityPoint> data;
    double initialEquity = 100000.0;
    EquityCurveStyle style;

    // 布局
    QRect chartRect;
    QRect labelRect;
    int margin = 60;
    int topMargin = 40;
    int bottomMargin = 40;

    // 范围
    double minEquity = 0;
    double maxEquity = 0;
    double minReturn = -0.5;
    double maxReturn = 0.5;

    // 鼠标交互
    bool isMouseInChart = false;
    int mouseX = -1;
    int mouseY = -1;
    int hoverIndex = -1;

    void calculateLayout(const QRect& rect)
    {
        chartRect = QRect(margin, topMargin,
                          rect.width() - margin - 80,
                          rect.height() - topMargin - bottomMargin);
        labelRect = QRect(chartRect.right() + 10, topMargin, 70, chartRect.height());
    }

    void calculateRange()
    {
        if (data.isEmpty())
        {
            minEquity = initialEquity * 0.8;
            maxEquity = initialEquity * 1.2;
            minReturn = -0.2;
            maxReturn = 0.2;
            return;
        }

        minEquity = std::numeric_limits<double>::max();
        maxEquity = std::numeric_limits<double>::lowest();
        minReturn = std::numeric_limits<double>::max();
        maxReturn = std::numeric_limits<double>::lowest();

        for (const auto& point : data)
        {
            minEquity = qMin(minEquity, point.equity);
            maxEquity = qMax(maxEquity, point.equity);
            minReturn = qMin(minReturn, point.returnRate);
            maxReturn = qMax(maxReturn, point.returnRate);
        }

        // 添加边距
        double equityMargin = (maxEquity - minEquity) * 0.1;
        minEquity -= equityMargin;
        maxEquity += equityMargin;

        // 收益率范围
        minReturn = qMin(minReturn, -0.1);
        maxReturn = qMax(maxReturn, 0.1);
    }
};

// ========== 构造函数和析构函数 ==========

EquityCurveWidget::EquityCurveWidget(QWidget* parent)
    : QWidget(parent)
      , d(std::make_unique<Impl>())
{
    setMouseTracking(true);
    setMinimumSize(600, 400);
}

EquityCurveWidget::~EquityCurveWidget() = default;

// ========== 公共方法 ==========

void EquityCurveWidget::setData(const QVector<EquityPoint>& data)
{
    d->data = data;
    d->calculateRange();
    update();
}

void EquityCurveWidget::setInitialEquity(double equity)
{
    d->initialEquity = equity;
    d->calculateRange();
    update();
}

QVector<EquityPoint> EquityCurveWidget::data() const
{
    return d->data;
}

void EquityCurveWidget::setStyle(const EquityCurveStyle& style)
{
    d->style = style;
    update();
}

void EquityCurveWidget::clear()
{
    d->data.clear();
    update();
}

void EquityCurveWidget::refresh()
{
    d->calculateRange();
    update();
}

// ========== 绘制方法 ==========

void EquityCurveWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    d->calculateLayout(rect());

    drawBackground(&painter);
    drawGrid(&painter);
    drawDrawdownArea(&painter);
    drawEquityCurve(&painter);
    drawReturnCurve(&painter);
    drawBenchmarkCurve(&painter);
    drawLabels(&painter);

    if (d->isMouseInChart && d->hoverIndex >= 0)
    {
        drawCrosshair(&painter);
        drawInfoBox(&painter);
    }
}

void EquityCurveWidget::drawBackground(QPainter* painter)
{
    painter->fillRect(rect(), QColor(Tokens::Colors::BgBase));
    painter->fillRect(d->chartRect, QColor(Tokens::Colors::BgSurface));
}

void EquityCurveWidget::drawGrid(QPainter* painter)
{
    if (!d->style.showGrid) return;

    painter->setPen(QPen(QColor(45, 55, 72, 50), 1));

    // 横向网格线
    int hLines = 5;
    for (int i = 0; i <= hLines; ++i)
    {
        int y = d->chartRect.top() + i * d->chartRect.height() / hLines;
        painter->drawLine(d->chartRect.left(), y, d->chartRect.right(), y);
    }

    // 纵向网格线
    int vLines = 6;
    for (int i = 0; i <= vLines; ++i)
    {
        int x = d->chartRect.left() + i * d->chartRect.width() / vLines;
        painter->drawLine(x, d->chartRect.top(), x, d->chartRect.bottom());
    }
}

void EquityCurveWidget::drawEquityCurve(QPainter* painter)
{
    if (d->data.isEmpty()) return;

    QPainterPath path;
    bool first = true;

    for (const auto& point : d->data)
    {
        int x = dateToX(point.date);
        int y = equityToY(point.equity);

        if (first)
        {
            path.moveTo(x, y);
            first = false;
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    QPen pen(d->style.equityColor, d->style.lineWidth);
    painter->setPen(pen);
    painter->drawPath(path);
}

void EquityCurveWidget::drawReturnCurve(QPainter* painter)
{
    if (d->data.isEmpty()) return;

    QPainterPath path;
    bool first = true;

    for (const auto& point : d->data)
    {
        int x = dateToX(point.date);
        int y = returnToY(point.returnRate);

        if (first)
        {
            path.moveTo(x, y);
            first = false;
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    QPen pen(d->style.returnColor, d->style.lineWidth);
    painter->setPen(pen);
    painter->drawPath(path);
}

void EquityCurveWidget::drawBenchmarkCurve(QPainter* painter)
{
    if (!d->style.showBenchmark || d->data.isEmpty()) return;

    QPainterPath path;
    bool first = true;

    for (const auto& point : d->data)
    {
        if (point.benchmarkReturn == 0) continue;

        int x = dateToX(point.date);
        int y = returnToY(point.benchmarkReturn);

        if (first)
        {
            path.moveTo(x, y);
            first = false;
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    QPen pen(d->style.benchmarkColor, 1, Qt::DashLine);
    painter->setPen(pen);
    painter->drawPath(path);
}

void EquityCurveWidget::drawDrawdownArea(QPainter* painter)
{
    if (!d->style.showDrawdown || d->data.isEmpty()) return;

    // 计算回撤区域
    double peak = d->initialEquity;
    QPainterPath path;
    bool inDrawdown = false;

    for (const auto& point : d->data)
    {
        if (point.equity > peak)
        {
            peak = point.equity;
            if (inDrawdown)
            {
                path.closeSubpath();
                inDrawdown = false;
            }
        }
        else
        {
            int x = dateToX(point.date);
            int y1 = equityToY(peak);
            int y2 = equityToY(point.equity);

            if (!inDrawdown)
            {
                path.moveTo(x, y1);
                inDrawdown = true;
            }
            path.lineTo(x, y2);
        }
    }

    QColor fillColor = d->style.drawdownColor;
    fillColor.setAlpha(50);
    painter->fillPath(path, fillColor);
}

void EquityCurveWidget::drawLabels(QPainter* painter)
{
    painter->setFont(QFont("Microsoft YaHei", 10));

    // 资产标签
    painter->setPen(d->style.equityColor);
    painter->drawText(d->labelRect.left(), d->chartRect.top() + 15, "资产");

    // 收益率标签
    painter->setPen(d->style.returnColor);
    painter->drawText(d->labelRect.left(), d->chartRect.top() + 35, "收益率");

    // Y轴标签
    painter->setPen(QColor(Tokens::Colors::TextSecondary));
    int hLines = 5;
    for (int i = 0; i <= hLines; ++i)
    {
        double equity = d->minEquity + (d->maxEquity - d->minEquity) * (hLines - i) / hLines;
        int y = d->chartRect.top() + i * d->chartRect.height() / hLines;

        QString text = QString("¥%1").arg(equity / 10000, 0, 'f', 1) + "万";
        painter->drawText(5, y + 4, text);
    }

    // X轴标签（日期）
    if (!d->data.isEmpty())
    {
        int vLines = 6;
        for (int i = 0; i <= vLines; ++i)
        {
            int index = i * (d->data.size() - 1) / vLines;
            if (index < d->data.size())
            {
                int x = d->chartRect.left() + i * d->chartRect.width() / vLines;
                QString text = d->data[index].date.toString("MM-dd");
                painter->drawText(x - 20, d->chartRect.bottom() + 15, text);
            }
        }
    }
}

void EquityCurveWidget::drawCrosshair(QPainter* painter)
{
    if (d->hoverIndex < 0 || d->hoverIndex >= d->data.size()) return;

    const auto& point = d->data[d->hoverIndex];
    int x = dateToX(point.date);

    QPen pen(QColor(Tokens::Colors::Primary), 1, Qt::DashLine);
    painter->setPen(pen);
    painter->drawLine(x, d->chartRect.top(), x, d->chartRect.bottom());
}

void EquityCurveWidget::drawInfoBox(QPainter* painter)
{
    if (d->hoverIndex < 0 || d->hoverIndex >= d->data.size()) return;

    const auto& point = d->data[d->hoverIndex];

    int boxWidth = 200;
    int boxHeight = 150;
    int boxX = qMin(d->mouseX + 10, width() - boxWidth - 10);
    int boxY = qMax(d->mouseY - boxHeight - 10, 10);

    QRect boxRect(boxX, boxY, boxWidth, boxHeight);

    painter->fillRect(boxRect, QColor(26, 35, 50, 230));
    painter->setPen(QPen(QColor(Tokens::Colors::Primary), 1));
    painter->drawRect(boxRect);

    painter->setFont(QFont("Microsoft YaHei", 11));
    painter->setPen(QColor(Tokens::Colors::TextPrimary));

    QStringList lines;
    lines << QString("日期: %1").arg(point.date.toString("yyyy-MM-dd"));
    lines << QString("总资产: ¥%1").arg(point.equity, 0, 'f', 2);
    lines << QString("现金: ¥%1").arg(point.cash, 0, 'f', 2);
    lines << QString("持仓市值: ¥%1").arg(point.marketValue, 0, 'f', 2);
    lines << QString("收益率: %1%").arg(point.returnRate * 100, 0, 'f', 2);

    if (d->style.showBenchmark)
    {
        lines << QString("基准收益: %1%").arg(point.benchmarkReturn * 100, 0, 'f', 2);
    }

    int textY = boxY + 20;
    for (const QString& line : lines)
    {
        painter->drawText(boxX + 10, textY, line);
        textY += 20;
    }
}

// ========== 坐标转换 ==========

int EquityCurveWidget::dateToX(const QDateTime& date) const
{
    if (d->data.isEmpty()) return d->chartRect.left();

    qint64 minTime = d->data.first().date.toMSecsSinceEpoch();
    qint64 maxTime = d->data.last().date.toMSecsSinceEpoch();
    qint64 targetTime = date.toMSecsSinceEpoch();

    if (maxTime <= minTime) return d->chartRect.left();

    double ratio = static_cast<double>(targetTime - minTime) / (maxTime - minTime);
    return d->chartRect.left() + static_cast<int>(ratio * d->chartRect.width());
}

int EquityCurveWidget::equityToY(double equity) const
{
    if (d->maxEquity <= d->minEquity) return d->chartRect.top();

    double ratio = (d->maxEquity - equity) / (d->maxEquity - d->minEquity);
    return d->chartRect.top() + static_cast<int>(ratio * d->chartRect.height());
}

int EquityCurveWidget::returnToY(double returnRate) const
{
    if (d->maxReturn <= d->minReturn) return d->chartRect.top();

    double ratio = (d->maxReturn - returnRate) / (d->maxReturn - d->minReturn);
    return d->chartRect.top() + static_cast<int>(ratio * d->chartRect.height());
}

double EquityCurveWidget::yToEquity(int y) const
{
    double ratio = static_cast<double>(d->chartRect.bottom() - y) / d->chartRect.height();
    return d->minEquity + ratio * (d->maxEquity - d->minEquity);
}

double EquityCurveWidget::yToReturn(int y) const
{
    double ratio = static_cast<double>(d->chartRect.bottom() - y) / d->chartRect.height();
    return d->minReturn + ratio * (d->maxReturn - d->minReturn);
}

int EquityCurveWidget::findDataIndex(int x) const
{
    if (d->data.isEmpty()) return -1;

    // 二分查找
    int left = 0;
    int right = d->data.size() - 1;

    while (left < right)
    {
        int mid = (left + right) / 2;
        int midX = dateToX(d->data[mid].date);

        if (midX < x)
        {
            left = mid + 1;
        }
        else
        {
            right = mid;
        }
    }

    return left;
}

// ========== 统计计算 ==========

double EquityCurveWidget::calculateMaxDrawdown() const
{
    if (d->data.isEmpty()) return 0;

    double peak = d->initialEquity;
    double maxDrawdown = 0;

    for (const auto& point : d->data)
    {
        if (point.equity > peak)
        {
            peak = point.equity;
        }

        double drawdown = (peak - point.equity) / peak;
        maxDrawdown = qMax(maxDrawdown, drawdown);
    }

    return maxDrawdown;
}

double EquityCurveWidget::calculateTotalReturn() const
{
    if (d->data.isEmpty()) return 0;
    return (d->data.last().equity - d->initialEquity) / d->initialEquity;
}

double EquityCurveWidget::calculateAnnualizedReturn() const
{
    if (d->data.size() < 2) return 0;

    int days = d->data.first().date.daysTo(d->data.last().date);
    if (days <= 0) return 0;

    double totalReturn = calculateTotalReturn();
    return std::pow(1 + totalReturn, 365.0 / days) - 1;
}

double EquityCurveWidget::calculateSharpeRatio() const
{
    if (d->data.size() < 2) return 0;

    // 计算日收益率的标准差
    QVector<double> dailyReturns;
    for (int i = 1; i < d->data.size(); ++i)
    {
        double dailyReturn = (d->data[i].equity - d->data[i - 1].equity) / d->data[i - 1].equity;
        dailyReturns.append(dailyReturn);
    }

    double mean = 0;
    for (double r : dailyReturns)
    {
        mean += r;
    }
    mean /= dailyReturns.size();

    double variance = 0;
    for (double r : dailyReturns)
    {
        variance += (r - mean) * (r - mean);
    }
    variance /= dailyReturns.size();

    double stdDev = std::sqrt(variance);
    if (stdDev < 0.0001) return 0;

    // 年化夏普比率（假设无风险利率为3%）
    double riskFreeRate = 0.03 / 252; // 日化无风险利率
    return (mean - riskFreeRate) / stdDev * std::sqrt(252);
}

// ========== 事件处理 ==========

void EquityCurveWidget::mouseMoveEvent(QMouseEvent* event)
{
    d->mouseX = event->position().x();
    d->mouseY = event->position().y();

    if (d->chartRect.contains(d->mouseX, d->mouseY))
    {
        d->isMouseInChart = true;
        d->hoverIndex = findDataIndex(d->mouseX);
    }
    else
    {
        d->isMouseInChart = false;
        d->hoverIndex = -1;
    }

    update();
}

void EquityCurveWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    d->isMouseInChart = false;
    d->hoverIndex = -1;
    update();
}

void EquityCurveWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    d->calculateLayout(rect());
}
