/**
 * @file KLineChartImpl.h
 * @brief KLineChart PIMPL 内部实现定义
 *
 * @details 此文件仅供 KLineChart 相关实现文件使用
 * 包含完整的 Impl 结构体定义，使拆分文件能够访问内部数据
 *
 * @author WealthPilot Team
 * @version 2.1.0
 */

#ifndef KLINECHARTIMPL_H
#define KLINECHARTIMPL_H

#include "KLineChart.h"
#include "infrastructure/config/Tokens.h"
#include <QVector>
#include <QMap>
#include <QColor>
#include <QRect>
#include <QDateTime>
#include <limits>

/**
 * @brief KLineChart::Impl 内部实现结构体
 */
struct KLineChart::Impl {
    // K线数据
    QVector<KLineData> data;
    
    // 懒加载支持
    bool lazyLoadEnabled = false;       // 是否启用懒加载
    int totalDataSize = 0;              // 总数据量（用于进度显示）
    int loadThreshold = 20;             // 滚动到边缘时触发加载的阈值
    bool isLoadingData = false;         // 是否正在加载数据
    
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
    QColor crosshairColor = QColor(100, 100, 100);
    QColor crosshairTextColor = QColor(200, 200, 200);
    
    // 拖拽状态
    bool isDragging = false;
    int lastMouseX = 0;
    
    // 图表区域
    QRect chartRect;        // 主图区域
    QRect volumeRect;       // 成交量区域
    QRect subChartRect;     // 副图指标区域
    
    // 技术指标（主图指标线）
    QMap<QString, QVector<double>> mainIndicatorLines;
    QMap<QString, QColor> mainIndicatorColors;
    
    // 副图指标数据
    QMap<QString, QVector<double>> subIndicatorLines;
    QMap<QString, QColor> subIndicatorColors;
    
    // 主图和副图指标类型
    MainIndicator currentMainIndicator = MainIndicator::MA;    // 默认MA均线
    SubIndicator currentSubIndicator = SubIndicator::MACD;     // 默认MACD
    
    // 副图区域参数
    double subMinValue = 0;         // 副图最小值
    double subMaxValue = 100;       // 副图最大值
    double subScale = 1.0;          // 副图缩放
    
    // 性能优化：压缩数据
    QVector<KLineData> compressedData;
    int compressionLevel = 1;
    
    // LOD (Level of Detail) 渲染参数（使用头文件定义的枚举）
    KLineChart::LODLevel currentLOD = KLineChart::LODLevel::Full;
    
    /**
     * @brief 根据可见数量自动选择 LOD 级别
     */
    void updateLODLevel() {
        if (visibleCount < 200) {
            currentLOD = KLineChart::LODLevel::Full;
        } else if (visibleCount < 500) {
            currentLOD = KLineChart::LODLevel::Medium;
        } else if (visibleCount < 1000) {
            currentLOD = KLineChart::LODLevel::Low;
        } else {
            currentLOD = KLineChart::LODLevel::Minimal;
        }
    }
    
    /**
     * @brief 计算可见范围
     */
    void calculateVisibleRange() {
        if (data.isEmpty()) {
            minPrice = maxPrice = 0;
            priceScale = 1.0;
            return;
        }
        
        // 确保可见范围有效
        visibleStart = qBound(0, visibleStart, data.size() - 1);
        visibleCount = qBound(10, visibleCount, data.size() - visibleStart);
        
        // 计算可见范围内的价格范围
        minPrice = std::numeric_limits<double>::max();
        maxPrice = std::numeric_limits<double>::lowest();
        
        for (int i = visibleStart; i < visibleStart + visibleCount && i < data.size(); ++i) {
            if (data[i].high > 0) {
                minPrice = qMin(minPrice, data[i].low);
                maxPrice = qMax(maxPrice, data[i].high);
            }
        }
        
        // 处理无效范围
        if (minPrice >= maxPrice || minPrice == std::numeric_limits<double>::max()) {
            minPrice = 0;
            maxPrice = 100;
        }
        
        // 添加边距（5%）
        double margin = (maxPrice - minPrice) * 0.05;
        if (margin <= 0) {
            margin = 1;
        }
        minPrice -= margin;
        maxPrice += margin;
        
        // 计算缩放
        if (chartRect.height() > 0) {
            priceScale = chartRect.height() / (maxPrice - minPrice);
        }
        
        // 更新LOD级别
        updateLODLevel();
    }
    
    /**
     * @brief 计算副图指标范围
     */
    void calculateSubChartRange() {
        if (subChartRect.height() <= 0 || subIndicatorLines.isEmpty()) {
            subMinValue = 0;
            subMaxValue = 100;
            subScale = 1.0;
            return;
        }
        
        // 找出所有副图指标的最大最小值
        subMinValue = std::numeric_limits<double>::max();
        subMaxValue = std::numeric_limits<double>::lowest();
        
        for (const auto& values : subIndicatorLines) {
            for (int i = visibleStart; i < visibleStart + visibleCount && i < values.size(); ++i) {
                if (values[i] != 0) {  // 跳过0值
                    subMinValue = qMin(subMinValue, values[i]);
                    subMaxValue = qMax(subMaxValue, values[i]);
                }
            }
        }
        
        // 处理无效范围
        if (subMinValue >= subMaxValue || subMinValue == std::numeric_limits<double>::max()) {
            subMinValue = 0;
            subMaxValue = 100;
        }
        
        // 添加边距
        double margin = (subMaxValue - subMinValue) * 0.1;
        if (margin <= 0) {
            margin = 5;
        }
        subMinValue -= margin;
        subMaxValue += margin;
        
        // 计算缩放
        subScale = subChartRect.height() / (subMaxValue - subMinValue);
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

    int timeToX(int index) const
    {
        if (data.isEmpty()) return 0;
        int candleWidth = style.candleWidth + style.candleSpacing;
        return chartRect.left() + (index - visibleStart) * candleWidth + candleWidth / 2;
    }

    /**
     * @brief 副图指标值转Y坐标
     */
    int subValueToY(double value) const {
        if (subScale <= 0) {
            return subChartRect.top();
        }
        double y = subChartRect.bottom() - (value - subMinValue) * subScale;
        return qBound(subChartRect.top(), static_cast<int>(y), subChartRect.bottom());
    }
    
    /**
     * @brief Y坐标转副图指标值
     */
    double yToSubValue(int y) const {
        if (subScale <= 0) {
            return subMinValue;
        }
        return subMinValue + (subChartRect.bottom() - y) / subScale;
    }
};

#endif // KLINECHARTIMPL_H