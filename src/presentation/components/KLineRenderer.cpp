/**
 * @file KLineRenderer.cpp
 * @brief K线渲染器实现
 */

#include "KLineRenderer.h"
#include "shared/types/MarketTypes.h"

#include <QPainterPath>
#include <QLinearGradient>
#include <algorithm>

namespace WealthPilot {

using KLineData = WealthPilot::KLineData;
using KLineStyle = WealthPilot::KLineStyle;

struct KLineRenderer::Impl {
    QColor upColor = QColor("#EF4444");      // 上涨（红）
    QColor downColor = QColor("#10B981");    // 下跌（绿）
    QColor flatColor = QColor("#9CA3AF");    // 平盘（灰）
    QColor gridColor = QColor("#2D3748");    // 网格
    QColor textColor = QColor("#9CA3AF");    // 文本
    QColor crossColor = QColor("#F59E0B");   // 十字光标
};

// ============================================================================
// RenderContext 实现
// ============================================================================

double RenderContext::priceToY(double price) const
{
    if (maxPrice == minPrice) return chartRect.center().y();
    
    double ratio = (maxPrice - price) / (maxPrice - minPrice);
    return chartRect.top() + ratio * chartRect.height();
}

double RenderContext::yToPrice(double y) const
{
    if (chartRect.height() == 0) return minPrice;
    
    double ratio = (y - chartRect.top()) / chartRect.height();
    return maxPrice - ratio * (maxPrice - minPrice);
}

int RenderContext::indexToX(int index) const
{
    if (visibleCount <= 0) return chartRect.left();
    
    // 假设每个蜡烛占用固定宽度
    double candleWidth = static_cast<double>(chartRect.width()) / visibleCount;
    return chartRect.left() + static_cast<int>((index - startIndex) * candleWidth);
}

int RenderContext::xToIndex(int x) const
{
    if (chartRect.width() <= 0) return startIndex;
    
    double candleWidth = static_cast<double>(chartRect.width()) / visibleCount;
    return startIndex + static_cast<int>((x - chartRect.left()) / candleWidth);
}

// ============================================================================
// KLineRenderer 实现
// ============================================================================

KLineRenderer::KLineRenderer()
    : d(std::make_unique<Impl>())
{
}

KLineRenderer::~KLineRenderer() = default;

void KLineRenderer::render(QPainter& painter, const RenderContext& ctx,
                           const QVector<KLineData>& data)
{
    // 按层次顺序渲染
    drawBackground(painter, ctx);
    drawGrid(painter, ctx);
    drawCandles(painter, ctx, data);
    drawVolume(painter, ctx, data);
    drawMainIndicators(painter, ctx, MainIndicator::MA);
    drawAxis(painter, ctx, data);
}

void KLineRenderer::drawBackground(QPainter& painter, const RenderContext& ctx)
{
    // 主图背景
    painter.fillRect(ctx.chartRect, QColor("#0A0E17"));
    
    // 成交量背景
    if (ctx.volumeRect.isValid()) {
        painter.fillRect(ctx.volumeRect, QColor("#111827"));
    }
    
    // 指标背景
    if (ctx.indicatorRect.isValid()) {
        painter.fillRect(ctx.indicatorRect, QColor("#111827"));
    }
}

void KLineRenderer::drawGrid(QPainter& painter, const RenderContext& ctx)
{
    QPen pen(d->gridColor);
    pen.setStyle(Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    
    // 横线（价格网格）
    int yGridCount = 5;
    for (int i = 0; i <= yGridCount; ++i) {
        int y = ctx.chartRect.top() + i * ctx.chartRect.height() / yGridCount;
        painter.drawLine(ctx.chartRect.left(), y, ctx.chartRect.right(), y);
    }
    
    // 竖线（时间网格）
    int xGridCount = 6;
    for (int i = 0; i <= xGridCount; ++i) {
        int x = ctx.chartRect.left() + i * ctx.chartRect.width() / xGridCount;
        painter.drawLine(x, ctx.chartRect.top(), x, ctx.chartRect.bottom());
    }
}

void KLineRenderer::drawCandles(QPainter& painter, const RenderContext& ctx,
                                 const QVector<KLineData>& data)
{
    if (data.isEmpty() || ctx.visibleCount <= 0) return;
    
    double candleWidth = static_cast<double>(ctx.chartRect.width()) / ctx.visibleCount;
    int bodyWidth = static_cast<int>(candleWidth * 0.8);
    if (bodyWidth < 1) bodyWidth = 1;
    
    int endIndex = qMin(ctx.startIndex + ctx.visibleCount, data.size());
    
    for (int i = ctx.startIndex; i < endIndex; ++i) {
        const auto& kline = data[i];
        int x = ctx.indexToX(i);
        
        QColor color = getCandleColor(kline.open, kline.close);
        
        drawCandle(painter, x, bodyWidth,
                   kline.open, kline.close, kline.high, kline.low, color);
    }
}

void KLineRenderer::drawCandle(QPainter& painter, int x, int candleWidth,
                                double open, double close, double high, double low,
                                const QColor& color)
{
    int bodyLeft = x - candleWidth / 2;
    
    // 计算Y坐标
    double highY = 0, lowY = 0, openY = 0, closeY = 0;
    // 这里需要 RenderContext 中的价格范围，简化实现
    
    // 绘制影线
    QPen pen(color, 1);
    painter.setPen(pen);
    painter.drawLine(x, static_cast<int>(highY), x, static_cast<int>(lowY));
    
    // 绘制实体
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    
    int bodyTop = static_cast<int>(std::min(openY, closeY));
    int bodyBottom = static_cast<int>(std::max(openY, closeY));
    int bodyHeight = std::max(1, bodyBottom - bodyTop);
    
    painter.drawRect(bodyLeft, bodyTop, candleWidth, bodyHeight);
}

void KLineRenderer::drawVolume(QPainter& painter, const RenderContext& ctx,
                                const QVector<KLineData>& data)
{
    if (!ctx.volumeRect.isValid() || data.isEmpty()) return;
    
    double barWidth = static_cast<double>(ctx.volumeRect.width()) / ctx.visibleCount;
    int barWidthInt = static_cast<int>(barWidth * 0.8);
    if (barWidthInt < 1) barWidthInt = 1;
    
    int endIndex = qMin(ctx.startIndex + ctx.visibleCount, data.size());
    
    for (int i = ctx.startIndex; i < endIndex; ++i) {
        const auto& kline = data[i];
        int x = ctx.indexToX(i);
        
        QColor color = getCandleColor(kline.open, kline.close);
        color.setAlpha(150);
        
        drawVolumeBar(painter, x, barWidthInt, kline.volume, ctx.maxVolume, color);
    }
}

void KLineRenderer::drawVolumeBar(QPainter& painter, int x, int barWidth,
                                   double volume, double maxVolume,
                                   const QColor& color)
{
    if (maxVolume <= 0) return;
    
    double ratio = volume / maxVolume;
    int barHeight = static_cast<int>(ratio * 50);  // 假设成交量区域高度为50
    
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawRect(x - barWidth / 2, 0, barWidth, barHeight);
}

void KLineRenderer::drawMainIndicators(QPainter& painter, const RenderContext& ctx,
                                        MainIndicator indicator)
{
    Q_UNUSED(indicator)
    
    // 绘制均线
    if (ctx.ma5 && !ctx.ma5->isEmpty()) {
        drawLine(painter, *ctx.ma5, QColor("#F59E0B"), 1);
    }
    
    if (ctx.ma10 && !ctx.ma10->isEmpty()) {
        drawLine(painter, *ctx.ma10, QColor("#3B82F6"), 1);
    }
    
    if (ctx.ma20 && !ctx.ma20->isEmpty()) {
        drawLine(painter, *ctx.ma20, QColor("#A855F7"), 1);
    }
}

void KLineRenderer::drawSubIndicators(QPainter& painter, const RenderContext& ctx,
                                       SubIndicator indicator)
{
    Q_UNUSED(indicator)
    
    // 绘制 MACD
    if (ctx.macd && ctx.dif && ctx.dea) {
        drawLine(painter, *ctx.dif, QColor("#3B82F6"), 1);
        drawLine(painter, *ctx.dea, QColor("#EF4444"), 1);
    }
    
    // 绘制 KDJ
    if (ctx.kdjK && ctx.kdjD && ctx.kdjJ) {
        drawLine(painter, *ctx.kdjK, QColor("#F59E0B"), 1);
        drawLine(painter, *ctx.kdjD, QColor("#3B82F6"), 1);
        drawLine(painter, *ctx.kdjJ, QColor("#A855F7"), 1);
    }
}

void KLineRenderer::drawCrosshair(QPainter& painter, const RenderContext& ctx,
                                   int mouseIndex, const KLineData& currentData)
{
    int mouseX = ctx.indexToX(mouseIndex);
    
    // 垂直线
    QPen pen(d->crossColor, 1, Qt::DashLine);
    painter.setPen(pen);
    painter.drawLine(mouseX, ctx.chartRect.top(), mouseX, ctx.chartRect.bottom());
    
    // 水平线
    double priceY = ctx.priceToY(currentData.close);
    painter.drawLine(ctx.chartRect.left(), static_cast<int>(priceY),
                     ctx.chartRect.right(), static_cast<int>(priceY));
}

void KLineRenderer::drawAxis(QPainter& painter, const RenderContext& ctx,
                              const QVector<KLineData>& data)
{
    Q_UNUSED(data)
    
    painter.setPen(d->textColor);
    QFont font("Arial", 9);
    painter.setFont(font);
    
    // Y轴价格标签
    int yGridCount = 5;
    for (int i = 0; i <= yGridCount; ++i) {
        double price = ctx.maxPrice - i * (ctx.maxPrice - ctx.minPrice) / yGridCount;
        int y = ctx.chartRect.top() + i * ctx.chartRect.height() / yGridCount;
        
        QString label = QString::number(price, 'f', 2);
        painter.drawText(5, y + 5, label);
    }
}

void KLineRenderer::drawLine(QPainter& painter, const QVector<double>& values,
                              const QColor& color, int lineWidth)
{
    if (values.isEmpty()) return;
    
    QPen pen(color, lineWidth);
    painter.setPen(pen);
    
    QPainterPath path;
    bool first = true;
    
    for (int i = 0; i < values.size(); ++i) {
        double value = values[i];
        if (qIsNaN(value)) continue;
        
        int x = 0;  // 需要从 ctx 获取
        int y = 0;  // 需要从 ctx 获取
        
        if (first) {
            path.moveTo(x, y);
            first = false;
        } else {
            path.lineTo(x, y);
        }
    }
    
    painter.drawPath(path);
}

QColor KLineRenderer::getCandleColor(double open, double close) const
{
    if (close > open) {
        return d->upColor;
    } else if (close < open) {
        return d->downColor;
    }
    return d->flatColor;
}

void KLineRenderer::setUpColor(const QColor& color)
{
    d->upColor = color;
}

void KLineRenderer::setDownColor(const QColor& color)
{
    d->downColor = color;
}

void KLineRenderer::setGridColor(const QColor& color)
{
    d->gridColor = color;
}

} // namespace WealthPilot
