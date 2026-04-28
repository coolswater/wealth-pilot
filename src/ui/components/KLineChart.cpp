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
    int crosshairIndex = -1;        // 当前十字光标对应的K线索引
    
    // 技术指标
    QMap<QString, QVector<double>> indicators;
    QMap<QString, QColor> indicatorColors;
    
    // 主图和副图指标
    MainIndicator mainIndicator = MainIndicator::MA;
    SubIndicator subIndicator = SubIndicator::MACD;
    
    // 性能优化：压缩数据
    QVector<KLineData> compressedData;
    int compressionLevel = 1;
    
    // 鼠标交互
    bool isDragging = false;
    int lastMouseX = 0;
    
    // 图表区域
    QRect chartRect;
    QRect volumeRect;
    
    // 十字光标颜色（主题适配）
    QColor crosshairColor = QColor("#3b82f6");  // 蓝色
    QColor crosshairTextColor = QColor("#ffffff");  // 白色
    
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
        // 绘制K线信息（右上角）
        drawKLineInfo(painter);
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
        
        // 计算当前K线索引
        int index = d->xToIndex(d->crosshairX);
        d->crosshairIndex = index;
        
        update();
        
        // 发送信号
        if (index >= 0 && index < d->data.size()) {
            double price = d->yToPrice(d->crosshairY);
            emit crosshairMoved(d->data[index].time, price);
            // 发送K线信息信号
            emit klineInfoChanged(d->data[index], index);
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
        painter.fillRect(0, d->crosshairY - 10, textWidth, 20, QColor("#2d3748"));
        painter.drawText(5, d->crosshairY + 4, priceText);
        
        // 时间标签（底部）
        QString timeText = d->data[index].time.toString("MM-dd hh:mm");
        int timeWidth = fm.horizontalAdvance(timeText) + 10;
        painter.fillRect(d->crosshairX - timeWidth/2, height() - 25, timeWidth, 20, QColor("#2d3748"));
        painter.drawText(d->crosshairX - timeWidth/2 + 5, height() - 10, timeText);
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

/**
 * @brief 绘制K线信息（右上角）
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
    
    // 振幅和换手率
    double amplitude = (kline.high - kline.low) / kline.open * 100;
    double turnover = 0;  // 换手率需要额外数据
    
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
    
    // 换手率（如果有）
    if (turnover > 0) {
        infoParts << QString(QStringLiteral("换手:%1%")).arg(turnover, 0, 'f', 2);
    }
    
    // 合并为一行
    QString infoText = infoParts.join("  ");
    
    // 计算文本宽度
    int textWidth = fm.horizontalAdvance(infoText) + 20;
    int textHeight = fm.height() + 8;
    
    // 绘制背景（左上角）
    int infoX = 60;  // 左边距
    int infoY = 10;
    
    painter.fillRect(infoX - 5, infoY - 2, textWidth + 10, textHeight + 4, QColor("#1a1f2e"));
    painter.fillRect(infoX, infoY, textWidth, textHeight, QColor("#2d3748"));
    
    // 绘制文本
    painter.setPen(QColor("#ffffff"));
    painter.drawText(infoX + 10, infoY + textHeight - 6, infoText);
}

/**
 * @brief 设置主图指标
 */
void KLineChart::setMainIndicator(MainIndicator indicator)
{
    d->mainIndicator = indicator;
    
    // 清除现有主图指标
    removeIndicator("MA5");
    removeIndicator("MA10");
    removeIndicator("MA20");
    removeIndicator("EMA12");
    removeIndicator("EMA26");
    removeIndicator("BOLL_UP");
    removeIndicator("BOLL_MID");
    removeIndicator("BOLL_LOW");
    
    switch (indicator) {
        case MainIndicator::MA:
            calculateMA(5);
            calculateMA(10);
            calculateMA(20);
            break;
        case MainIndicator::EMA:
            calculateEMA(12);
            calculateEMA(26);
            break;
        case MainIndicator::BOLL:
            calculateBOLL(20);
            break;
        default:
            break;
    }
    
    update();
}

/**
 * @brief 设置副图指标
 */
void KLineChart::setSubIndicator(SubIndicator indicator)
{
    d->subIndicator = indicator;
    
    // 清除现有副图指标
    removeIndicator("MACD");
    removeIndicator("MACD_SIGNAL");
    removeIndicator("MACD_HIST");
    removeIndicator("KDJ_K");
    removeIndicator("KDJ_D");
    removeIndicator("KDJ_J");
    removeIndicator("RSI");
    
    switch (indicator) {
        case SubIndicator::MACD:
            calculateMACD();
            break;
        case SubIndicator::KDJ:
            calculateKDJ();
            break;
        case SubIndicator::RSI:
            calculateRSI(14);
            break;
        default:
            break;
    }
    
    update();
}

/**
 * @brief 计算MA移动平均线
 */
void KLineChart::calculateMA(int period)
{
    if (d->data.size() < period) return;
    
    QVector<double> maValues;
    maValues.resize(d->data.size());
    
    for (int i = 0; i < d->data.size(); ++i) {
        if (i < period - 1) {
            maValues[i] = 0;
        } else {
            double sum = 0;
            for (int j = 0; j < period; ++j) {
                sum += d->data[i - j].close;
            }
            maValues[i] = sum / period;
        }
    }
    
    QColor color;
    switch (period) {
        case 5: color = QColor("#ffffff"); break;
        case 10: color = QColor("#ffff00"); break;
        case 20: color = QColor("#ff00ff"); break;
        default: color = QColor("#ffffff"); break;
    }
    
    addIndicator(QString("MA%1").arg(period), maValues, color);
}

/**
 * @brief 计算EMA指数移动平均
 */
void KLineChart::calculateEMA(int period)
{
    if (d->data.isEmpty()) return;
    
    QVector<double> emaValues;
    emaValues.resize(d->data.size());
    
    double multiplier = 2.0 / (period + 1);
    emaValues[0] = d->data[0].close;
    
    for (int i = 1; i < d->data.size(); ++i) {
        emaValues[i] = (d->data[i].close - emaValues[i-1]) * multiplier + emaValues[i-1];
    }
    
    QColor color = (period == 12) ? QColor("#00ff00") : QColor("#ff6600");
    addIndicator(QString("EMA%1").arg(period), emaValues, color);
}

/**
 * @brief 计算BOLL布林带
 */
void KLineChart::calculateBOLL(int period)
{
    if (d->data.size() < period) return;
    
    QVector<double> midValues, upValues, lowValues;
    midValues.resize(d->data.size());
    upValues.resize(d->data.size());
    lowValues.resize(d->data.size());
    
    for (int i = 0; i < d->data.size(); ++i) {
        if (i < period - 1) {
            midValues[i] = upValues[i] = lowValues[i] = 0;
        } else {
            // 计算中轨（MA）
            double sum = 0;
            for (int j = 0; j < period; ++j) {
                sum += d->data[i - j].close;
            }
            double mid = sum / period;
            
            // 计算标准差
            double variance = 0;
            for (int j = 0; j < period; ++j) {
                variance += qPow(d->data[i - j].close - mid, 2);
            }
            double stdDev = qSqrt(variance / period);
            
            midValues[i] = mid;
            upValues[i] = mid + 2 * stdDev;
            lowValues[i] = mid - 2 * stdDev;
        }
    }
    
    addIndicator("BOLL_MID", midValues, QColor("#ffffff"));
    addIndicator("BOLL_UP", upValues, QColor("#ff4d4f"));
    addIndicator("BOLL_LOW", lowValues, QColor("#00b578"));
}

/**
 * @brief 计算MACD
 */
void KLineChart::calculateMACD()
{
    if (d->data.size() < 26) return;
    
    QVector<double> difValues, deaValues, macdValues;
    difValues.resize(d->data.size());
    deaValues.resize(d->data.size());
    macdValues.resize(d->data.size());
    
    // 计算EMA12和EMA26
    QVector<double> ema12(d->data.size());
    QVector<double> ema26(d->data.size());
    
    double mult12 = 2.0 / 13;
    double mult26 = 2.0 / 27;
    
    ema12[0] = d->data[0].close;
    ema26[0] = d->data[0].close;
    
    for (int i = 1; i < d->data.size(); ++i) {
        ema12[i] = (d->data[i].close - ema12[i-1]) * mult12 + ema12[i-1];
        ema26[i] = (d->data[i].close - ema26[i-1]) * mult26 + ema26[i-1];
        difValues[i] = ema12[i] - ema26[i];
    }
    
    // 计算DEA（EMA9 of DIF）
    double mult9 = 2.0 / 10;
    deaValues[0] = difValues[0];
    for (int i = 1; i < d->data.size(); ++i) {
        deaValues[i] = (difValues[i] - deaValues[i-1]) * mult9 + deaValues[i-1];
        macdValues[i] = (difValues[i] - deaValues[i]) * 2;
    }
    
    addIndicator("MACD_DIF", difValues, QColor("#ffffff"));
    addIndicator("MACD_DEA", deaValues, QColor("#ffff00"));
}

/**
 * @brief 计算KDJ
 */
void KLineChart::calculateKDJ()
{
    if (d->data.size() < 9) return;
    
    QVector<double> kValues, dValues, jValues;
    kValues.resize(d->data.size());
    dValues.resize(d->data.size());
    jValues.resize(d->data.size());
    
    for (int i = 0; i < d->data.size(); ++i) {
        if (i < 8) {
            kValues[i] = dValues[i] = jValues[i] = 50;
        } else {
            // 计算最高价和最低价
            double highest = d->data[i].high;
            double lowest = d->data[i].low;
            for (int j = 1; j < 9; ++j) {
                highest = qMax(highest, d->data[i-j].high);
                lowest = qMin(lowest, d->data[i-j].low);
            }
            
            double rsv = (highest == lowest) ? 50 : (d->data[i].close - lowest) / (highest - lowest) * 100;
            kValues[i] = (2.0 / 3) * kValues[i-1] + (1.0 / 3) * rsv;
            dValues[i] = (2.0 / 3) * dValues[i-1] + (1.0 / 3) * kValues[i];
            jValues[i] = 3 * kValues[i] - 2 * dValues[i];
        }
    }
    
    addIndicator("KDJ_K", kValues, QColor("#ffffff"));
    addIndicator("KDJ_D", dValues, QColor("#ffff00"));
    addIndicator("KDJ_J", jValues, QColor("#ff4d4f"));
}

/**
 * @brief 计算RSI
 */
void KLineChart::calculateRSI(int period)
{
    if (d->data.size() < period + 1) return;
    
    QVector<double> rsiValues;
    rsiValues.resize(d->data.size());
    
    for (int i = 0; i < d->data.size(); ++i) {
        if (i < period) {
            rsiValues[i] = 50;
        } else {
            double gain = 0, loss = 0;
            for (int j = 0; j < period; ++j) {
                double change = d->data[i - j].close - d->data[i - j - 1].close;
                if (change > 0) gain += change;
                else loss -= change;
            }
            
            if (gain + loss == 0) {
                rsiValues[i] = 50;
            } else {
                rsiValues[i] = gain / (gain + loss) * 100;
            }
        }
    }
    
    addIndicator("RSI", rsiValues, QColor("#ff6600"));
}
