/**
 * @file SignalMarker.cpp
 * @brief 信号标记组件实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "SignalMarker.h"
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <QMouseEvent>
#include <QDebug>
#include <QtMath>

namespace WealthPilot {
namespace UI {

struct SignalMarker::Impl {
    QVector<SignalMarkerData> markers;
    SignalMarkerStyle style;

    // 坐标映射函数
    std::function<int(int)> timeToX;
    std::function<int(double)> priceToY;

    // 可见范围
    int visibleStartIndex = 0;
    int visibleCount = 100;

    // 显示控制
    bool visible = true;
    Analysis::TheoryType filterTheory = Analysis::TheoryType::ElliottWave; // 默认显示所有

    // 鼠标悬停
    SignalMarkerData* hoveredMarker = nullptr;

    Impl() {
        style.labelFont = QFont("Microsoft YaHei", 8);
    }
};

SignalMarker::SignalMarker(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setMouseTracking(true);
}

SignalMarker::~SignalMarker() = default;

void SignalMarker::setSignals(const QVector<Analysis::UnifiedSignal>& signalList)
{
    d->markers.clear();

    for (const auto& signal : signalList) {
        SignalMarkerData marker;
        marker.direction = signal.direction;
        marker.strength = signal.strength;
        marker.theory = signal.source;
        marker.description = signal.description;
        marker.time = signal.time;
        marker.price = signal.price;
        d->markers.append(marker);
    }

    updatePositions();
    update();
}

void SignalMarker::addSignal(const Analysis::UnifiedSignal& unifiedSignal)
{
    SignalMarkerData marker;
    marker.direction = unifiedSignal.direction;
    marker.strength = unifiedSignal.strength;
    marker.theory = unifiedSignal.source;
    marker.description = unifiedSignal.description;
    marker.time = unifiedSignal.time;
    marker.price = unifiedSignal.price;

    d->markers.append(marker);
    updatePositions();
    update();
}

void SignalMarker::setCompositeSignal(const Analysis::CompositeSignal& compositeSignal)
{
    clearSignals();
    setSignals(compositeSignal.sourceSignals);
}

void SignalMarker::clearSignals()
{
    d->markers.clear();
    update();
}

QVector<SignalMarkerData> SignalMarker::markers() const
{
    return d->markers;
}

void SignalMarker::setStyle(const SignalMarkerStyle& style)
{
    d->style = style;
    update();
}

SignalMarkerStyle SignalMarker::style() const
{
    return d->style;
}

void SignalMarker::setCoordinateMapping(
    std::function<int(int)> timeToX,
    std::function<int(double)> priceToY)
{
    d->timeToX = timeToX;
    d->priceToY = priceToY;
    updatePositions();
}

void SignalMarker::updatePositions()
{
    if (!d->timeToX || !d->priceToY) {
        return;
    }

    for (auto& marker : d->markers) {
        // 计算X坐标（基于时间/K线索引）
        if (marker.barIndex >= 0) {
            marker.x = d->timeToX(marker.barIndex);
        } else {
            // 如果没有索引，根据时间计算（简化处理）
            marker.x = width() / 2;
        }

        // 计算Y坐标（基于价格）
        marker.y = d->priceToY(marker.price);

        // 根据信号方向调整位置
        if (marker.direction == Analysis::SignalDirection::Bullish) {
            // 买入信号显示在下方
            marker.y += d->style.markerSize + d->style.markerSpacing;
        } else if (marker.direction == Analysis::SignalDirection::Bearish) {
            // 卖出信号显示在上方
            marker.y -= d->style.markerSize + d->style.markerSpacing;
        }
    }

    update();
}

void SignalMarker::setVisibleRange(int startIndex, int count)
{
    d->visibleStartIndex = startIndex;
    d->visibleCount = count;
    update();
}

void SignalMarker::setVisible(bool visible)
{
    d->visible = visible;
    if (visible) {
        show();
    } else {
        hide();
    }
}

void SignalMarker::setFilterTheory(Analysis::TheoryType theory)
{
    d->filterTheory = theory;
    update();
}

void SignalMarker::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    if (!d->visible) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制所有信号标记
    for (const auto& marker : d->markers) {
        // 应用理论过滤
        if (d->filterTheory != Analysis::TheoryType::ElliottWave &&
            marker.theory != d->filterTheory) {
            continue;
        }

        // 检查是否在可见范围内
        if (marker.barIndex >= 0 &&
            (marker.barIndex < d->visibleStartIndex ||
             marker.barIndex >= d->visibleStartIndex + d->visibleCount)) {
            continue;
        }

        drawMarker(painter, marker);
    }
}

void SignalMarker::drawMarker(QPainter& painter, const SignalMarkerData& marker)
{
    switch (marker.direction) {
        case Analysis::SignalDirection::Bullish:
            drawBuySignal(painter, marker);
            break;
        case Analysis::SignalDirection::Bearish:
            drawSellSignal(painter, marker);
            break;
        default:
            drawNeutralSignal(painter, marker);
            break;
    }

    // 绘制标签
    if (d->style.showLabels && !marker.description.isEmpty()) {
        painter.setFont(d->style.labelFont);

        // 背景
        QFontMetrics fm(d->style.labelFont);
        QRect textRect = fm.boundingRect(marker.description);
        textRect.moveCenter(QPoint(marker.x, marker.y - d->style.markerSize - textRect.height() / 2 - 2));

        painter.fillRect(textRect.adjusted(-2, -1, 2, 1), QColor(0, 0, 0, 180));
        painter.setPen(Qt::white);
        painter.drawText(textRect, Qt::AlignCenter, marker.description);
    }
}

void SignalMarker::drawBuySignal(QPainter& painter, const SignalMarkerData& marker)
{
    QColor color = getMarkerColor(marker);
    painter.setPen(QPen(color, 2));
    painter.setBrush(color);

    // 绘制向上箭头
    int size = d->style.markerSize;
    int x = marker.x;
    int y = marker.y;

    QPainterPath path;
    path.moveTo(x, y - size);
    path.lineTo(x - size / 2, y);
    path.lineTo(x - size / 4, y);
    path.lineTo(x - size / 4, y + size / 2);
    path.lineTo(x + size / 4, y + size / 2);
    path.lineTo(x + size / 4, y);
    path.lineTo(x + size / 2, y);
    path.closeSubpath();

    painter.drawPath(path);

    // 根据强度添加装饰
    if (marker.strength == Analysis::SignalStrength::VeryStrong) {
        // 添加发光效果
        painter.setPen(QPen(color.lighter(150), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QPoint(x, y - size / 2), size, size);
    }
}

void SignalMarker::drawSellSignal(QPainter& painter, const SignalMarkerData& marker)
{
    QColor color = getMarkerColor(marker);
    painter.setPen(QPen(color, 2));
    painter.setBrush(color);

    // 绘制向下箭头
    int size = d->style.markerSize;
    int x = marker.x;
    int y = marker.y;

    QPainterPath path;
    path.moveTo(x, y + size);
    path.lineTo(x - size / 2, y);
    path.lineTo(x - size / 4, y);
    path.lineTo(x - size / 4, y - size / 2);
    path.lineTo(x + size / 4, y - size / 2);
    path.lineTo(x + size / 4, y);
    path.lineTo(x + size / 2, y);
    path.closeSubpath();

    painter.drawPath(path);

    // 根据强度添加装饰
    if (marker.strength == Analysis::SignalStrength::VeryStrong) {
        painter.setPen(QPen(color.lighter(150), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QPoint(x, y + size / 2), size, size);
    }
}

void SignalMarker::drawNeutralSignal(QPainter& painter, const SignalMarkerData& marker)
{
    QColor color = d->style.neutralColor;
    painter.setPen(QPen(color, 2));
    painter.setBrush(color);

    // 绘制圆形标记
    int size = d->style.markerSize / 2;
    painter.drawEllipse(QPoint(marker.x, marker.y), size, size);
}

QColor SignalMarker::getMarkerColor(const SignalMarkerData& marker) const
{
    if (marker.direction == Analysis::SignalDirection::Bullish) {
        if (marker.strength == Analysis::SignalStrength::VeryStrong ||
            marker.strength == Analysis::SignalStrength::Strong) {
            return d->style.buyStrongColor;
        }
        return d->style.buyColor;
    } else if (marker.direction == Analysis::SignalDirection::Bearish) {
        if (marker.strength == Analysis::SignalStrength::VeryStrong ||
            marker.strength == Analysis::SignalStrength::Strong) {
            return d->style.sellStrongColor;
        }
        return d->style.sellColor;
    }

    return d->style.neutralColor;
}

SignalMarkerData* SignalMarker::getMarkerAt(const QPoint& pos)
{
    for (auto& marker : d->markers) {
        int dx = pos.x() - marker.x;
        int dy = pos.y() - marker.y;
        int distance = static_cast<int>(qSqrt(dx * dx + dy * dy));

        if (distance <= d->style.markerSize) {
            return &marker;
        }
    }

    return nullptr;
}

void SignalMarker::mouseMoveEvent(QMouseEvent* event)
{
    auto* marker = getMarkerAt(event->pos());

    if (marker != d->hoveredMarker) {
        d->hoveredMarker = marker;

        if (marker) {
            // 显示工具提示
            QString tooltip = QString("%1\n%2\n置信度: %3%\n强度: %4")
                .arg(marker->theory == Analysis::TheoryType::ElliottWave ? "波浪理论" :
                     marker->theory == Analysis::TheoryType::ChanLun ? "缠论" :
                     marker->theory == Analysis::TheoryType::DowTheory ? "道氏理论" : "量价形态")
                .arg(marker->description)
                .arg(marker->strength == Analysis::SignalStrength::VeryStrong ? 95 :
                     marker->strength == Analysis::SignalStrength::Strong ? 75 :
                     marker->strength == Analysis::SignalStrength::Moderate ? 55 : 35)
                .arg(marker->strength == Analysis::SignalStrength::VeryStrong ? "极强" :
                     marker->strength == Analysis::SignalStrength::Strong ? "强" :
                     marker->strength == Analysis::SignalStrength::Moderate ? "中" : "弱");

            QToolTip::showText(mapToGlobal(event->pos()), tooltip);
            emit signalHovered(*marker);
        } else {
            QToolTip::hideText();
        }
    }

    QWidget::mouseMoveEvent(event);
}

void SignalMarker::mousePressEvent(QMouseEvent* event)
{
    auto* marker = getMarkerAt(event->pos());

    if (marker) {
        emit signalClicked(*marker);
    }

    QWidget::mousePressEvent(event);
}

void SignalMarker::leaveEvent(QEvent* event)
{
    d->hoveredMarker = nullptr;
    QToolTip::hideText();
    QWidget::leaveEvent(event);
}

} // namespace UI
} // namespace WealthPilot
