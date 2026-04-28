/**
 * @file KLineChart.cpp
 * @brief K线图组件实现 - 高性能K线图表
 *
 * @details 实现功能：
 * - K线绘制（开高低收）
 * - 成交量柱状图
 * - 技术指标叠加
 * - 十字光标
 * - 缩放和平移
 * - 性能优化：双缓冲绘制、数据压缩
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "KLineChart.h"
#include "core/config/Tokens.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QElapsedTimer>
#include <QtMath>

#include "../../utils/Logger.h"

// ========== PIMPL实现 ==========

struct KLineChart::Impl {
    // K线数据
    QVector<KLineData> data;
    
    // 样式配置
    KLineStyle style;
    
    // 视图参数
    int visibleStart = 0;           // 可见起始索引
    int visibleCount = 100;         // 可见数量
    double minPrice = 0;            // 最小价格
    double maxPrice = 0;            // 最大价格
    double priceScale = 1.0;        // 价格缩放
    
    // 十字光标
    bool showCrosshair = false;
    int crosshairX = 0;
    int crosshairY = 0;
    
    // 技术指标
    QMap<QString, QVector<double>> indicators;
    QMap<QString, QColor> indicatorColors;
    
    // 性能优化：压缩数据
    QVector<KLineData> compressedData;
    int compressionLevel = 1;
    
    // 鼠标交互
    bool isDragging = false;
    int lastMouseX = 0;
    
    // 图表区域
    QRect chartRect;
    QRect volumeRect;
    
    /**
     * @brief 计算可见范围
     */
    void calculateVisibleRange() {
        if (data.isEmpty()) {
            visibleStart = 0;
            visibleCount = 100;
            minPrice = 0;
            maxPrice = 100;
            priceScale = 1.0;
            return;
        }
        
        // 确保范围有效
        visibleStart = qBound(0, visibleStart, data.size() - 1);
        visibleCount = qBound(10, visibleCount, data.size() - visibleStart);
        
        // 计算价格范围
        minPrice = std::numeric_limits<double>::max();
        maxPrice = std::numeric_limits<double>::lowest();
        
        for (int i = visibleStart; i < visibleStart + visibleCount && i < data.size(); ++i) {
            // 跳过无效数据
            if (data[i].low > 0) {
                minPrice = qMin(minPrice, data[i].low);
            }
            if (data[i].high > 0) {
                maxPrice = qMax(maxPrice, data[i].high);
            }
        }
        
        // 处理无效价格范围
        if (minPrice <= 0 || maxPrice <= 0 || minPrice >= maxPrice) {
            minPrice = 0;
            maxPrice = 100;
        }
        
        // 添加边距
        double margin = (maxPrice - minPrice) * 0.1;
        if (margin <= 0) {
            margin = 10;
        }
        minPrice -= margin;
        maxPrice += margin;
        
        // 计算缩放
        if (chartRect.height() > 0 && maxPrice > minPrice) {
            priceScale = chartRect.height() / (maxPrice - minPrice);
        } else {
            priceScale = 1.0;
        }
    }
    
    /**
     * @brief 索引转X坐标
     */
    int indexToX(int index) const {
        int relativeIndex = index - visibleStart;
        int candleWidth = style.candleWidth + style.candleSpacing;
        return chartRect.left() + relativeIndex * candleWidth + candleWidth / 2;
    }
    
    /**
     * @brief X坐标转索引
     */
    int xToIndex(int x) const {
        int candleWidth = style.candleWidth + style.candleSpacing;
        int relativeIndex = (x - chartRect.left()) / candleWidth;
        return visibleStart + relativeIndex;
    }
    
    /**
     * @brief 价格转Y坐标
     */
    int priceToY(double price) const {
        if (priceScale <= 0) {
            return chartRect.top();
        }
        double y = chartRect.bottom() - (price - minPrice) * priceScale;
        // 限制在图表区域内
        return qBound(chartRect.top(), static_cast<int>(y), chartRect.bottom());
    }
    
    /**
     * @brief Y坐标转价格
     */
    double yToPrice(int y) const {
        if (priceScale <= 0) {
            return minPrice;
        }
        return minPrice + (chartRect.bottom() - y) / priceScale;
    }
};

// ========== 构造和析构 ==========

/**
 * @brief 构造函数
 * @param parent 父控件
 */
KLineChart::KLineChart(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    // 设置默认样式
    d->style.upColor = QColor(Tokens::Colors::Success);      // 上涨颜色（绿色）
    d->style.downColor = QColor(Tokens::Colors::Danger);    // 下跌颜色（红色）
    d->style.flatColor = QColor(Tokens::Colors::TextSecondary);    // 平盘颜色（灰色）
    d->style.candleWidth = 8;                  // 蜡烛宽度
    d->style.candleSpacing = 2;                // 蜡烛间距
    d->style.showVolume = true;                // 显示成交量
    d->style.volumeHeightRatio = 0.2;          // 成交量高度比例
    
    // 启用鼠标追踪
    setMouseTracking(true);
    
    // 设置最小尺寸
    setMinimumSize(400, 300);
    
    LOG_DEBUG("KLineChart created");
}

/**
 * @brief 析构函数
 */
KLineChart::~KLineChart()
{
    LOG_DEBUG("KLineChart destroyed");
}

// ========== 数据管理 ==========

/**
 * @brief 设置K线数据
 * @param data K线数据
 */
void KLineChart::setData(const QVector<KLineData>& data)
{
    d->data = data;
    d->calculateVisibleRange();
    update();
    
    LOG_DEBUG(QString("KLine data set: %1 items").arg(data.size()));
}

/**
 * @brief 添加K线数据
 * @param data K线数据
 */
void KLineChart::addData(const KLineData& data)
{
    d->data.append(data);
    d->calculateVisibleRange();
    update();
}

/**
 * @brief 更新最后一条K线
 * @param data K线数据
 */
void KLineChart::updateLastData(const KLineData& data)
{
    if (!d->data.isEmpty()) {
        d->data.last() = data;
        d->calculateVisibleRange();
        update();
    }
}

/**
 * @brief 清空数据
 */
void KLineChart::clearData()
{
    d->data.clear();
    d->indicators.clear();
    update();
}

/**
 * @brief 获取数据
 */
QVector<KLineData> KLineChart::data() const
{
    return d->data;
}

// ========== 样式设置 ==========

/**
 * @brief 设置样式
 */
void KLineChart::setStyle(const KLineStyle& style)
{
    d->style = style;
    update();
}

/**
 * @brief 获取样式
 */
KLineStyle KLineChart::style() const
{
    return d->style;
}

// ========== 视图控制 ==========

/**
 * @brief 缩放
 * @param factor 缩放因子
 */
void KLineChart::zoom(double factor)
{
    // 如果没有数据，不执行缩放
    if (d->data.isEmpty()) {
        return;
    }
    
    int newCount = static_cast<int>(d->visibleCount / factor);
    newCount = qBound(10, newCount, d->data.size());
    
    // 保持中心位置
    int center = d->visibleStart + d->visibleCount / 2;
    d->visibleStart = center - newCount / 2;
    d->visibleCount = newCount;
    
    d->calculateVisibleRange();
    update();
}

/**
 * @brief 平移
 * @param dx 水平偏移
 */
void KLineChart::pan(int dx)
{
    // 如果没有数据，不执行平移
    if (d->data.isEmpty()) {
        return;
    }
    
    int candleWidth = d->style.candleWidth + d->style.candleSpacing;
    int indexDelta = dx / candleWidth;
    
    d->visibleStart -= indexDelta;
    d->visibleStart = qBound(0, d->visibleStart, 
                            d->data.size() - d->visibleCount);
    
    d->calculateVisibleRange();
    update();
}

/**
 * @brief 重置视图
 */
void KLineChart::resetView()
{
    d->visibleStart = 0;
    d->visibleCount = qMin(100, d->data.size());
    d->calculateVisibleRange();
    update();
}

/**
 * @brief 显示指定范围
 */
void KLineChart::showRange(int startIndex, int count)
{
    d->visibleStart = qBound(0, startIndex, d->data.size() - 1);
    d->visibleCount = qBound(10, count, d->data.size() - d->visibleStart);
    d->calculateVisibleRange();
    update();
}

/**
 * @brief 显示最新数据
 */
void KLineChart::showLatest(int count)
{
    d->visibleCount = qMin(count, d->data.size());
    d->visibleStart = d->data.size() - d->visibleCount;
    d->calculateVisibleRange();
    update();
}

// ========== 技术指标 ==========

/**
 * @brief 添加技术指标
 */
void KLineChart::addIndicator(const QString& name, 
                             const QVector<double>& values,
                             const QColor& color)
{
    d->indicators[name] = values;
    d->indicatorColors[name] = color;
    update();
}

/**
 * @brief 移除技术指标
 */
void KLineChart::removeIndicator(const QString& name)
{
    d->indicators.remove(name);
    d->indicatorColors.remove(name);
    update();
}

/**
 * @brief 清空所有指标
 */
void KLineChart::clearIndicators()
{
    d->indicators.clear();
    d->indicatorColors.clear();
    update();
}

// ========== 绘制事件 ==========

/**
 * @brief 绘制事件
 */
void KLineChart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QElapsedTimer timer;
    timer.start();
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false); // 性能优化：关闭抗锯齿
    
    // 绘制背景
    drawBackground(painter);
    
    // 绘制网格
    drawGrid(painter);
    
    // 绘制K线
    drawCandles(painter);
    
    // 绘制成交量
    if (d->style.showVolume) {
        drawVolume(painter);
    }
    
    // 绘制技术指标
    drawIndicators(painter);
    
    // 绘制十字光标
    if (d->showCrosshair) {
        drawCrosshair(painter);
    }
    
    // 绘制坐标轴
    drawAxis(painter);
    
    LOG_DEBUG(QString("KLineChart painted in %1ms").arg(timer.elapsed()));
}

/**
 * @brief 调整大小事件
 */
void KLineChart::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // 更新图表区域
    int volumeHeight = height() * d->style.volumeHeightRatio;
    
    d->chartRect = QRect(60, 30, width() - 80, height() - 60 - volumeHeight);
    d->volumeRect = QRect(60, height() - 30 - volumeHeight, width() - 80, volumeHeight);
    
    d->calculateVisibleRange();
}

/**
 * @brief 鼠标按下事件
 */
void KLineChart::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        d->isDragging = true;
        d->lastMouseX = event->pos().x();
        setCursor(Qt::ClosedHandCursor);
    }
}

/**
 * @brief 鼠标移动事件
 */
void KLineChart::mouseMoveEvent(QMouseEvent *event)
{
    if (d->isDragging) {
        int dx = event->pos().x() - d->lastMouseX;
        pan(dx);
        d->lastMouseX = event->pos().x();
    } else {
        // 显示十字光标
        d->showCrosshair = true;
        d->crosshairX = event->pos().x();
        d->crosshairY = event->pos().y();
        update();
        
        // 发送信号
        int index = d->xToIndex(d->crosshairX);
        if (index >= 0 && index < d->data.size()) {
            double price = d->yToPrice(d->crosshairY);
            emit crosshairMoved(d->data[index].time, price);
        }
    }
}

/**
 * @brief 鼠标释放事件
 */
void KLineChart::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        d->isDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

/**
 * @brief 滚轮事件
 */
void KLineChart::wheelEvent(QWheelEvent *event)
{
    double factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    zoom(factor);
}

/**
 * @brief 键盘按下事件
 */
void KLineChart::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Left:
            pan(-20);
            break;
        case Qt::Key_Right:
            pan(20);
            break;
        case Qt::Key_Up:
            zoom(1.2);
            break;
        case Qt::Key_Down:
            zoom(0.8);
            break;
        case Qt::Key_Home:
            showRange(0, d->visibleCount);
            break;
        case Qt::Key_End:
            showLatest(d->visibleCount);
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}

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
}

/**
 * @brief 绘制K线
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
 * @brief 绘制技术指标
 */
void KLineChart::drawIndicators(QPainter& painter)
{
    for (auto it = d->indicators.begin(); it != d->indicators.end(); ++it) {
        const QString& name = it.key();
        const QVector<double>& values = it.value();
        QColor color = d->indicatorColors.value(name, Qt::white);
        
        painter.setPen(QPen(color, 1));
        
        bool first = true;
        QPointF lastPoint;
        
        for (int i = d->visibleStart; i < d->visibleStart + d->visibleCount && i < values.size(); ++i) {
            int x = d->indexToX(i);
            int y = d->priceToY(values[i]);
            
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
    painter.setPen(QPen(QColor(Tokens::Colors::TextPrimary), 1, Qt::DashLine));
    
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
        
        // 价格标签
        painter.setPen(Qt::white);
        QString priceText = QString::number(price, 'f', 2);
        painter.drawText(5, d->crosshairY, priceText);
        
        // 时间标签
        QString timeText = d->data[index].time.toString("MM-dd hh:mm");
        painter.drawText(d->crosshairX - 30, height() - 10, timeText);
    }
}

/**
 * @brief 绘制坐标轴
 */
void KLineChart::drawAxis(QPainter& painter)
{
    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei", 9));
    
    // 价格轴（右侧）
    int priceSteps = 5;
    for (int i = 0; i <= priceSteps; ++i) {
        double price = d->minPrice + i * (d->maxPrice - d->minPrice) / priceSteps;
        int y = d->priceToY(price);
        
        QString text = QString::number(price, 'f', 2);
        painter.drawText(width() - 55, y + 4, text);
    }
    
    // 时间轴（底部）
    int timeSteps = 5;
    for (int i = 0; i <= timeSteps; ++i) {
        int index = d->visibleStart + i * d->visibleCount / timeSteps;
        if (index < d->data.size()) {
            int x = d->indexToX(index);
            QString text = d->data[index].time.toString("MM-dd");
            painter.drawText(x - 20, height() - 10, text);
        }
    }
}
