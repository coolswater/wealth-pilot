/**
 * @file DrawingToolManager.h
 * @brief 画线工具管理器 - 高级图表功能
 *
 * @details 功能：
 * - 趋势线绘制
 * - 平行通道
 * - 斐波那契回调
 * - 自由绘制
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef CHARTDRAWINGTOOLMANAGER_H
#define CHARTDRAWINGTOOLMANAGER_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QDateTime>

/**
 * @brief 绘图工具类型
 */
enum class DrawingToolType {
    None,               ///< 无
    TrendLine,          ///< 趋势线
    HorizontalLine,     ///< 水平线
    VerticalLine,       ///< 垂直线
    ParallelChannel,    ///< 平行通道
    FibonacciRetracement, ///< 斐波那契回调
    Rectangle,          ///< 矩形
    Text,               ///< 文本
    FreeDraw            ///< 自由绘制
};

/**
 * @brief 绘图对象（图表工具）
 */
struct ChartDrawingObject {
    QString id;                 ///< 对象ID
    DrawingToolType type;       ///< 类型
    QVector<QPointF> points;    ///< 点集合
    QColor color;               ///< 颜色
    int lineWidth = 1;          ///< 线宽
    Qt::PenStyle lineStyle = Qt::SolidLine; ///< 线型
    bool visible = true;        ///< 是否可见
    bool locked = false;        ///< 是否锁定
    QString text;               ///< 文本内容
    QDateTime createTime;       ///< 创建时间
    QDateTime updateTime;       ///< 更新时间
};

/**
 * @brief 斐波那契回调线
 */
struct FibonacciLevel {
    double level;               ///< 水平 (0.0, 0.236, 0.382, 0.5, 0.618, 0.786, 1.0)
    double price;               ///< 价格
    QString label;              ///< 标签
};

/**
 * @brief 画线工具管理器
 *
 * 提供图表绘图功能：
 * - 多种绘图工具
 * - 对象管理
 * - 导入/导出
 */
class ChartDrawingToolManager : public QObject {
    Q_OBJECT

public:
    static ChartDrawingToolManager* instance();

    // ========== 工具选择 ==========

    /**
     * @brief 设置当前工具
     */
    void setCurrentTool(DrawingToolType type);

    /**
     * @brief 获取当前工具
     */
    DrawingToolType currentTool() const { return m_currentTool; }

    // ========== 绘图操作 ==========

    /**
     * @brief 开始绘制
     * @param point 起点
     */
    void startDrawing(const QPointF& point);

    /**
     * @brief 更新绘制
     * @param point 当前点
     */
    void updateDrawing(const QPointF& point);

    /**
     * @brief 结束绘制
     * @param point 终点
     * @return 创建的绘图对象
     */
    ChartDrawingObject finishDrawing(const QPointF& point);

    /**
     * @brief 取消绘制
     */
    void cancelDrawing();

    // ========== 对象管理 ==========

    /**
     * @brief 添加绘图对象
     */
    void addDrawing(const ChartDrawingObject& drawing);

    /**
     * @brief 更新绘图对象
     */
    void updateDrawing(const QString& id, const ChartDrawingObject& drawing);

    /**
     * @brief 删除绘图对象
     */
    void deleteDrawing(const QString& id);

    /**
     * @brief 清除所有绘图
     */
    void clearAllDrawings();

    /**
     * @brief 获取所有绘图
     */
    QVector<ChartDrawingObject> getAllDrawings() const { return m_drawings; }

    /**
     * @brief 获取绘图对象
     */
    ChartDrawingObject getDrawing(const QString& id) const;

    /**
     * @brief 选中绘图对象
     */
    void selectDrawing(const QString& id);

    /**
     * @brief 取消选中
     */
    void deselectAll();

    /**
     * @brief 获取选中的绘图
     */
    QString selectedDrawing() const { return m_selectedId; }

    // ========== 样式设置 ==========

    /**
     * @brief 设置默认颜色
     */
    void setDefaultColor(const QColor& color);

    /**
     * @brief 设置默认线宽
     */
    void setDefaultLineWidth(int width);

    /**
     * @brief 设置默认线型
     */
    void setDefaultLineStyle(Qt::PenStyle style);

    // ========== 斐波那契计算 ==========

    /**
     * @brief 计算斐波那契回调水平
     */
    QVector<FibonacciLevel> calculateFibonacciLevels(double high, double low) const;

    // ========== 导入导出 ==========

    /**
     * @brief 导出绘图数据
     */
    QString exportToJson() const;

    /**
     * @brief 导入绘图数据
     */
    bool importFromJson(const QString& json);

signals:
    /**
     * @brief 绘图开始信号
     */
    void drawingStarted(const QPointF& point);

    /**
     * @brief 绘图更新信号
     */
    void drawingUpdated(const ChartDrawingObject& drawing);

    /**
     * @brief 绘图完成信号
     */
    void drawingFinished(const ChartDrawingObject& drawing);

    /**
     * @brief 绘图删除信号
     */
    void drawingDeleted(const QString& id);

    /**
     * @brief 选择变化信号
     */
    void selectionChanged(const QString& id);

private:
    explicit DrawingToolManager(QObject* parent = nullptr);
    ~DrawingToolManager() override = default;

    QString generateId() const;
    ChartDrawingObject createDrawing(DrawingToolType type, const QVector<QPointF>& points);

    DrawingToolType m_currentTool = DrawingToolType::None;
    QVector<ChartDrawingObject> m_drawings;
    QString m_selectedId;

    // 当前绘制状态
    bool m_isDrawing = false;
    QVector<QPointF> m_currentPoints;
    ChartDrawingObject m_currentDrawing;

    // 默认样式
    QColor m_defaultColor = Qt::red;
    int m_defaultLineWidth = 2;
    Qt::PenStyle m_defaultLineStyle = Qt::SolidLine;
};

#endif // CHARTDRAWINGTOOLMANAGER_H