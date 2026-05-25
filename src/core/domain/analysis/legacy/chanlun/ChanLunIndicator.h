/**
 * @file ChanLunIndicator.h
 * @brief 缠论指标组件 - K线图上的缠论可视化
 *
 * @details 功能：
 * - 在K线主图上绘制笔、线段、中枢
 * - 标记买卖点信号
 * - 支持开关显示不同元素
 * - 与副图联动（MACD等）
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef CHANLUN_INDICATOR_H
#define CHANLUN_INDICATOR_H

#include "ChanLunTypes.h"
#include <QObject>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QVector>
#include <memory>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace WealthPilot {
namespace ChanLun {

/**
 * @brief 缠论指标可视化组件
 *
 * @details 在K线图上绘制缠论分析结果
 */
class ChanLunIndicator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ChanLunIndicator(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ChanLunIndicator() override;

    // ========== 数据设置 ==========
    
    /**
     * @brief 设置分析结果
     */
    void setResult(const ChanLunResult& result);
    
    /**
     * @brief 设置坐标转换函数
     * @param timeToX 时间转X坐标
     * @param priceToY 价格转Y坐标
     */
    void setCoordinateTransform(std::function<double(const QDateTime&)> timeToX,
                                std::function<double(double)> priceToY);

    // ========== 显示控制 ==========
    
    /**
     * @brief 设置是否显示笔
     */
    void setShowPens(bool show);
    
    /**
     * @brief 设置是否显示线段
     */
    void setShowSegments(bool show);
    
    /**
     * @brief 设置是否显示中枢
     */
    void setShowPivots(bool show);
    
    /**
     * @brief 设置是否显示买卖点
     */
    void setShowSignals(bool show);
    
    /**
     * @brief 设置是否显示分型
     */
    void setShowFractals(bool show);

    // ========== 图形项获取 ==========
    
    /**
     * @brief 获取所有图形项
     */
    QVector<QGraphicsItem*> graphicsItems() const;
    
    /**
     * @brief 更新图形项
     */
    void updateGraphics();

signals:
    /**
     * @brief 图形更新信号
     */
    void graphicsUpdated();
    
    /**
     * @brief 信号点点击
     */
    void signalClicked(const TradeSignal& signal);

private:
    // ========== 绘制方法 ==========
    
    /**
     * @brief 创建笔图形
     */
    void createPenGraphics();
    
    /**
     * @brief 创建线段图形
     */
    void createSegmentGraphics();
    
    /**
     * @brief 创建中枢图形
     */
    void createPivotGraphics();
    
    /**
     * @brief 创建信号图形
     */
    void createSignalGraphics();
    
    /**
     * @brief 创建分型图形
     */
    void createFractalGraphics();
    
    /**
     * @brief 清除所有图形
     */
    void clearGraphics();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

// ============================================================================
// 图形项类
// ============================================================================

/**
 * @brief 笔图形项
 */
class PenGraphicsItem : public QGraphicsPathItem
{
public:
    explicit PenGraphicsItem(const Pen& pen, QGraphicsItem* parent = nullptr);
    
    const Pen& pen() const { return m_pen; }
    
private:
    Pen m_pen;
};

/**
 * @brief 中枢图形项
 */
class PivotGraphicsItem : public QGraphicsRectItem
{
public:
    explicit PivotGraphicsItem(const Pivot& pivot, QGraphicsItem* parent = nullptr);
    
    const Pivot& pivot() const { return m_pivot; }
    
private:
    Pivot m_pivot;
};

/**
 * @brief 买卖点图形项
 */
class SignalGraphicsItem : public QGraphicsItem
{
public:
    explicit SignalGraphicsItem(const TradeSignal& signal, QGraphicsItem* parent = nullptr);
    
    const TradeSignal& signal() const { return m_signal; }
    
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    
private:
    TradeSignal m_signal;
    QRectF m_boundingRect;
};

} // namespace ChanLun
} // namespace WealthPilot

#endif // CHANLUN_INDICATOR_H
