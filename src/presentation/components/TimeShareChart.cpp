/**
 * @file TimeShareChart.cpp
 * @brief 分时图组件实现
 */

#include "TimeShareChart.h"
#include "core/config/Tokens.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QDateTime>
#include <QtMath>
#include <algorithm>

// ========== PIMPL 实现 ==========

struct TimeShareChart::Impl
{
    // 数据
    QVector<TimeShareData> data;
    double yesterdayClose = 0.0;
    QString symbol;
    QString name;

    // 样式
    TimeShareStyle style;

    // 布局
    QRect chartRect; ///< 价格图区域
    QRect volumeRect; ///< 成交量图区域
    QRect labelRect; ///< 标签区域

    // 价格范围
    double minPrice = 0.0;
    double maxPrice = 0.0;
    double priceScale = 1.0;

    // 成交量范围
    qint64 maxVolume = 0;
    double volumeScale = 1.0;

    // 时间范围
    qint64 startTime = 0;
    qint64 endTime = 0;

    // 鼠标交互
    bool isMouseInChart = false;
    int mouseX = -1;
    int mouseY = -1;
    int hoverIndex = -1;

    /**
     * @brief 计算布局
     */
    void calculateLayout(const QRect& rect)
    {
        int labelWidth = 60;
        int volumeHeight = 80;
        int topMargin = 20;
        int bottomMargin = 20;

        labelRect = QRect(rect.right() - labelWidth, topMargin, labelWidth,
                          rect.height() - topMargin - bottomMargin - volumeHeight);
        chartRect = QRect(rect.left(), topMargin, rect.width() - labelWidth,
                          rect.height() - topMargin - bottomMargin - volumeHeight);
        volumeRect = QRect(rect.left(), chartRect.bottom() + 10, rect.width() - labelWidth, volumeHeight - 10);
    }

    /**
     * @brief 计算价格范围
     */
    void calculatePriceRange()
    {
        if (data.isEmpty() || yesterdayClose <= 0)
        {
            minPrice = 0;
            maxPrice = 100;
            priceScale = 1.0;
            return;
        }

        // 找出最高价和最低价
        double dataMax = std::numeric_limits<double>::lowest();
        double dataMin = std::numeric_limits<double>::max();

        for (const auto& point : data)
        {
            if (point.price > 0)
            {
                dataMax = qMax(dataMax, point.price);
                dataMin = qMin(dataMin, point.price);
            }
            if (point.avgPrice > 0)
            {
                dataMax = qMax(dataMax, point.avgPrice);
                dataMin = qMin(dataMin, point.avgPrice);
            }
        }

        // 以昨收价为基准，计算涨跌幅限制
        double maxChange = qMax(qAbs(dataMax - yesterdayClose), qAbs(dataMin - yesterdayClose));
        maxChange = qMax(maxChange, yesterdayClose * 0.05); // 至少5%的范围

        minPrice = yesterdayClose - maxChange;
        maxPrice = yesterdayClose + maxChange;

        // 计算缩放比例
        if (chartRect.height() > 0 && maxPrice > minPrice)
        {
            priceScale = chartRect.height() / (maxPrice - minPrice);
        }
        else
        {
            priceScale = 1.0;
        }
    }

    /**
     * @brief 计算成交量范围
     */
    void calculateVolumeRange()
    {
        maxVolume = 0;
        for (const auto& point : data)
        {
            maxVolume = qMax(maxVolume, point.volume);
        }

        if (volumeRect.height() > 0 && maxVolume > 0)
        {
            volumeScale = static_cast<double>(volumeRect.height()) / maxVolume;
        }
        else
        {
            volumeScale = 1.0;
        }
    }

    /**
     * @brief 计算时间范围
     */
    void calculateTimeRange()
    {
        if (data.isEmpty())
        {
            // 默认交易时间：9:30-11:30, 13:00-15:00
            QDate today = QDate::currentDate();
            startTime = QDateTime(today, QTime(9, 30)).toMSecsSinceEpoch();
            endTime = QDateTime(today, QTime(15, 0)).toMSecsSinceEpoch();
            return;
        }

        startTime = data.first().time.toMSecsSinceEpoch();
        endTime = data.last().time.toMSecsSinceEpoch();
    }
};

// ========== 构造函数和析构函数 ==========

TimeShareChart::TimeShareChart(QWidget* parent)
    : QWidget(parent)
      , d(std::make_unique<Impl>())
{
    setMouseTracking(true);
    setMinimumSize(400, 300);
}

TimeShareChart::~TimeShareChart() = default;

// ========== 公共方法 ==========

void TimeShareChart::setData(const QVector<TimeShareData>& data)
{
    d->data = data;
    d->calculateTimeRange();
    d->calculatePriceRange();
    d->calculateVolumeRange();
    update();
}

void TimeShareChart::setYesterdayClose(double price)
{
    d->yesterdayClose = price;
    d->calculatePriceRange();
    update();
}

void TimeShareChart::setInstrumentInfo(const QString& symbol, const QString& name)
{
    d->symbol = symbol;
    d->name = name;
    update();
}

QVector<TimeShareData> TimeShareChart::data() const
{
    return d->data;
}

double TimeShareChart::yesterdayClose() const
{
    return d->yesterdayClose;
}

QVector<QPair<QDateTime, double>> TimeShareChart::prices() const
{
    QVector<QPair<QDateTime, double>> result;
    for (const auto& point : d->data)
    {
        result.append({point.time, point.price});
    }
    return result;
}

QVector<qint64> TimeShareChart::volumes() const
{
    QVector<qint64> result;
    for (const auto& point : d->data)
    {
        result.append(point.volume);
    }
    return result;
}

double TimeShareChart::basePrice() const
{
    return d->yesterdayClose;
}

void TimeShareChart::setStyle(const TimeShareStyle& style)
{
    d->style = style;
    update();
}

TimeShareStyle TimeShareChart::style() const
{
    return d->style;
}

void TimeShareChart::clear()
{
    d->data.clear();
    d->yesterdayClose = 0.0;
    d->minPrice = 0.0;
    d->maxPrice = 0.0;
    d->maxVolume = 0;
    update();
}

void TimeShareChart::refresh()
{
    d->calculatePriceRange();
    d->calculateVolumeRange();
    update();
}

// ========== 绘制方法 ==========

void TimeShareChart::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    d->calculateLayout(rect());

    drawBackground(&painter);
    drawGrid(&painter);
    drawBaseline(&painter);
    drawPriceArea(&painter);
    drawPriceLine(&painter);
    drawAvgLine(&painter);
    drawVolume(&painter);
    drawLabels(&painter);

    if (d->isMouseInChart && d->hoverIndex >= 0)
    {
        drawCrosshair(&painter);
        drawInfoBox(&painter);
    }
}

void TimeShareChart::drawBackground(QPainter* painter)
{
    // 主背景
    painter->fillRect(rect(), QColor(Tokens::Colors::BgBase));

    // 图表区域背景
    painter->fillRect(d->chartRect, QColor(Tokens::Colors::BgSurface));
    painter->fillRect(d->volumeRect, QColor(Tokens::Colors::BgSurface));
}

void TimeShareChart::drawGrid(QPainter* painter)
{
    if (!d->style.showGrid) return;

    painter->setPen(QPen(d->style.gridColor, 1));

    // 横向网格线（价格）
    int hLines = 5;
    for (int i = 0; i <= hLines; ++i)
    {
        int y = d->chartRect.top() + i * d->chartRect.height() / hLines;
        painter->drawLine(d->chartRect.left(), y, d->chartRect.right(), y);
    }

    // 纵向网格线（时间）
    int vLines = 4;
    for (int i = 0; i <= vLines; ++i)
    {
        int x = d->chartRect.left() + i * d->chartRect.width() / vLines;
        painter->drawLine(x, d->chartRect.top(), x, d->chartRect.bottom());
    }
}

void TimeShareChart::drawBaseline(QPainter* painter)
{
    if (d->yesterdayClose <= 0) return;

    int y = priceToY(d->yesterdayClose);

    QPen pen(d->style.baselineColor, d->style.baselineWidth, d->style.baselineStyle);
    painter->setPen(pen);
    painter->drawLine(d->chartRect.left(), y, d->chartRect.right(), y);

    // 标注"昨收"
    painter->setPen(QPen(d->style.baselineColor, 1));
    painter->setFont(QFont("Microsoft YaHei", d->style.labelFontSize));
    painter->drawText(d->chartRect.right() + 5, y + 4, QString("昨收 %1").arg(d->yesterdayClose, 0, 'f', 2));
}

void TimeShareChart::drawPriceArea(QPainter* painter)
{
    if (d->data.isEmpty()) return;

    QPainterPath path;
    bool first = true;

    for (const auto& point : d->data)
    {
        int x = timeToX(point.time);
        int y = priceToY(point.price);

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

    // 闭合路径
    if (!d->data.isEmpty())
    {
        path.lineTo(timeToX(d->data.last().time), d->chartRect.bottom());
        path.lineTo(timeToX(d->data.first().time), d->chartRect.bottom());
        path.closeSubpath();
    }

    // 根据涨跌选择填充颜色
    if (!d->data.isEmpty())
    {
        double lastPrice = d->data.last().price;
        QColor fillColor = (lastPrice >= d->yesterdayClose) ? d->style.upFillColor : d->style.downFillColor;
        painter->fillPath(path, fillColor);
    }
}

void TimeShareChart::drawPriceLine(QPainter* painter)
{
    if (d->data.isEmpty()) return;

    QPen pen(d->style.priceLineColor, d->style.priceLineWidth);
    painter->setPen(pen);

    for (int i = 1; i < d->data.size(); ++i)
    {
        int x1 = timeToX(d->data[i - 1].time);
        int y1 = priceToY(d->data[i - 1].price);
        int x2 = timeToX(d->data[i].time);
        int y2 = priceToY(d->data[i].price);

        painter->drawLine(x1, y1, x2, y2);
    }
}

void TimeShareChart::drawAvgLine(QPainter* painter)
{
    if (d->data.isEmpty()) return;

    QPen pen(d->style.avgLineColor, d->style.avgLineWidth);
    painter->setPen(pen);

    for (int i = 1; i < d->data.size(); ++i)
    {
        if (d->data[i - 1].avgPrice <= 0 || d->data[i].avgPrice <= 0) continue;

        int x1 = timeToX(d->data[i - 1].time);
        int y1 = priceToY(d->data[i - 1].avgPrice);
        int x2 = timeToX(d->data[i].time);
        int y2 = priceToY(d->data[i].avgPrice);

        painter->drawLine(x1, y1, x2, y2);
    }
}

void TimeShareChart::drawVolume(QPainter* painter)
{
    if (d->data.isEmpty()) return;

    for (int i = 0; i < d->data.size(); ++i)
    {
        const auto& point = d->data[i];
        int x = timeToX(point.time);
        int barHeight = static_cast<int>(point.volume * d->volumeScale);
        int y = d->volumeRect.bottom() - barHeight;

        // 根据价格涨跌选择颜色
        QColor color = (point.price >= d->yesterdayClose) ? d->style.volumeUpColor : d->style.volumeDownColor;

        painter->fillRect(x - d->style.volumeBarWidth / 2, y, d->style.volumeBarWidth, barHeight, color);
    }
}

void TimeShareChart::drawCrosshair(QPainter* painter)
{
    if (d->hoverIndex < 0 || d->hoverIndex >= d->data.size()) return;

    const auto& point = d->data[d->hoverIndex];
    int x = timeToX(point.time);
    int y = priceToY(point.price);

    QPen pen(d->style.crosshairColor, d->style.crosshairWidth, Qt::DashLine);
    painter->setPen(pen);

    // 垂直线
    painter->drawLine(x, d->chartRect.top(), x, d->volumeRect.bottom());

    // 水平线
    painter->drawLine(d->chartRect.left(), y, d->chartRect.right(), y);

    // 价格点
    painter->setBrush(d->style.priceLineColor);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QPoint(x, y), 4, 4);
}

void TimeShareChart::drawLabels(QPainter* painter)
{
    painter->setFont(QFont("Microsoft YaHei", d->style.labelFontSize));
    painter->setPen(QColor(Tokens::Colors::TextSecondary));

    // 价格标签
    int hLines = 5;
    for (int i = 0; i <= hLines; ++i)
    {
        double price = d->minPrice + (d->maxPrice - d->minPrice) * (hLines - i) / hLines;
        int y = d->chartRect.top() + i * d->chartRect.height() / hLines;

        QString text;
        if (d->yesterdayClose > 0)
        {
            double change = (price - d->yesterdayClose) / d->yesterdayClose * 100;
            text = QString("%1 (%2%)").arg(price, 0, 'f', 2).arg(change, 0, 'f', 2);
        }
        else
        {
            text = QString::number(price, 'f', 2);
        }

        painter->drawText(d->chartRect.right() + 5, y + 4, text);
    }

    // 时间标签
    painter->drawText(d->chartRect.left(), d->volumeRect.bottom() + 15, "09:30");
    painter->drawText(d->chartRect.left() + d->chartRect.width() / 3, d->volumeRect.bottom() + 15, "11:30");
    painter->drawText(d->chartRect.left() + d->chartRect.width() * 2 / 3, d->volumeRect.bottom() + 15, "14:00");
    painter->drawText(d->chartRect.right() - 30, d->volumeRect.bottom() + 15, "15:00");
}

void TimeShareChart::drawInfoBox(QPainter* painter)
{
    if (d->hoverIndex < 0 || d->hoverIndex >= d->data.size()) return;

    const auto& point = d->data[d->hoverIndex];

    // 信息框
    int boxWidth = 180;
    int boxHeight = 120;
    int boxX = qMin(d->mouseX + 10, width() - boxWidth - 10);
    int boxY = qMax(d->mouseY - boxHeight - 10, 10);

    QRect boxRect(boxX, boxY, boxWidth, boxHeight);

    // 背景
    painter->fillRect(boxRect, d->style.crosshairBgColor);
    painter->setPen(QPen(d->style.crosshairColor, 1));
    painter->drawRect(boxRect);

    // 文本
    painter->setFont(QFont("Microsoft YaHei", d->style.dataFontSize));

    double change = d->yesterdayClose > 0 ? (point.price - d->yesterdayClose) / d->yesterdayClose * 100 : 0;

    QStringList lines;
    lines << QString("时间: %1").arg(point.time.toString("HH:mm:ss"));
    lines << QString("价格: %1").arg(point.price, 0, 'f', 2);
    lines << QString("涨跌: %1%").arg(change, 0, 'f', 2);
    lines << QString("均价: %1").arg(point.avgPrice, 0, 'f', 2);
    lines << QString("成交量: %1").arg(point.volume);

    int textY = boxY + 20;
    for (const QString& line : lines)
    {
        painter->setPen(QColor(Tokens::Colors::TextPrimary));
        painter->drawText(boxX + 10, textY, line);
        textY += 20;
    }
}

// ========== 坐标转换 ==========

int TimeShareChart::timeToX(qint64 timestamp) const
{
    if (d->startTime >= d->endTime) return d->chartRect.left();

    double ratio = static_cast<double>(timestamp - d->startTime) / (d->endTime - d->startTime);
    return d->chartRect.left() + static_cast<int>(ratio * d->chartRect.width());
}

int TimeShareChart::timeToX(const QDateTime& time) const
{
    return timeToX(time.toMSecsSinceEpoch());
}

int TimeShareChart::priceToY(double price) const
{
    if (d->priceScale <= 0) return d->chartRect.top();

    return d->chartRect.bottom() - static_cast<int>((price - d->minPrice) * d->priceScale);
}

double TimeShareChart::yToPrice(int y) const
{
    if (d->priceScale <= 0) return d->minPrice;

    return d->minPrice + (d->chartRect.bottom() - y) / d->priceScale;
}

qint64 TimeShareChart::xToTime(int x) const
{
    if (d->chartRect.width() <= 0) return d->startTime;

    double ratio = static_cast<double>(x - d->chartRect.left()) / d->chartRect.width();
    return d->startTime + static_cast<qint64>(ratio * (d->endTime - d->startTime));
}

int TimeShareChart::findDataIndex(int x) const
{
    if (d->data.isEmpty()) return -1;

    qint64 targetTime = xToTime(x);

    // 二分查找
    int left = 0;
    int right = d->data.size() - 1;

    while (left < right)
    {
        int mid = (left + right) / 2;
        if (d->data[mid].time.toMSecsSinceEpoch() < targetTime)
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

// ========== 事件处理 ==========

void TimeShareChart::mouseMoveEvent(QMouseEvent* event)
{
    d->mouseX = event->position().x();
    d->mouseY = event->position().y();

    if (d->chartRect.contains(d->mouseX, d->mouseY) || d->volumeRect.contains(d->mouseX, d->mouseY))
    {
        d->isMouseInChart = true;
        d->hoverIndex = findDataIndex(d->mouseX);

        if (d->hoverIndex >= 0 && d->hoverIndex < d->data.size())
        {
            emit mouseMoved(d->data[d->hoverIndex], d->hoverIndex);
        }
    }
    else
    {
        d->isMouseInChart = false;
        d->hoverIndex = -1;
    }

    update();
}

void TimeShareChart::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
}

void TimeShareChart::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    d->isMouseInChart = false;
    d->hoverIndex = -1;
    emit mouseLeft();
    update();
}

void TimeShareChart::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    d->calculateLayout(rect());
    d->calculatePriceRange();
    d->calculateVolumeRange();
}
