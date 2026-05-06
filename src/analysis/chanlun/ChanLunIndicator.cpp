/**
 * @file ChanLunIndicator.cpp
 * @brief 缠论指标可视化实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ChanLunIndicator.h"
#include "core/config/Tokens.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QBrush>
#include <QFont>

namespace WealthPilot {
namespace ChanLun {

// ============================================================================
// ChanLunIndicator::Impl
// ============================================================================

struct ChanLunIndicator::Impl {
    ChanLunResult result;
    std::function<double(const QDateTime&)> timeToX;
    std::function<double(double)> priceToY;
    
    bool showPens = true;
    bool showSegments = true;
    bool showPivots = true;
    bool showSignals = true;
    bool showFractals = false;
    
    QVector<QGraphicsItem*> items;
};

// ============================================================================
// ChanLunIndicator 实现
// ============================================================================

ChanLunIndicator::ChanLunIndicator(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
}

ChanLunIndicator::~ChanLunIndicator()
{
    clearGraphics();
}

void ChanLunIndicator::setResult(const ChanLunResult& result)
{
    d->result = result;
    updateGraphics();
}

void ChanLunIndicator::setCoordinateTransform(std::function<double(const QDateTime&)> timeToX,
                                               std::function<double(double)> priceToY)
{
    d->timeToX = timeToX;
    d->priceToY = priceToY;
}

void ChanLunIndicator::setShowPens(bool show) { d->showPens = show; updateGraphics(); }
void ChanLunIndicator::setShowSegments(bool show) { d->showSegments = show; updateGraphics(); }
void ChanLunIndicator::setShowPivots(bool show) { d->showPivots = show; updateGraphics(); }
void ChanLunIndicator::setShowSignals(bool show) { d->showSignals = show; updateGraphics(); }
void ChanLunIndicator::setShowFractals(bool show) { d->showFractals = show; updateGraphics(); }

QVector<QGraphicsItem*> ChanLunIndicator::graphicsItems() const
{
    return d->items;
}

void ChanLunIndicator::updateGraphics()
{
    clearGraphics();
    
    if (!d->timeToX || !d->priceToY) {
        return;
    }
    
    if (d->showPens) {
        createPenGraphics();
    }
    
    if (d->showSegments) {
        createSegmentGraphics();
    }
    
    if (d->showPivots) {
        createPivotGraphics();
    }
    
    if (d->showSignals) {
        createSignalGraphics();
    }
    
    if (d->showFractals) {
        createFractalGraphics();
    }
    
    emit graphicsUpdated();
}

void ChanLunIndicator::createPenGraphics()
{
    for (const auto& pen : d->result.pens) {
        auto* item = new PenGraphicsItem(pen);
        
        // 设置坐标
        double x1 = d->timeToX(pen.startTime);
        double y1 = d->priceToY(pen.startValue);
        double x2 = d->timeToX(pen.endTime);
        double y2 = d->priceToY(pen.endValue);
        
        QPainterPath path;
        path.moveTo(x1, y1);
        path.lineTo(x2, y2);
        item->setPath(path);
        
        // 设置颜色
        QColor color = pen.isUp() ? QColor(Tokens::Colors::Danger) : QColor(Tokens::Colors::Success);
        item->setPen(QPen(color, 2));
        
        d->items.append(item);
    }
}

void ChanLunIndicator::createSegmentGraphics()
{
    for (const auto& seg : d->result.segments) {
        auto* pathItem = new QGraphicsPathItem();
        
        double x1 = d->timeToX(seg.startTime);
        double y1 = d->priceToY(seg.startValue);
        double x2 = d->timeToX(seg.endTime);
        double y2 = d->priceToY(seg.endValue);
        
        QPainterPath path;
        path.moveTo(x1, y1);
        path.lineTo(x2, y2);
        pathItem->setPath(path);
        
        // 线段用更粗的线
        QColor color = seg.direction == SegmentDirection::Up 
            ? QColor(Tokens::Colors::Danger) 
            : QColor(Tokens::Colors::Success);
        pathItem->setPen(QPen(color, 3));
        
        d->items.append(pathItem);
    }
}

void ChanLunIndicator::createPivotGraphics()
{
    for (const auto& pivot : d->result.pivots) {
        auto* item = new PivotGraphicsItem(pivot);
        
        double x1 = d->timeToX(pivot.startTime);
        double x2 = d->timeToX(pivot.endTime);
        double y1 = d->priceToY(pivot.zd); // 上沿
        double y2 = d->priceToY(pivot.zg); // 下沿
        
        item->setRect(x1, y1, x2 - x1, y2 - y1);
        
        // 中枢用半透明填充
        QColor fillColor(100, 100, 255, 50);
        item->setBrush(QBrush(fillColor));
        item->setPen(QPen(QColor(100, 100, 255), 1, Qt::DashLine));
        
        d->items.append(item);
    }
}

void ChanLunIndicator::createSignalGraphics()
{
    for (const auto& signal : d->result.tradeSignals) {
        auto* item = new SignalGraphicsItem(signal);
        
        double x = d->timeToX(signal.time);
        double y = d->priceToY(signal.price);
        
        item->setPos(x, y);
        
        d->items.append(item);
    }
}

void ChanLunIndicator::createFractalGraphics()
{
    for (const auto& fractal : d->result.fractals) {
        if (!fractal.isValid()) continue;
        
        auto* textItem = new QGraphicsTextItem();
        
        double x = d->timeToX(fractal.time);
        double y = d->priceToY(fractal.value);
        
        textItem->setPos(x, y);
        textItem->setPlainText(fractal.isTop() ? "顶" : "底");
        textItem->setDefaultTextColor(fractal.isTop() 
            ? QColor(Tokens::Colors::Danger) 
            : QColor(Tokens::Colors::Success));
        
        d->items.append(textItem);
    }
}

void ChanLunIndicator::clearGraphics()
{
    for (auto* item : d->items) {
        delete item;
    }
    d->items.clear();
}

// ============================================================================
// PenGraphicsItem 实现
// ============================================================================

PenGraphicsItem::PenGraphicsItem(const Pen& pen, QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
    , m_pen(pen)
{
    setZValue(10); // 笔在K线上方
}

// ============================================================================
// PivotGraphicsItem 实现
// ============================================================================

PivotGraphicsItem::PivotGraphicsItem(const Pivot& pivot, QGraphicsItem* parent)
    : QGraphicsRectItem(parent)
    , m_pivot(pivot)
{
    setZValue(5); // 中枢在K线下方
}

// ============================================================================
// SignalGraphicsItem 实现
// ============================================================================

SignalGraphicsItem::SignalGraphicsItem(const TradeSignal& signal, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_signal(signal)
{
    setZValue(20); // 信号在最上层
    m_boundingRect = QRectF(-15, -15, 30, 30);
    
    setAcceptHoverEvents(true);
}

QRectF SignalGraphicsItem::boundingRect() const
{
    return m_boundingRect;
}

void SignalGraphicsItem::paint(QPainter* painter, 
                                const QStyleOptionGraphicsItem* option,
                                QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    // 根据信号类型选择颜色和形状
    QColor color;
    QString text;
    
    switch (m_signal.type) {
        case SignalType::Buy1:
            color = QColor(Tokens::Colors::Danger);
            text = "一买";
            break;
        case SignalType::Sell1:
            color = QColor(Tokens::Colors::Success);
            text = "一卖";
            break;
        case SignalType::Buy2:
            color = QColor(255, 165, 0); // 橙色
            text = "二买";
            break;
        case SignalType::Sell2:
            color = QColor(0, 191, 255); // 深天蓝
            text = "二卖";
            break;
        case SignalType::Buy3:
            color = QColor(148, 0, 211); // 深紫
            text = "三买";
            break;
        case SignalType::Sell3:
            color = QColor(0, 128, 128); // 深青
            text = "三卖";
            break;
        default:
            return;
    }
    
    // 绘制标记
    painter->setRenderHint(QPainter::Antialiasing);
    
    // 外圈
    painter->setPen(QPen(color, 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawEllipse(boundingRect().center(), 12, 12);
    
    // 内圈填充
    painter->setBrush(QBrush(color.lighter(150)));
    painter->drawEllipse(boundingRect().center(), 8, 8);
    
    // 文字
    QFont font;
    font.setPointSize(8);
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect(), Qt::AlignCenter, text);
}

void SignalGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
    // 可以在这里发出点击信号
}

} // namespace ChanLun
} // namespace WealthPilot
