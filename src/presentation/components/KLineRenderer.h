/**
 * @file KLineRenderer.h
 * @brief K线渲染器 - 负责K线图表的绑制逻辑
 *
 * @details 从 KLineChart 拆分出的渲染逻辑：
 * - 背景绘制
 * - 网格绘制
 * - K线蜡烛绘制
 * - 成交量绘制
 * - 指标线绘制
 * - 十字光标绘制
 * - 坐标轴绘制
 *
 * @details 设计原则：
 * - 单一职责：只负责绑制，不处理数据和交互
 * - 无状态：所有状态由 KLineChart 管理
 * - 可测试：纯绑制逻辑，易于单元测试
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef KLINERENDERER_H
#define KLINERENDERER_H

#include <QPainter>
#include <QRect>
#include <QColor>
#include <QVector>
#include <QPen>
#include <QBrush>
#include <memory>

// 前向声明
class KLineChart;

namespace WealthPilot {

// 前向声明
struct KLineData;
struct KLineStyle;
enum class MainIndicator;
enum class SubIndicator;

/**
 * @brief 渲染上下文 - 包含渲染所需的所有状态
 */
struct RenderContext {
    // 图表区域
    QRect chartRect;            ///< 主图区域
    QRect volumeRect;           ///< 成交量区域
    QRect indicatorRect;        ///< 副图指标区域
    
    // 数据范围
    int startIndex = 0;         ///< 可见起始索引
    int visibleCount = 0;       ///< 可见数量
    double minPrice = 0;        ///< 最低价
    double maxPrice = 0;        ///< 最高价
    double maxVolume = 0;       ///< 最大成交量
    
    // 坐标转换
    double priceToY(double price) const;
    double yToPrice(double y) const;
    int indexToX(int index) const;
    int xToIndex(int x) const;
    
    // 样式
    const KLineStyle* style = nullptr;
    
    // 指标数据
    const QVector<double>* ma5 = nullptr;
    const QVector<double>* ma10 = nullptr;
    const QVector<double>* ma20 = nullptr;
    const QVector<double>* macd = nullptr;
    const QVector<double>* dif = nullptr;
    const QVector<double>* dea = nullptr;
    const QVector<double>* kdjK = nullptr;
    const QVector<double>* kdjD = nullptr;
    const QVector<double>* kdjJ = nullptr;
};

/**
 * @brief K线渲染器
 */
class KLineRenderer {
public:
    explicit KLineRenderer();
    ~KLineRenderer();
    
    // ========== 主渲染入口 ==========
    
    /**
     * @brief 渲染整个图表
     */
    void render(QPainter& painter, const RenderContext& ctx, 
                const QVector<KLineData>& data);
    
    // ========== 分层渲染 ==========
    
    /**
     * @brief 绘制背景
     */
    void drawBackground(QPainter& painter, const RenderContext& ctx);
    
    /**
     * @brief 绘制网格
     */
    void drawGrid(QPainter& painter, const RenderContext& ctx);
    
    /**
     * @brief 绘制K线蜡烛
     */
    void drawCandles(QPainter& painter, const RenderContext& ctx,
                     const QVector<KLineData>& data);
    
    /**
     * @brief 绘制成交量
     */
    void drawVolume(QPainter& painter, const RenderContext& ctx,
                    const QVector<KLineData>& data);
    
    /**
     * @brief 绘制主图指标
     */
    void drawMainIndicators(QPainter& painter, const RenderContext& ctx,
                            MainIndicator indicator);
    
    /**
     * @brief 绘制副图指标
     */
    void drawSubIndicators(QPainter& painter, const RenderContext& ctx,
                           SubIndicator indicator);
    
    /**
     * @brief 绘制十字光标
     */
    void drawCrosshair(QPainter& painter, const RenderContext& ctx,
                       int mouseIndex, const KLineData& currentData);
    
    /**
     * @brief 绘制坐标轴
     */
    void drawAxis(QPainter& painter, const RenderContext& ctx,
                  const QVector<KLineData>& data);
    
    // ========== 样式配置 ==========
    
    /**
     * @brief 设置上涨颜色
     */
    void setUpColor(const QColor& color);
    
    /**
     * @brief 设置下跌颜色
     */
    void setDownColor(const QColor& color);
    
    /**
     * @brief 设置网格颜色
     */
    void setGridColor(const QColor& color);

private:
    // 绘制辅助方法
    void drawCandle(QPainter& painter, int x, int candleWidth,
                    double open, double close, double high, double low,
                    const QColor& color);
    
    void drawVolumeBar(QPainter& painter, int x, int barWidth,
                       double volume, double maxVolume,
                       const QColor& color);
    
    void drawLine(QPainter& painter, const QVector<double>& values,
                  const QColor& color, int lineWidth);
    
    QColor getCandleColor(double open, double close) const;
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // KLINERENDERER_H
