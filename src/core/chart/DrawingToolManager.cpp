/**
 * @file DrawingToolManager.cpp
 * @brief 画线工具管理器实现
 */

#include "DrawingToolManager.h"
#include "utils/Logger.h"
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

DrawingToolManager* DrawingToolManager::instance()
{
    static DrawingToolManager* inst = new DrawingToolManager();
    return inst;
}

DrawingToolManager::DrawingToolManager(QObject* parent)
    : QObject(parent)
{
    LOG_INFO("DrawingToolManager initialized");
}

void DrawingToolManager::setCurrentTool(DrawingToolType type)
{
    m_currentTool = type;
    LOG_DEBUG(QString("Tool changed to: %1").arg(static_cast<int>(type)));
}

void DrawingToolManager::startDrawing(const QPointF& point)
{
    if (m_currentTool == DrawingToolType::None) {
        return;
    }

    m_isDrawing = true;
    m_currentPoints.clear();
    m_currentPoints.append(point);

    emit drawingStarted(point);
    LOG_DEBUG(QString("Drawing started at (%1, %2)").arg(point.x()).arg(point.y()));
}

void DrawingToolManager::updateDrawing(const QPointF& point)
{
    if (!m_isDrawing) return;

    // 更新当前点
    if (m_currentPoints.size() >= 2) {
        m_currentPoints.removeLast();
    }
    m_currentPoints.append(point);

    // 创建临时绘图对象
    m_currentDrawing = createDrawing(m_currentTool, m_currentPoints);
    emit drawingUpdated(m_currentDrawing);
}

ChartDrawingObject DrawingToolManager::finishDrawing(const QPointF& point)
{
    if (!m_isDrawing) {
        return ChartDrawingObject();
    }

    // 更新终点
    if (m_currentPoints.size() >= 2) {
        m_currentPoints.removeLast();
    }
    m_currentPoints.append(point);

    // 创建最终绘图对象
    ChartDrawingObject drawing = createDrawing(m_currentTool, m_currentPoints);
    drawing.id = generateId();
    drawing.createTime = QDateTime::currentDateTime();
    drawing.updateTime = drawing.createTime;

    // 添加到列表
    m_drawings.append(drawing);

    m_isDrawing = false;
    m_currentPoints.clear();

    emit drawingFinished(drawing);
    LOG_INFO(QString("Drawing finished: %1").arg(drawing.id));

    return drawing;
}

void DrawingToolManager::cancelDrawing()
{
    m_isDrawing = false;
    m_currentPoints.clear();
    m_currentDrawing = ChartDrawingObject();
    LOG_DEBUG("Drawing cancelled");
}

void DrawingToolManager::addDrawing(const ChartDrawingObject& drawing)
{
    m_drawings.append(drawing);
    LOG_INFO(QString("Drawing added: %1").arg(drawing.id));
}

void DrawingToolManager::updateDrawing(const QString& id, const ChartDrawingObject& drawing)
{
    for (int i = 0; i < m_drawings.size(); ++i) {
        if (m_drawings[i].id == id) {
            m_drawings[i] = drawing;
            m_drawings[i].updateTime = QDateTime::currentDateTime();
            LOG_DEBUG(QString("Drawing updated: %1").arg(id));
            break;
        }
    }
}

void DrawingToolManager::deleteDrawing(const QString& id)
{
    for (int i = 0; i < m_drawings.size(); ++i) {
        if (m_drawings[i].id == id) {
            m_drawings.removeAt(i);
            emit drawingDeleted(id);
            LOG_INFO(QString("Drawing deleted: %1").arg(id));
            break;
        }
    }
}

void DrawingToolManager::clearAllDrawings()
{
    m_drawings.clear();
    LOG_INFO("All drawings cleared");
}

ChartDrawingObject DrawingToolManager::getDrawing(const QString& id) const
{
    for (const ChartDrawingObject& drawing : m_drawings) {
        if (drawing.id == id) {
            return drawing;
        }
    }
    return ChartDrawingObject();
}

void DrawingToolManager::selectDrawing(const QString& id)
{
    m_selectedId = id;
    emit selectionChanged(id);
    LOG_DEBUG(QString("Drawing selected: %1").arg(id));
}

void DrawingToolManager::deselectAll()
{
    m_selectedId.clear();
    emit selectionChanged(QString());
}

void DrawingToolManager::setDefaultColor(const QColor& color)
{
    m_defaultColor = color;
}

void DrawingToolManager::setDefaultLineWidth(int width)
{
    m_defaultLineWidth = width;
}

void DrawingToolManager::setDefaultLineStyle(Qt::PenStyle style)
{
    m_defaultLineStyle = style;
}

QVector<FibonacciLevel> DrawingToolManager::calculateFibonacciLevels(double high, double low) const
{
    QVector<FibonacciLevel> levels;
    double diff = high - low;

    // 标准斐波那契回调水平
    QVector<double> fibLevels = {0.0, 0.236, 0.382, 0.5, 0.618, 0.786, 1.0};
    QStringList labels = {"0%", "23.6%", "38.2%", "50%", "61.8%", "78.6%", "100%"};

    for (int i = 0; i < fibLevels.size(); ++i) {
        FibonacciLevel level;
        level.level = fibLevels[i];
        level.price = high - diff * fibLevels[i];
        level.label = labels[i];
        levels.append(level);
    }

    return levels;
}

QString DrawingToolManager::exportToJson() const
{
    QJsonArray array;
    for (const ChartDrawingObject& drawing : m_drawings) {
        QJsonObject obj;
        obj["id"] = drawing.id;
        obj["type"] = static_cast<int>(drawing.type);
        obj["color"] = drawing.color.name();
        obj["lineWidth"] = drawing.lineWidth;
        obj["lineStyle"] = static_cast<int>(drawing.lineStyle);
        obj["visible"] = drawing.visible;
        obj["locked"] = drawing.locked;
        obj["text"] = drawing.text;

        QJsonArray points;
        for (const QPointF& p : drawing.points) {
            QJsonObject pt;
            pt["x"] = p.x();
            pt["y"] = p.y();
            points.append(pt);
        }
        obj["points"] = points;

        array.append(obj);
    }

    return QJsonDocument(array).toJson();
}

bool DrawingToolManager::importFromJson(const QString& json)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return false;
    }

    QJsonArray array = doc.array();
    m_drawings.clear();

    for (const QJsonValue& val : array) {
        QJsonObject obj = val.toObject();
        ChartDrawingObject drawing;
        drawing.id = obj["id"].toString();
        drawing.type = static_cast<DrawingToolType>(obj["type"].toInt());
        drawing.color = QColor(obj["color"].toString());
        drawing.lineWidth = obj["lineWidth"].toInt();
        drawing.lineStyle = static_cast<Qt::PenStyle>(obj["lineStyle"].toInt());
        drawing.visible = obj["visible"].toBool();
        drawing.locked = obj["locked"].toBool();
        drawing.text = obj["text"].toString();

        QJsonArray points = obj["points"].toArray();
        for (const QJsonValue& pv : points) {
            QJsonObject pt = pv.toObject();
            drawing.points.append(QPointF(pt["x"].toDouble(), pt["y"].toDouble()));
        }

        m_drawings.append(drawing);
    }

    LOG_INFO(QString("Imported %1 drawings").arg(m_drawings.size()));
    return true;
}

QString DrawingToolManager::generateId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

ChartDrawingObject DrawingToolManager::createDrawing(DrawingToolType type,
                                                const QVector<QPointF>& points)
{
    ChartDrawingObject drawing;
    drawing.type = type;
    drawing.points = points;
    drawing.color = m_defaultColor;
    drawing.lineWidth = m_defaultLineWidth;
    drawing.lineStyle = m_defaultLineStyle;
    drawing.visible = true;
    drawing.locked = false;

    return drawing;
}