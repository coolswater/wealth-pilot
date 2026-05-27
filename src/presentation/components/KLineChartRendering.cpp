/**
 * @file KLineChartRendering.cpp
 * @brief K线图绘制方法实现
 *
 * @details 实现功能：
 * - 背景绘制
 * - 网格绘制
 * - K线蜡烛绘制
 * - 成交量绘制
 * - 主图/副图指标绘制
 * - 十字光标绘制
 * - 坐标轴绘制
 * - K线信息绘制
 *
 * @author WealthPilot Team
 * @version 2.1.0
 */

#include "KLineChartImpl.h"
#include "infrastructure/config/Tokens.h"
#include <QPainter>
#include <QFontMetrics>

// ========== 绘制方法 ==========

/**
 * @brief 绘制背景
 */
void KLineChart::drawBackground(QPainter& painter)
{
    painter.fillRect(rect(), QColor(Tokens::Colors::BgBase));
}

/**
 * @brief 绘制网格
 */
void KLineChart::drawGrid(QPainter& painter)
{
    painter.setPen(QPen(QColor(Tokens::Colors::Border), 1, Qt::DashLine));
    
    // 主图区域网格线
    // 水平网格线
    int hLines = 5;
    for (int i = 0; i <= hLines; ++i) {
        int y = d->chartRect.top() + i * d->chartRect.height() / hLines;
        painter.drawLine(d->chartRect.left(), y, d->chartRect.right(), y);
    }
    
    // 垂直网格线
    int vLines = 6;
    for (int i = 0; i <= vLines; ++i) {
        int x = d->chartRect.left() + i * d->chartRect.width() / vLines;
        painter.drawLine(x, d->chartRect.top(), x, d->chartRect.bottom());
    }
    
    // 副图区域网格线
    if (d->subChartRect.height() > 0) {
        // 水平网格线
        for (int i = 0; i <= 2; ++i) {
            int y = d->subChartRect.top() + i * d->subChartRect.height() / 2;
            painter.drawLine(d->subChartRect.left(), y, d->subChartRect.right(), y);
        }
        
        // 垂直网格线
        for (int i = 0; i <= vLines; ++i) {
            int x = d->subChartRect.left() + i * d->subChartRect.width() / vLines;
            painter.drawLine(x, d->subChartRect.top(), x, d->subChartRect.bottom());
        }
    }
    
    // 绘制分隔线
    painter.setPen(QPen(QColor(Tokens::Colors::Border), 1, Qt::SolidLine));
    painter.drawLine(d->chartRect.left(), d->chartRect.bottom(), 
                    d->chartRect.right(), d->chartRect.bottom());
    painter.drawLine(d->volumeRect.left(), d->volumeRect.bottom(), 
                    d->volumeRect.right(), d->volumeRect.bottom());
    painter.drawLine(d->subChartRect.left(), d->subChartRect.bottom(), 
                    d->subChartRect.right(), d->subChartRect.bottom());
}

/**
 * @brief 绘制K线蜡烛
 */
void KLineChart::drawCandles(QPainter& painter)
{
    if (d->data.isEmpty() || d->priceScale <= 0) {
        return;
    }
    
    int candleWidth = d->style.candleWidth;
    int halfWidth = candleWidth / 2;
    
    for (int i = d->visibleStart; i < d->visibleStart + d->visibleCount && i < d->data.size(); ++i) {
        const KLineData& kline = d->data[i];
        
        // 跳过无效数据
        if (kline.open <= 0 || kline.close <= 0 || kline.high <= 0 || kline.low <= 0) {
            continue;
        }
        
        int x = d->indexToX(i);
        
        // 判断涨跌
        QColor color;
        if (kline.close > kline.open) {
            color = d->style.upColor;
        } else if (kline.close < kline.open) {
            color = d->style.downColor;
        } else {
            color = d->style.flatColor;
        }
        
        painter.setPen(QPen(color, 1));
        painter.setBrush(Qt::NoBrush);
        
        // 绘制上下影线
        int highY = d->priceToY(kline.high);
        int lowY = d->priceToY(kline.low);
        
        // 确保坐标有效
        if (highY >= d->chartRect.top() && lowY <= d->chartRect.bottom()) {
            painter.drawLine(x, highY, x, lowY);
        }
        
        // 绘制实体
        int openY = d->priceToY(kline.open);
        int closeY = d->priceToY(kline.close);
        
        // 确保实体高度至少为1像素
        int bodyHeight = qAbs(closeY - openY);
        if (bodyHeight < 1) {
            bodyHeight = 1;
        }
        
        int bodyTop = qMin(openY, closeY);
        
        if (kline.close > kline.open) {
            // 上涨：空心
            painter.drawRect(x - halfWidth, bodyTop, candleWidth, bodyHeight);
        } else {
            // 下跌：实心
            painter.fillRect(x - halfWidth, bodyTop, candleWidth, bodyHeight, color);
        }
    }
}

/**
 * @brief 绘制成交量
 */
void KLineChart::drawVolume(QPainter& painter)
{
    if (d->data.isEmpty() || !d->style.showVolume || d->volumeRect.height() <= 0) {
        return;
    }
    
    // 找到最大成交量
    qint64 maxVolume = 0;
    for (int i = d->visibleStart; i < d->visibleStart + d->visibleCount && i < d->data.size(); ++i) {
        if (d->data[i].volume > 0) {
            maxVolume = qMax(maxVolume, d->data[i].volume);
        }
    }
    
    if (maxVolume <= 0) {
        return;
    }
    
    int candleWidth = d->style.candleWidth;
    int halfWidth = candleWidth / 2;
    
    for (int i = d->visibleStart; i < d->visibleStart + d->visibleCount && i < d->data.size(); ++i) {
        const KLineData& kline = d->data[i];
        
        // 跳过无效数据
        if (kline.volume <= 0) {
            continue;
        }
        
        int x = d->indexToX(i);
        
        // 判断涨跌
        QColor color = (kline.close >= kline.open) ? d->style.upColor : d->style.downColor;
        color.setAlpha(150);
        
        // 计算高度
        int height = static_cast<int>(kline.volume * d->volumeRect.height() / maxVolume);
        if (height < 1) {
            height = 1;
        }
        int y = d->volumeRect.bottom() - height;
        
        painter.fillRect(x - halfWidth, y, candleWidth, height, color);
    }
}

/**
 * @brief 绘制主图指标线（MA/EMA/BOLL等）
 */
void KLineChart::drawMainIndicators(QPainter& painter)
{
    for (auto it = d->mainIndicatorLines.begin(); it != d->mainIndicatorLines.end(); ++it) {
        const QString& name = it.key();
        const QVector<double>& values = it.value();
        QColor color = d->mainIndicatorColors.value(name, Qt::white);
        
        painter.setPen(QPen(color, 1));
        
        bool first = true;
        QPointF lastPoint;
        
        for (int i = d->visibleStart; i < d->visibleStart + d->visibleCount && i < values.size(); ++i) {
            int x = d->indexToX(i);
            int y = d->priceToY(values[i]);
            
            // 确保在主图区域内
            if (y < d->chartRect.top() || y > d->chartRect.bottom()) {
                first = true;
                continue;
            }
            
            QPointF point(x, y);
            
            if (!first) {
                painter.drawLine(lastPoint, point);
            }
            
            lastPoint = point;
            first = false;
        }
    }
}

/**
 * @brief 绘制副图指标线（MACD/KDJ/RSI等）
 */
void KLineChart::drawSubIndicators(QPainter& painter)
{
    if (d->subChartRect.height() <= 0 || d->subIndicatorLines.isEmpty()) {
        return;
    }
    
    for (auto it = d->subIndicatorLines.begin(); it != d->subIndicatorLines.end(); ++it) {
        const QString& name = it.key();
        const QVector<double>& values = it.value();
        QColor color = d->subIndicatorColors.value(name, Qt::white);
        
        painter.setPen(QPen(color, 1));
        
        bool first = true;
        QPointF lastPoint;
        
        for (int i = d->visibleStart; i < d->visibleStart + d->visibleCount && i < values.size(); ++i) {
            int x = d->indexToX(i);
            int y = d->subValueToY(values[i]);
            
            // 确保在副图区域内
            if (y < d->subChartRect.top() || y > d->subChartRect.bottom()) {
                first = true;
                continue;
            }
            
            QPointF point(x, y);
            
            if (!first) {
                painter.drawLine(lastPoint, point);
            }
            
            lastPoint = point;
            first = false;
        }
    }
}

/**
 * @brief 绘制十字光标
 */
void KLineChart::drawCrosshair(QPainter& painter)
{
    // 使用主题适配的十字光标颜色
    painter.setPen(QPen(d->crosshairColor, 1, Qt::DashLine));
    
    // 垂直线
    painter.drawLine(d->crosshairX, d->chartRect.top(), 
                    d->crosshairX, d->chartRect.bottom());
    
    // 水平线
    painter.drawLine(d->chartRect.left(), d->crosshairY,
                    d->chartRect.right(), d->crosshairY);
    
    // 显示价格和时间
    int index = d->xToIndex(d->crosshairX);
    if (index >= 0 && index < d->data.size()) {
        double price = d->yToPrice(d->crosshairY);
        
        // 价格标签（左侧）
        painter.setPen(d->crosshairTextColor);
        painter.setFont(QFont("Microsoft YaHei", 9));
        QString priceText = QString::number(price, 'f', 2);
        
        // 绘制价格标签背景
        QFontMetrics fm(painter.font());
        int textWidth = fm.horizontalAdvance(priceText) + 10;
        painter.fillRect(0, d->crosshairY - 10, textWidth, 20, QColor(Tokens::Colors::BgElevated));
        painter.drawText(5, d->crosshairY + 4, priceText);
        
        // 时间标签（底部）
        QString timeText = d->data[index].time.toString("MM-dd hh:mm");
        int timeWidth = fm.horizontalAdvance(timeText) + 10;
        painter.fillRect(d->crosshairX - timeWidth/2, height() - 25, timeWidth, 20, QColor(Tokens::Colors::BgElevated));
        painter.drawText(d->crosshairX - timeWidth/2 + 5, height() - 10, timeText);
    }
}

/**
 * @brief 绘制坐标轴（价格轴、时间轴）
 */
void KLineChart::drawAxis(QPainter& painter)
{
    painter.setPen(QColor(Tokens::Colors::TextSecondary));
    painter.setFont(QFont("Microsoft YaHei", 9));
    
    // 价格轴（右侧）
    int priceSteps = 5;
    int axisX = width() - 50;  // 右侧坐标轴位置
    for (int i = 0; i <= priceSteps; ++i) {
        double price = d->minPrice + i * (d->maxPrice - d->minPrice) / priceSteps;
        int y = d->priceToY(price);
        
        QString text = QString::number(price, 'f', 2);
        painter.drawText(axisX, y + 4, text);
    }
    
    // 副图指标轴（右侧）
    if (d->subChartRect.height() > 0 && !d->subIndicatorLines.isEmpty()) {
        int subSteps = 2;
        for (int i = 0; i <= subSteps; ++i) {
            double value = d->subMinValue + i * (d->subMaxValue - d->subMinValue) / subSteps;
            int y = d->subValueToY(value);
            QString text = QString::number(value, 'f', 1);
            painter.drawText(axisX, y + 4, text);
        }
    }
    
    // 时间轴（底部）
    int timeSteps = 5;
    for (int i = 0; i <= timeSteps; ++i) {
        int index = d->visibleStart + i * d->visibleCount / timeSteps;
        if (index < d->data.size()) {
            int x = d->indexToX(index);
            QString text = d->data[index].time.toString("MM-dd");
            painter.drawText(x - 20, height() - 8, text);
        }
    }
}

/**
 * @brief 绘制K线信息（右上角悬浮信息）
 */
void KLineChart::drawKLineInfo(QPainter& painter)
{
    if (d->crosshairIndex < 0 || d->crosshairIndex >= d->data.size()) {
        return;
    }
    
    const KLineData& kline = d->data[d->crosshairIndex];
    
    // 跳过无效数据
    if (kline.open <= 0 || kline.close <= 0) {
        return;
    }
    
    painter.setFont(QFont("Microsoft YaHei", 10));
    QFontMetrics fm(painter.font());
    
    // 计算涨跌
    double change = kline.close - kline.open;
    double changePercent = (kline.open > 0) ? (change / kline.open * 100) : 0;
    
    // 振幅
    double amplitude = (kline.high - kline.low) / kline.open * 100;
    
    // 构建信息文本
    QStringList infoParts;
    infoParts << kline.time.toString("yyyy-MM-dd");
    infoParts << QString(QStringLiteral("开:%1")).arg(kline.open, 0, 'f', 2);
    infoParts << QString(QStringLiteral("高:%1")).arg(kline.high, 0, 'f', 2);
    infoParts << QString(QStringLiteral("低:%1")).arg(kline.low, 0, 'f', 2);
    infoParts << QString(QStringLiteral("收:%1")).arg(kline.close, 0, 'f', 2);
    
    // 涨跌
    QString changeText = change >= 0 ? QString("+%1").arg(change, 0, 'f', 2) : QString::number(change, 'f', 2);
    infoParts << QString(QStringLiteral("涨跌:%1")).arg(changeText);
    
    // 涨跌幅
    QString changePercentText = changePercent >= 0 ? QString("+%1%").arg(changePercent, 0, 'f', 2) : QString("%1%").arg(changePercent, 0, 'f', 2);
    infoParts << QString(QStringLiteral("涨跌幅:%1")).arg(changePercentText);
    
    // 成交量
    if (kline.volume > 0) {
        double volWan = kline.volume / 10000.0;
        infoParts << QString(QStringLiteral("量:%1万")).arg(volWan, 0, 'f', 2);
    }
    
    // 成交金额
    if (kline.turnover > 0) {
        double amtYi = kline.turnover / 100000000.0;
        infoParts << QString(QStringLiteral("额:%1亿")).arg(amtYi, 0, 'f', 2);
    }
    
    // 振幅
    infoParts << QString(QStringLiteral("振幅:%1%")).arg(amplitude, 0, 'f', 2);
    
    // 合并为一行
    QString infoText = infoParts.join("  ");
    
    // 计算文本宽度
    int textWidth = fm.horizontalAdvance(infoText) + 20;
    int textHeight = fm.height() + 8;
    
    // 绘制背景（左上角）
    int infoX = 10;
    int infoY = 5;
    
    painter.fillRect(infoX - 5, infoY - 2, textWidth + 10, textHeight + 4, QColor(Tokens::Colors::BgSurface));
    painter.fillRect(infoX, infoY, textWidth, textHeight, QColor(Tokens::Colors::BgElevated));
    
    // 绘制文本
    painter.setPen(QColor(Tokens::Colors::TextPrimary));
    painter.drawText(infoX + 10, infoY + textHeight - 6, infoText);
}