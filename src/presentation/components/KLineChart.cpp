/**
 * @file KLineChart.cpp
 * @brief K线图组件实现 - 核心逻辑
 *
 * @details 实现功能：
 * - 构造和析构
 * - 数据管理（设置、添加、清空）
 * - 视图控制（缩放、平移、重置）
 * - 坐标转换
 * - 性能优化：数据压缩、分片加载
 *
 * 拆分文件：
 * - KLineChartRendering.cpp - 绘制方法
 * - KLineChartIndicators.cpp - 指标计算
 * - KLineChartEvents.cpp - 事件处理
 *
 * @author WealthPilot Team
 * @version 2.1.0
 */

#include "KLineChartImpl.h"
#include "infrastructure/config/Tokens.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QElapsedTimer>
#include <QtMath>

#include "shared/utils/Logger.h"

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
    d->style.upColor = QColor(Tokens::Colors::Danger);     // 上涨颜色（红色）- 中国市场
    d->style.downColor = QColor(Tokens::Colors::Success);   // 下跌颜色（绿色）- 中国市场
    d->style.flatColor = QColor(Tokens::Colors::TextSecondary);    // 平盘颜色（灰色）
    d->style.candleWidth = 8;                  // 蜡烛宽度
    d->style.candleSpacing = 2;                // 蜡烛间距
    d->style.showVolume = true;                // 显示成交量
    d->style.volumeHeightRatio = 0.05;         // 成交量高度比例（减少一半）
    
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

    // 性能优化：数据压缩
    compressData();

    d->calculateVisibleRange();
    
    // 自动计算默认指标
    if (!data.isEmpty()) {
        // 计算默认主图指标（MA）
        if (d->mainIndicatorLines.isEmpty()) {
            setMainIndicator(d->currentMainIndicator);
        }
        // 计算默认副图指标（MACD）
        if (d->subIndicatorLines.isEmpty()) {
            setSubIndicator(d->currentSubIndicator);
        }
    }
    
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

    // 性能优化：增量绘制
    int lastIndex = d->data.size() - 1;
    if (lastIndex >= d->visibleStart && lastIndex < d->visibleStart + d->visibleCount)
    {
        int candleWidth = d->style.candleWidth + d->style.candleSpacing;
        int x = timeToX(lastIndex);
        QRect updateRect(x - candleWidth, 0, candleWidth * 2, height());
        update(updateRect);
    }
    else
    {
        d->calculateVisibleRange();
        update();
    }
    
    LOG_DEBUG(QString("KLine data added, total: %1").arg(d->data.size()));
}

/**
 * @brief 更新最后一条K线
 * @param data K线数据
 */
void KLineChart::updateLastData(const KLineData& data)
{
    if (d->data.isEmpty()) return;
    
    d->data.last() = data;
    
    // 增量更新
    int lastIndex = d->data.size() - 1;
    if (lastIndex >= d->visibleStart && lastIndex < d->visibleStart + d->visibleCount) {
        int candleWidth = d->style.candleWidth + d->style.candleSpacing;
        int x = timeToX(lastIndex);
        QRect updateRect(x - candleWidth, 0, candleWidth * 2, height());
        update(updateRect);
    }
}

/**
 * @brief 清空数据
 */
void KLineChart::clearData()
{
    d->data.clear();
    d->compressedData.clear();
    d->mainIndicatorLines.clear();
    d->mainIndicatorColors.clear();
    d->subIndicatorLines.clear();
    d->subIndicatorColors.clear();
    d->visibleStart = 0;
    d->visibleCount = 100;
    update();
    
    LOG_DEBUG("KLine data cleared");
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
    // 调整可见数量
    int newCount = static_cast<int>(d->visibleCount / factor);
    newCount = qBound(20, newCount, 500);  // 限制范围
    
    // 调整起始位置（以中心为缩放点）
    int center = d->visibleStart + d->visibleCount / 2;
    d->visibleCount = newCount;
    d->visibleStart = qBound(0, center - newCount / 2, d->data.size() - newCount);
    
    d->calculateVisibleRange();
    
    // 检查是否需要加载更多数据
    checkAndRequestMoreData();
    
    update();
    emit visibleRangeChanged(d->visibleStart, d->visibleCount);
}

/**
 * @brief 平移
 * @param dx 水平偏移量（像素）
 */
void KLineChart::pan(int dx)
{
    if (d->data.isEmpty()) return;
    
    int candleWidth = d->style.candleWidth + d->style.candleSpacing;
    int indexDelta = dx / candleWidth;
    
    d->visibleStart = qBound(0, d->visibleStart - indexDelta, d->data.size() - d->visibleCount);
    
    d->calculateVisibleRange();
    
    // 检查是否需要加载更多数据
    checkAndRequestMoreData();
    
    update();
    emit visibleRangeChanged(d->visibleStart, d->visibleCount);
}

/**
 * @brief 重置视图
 */
void KLineChart::resetView()
{
    d->visibleCount = 100;
    d->visibleStart = 0;
    d->calculateVisibleRange();
    update();
    emit visibleRangeChanged(d->visibleStart, d->visibleCount);
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
    emit visibleRangeChanged(d->visibleStart, d->visibleCount);
}

/**
 * @brief 显示最新数据
 */
void KLineChart::showLatest(int count)
{
    if (d->data.isEmpty()) return;
    
    d->visibleCount = qMin(count, d->data.size());
    d->visibleStart = d->data.size() - d->visibleCount;
    d->calculateVisibleRange();
    update();
    emit visibleRangeChanged(d->visibleStart, d->visibleCount);
}

/**
 * @brief 获取可见起始索引
 */
int KLineChart::visibleStartIndex() const
{
    return d->visibleStart;
}

/**
 * @brief 获取可见数量
 */
int KLineChart::visibleCount() const
{
    return d->visibleCount;
}

// ==================== 数据压缩 ====================

void KLineChart::compressData()
{
    if (d->data.isEmpty())
    {
        d->compressedData.clear();
        return;
    }

    int visibleCount = width() / (d->style.candleWidth + d->style.candleSpacing);
    if (visibleCount <= 0) visibleCount = 100;

    int total = d->data.size();
    if (total <= visibleCount)
    {
        d->compressedData.clear();
        return;
    }

    int ratio = (total + visibleCount - 1) / visibleCount;
    ratio = qMax(2, ratio);

    d->compressedData.clear();
    d->compressedData.reserve(total / ratio + 1);

    for (int i = 0; i < total; i += ratio)
    {
        KLineData compressed;
        compressed.time = d->data[i].time;
        compressed.open = d->data[i].open;
        compressed.high = d->data[i].high;
        compressed.low = d->data[i].low;
        compressed.close = d->data[i].close;
        compressed.volume = 0;

        int end = qMin(i + ratio, total);
        for (int j = i; j < end; ++j)
        {
            compressed.high = qMax(compressed.high, d->data[j].high);
            compressed.low = qMin(compressed.low, d->data[j].low);
            compressed.volume += d->data[j].volume;
        }
        compressed.close = d->data[end - 1].close;

        d->compressedData.append(compressed);
    }

    LOG_DEBUG(QString("Data compressed: %1 -> %2 (ratio: %3)")
              .arg(total).arg(d->compressedData.size()).arg(ratio));
}

int KLineChart::timeToX(int index) const
{
    return d->timeToX(index);
}

// ==================== 分片加载功能 ====================

void KLineChart::setDataWithLazyLoad(const QVector<KLineData>& initialData, int totalCount)
{
    d->lazyLoadEnabled = true;
    d->totalDataSize = totalCount;
    d->data = initialData;
    
    // 性能优化：数据压缩
    compressData();
    
    // 显示最新数据
    d->visibleCount = qMin(100, d->data.size());
    d->visibleStart = d->data.size() - d->visibleCount;
    d->calculateVisibleRange();
    
    // 计算默认指标
    if (!initialData.isEmpty()) {
        if (d->mainIndicatorLines.isEmpty()) {
            setMainIndicator(d->currentMainIndicator);
        }
        if (d->subIndicatorLines.isEmpty()) {
            setSubIndicator(d->currentSubIndicator);
        }
    }
    
    update();
    emit dataLoadProgress(initialData.size(), totalCount);
    
    LOG_DEBUG(QString("KLine data set with lazy load: %1/%2 items")
              .arg(initialData.size()).arg(totalCount));
}

void KLineChart::prependHistoricalData(const QVector<KLineData>& historicalData)
{
    if (historicalData.isEmpty()) return;
    
    d->isLoadingData = false;
    
    // 计算新增数据后的起始位置偏移
    int addedCount = historicalData.size();
    int oldSize = d->data.size();
    
    // 预分配空间
    d->data.reserve(oldSize + addedCount);
    
    // 在前面插入历史数据
    QVector<KLineData> newData;
    newData.reserve(oldSize + addedCount);
    newData.append(historicalData);
    newData.append(d->data);
    d->data = std::move(newData);
    
    // 调整可见起始位置（保持当前视图不变）
    d->visibleStart += addedCount;
    
    // 重新计算指标
    setMainIndicator(d->currentMainIndicator);
    setSubIndicator(d->currentSubIndicator);
    
    // 性能优化：数据压缩
    compressData();
    d->calculateVisibleRange();
    
    update();
    emit dataLoadProgress(d->data.size(), d->totalDataSize);
    
    LOG_DEBUG(QString("Prepended %1 historical KLine items, total: %2")
              .arg(addedCount).arg(d->data.size()));
}

void KLineChart::checkAndRequestMoreData()
{
    if (!d->lazyLoadEnabled || d->isLoadingData) return;
    
    // 检查是否滚动到左侧边缘
    if (d->visibleStart <= d->loadThreshold) {
        // 请求加载更多历史数据
        int requestedCount = 200;  // 每次加载200条
        d->isLoadingData = true;
        emit historicalDataRequested(d->data.size(), requestedCount);
    }
}

int KLineChart::dataSize() const
{
    return d->data.size();
}

bool KLineChart::isLazyLoadEnabled() const
{
    return d->lazyLoadEnabled;
}

void KLineChart::setLazyLoadEnabled(bool enabled)
{
    d->lazyLoadEnabled = enabled;
}

// ==================== LOD渲染支持 ====================

KLineChart::LODLevel KLineChart::currentLODLevel() const
{
    return d->currentLOD;
}

void KLineChart::setLODLevel(LODLevel level)
{
    d->currentLOD = level;
}

int KLineChart::getLODCandleWidth() const
{
    switch (d->currentLOD) {
        case LODLevel::Full:
            return d->style.candleWidth;
        case LODLevel::Medium:
            return d->style.candleWidth - 2;
        case LODLevel::Low:
            return 4;
        case LODLevel::Minimal:
            return 2;
    }
    return d->style.candleWidth;
}