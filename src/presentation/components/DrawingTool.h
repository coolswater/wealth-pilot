/**
 * @file DrawingTool.h
 * @brief K线图画线工具
 *
 * @details 实现功能：
 * - 趋势线绘制
 * - 水平线绘制
 * - 平行通道绘制
 * - 矩形区域绘制
 * - 文本标注
 * - 图形选择、移动、删除
 * - 图形持久化
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DRAWINGTOOL_H
#define DRAWINGTOOL_H

#include <QObject>
#include <QPointF>
#include <QColor>
#include <QVector>
#include <QDateTime>
#include <QKeyEvent>
#include <memory>

class KLineChart;

/**
 * @brief 绘图工具类型
 */
enum class DrawingType
{
    None = 0, ///< 无工具
    TrendLine, ///< 趋势线
    HorizontalLine, ///< 水平线
    VerticalLine, ///< 垂直线
    ParallelChannel, ///< 平行通道
    Rectangle, ///< 矩形
    Fibonacci, ///< 斐波那契回调线
    Text, ///< 文本标注
    Arrow ///< 箭头
};

/**
 * @brief 绘图对象基类
 */
struct DrawingObject
{
    QString id; ///< 对象ID
    DrawingType type; ///< 对象类型
    QColor color; ///< 颜色
    int lineWidth = 1; ///< 线宽
    Qt::PenStyle lineStyle = Qt::SolidLine; ///< 线型
    bool selected = false; ///< 是否选中
    QDateTime createTime; ///< 创建时间
    QString text; ///< 文本内容（文本标注用）

    virtual ~DrawingObject() = default;

    // 序列化
    virtual QJsonObject toJson() const;
    static std::shared_ptr<DrawingObject> fromJson(const QJsonObject& json);
};

/**
 * @brief 趋势线
 */
struct TrendLine : public DrawingObject
{
    QPointF startPoint; ///< 起点（x=索引, y=价格）
    QPointF endPoint; ///< 终点

    TrendLine() { type = DrawingType::TrendLine; }
    QJsonObject toJson() const override;
};

/**
 * @brief 水平线
 */
struct HorizontalLine : public DrawingObject
{
    double price; ///< 价格位置

    HorizontalLine() { type = DrawingType::HorizontalLine; }
    QJsonObject toJson() const override;
};

/**
 * @brief 平行通道
 */
struct ParallelChannel : public DrawingObject
{
    QPointF point1; ///< 第一点
    QPointF point2; ///< 第二点
    double width; ///< 通道宽度

    ParallelChannel() { type = DrawingType::ParallelChannel; }
    QJsonObject toJson() const override;
};

/**
 * @brief 矩形区域
 */
struct RectangleArea : public DrawingObject
{
    QPointF topLeft; ///< 左上角
    QPointF bottomRight; ///< 右下角
    QColor fillColor; ///< 填充颜色

    RectangleArea() { type = DrawingType::Rectangle; }
    QJsonObject toJson() const override;
};

/**
 * @brief 斐波那契回调线
 */
struct FibonacciRetracement : public DrawingObject
{
    QPointF startPoint; ///< 起点
    QPointF endPoint; ///< 终点
    bool showLevels = true; ///< 显示回调位

    FibonacciRetracement() { type = DrawingType::Fibonacci; }
    QJsonObject toJson() const override;
};

/**
 * @brief 文本标注
 */
struct TextAnnotation : public DrawingObject
{
    QPointF position; ///< 位置
    QString text; ///< 文本内容
    int fontSize = 12; ///< 字体大小

    TextAnnotation() { type = DrawingType::Text; }
    QJsonObject toJson() const override;
};

/**
 * @brief 画线工具管理器
 */
class DrawingToolManager : public QObject
{
    Q_OBJECT

public:
    explicit DrawingToolManager(KLineChart* chart, QObject* parent = nullptr);
    ~DrawingToolManager() override;

    // 工具选择
    void setCurrentTool(DrawingType type);
    DrawingType currentTool() const { return m_currentTool; }

    // 绘图对象管理
    void addDrawing(std::shared_ptr<DrawingObject> drawing);
    void removeDrawing(const QString& id);
    void clearAllDrawings();
    std::shared_ptr<DrawingObject> getDrawing(const QString& id) const;
    QVector<std::shared_ptr<DrawingObject>> allDrawings() const;

    // 选择管理
    void selectDrawing(const QString& id);
    void deselectAll();
    QString selectedDrawingId() const;

    // 持久化
    bool saveToFile(const QString& filePath);
    bool loadFromFile(const QString& filePath);
    QJsonArray toJson() const;
    void fromJson(const QJsonArray& json);

    // 绘制
    void paint(QPainter* painter);

    // 事件处理
    bool mousePressEvent(QMouseEvent* event);
    bool mouseMoveEvent(QMouseEvent* event);
    bool mouseReleaseEvent(QMouseEvent* event);
    bool keyPressEvent(QKeyEvent* event);

    signals :

    void drawingAdded(const QString& id);
    void drawingRemoved(const QString& id);
    void drawingSelected(const QString& id);
    void toolChanged(DrawingType type);

private:
    // 绘制方法
    void drawTrendLine(QPainter* painter, const TrendLine& line);
    void drawHorizontalLine(QPainter* painter, const HorizontalLine& line);
    void drawParallelChannel(QPainter* painter, const ParallelChannel& channel);
    void drawRectangle(QPainter* painter, const RectangleArea& rect);
    void drawFibonacci(QPainter* painter, const FibonacciRetracement& fib);
    void drawText(QPainter* painter, const TextAnnotation& text);

    // 坐标转换
    QPointF screenToChart(const QPoint& screenPos) const;
    QPoint chartToScreen(const QPointF& chartPos) const;

    // 命中测试
    QString hitTest(const QPoint& screenPos) const;

    // 生成唯一ID
    QString generateId() const;

private:
    KLineChart* m_chart;
    DrawingType m_currentTool = DrawingType::None;

    QVector<std::shared_ptr<DrawingObject>> m_drawings;
    QString m_selectedId;

    // 绘制状态
    bool m_isDrawing = false;
    std::shared_ptr<DrawingObject> m_currentDrawing;
    QPointF m_startPoint;

    // 默认样式
    QColor m_defaultColor = QColor("#3B82F6");
    int m_defaultLineWidth = 2;
};

#endif // DRAWINGTOOL_H
