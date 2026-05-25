/**
 * @file DrawingTool.cpp
 * @brief K线图画线工具实现
 */

#include "DrawingTool.h"
#include "KLineChart.h"
#include "core/config/Tokens.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QUuid>
#include <QtMath>

// ========== 构造函数和析构函数 ==========

DrawingToolManager::DrawingToolManager(KLineChart* chart, QObject* parent)
    : QObject(parent)
      , m_chart(chart)
{
}

DrawingToolManager::~DrawingToolManager() = default;

// ========== 工具选择 ==========

void DrawingToolManager::setCurrentTool(DrawingType type)
{
    if (m_currentTool != type)
    {
        m_currentTool = type;
        m_isDrawing = false;
        m_currentDrawing.reset();
        emit toolChanged(type);
    }
}

// ========== 绘图对象管理 ==========

void DrawingToolManager::addDrawing(std::shared_ptr<DrawingObject> drawing)
{
    if (!drawing) return;

    m_drawings.append(drawing);
    emit drawingAdded(drawing->id);
}

void DrawingToolManager::removeDrawing(const QString& id)
{
    for (int i = m_drawings.size() - 1; i >= 0; --i)
    {
        if (m_drawings[i]->id == id)
        {
            m_drawings.removeAt(i);
            emit drawingRemoved(id);
            break;
        }
    }
}

void DrawingToolManager::clearAllDrawings()
{
    QStringList ids;
    for (const auto& d : m_drawings)
    {
        ids.append(d->id);
    }

    m_drawings.clear();
    m_selectedId.clear();

    for (const QString& id : ids)
    {
        emit drawingRemoved(id);
    }
}

std::shared_ptr<DrawingObject> DrawingToolManager::getDrawing(const QString& id) const
{
    for (const auto& d : m_drawings)
    {
        if (d->id == id)
        {
            return d;
        }
    }
    return nullptr;
}

QVector<std::shared_ptr<DrawingObject>> DrawingToolManager::allDrawings() const
{
    return m_drawings;
}

// ========== 选择管理 ==========

void DrawingToolManager::selectDrawing(const QString& id)
{
    // 取消之前的选择
    for (auto& d : m_drawings)
    {
        d->selected = (d->id == id);
    }

    m_selectedId = id;
    emit drawingSelected(id);
}

void DrawingToolManager::deselectAll()
{
    for (auto& d : m_drawings)
    {
        d->selected = false;
    }
    m_selectedId.clear();
}

QString DrawingToolManager::selectedDrawingId() const
{
    return m_selectedId;
}

// ========== 持久化 ==========

bool DrawingToolManager::saveToFile(const QString& filePath)
{
    QJsonArray json = toJson();

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    QJsonDocument doc(json);
    file.write(doc.toJson());
    file.close();

    return true;
}

bool DrawingToolManager::loadFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    fromJson(doc.array());
    return true;
}

QJsonArray DrawingToolManager::toJson() const
{
    QJsonArray array;
    for (const auto& d : m_drawings)
    {
        array.append(d->toJson());
    }
    return array;
}

void DrawingToolManager::fromJson(const QJsonArray& json)
{
    m_drawings.clear();

    for (const QJsonValue& value : json)
    {
        auto obj = DrawingObject::fromJson(value.toObject());
        if (obj)
        {
            m_drawings.append(obj);
        }
    }
}

// ========== 绘制 ==========

void DrawingToolManager::paint(QPainter* painter)
{
    for (const auto& drawing : m_drawings)
    {
        switch (drawing->type)
        {
        case DrawingType::TrendLine:
            drawTrendLine(painter, *std::static_pointer_cast<TrendLine>(drawing));
            break;
        case DrawingType::HorizontalLine:
            drawHorizontalLine(painter, *std::static_pointer_cast<HorizontalLine>(drawing));
            break;
        case DrawingType::ParallelChannel:
            drawParallelChannel(painter, *std::static_pointer_cast<ParallelChannel>(drawing));
            break;
        case DrawingType::Rectangle:
            drawRectangle(painter, *std::static_pointer_cast<RectangleArea>(drawing));
            break;
        case DrawingType::Fibonacci:
            drawFibonacci(painter, *std::static_pointer_cast<FibonacciRetracement>(drawing));
            break;
        case DrawingType::Text:
            drawText(painter, *std::static_pointer_cast<TextAnnotation>(drawing));
            break;
        default:
            break;
        }
    }

    // 绘制当前正在创建的对象
    if (m_isDrawing && m_currentDrawing)
    {
        // TODO: 绘制临时对象
    }
}

void DrawingToolManager::drawTrendLine(QPainter* painter, const TrendLine& line)
{
    QPoint p1 = chartToScreen(line.startPoint);
    QPoint p2 = chartToScreen(line.endPoint);

    QPen pen(line.color, line.lineWidth, line.lineStyle);
    if (line.selected)
    {
        pen.setWidth(line.lineWidth + 2);
    }
    painter->setPen(pen);
    painter->drawLine(p1, p2);

    // 绘制端点
    if (line.selected)
    {
        painter->setBrush(line.color);
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(p1, 5, 5);
        painter->drawEllipse(p2, 5, 5);
    }
}

void DrawingToolManager::drawHorizontalLine(QPainter* painter, const HorizontalLine& line)
{
    // 水平线跨越整个图表宽度
    QRect chartRect = m_chart->rect();
    int y = chartToScreen(QPointF(0, line.price)).y();

    QPen pen(line.color, line.lineWidth, line.lineStyle);
    if (line.selected)
    {
        pen.setWidth(line.lineWidth + 2);
    }
    painter->setPen(pen);
    painter->drawLine(chartRect.left(), y, chartRect.right(), y);

    // 显示价格标签
    painter->setFont(QFont("Microsoft YaHei", 10));
    painter->setPen(line.color);
    QString priceText = QString::number(line.price, 'f', 2);
    painter->drawText(chartRect.right() - 80, y - 5, priceText);
}

void DrawingToolManager::drawParallelChannel(QPainter* painter, const ParallelChannel& channel)
{
    QPoint p1 = chartToScreen(channel.point1);
    QPoint p2 = chartToScreen(channel.point2);

    // 计算平行线
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    double len = qSqrt(dx * dx + dy * dy);

    if (len < 1) return;

    // 垂直方向偏移
    double offsetX = -dy / len * channel.width;
    double offsetY = dx / len * channel.width;

    QPoint p3(p1.x() + offsetX, p1.y() + offsetY);
    QPoint p4(p2.x() + offsetX, p2.y() + offsetY);

    QPen pen(channel.color, channel.lineWidth, channel.lineStyle);
    painter->setPen(pen);

    // 绘制两条平行线
    painter->drawLine(p1, p2);
    painter->drawLine(p3, p4);

    // 填充区域
    QPainterPath path;
    path.moveTo(p1);
    path.lineTo(p2);
    path.lineTo(p4);
    path.lineTo(p3);
    path.closeSubpath();

    QColor fillColor = channel.color;
    fillColor.setAlpha(30);
    painter->fillPath(path, fillColor);
}

void DrawingToolManager::drawRectangle(QPainter* painter, const RectangleArea& rect)
{
    QPoint tl = chartToScreen(rect.topLeft);
    QPoint br = chartToScreen(rect.bottomRight);

    QRect r(tl, br);

    QPen pen(rect.color, rect.lineWidth);
    painter->setPen(pen);
    painter->drawRect(r);

    if (rect.fillColor.isValid())
    {
        painter->fillRect(r, rect.fillColor);
    }
}

void DrawingToolManager::drawFibonacci(QPainter* painter, const FibonacciRetracement& fib)
{
    QPoint p1 = chartToScreen(fib.startPoint);
    QPoint p2 = chartToScreen(fib.endPoint);

    QPen pen(fib.color, fib.lineWidth);
    painter->setPen(pen);

    // 绘制主线
    painter->drawLine(p1, p2);

    if (fib.showLevels)
    {
        // 计算斐波那契回调位
        double high = qMax(fib.startPoint.y(), fib.endPoint.y());
        double low = qMin(fib.startPoint.y(), fib.endPoint.y());
        double range = high - low;

        QVector<double> levels = {0.0, 0.236, 0.382, 0.5, 0.618, 0.786, 1.0};
        QVector<QString> labels = {"0%", "23.6%", "38.2%", "50%", "61.8%", "78.6%", "100%"};

        QFont font("Microsoft YaHei", 9);
        painter->setFont(font);

        int left = qMin(p1.x(), p2.x());
        int right = qMax(p1.x(), p2.x());

        for (int i = 0; i < levels.size(); ++i)
        {
            double price = low + range * levels[i];
            int y = chartToScreen(QPointF(0, price)).y();

            // 绘制水平线
            painter->setPen(QPen(fib.color, 1, Qt::DashLine));
            painter->drawLine(left, y, right, y);

            // 绘制标签
            painter->setPen(fib.color);
            painter->drawText(right + 5, y + 4, labels[i]);
        }
    }
}

void DrawingToolManager::drawText(QPainter* painter, const TextAnnotation& text)
{
    QPoint pos = chartToScreen(text.position);

    QFont font("Microsoft YaHei", text.fontSize);
    painter->setFont(font);
    painter->setPen(text.color);
    painter->drawText(pos, text.text);
}

// ========== 事件处理 ==========

bool DrawingToolManager::mousePressEvent(QMouseEvent* event)
{
    if (m_currentTool == DrawingType::None)
    {
        // 选择模式
        QString hitId = hitTest(event->pos());
        if (!hitId.isEmpty())
        {
            selectDrawing(hitId);
            return true;
        }
        deselectAll();
        return false;
    }

    // 开始绘制
    m_isDrawing = true;
    m_startPoint = screenToChart(event->pos());

    // 创建对应的绘图对象
    switch (m_currentTool)
    {
    case DrawingType::TrendLine:
        {
            auto line = std::make_shared<TrendLine>();
            line->id = generateId();
            line->color = m_defaultColor;
            line->lineWidth = m_defaultLineWidth;
            line->startPoint = m_startPoint;
            line->endPoint = m_startPoint;
            line->createTime = QDateTime::currentDateTime();
            m_currentDrawing = line;
            break;
        }
    case DrawingType::HorizontalLine:
        {
            auto line = std::make_shared<HorizontalLine>();
            line->id = generateId();
            line->color = m_defaultColor;
            line->lineWidth = m_defaultLineWidth;
            line->price = m_startPoint.y();
            line->createTime = QDateTime::currentDateTime();
            m_currentDrawing = line;
            break;
        }
    default:
        break;
    }

    return true;
}

bool DrawingToolManager::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_isDrawing || !m_currentDrawing)
    {
        return false;
    }

    QPointF currentPoint = screenToChart(event->pos());

    // 更新终点
    switch (m_currentTool)
    {
    case DrawingType::TrendLine:
        {
            auto line = std::static_pointer_cast<TrendLine>(m_currentDrawing);
            line->endPoint = currentPoint;
            break;
        }
    default:
        break;
    }

    return true;
}

bool DrawingToolManager::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_isDrawing || !m_currentDrawing)
    {
        return false;
    }

    // 完成绘制
    addDrawing(m_currentDrawing);
    m_currentDrawing.reset();
    m_isDrawing = false;

    return true;
}

bool DrawingToolManager::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        if (!m_selectedId.isEmpty())
        {
            removeDrawing(m_selectedId);
            return true;
        }
    }

    if (event->key() == Qt::Key_Escape)
    {
        if (m_isDrawing)
        {
            m_isDrawing = false;
            m_currentDrawing.reset();
            return true;
        }
        deselectAll();
        return true;
    }

    return false;
}

// ========== 坐标转换 ==========

QPointF DrawingToolManager::screenToChart(const QPoint& screenPos) const
{
    // TODO: 调用 KLineChart 的坐标转换方法
    // 暂时返回简单映射
    return QPointF(screenPos.x(), screenPos.y());
}

QPoint DrawingToolManager::chartToScreen(const QPointF& chartPos) const
{
    // TODO: 调用 KLineChart 的坐标转换方法
    return QPoint(chartPos.x(), chartPos.y());
}

// ========== 命中测试 ==========

QString DrawingToolManager::hitTest(const QPoint& screenPos) const
{
    const int hitRadius = 10;

    for (const auto& drawing : m_drawings)
    {
        switch (drawing->type)
        {
        case DrawingType::TrendLine:
            {
                auto line = std::static_pointer_cast<TrendLine>(drawing);
                QPoint p1 = chartToScreen(line->startPoint);
                QPoint p2 = chartToScreen(line->endPoint);

                // 检查是否在线段附近
                // 简化距离计算
                double dx = p2.x() - p1.x();
                double dy = p2.y() - p1.y();
                double len = qSqrt(dx * dx + dy * dy);
                if (len < 1) len = 1;

                // 点到线段的距离
                double t = qMax(0.0, qMin(1.0,
                                          ((screenPos.x() - p1.x()) * dx + (screenPos.y() - p1.y()) * dy) / (len *
                                              len)));
                double nearX = p1.x() + t * dx;
                double nearY = p1.y() + t * dy;
                double dist = qSqrt((screenPos.x() - nearX) * (screenPos.x() - nearX) +
                    (screenPos.y() - nearY) * (screenPos.y() - nearY));

                if (dist < hitRadius)
                {
                    return drawing->id;
                }
                break;
            }
        case DrawingType::HorizontalLine:
            {
                auto line = std::static_pointer_cast<HorizontalLine>(drawing);
                int y = chartToScreen(QPointF(0, line->price)).y();

                if (qAbs(screenPos.y() - y) < hitRadius)
                {
                    return drawing->id;
                }
                break;
            }
        default:
            break;
        }
    }

    return QString();
}

// ========== 工具方法 ==========

QString DrawingToolManager::generateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// ========== 序列化 ==========

QJsonObject DrawingObject::toJson() const
{
    QJsonObject json;
    json["id"] = id;
    json["type"] = static_cast<int>(type);
    json["color"] = color.name();
    json["lineWidth"] = lineWidth;
    json["lineStyle"] = static_cast<int>(lineStyle);
    json["createTime"] = createTime.toMSecsSinceEpoch();
    json["text"] = text;
    return json;
}

std::shared_ptr<DrawingObject> DrawingObject::fromJson(const QJsonObject& json)
{
    DrawingType type = static_cast<DrawingType>(json["type"].toInt());

    std::shared_ptr<DrawingObject> obj;

    switch (type)
    {
    case DrawingType::TrendLine:
        obj = std::make_shared<TrendLine>();
        break;
    case DrawingType::HorizontalLine:
        obj = std::make_shared<HorizontalLine>();
        break;
    case DrawingType::ParallelChannel:
        obj = std::make_shared<ParallelChannel>();
        break;
    case DrawingType::Rectangle:
        obj = std::make_shared<RectangleArea>();
        break;
    case DrawingType::Fibonacci:
        obj = std::make_shared<FibonacciRetracement>();
        break;
    case DrawingType::Text:
        obj = std::make_shared<TextAnnotation>();
        break;
    default:
        return nullptr;
    }

    obj->id = json["id"].toString();
    obj->color = QColor(json["color"].toString());
    obj->lineWidth = json["lineWidth"].toInt();
    obj->lineStyle = static_cast<Qt::PenStyle>(json["lineStyle"].toInt());
    obj->createTime = QDateTime::fromMSecsSinceEpoch(json["createTime"].toVariant().toLongLong());
    obj->text = json["text"].toString();

    return obj;
}

QJsonObject TrendLine::toJson() const
{
    QJsonObject json = DrawingObject::toJson();
    json["startX"] = startPoint.x();
    json["startY"] = startPoint.y();
    json["endX"] = endPoint.x();
    json["endY"] = endPoint.y();
    return json;
}

QJsonObject HorizontalLine::toJson() const
{
    QJsonObject json = DrawingObject::toJson();
    json["price"] = price;
    return json;
}

QJsonObject ParallelChannel::toJson() const
{
    QJsonObject json = DrawingObject::toJson();
    json["point1X"] = point1.x();
    json["point1Y"] = point1.y();
    json["point2X"] = point2.x();
    json["point2Y"] = point2.y();
    json["width"] = width;
    return json;
}

QJsonObject RectangleArea::toJson() const
{
    QJsonObject json = DrawingObject::toJson();
    json["topLeftX"] = topLeft.x();
    json["topLeftY"] = topLeft.y();
    json["bottomRightX"] = bottomRight.x();
    json["bottomRightY"] = bottomRight.y();
    json["fillColor"] = fillColor.name();
    return json;
}

QJsonObject FibonacciRetracement::toJson() const
{
    QJsonObject json = DrawingObject::toJson();
    json["startX"] = startPoint.x();
    json["startY"] = startPoint.y();
    json["endX"] = endPoint.x();
    json["endY"] = endPoint.y();
    json["showLevels"] = showLevels;
    return json;
}

QJsonObject TextAnnotation::toJson() const
{
    QJsonObject json = DrawingObject::toJson();
    json["posX"] = position.x();
    json["posY"] = position.y();
    json["text"] = text;
    json["fontSize"] = fontSize;
    return json;
}
