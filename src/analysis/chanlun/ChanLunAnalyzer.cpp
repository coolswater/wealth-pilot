/**
 * @file ChanLunAnalyzer.cpp
 * @brief 缠论分析器实现 - 核心算法
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ChanLunAnalyzer.h"
#include "utils/Logger.h"
#include <algorithm>
#include <cmath>

namespace WealthPilot {
namespace ChanLun {

// ============================================================================
// Impl 定义
// ============================================================================

struct ChanLunAnalyzer::Impl {
    ChanLunResult result;           ///< 分析结果缓存
    QVector<RawKLine> rawKlines;    ///< 原始K线缓存
    
    // MACD参数
    int macdFast = 12;
    int macdSlow = 26;
    int macdSignal = 9;
    
    // 笔的最小K线数量
    int minPenKLines = 4;
};

// ============================================================================
// 构造与析构
// ============================================================================

ChanLunAnalyzer::ChanLunAnalyzer(QObject* parent)
    : IAnalyzer(parent)
    , d(std::make_unique<Impl>())
{
    LOG_DEBUG("ChanLunAnalyzer created");
}

ChanLunAnalyzer::~ChanLunAnalyzer() = default;

// ============================================================================
// 主要接口实现
// ============================================================================

ChanLunResult ChanLunAnalyzer::analyze(const QVector<RawKLine>& klines)
{
    if (klines.size() < 5) {
        LOG_WARNING("Not enough K-lines for ChanLun analysis");
        return ChanLunResult();
    }
    
    LOG_INFO(QString("Starting ChanLun analysis with %1 K-lines").arg(klines.size()));
    
    // 清空之前的结果
    d->result.clear();
    d->rawKlines = klines;
    
    // 1. K线包含处理
    d->result.klines = processContainment(klines);
    LOG_DEBUG(QString("Processed containment: %1 standard K-lines").arg(d->result.klines.size()));
    
    if (d->result.klines.size() < 5) {
        return d->result;
    }
    
    // 2. 识别分型
    d->result.fractals = identifyFractals(d->result.klines);
    LOG_DEBUG(QString("Found %1 fractals").arg(d->result.fractals.size()));
    
    if (d->result.fractals.size() < 2) {
        return d->result;
    }
    
    // 3. 划分笔
    d->result.pens = identifyPens(d->result.klines, d->result.fractals);
    LOG_DEBUG(QString("Identified %1 pens").arg(d->result.pens.size()));
    
    if (d->result.pens.size() < 3) {
        return d->result;
    }
    
    // 4. 划分线段
    d->result.segments = identifySegments(d->result.pens);
    LOG_DEBUG(QString("Identified %1 segments").arg(d->result.segments.size()));
    
    // 5. 识别中枢
    if (d->result.segments.size() >= 3) {
        d->result.pivots = identifyPivots(d->result.segments, d->result.pens);
        LOG_DEBUG(QString("Found %1 pivots").arg(d->result.pivots.size()));
    }
    
    // 6. 判断背驰
    d->result.divergences = detectDivergence(d->result.klines, d->result.pens, d->result.pivots);
    LOG_DEBUG(QString("Detected %1 divergences").arg(d->result.divergences.size()));
    
    // 7. 识别买卖点
    d->result.tradeSignals = identifySignals(d->result.klines, d->result.pens, 
                                        d->result.pivots, d->result.divergences);
    LOG_DEBUG(QString("Found %1 trade signals").arg(d->result.tradeSignals.size()));
    
    emit analysisCompleted(d->result);
    
    LOG_INFO("ChanLun analysis completed");
    return d->result;
}

ChanLunResult ChanLunAnalyzer::update(const QVector<RawKLine>& newKlines)
{
    // 追加新K线并重新分析
    d->rawKlines.append(newKlines);
    return analyze(d->rawKlines);
}

const ChanLunResult& ChanLunAnalyzer::result() const
{
    return d->result;
}

void ChanLunAnalyzer::clear()
{
    d->result.clear();
    d->rawKlines.clear();
}

// ============================================================================
// K线包含处理
// ============================================================================

QVector<StandardKLine> ChanLunAnalyzer::processContainment(const QVector<RawKLine>& klines)
{
    if (klines.isEmpty()) {
        return {};
    }
    
    QVector<StandardKLine> result;
    result.reserve(klines.size());
    
    // 转换第一根K线
    StandardKLine prev;
    prev.originalIndex = 0;
    prev.time = klines[0].time;
    prev.high = klines[0].high;
    prev.low = klines[0].low;
    prev.open = klines[0].open;
    prev.close = klines[0].close;
    prev.volume = klines[0].volume;
    prev.direction = 0;
    result.append(prev);
    
    // 处理后续K线
    for (int i = 1; i < klines.size(); ++i) {
        StandardKLine curr;
        curr.originalIndex = i;
        curr.time = klines[i].time;
        curr.high = klines[i].high;
        curr.low = klines[i].low;
        curr.open = klines[i].open;
        curr.close = klines[i].close;
        curr.volume = klines[i].volume;
        curr.direction = 0;
        
        // 检查包含关系
        int containment = checkContainment(prev, curr);
        
        if (containment != 0) {
            // 存在包含关系，进行合并
            // 方向由前一根非包含K线决定
            int mergeDirection = prev.direction;
            if (mergeDirection == 0) {
                // 如果前一根也没有方向，根据价格关系确定
                mergeDirection = (curr.high > prev.high) ? 1 : -1;
            }
            
            StandardKLine merged = mergeKLines(prev, curr, mergeDirection);
            merged.direction = mergeDirection;
            
            // 替换最后一根
            result[result.size() - 1] = merged;
            prev = merged;
        } else {
            // 无包含关系，添加新K线
            // 确定方向
            if (curr.high > prev.high && curr.low > prev.low) {
                curr.direction = 1;  // 向上
            } else if (curr.high < prev.high && curr.low < prev.low) {
                curr.direction = -1; // 向下
            } else {
                curr.direction = prev.direction; // 继承前一根方向
            }
            
            result.append(curr);
            prev = curr;
        }
    }
    
    return result;
}

int ChanLunAnalyzer::checkContainment(const StandardKLine& k1, const StandardKLine& k2)
{
    // k1包含k2：k1.high >= k2.high && k1.low <= k2.low
    if (k1.high >= k2.high && k1.low <= k2.low) {
        return 1;
    }
    // k2包含k1：k2.high >= k1.high && k2.low <= k1.low
    if (k2.high >= k1.high && k2.low <= k1.low) {
        return -1;
    }
    return 0;
}

StandardKLine ChanLunAnalyzer::mergeKLines(const StandardKLine& k1, const StandardKLine& k2, int direction)
{
    StandardKLine merged;
    merged.originalIndex = k1.originalIndex; // 保留第一根的索引
    merged.time = k1.time;
    merged.open = k1.open;
    merged.close = k2.close;
    merged.volume = k1.volume + k2.volume;
    merged.direction = direction;
    
    if (direction > 0) {
        // 向上处理：取高高、取高低
        merged.high = std::max(k1.high, k2.high);
        merged.low = std::max(k1.low, k2.low);
    } else {
        // 向下处理：取低高、取低低
        merged.high = std::min(k1.high, k2.high);
        merged.low = std::min(k1.low, k2.low);
    }
    
    return merged;
}

// ============================================================================
// 分型识别
// ============================================================================

QVector<Fractal> ChanLunAnalyzer::identifyFractals(const QVector<StandardKLine>& klines)
{
    QVector<Fractal> fractals;
    
    if (klines.size() < 3) {
        return fractals;
    }
    
    for (int i = 1; i < klines.size() - 1; ++i) {
        const auto& k1 = klines[i - 1];
        const auto& k2 = klines[i];
        const auto& k3 = klines[i + 1];
        
        Fractal fractal;
        fractal.index = i;
        fractal.time = k2.time;
        
        if (isTopFractal(k1, k2, k3)) {
            fractal.type = FractalType::Top;
            fractal.value = k2.high;
            fractals.append(fractal);
        } else if (isBottomFractal(k1, k2, k3)) {
            fractal.type = FractalType::Bottom;
            fractal.value = k2.low;
            fractals.append(fractal);
        }
    }
    
    return fractals;
}

bool ChanLunAnalyzer::isTopFractal(const StandardKLine& k1, const StandardKLine& k2, const StandardKLine& k3)
{
    // 顶分型：中间K线的高点和低点都是最高的
    return k2.high > k1.high && k2.high > k3.high &&
           k2.low > k1.low && k2.low > k3.low;
}

bool ChanLunAnalyzer::isBottomFractal(const StandardKLine& k1, const StandardKLine& k2, const StandardKLine& k3)
{
    // 底分型：中间K线的高点和低点都是最低的
    return k2.high < k1.high && k2.high < k3.high &&
           k2.low < k1.low && k2.low < k3.low;
}

// ============================================================================
// 笔划分
// ============================================================================

QVector<Pen> ChanLunAnalyzer::identifyPens(const QVector<StandardKLine>& klines, 
                                            const QVector<Fractal>& fractals)
{
    QVector<Pen> pens;
    
    if (fractals.size() < 2) {
        return pens;
    }
    
    // 笔的划分规则：
    // 1. 相邻的顶底分型之间形成一笔
    // 2. 顶底分型之间至少有1根独立K线（处理后至少3根）
    // 3. 笔的终点分型要比起点分型更极端
    
    int i = 0;
    while (i < fractals.size() - 1) {
        const Fractal& f1 = fractals[i];
        
        // 寻找下一个不同类型的分型
        int j = i + 1;
        while (j < fractals.size()) {
            const Fractal& f2 = fractals[j];
            
            // 必须是不同类型的分型
            if (f1.type == f2.type) {
                // 同类型分型，取更极端的
                if (f1.isTop() && f2.value > f1.value) {
                    i = j; // 更新起点
                } else if (f1.isBottom() && f2.value < f1.value) {
                    i = j;
                }
                j++;
                continue;
            }
            
            // 检查中间是否有足够的K线
            int klineCount = f2.index - f1.index;
            if (klineCount < d->minPenKLines) {
                j++;
                continue;
            }
            
            // 检查是否满足笔的条件
            // 向上笔：底分型的低点要低于顶分型的高点
            // 向下笔：顶分型的高点要高于底分型的低点
            bool validPen = false;
            if (f1.isBottom() && f2.isTop()) {
                // 向上笔
                // 检查中间是否有价格高于顶分型的
                bool hasHigher = false;
                for (int k = f1.index + 1; k < f2.index; ++k) {
                    if (klines[k].high > f2.value) {
                        hasHigher = true;
                        break;
                    }
                }
                if (!hasHigher) {
                    validPen = true;
                }
            } else if (f1.isTop() && f2.isBottom()) {
                // 向下笔
                bool hasLower = false;
                for (int k = f1.index + 1; k < f2.index; ++k) {
                    if (klines[k].low < f2.value) {
                        hasLower = true;
                        break;
                    }
                }
                if (!hasLower) {
                    validPen = true;
                }
            }
            
            if (validPen) {
                Pen pen;
                pen.startIndex = i;
                pen.endIndex = j;
                pen.startKLineIndex = f1.index;
                pen.endKLineIndex = f2.index;
                pen.startValue = f1.value;
                pen.endValue = f2.value;
                pen.startTime = f1.time;
                pen.endTime = f2.time;
                pen.direction = f1.isBottom() ? PenDirection::Up : PenDirection::Down;
                
                pens.append(pen);
                i = j;
                break;
            }
            
            j++;
        }
        
        if (j >= fractals.size()) {
            break;
        }
    }
    
    return pens;
}

// ============================================================================
// 线段划分
// ============================================================================

QVector<Segment> ChanLunAnalyzer::identifySegments(const QVector<Pen>& pens)
{
    QVector<Segment> segments;
    
    if (pens.size() < 3) {
        return segments;
    }
    
    // 线段划分规则：
    // 1. 至少由3笔构成
    // 2. 线段被破坏的条件：出现新的笔破坏了线段的趋势
    
    int segStart = 0;
    int segDirection = pens[0].isUp() ? 1 : -1;
    
    for (int i = 1; i < pens.size(); ++i) {
        // 检查线段是否被破坏
        if (isSegmentBroken(pens, segStart, i - 1, i)) {
            // 形成新线段
            if (i - segStart >= 3) {
                Segment seg;
                seg.startIndex = segStart;
                seg.endIndex = i - 1;
                seg.startKLineIndex = pens[segStart].startKLineIndex;
                seg.endKLineIndex = pens[i - 1].endKLineIndex;
                seg.startValue = pens[segStart].startValue;
                seg.endValue = pens[i - 1].endValue;
                seg.startTime = pens[segStart].startTime;
                seg.endTime = pens[i - 1].endTime;
                seg.direction = segDirection > 0 ? SegmentDirection::Up : SegmentDirection::Down;
                seg.isConfirmed = true;
                
                segments.append(seg);
            }
            
            segStart = i;
            segDirection = pens[i].isUp() ? 1 : -1;
        }
    }
    
    // 处理最后一个未完成的线段
    if (pens.size() - segStart >= 3) {
        Segment seg;
        seg.startIndex = segStart;
        seg.endIndex = pens.size() - 1;
        seg.startKLineIndex = pens[segStart].startKLineIndex;
        seg.endKLineIndex = pens.last().endKLineIndex;
        seg.startValue = pens[segStart].startValue;
        seg.endValue = pens.last().endValue;
        seg.startTime = pens[segStart].startTime;
        seg.endTime = pens.last().endTime;
        seg.direction = pens[segStart].isUp() ? SegmentDirection::Up : SegmentDirection::Down;
        seg.isConfirmed = false;
        
        segments.append(seg);
    }
    
    return segments;
}

bool ChanLunAnalyzer::isSegmentBroken(const QVector<Pen>& pens, int segStart, int segEnd, int newPenIndex)
{
    // 简化的线段破坏判断
    // 如果新笔的方向与线段方向相反，且幅度足够大，则认为线段被破坏
    
    const Pen& newPen = pens[newPenIndex];
    
    // 计算线段的主要方向
    double upCount = 0, downCount = 0;
    for (int i = segStart; i <= segEnd; ++i) {
        if (pens[i].isUp()) upCount++;
        else downCount++;
    }
    
    bool segIsUp = upCount > downCount;
    
    // 检查新笔是否破坏线段
    if (segIsUp && newPen.isDown()) {
        // 向上线段被向下笔破坏
        // 检查是否跌破了线段起始点
        if (newPen.endValue < pens[segStart].startValue) {
            return true;
        }
    } else if (!segIsUp && newPen.isUp()) {
        // 向下线段被向上笔破坏
        if (newPen.endValue > pens[segStart].startValue) {
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// 中枢识别
// ============================================================================

QVector<Pivot> ChanLunAnalyzer::identifyPivots(const QVector<Segment>& segments,
                                                const QVector<Pen>& pens)
{
    QVector<Pivot> pivots;
    
    if (segments.size() < 3) {
        return pivots;
    }
    
    // 中枢定义：至少3段连续且有重叠的区间
    for (int i = 0; i < segments.size() - 2; ++i) {
        const Segment& s1 = segments[i];
        const Segment& s2 = segments[i + 1];
        const Segment& s3 = segments[i + 2];
        
        // 计算3段的重叠区间
        double minHigh = std::min({s1.endValue, s2.endValue, s3.endValue});
        double maxLow = std::max({s1.startValue, s2.startValue, s3.startValue});
        
        // 如果有重叠，形成中枢
        if (minHigh > maxLow) {
            Pivot pivot;
            pivot.startIndex = i;
            pivot.endIndex = i + 2;
            pivot.startKLineIndex = s1.startKLineIndex;
            pivot.endKLineIndex = s3.endKLineIndex;
            pivot.zg = maxLow;  // 中枢下沿
            pivot.zd = minHigh; // 中枢上沿
            pivot.gg = std::min({s1.startValue, s2.startValue, s3.startValue}); // 波动下沿
            pivot.dd = std::max({s1.endValue, s2.endValue, s3.endValue});       // 波动上沿
            pivot.startTime = s1.startTime;
            pivot.endTime = s3.endTime;
            pivot.level = 0; // 本级别
            
            pivots.append(pivot);
        }
    }
    
    return pivots;
}

// ============================================================================
// 背驰判断
// ============================================================================

QVector<Divergence> ChanLunAnalyzer::detectDivergence(const QVector<StandardKLine>& klines,
                                                       const QVector<Pen>& pens,
                                                       const QVector<Pivot>& pivots)
{
    QVector<Divergence> divergences;
    
    if (pens.size() < 4) {
        return divergences;
    }
    
    // 背驰判断：比较相邻同向笔的力度
    // 力度用MACD面积或价格幅度来衡量
    
    for (int i = 1; i < pens.size() - 2; ++i) {
        const Pen& p1 = pens[i];
        const Pen& p2 = pens[i + 1];
        const Pen& p3 = pens[i + 2];
        
        // 寻找同向笔进行比较
        if (p1.direction == p3.direction) {
            // 计算两笔的力度（简化：用价格幅度）
            double strength1 = p1.length();
            double strength3 = p3.length();
            
            // 背驰条件：
            // 1. 第二笔创新高/新低
            // 2. 第三笔力度小于第一笔
            
            if (p1.isUp()) {
                // 向上笔背驰
                if (p3.endValue > p1.endValue && strength3 < strength1 * 0.8) {
                    Divergence div;
                    div.index1 = p1.endKLineIndex;
                    div.index2 = p3.endKLineIndex;
                    div.type = DivergenceType::TopDivergence;
                    div.price1 = p1.endValue;
                    div.price2 = p3.endValue;
                    div.strength = 1.0 - strength3 / strength1;
                    
                    divergences.append(div);
                }
            } else {
                // 向下笔背驰
                if (p3.endValue < p1.endValue && strength3 < strength1 * 0.8) {
                    Divergence div;
                    div.index1 = p1.endKLineIndex;
                    div.index2 = p3.endKLineIndex;
                    div.type = DivergenceType::BottomDivergence;
                    div.price1 = p1.endValue;
                    div.price2 = p3.endValue;
                    div.strength = 1.0 - strength3 / strength1;
                    
                    divergences.append(div);
                }
            }
        }
    }
    
    return divergences;
}

// ============================================================================
// 买卖点识别
// ============================================================================

QVector<TradeSignal> ChanLunAnalyzer::identifySignals(const QVector<StandardKLine>& klines,
                                                       const QVector<Pen>& pens,
                                                       const QVector<Pivot>& pivots,
                                                       const QVector<Divergence>& divergences)
{
    QVector<TradeSignal> resultSignals;
    
    // 第一类买卖点：趋势背驰点
    for (const auto& div : divergences) {
        TradeSignal sig;
        sig.index = div.index2;
        sig.price = div.price2;
        sig.time = klines[div.index2].time;
        sig.strength = div.strength;
        
        if (div.type == DivergenceType::BottomDivergence) {
            sig.type = SignalType::Buy1;
            sig.description = QStringLiteral("底背驰一买");
        } else {
            sig.type = SignalType::Sell1;
            sig.description = QStringLiteral("顶背驰一卖");
        }
        
        resultSignals.append(sig);
    }
    
    // 第二类买卖点：第一次次级别回拉不破中枢
    for (int i = 0; i < pivots.size(); ++i) {
        const Pivot& pivot = pivots[i];
        
        // 在中枢之后寻找回拉
        for (int j = 0; j < pens.size(); ++j) {
            const Pen& pen = pens[j];
            
            if (pen.startKLineIndex > pivot.endKLineIndex) {
                // 检查是否回拉不破中枢
                if (pen.isDown() && pen.endValue > pivot.zg) {
                    // 向下回拉不破中枢下沿，二买
                    TradeSignal sig;
                    sig.index = pen.endKLineIndex;
                    sig.price = pen.endValue;
                    sig.time = pen.endTime;
                    sig.type = SignalType::Buy2;
                    sig.pivotIndex = i;
                    sig.description = QStringLiteral("二买：回拉不破中枢");
                    resultSignals.append(sig);
                    break;
                } else if (pen.isUp() && pen.endValue < pivot.zd) {
                    // 向上回拉不破中枢上沿，二卖
                    TradeSignal sig;
                    sig.index = pen.endKLineIndex;
                    sig.price = pen.endValue;
                    sig.time = pen.endTime;
                    sig.type = SignalType::Sell2;
                    sig.pivotIndex = i;
                    sig.description = QStringLiteral("二卖：回拉不破中枢");
                    resultSignals.append(sig);
                    break;
                }
            }
        }
    }
    
    // 第三类买卖点：突破中枢后的回拉确认
    for (int i = 0; i < pivots.size(); ++i) {
        const Pivot& pivot = pivots[i];
        
        for (int j = 0; j < pens.size(); ++j) {
            const Pen& pen = pens[j];
            
            if (pen.startKLineIndex > pivot.endKLineIndex) {
                // 检查是否突破中枢
                if (pen.isUp() && pen.startValue > pivot.zd) {
                    // 向上突破中枢，寻找回拉确认
                    for (int k = j + 1; k < pens.size(); ++k) {
                        const Pen& pullback = pens[k];
                        if (pullback.isDown() && pullback.endValue > pivot.zd) {
                            // 回拉不破中枢上沿，三买
                            TradeSignal sig;
                            sig.index = pullback.endKLineIndex;
                            sig.price = pullback.endValue;
                            sig.time = pullback.endTime;
                            sig.type = SignalType::Buy3;
                            sig.pivotIndex = i;
                            sig.description = QStringLiteral("三买：突破回拉确认");
                            resultSignals.append(sig);
                            break;
                        }
                    }
                    break;
                } else if (pen.isDown() && pen.startValue < pivot.zg) {
                    // 向下突破中枢
                    for (int k = j + 1; k < pens.size(); ++k) {
                        const Pen& pullback = pens[k];
                        if (pullback.isUp() && pullback.endValue < pivot.zg) {
                            // 回拉不破中枢下沿，三卖
                            TradeSignal sig;
                            sig.index = pullback.endKLineIndex;
                            sig.price = pullback.endValue;
                            sig.time = pullback.endTime;
                            sig.type = SignalType::Sell3;
                            sig.pivotIndex = i;
                            sig.description = QStringLiteral("三卖：突破回拉确认");
                            resultSignals.append(sig);
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    
    // 按时间排序
    std::sort(resultSignals.begin(), resultSignals.end(), [](const TradeSignal& a, const TradeSignal& b) {
        return a.index < b.index;
    });
    
    return resultSignals;
}

// ============================================================================
// 辅助方法
// ============================================================================

double ChanLunAnalyzer::calculateMACDArea(const QVector<StandardKLine>& klines, int start, int end)
{
    // 简化的MACD面积计算
    // 实际应该使用EMA计算
    double area = 0.0;
    for (int i = start; i <= end && i < klines.size(); ++i) {
        area += klines[i].close - klines[i].open;
    }
    return area;
}

double ChanLunAnalyzer::calculatePenMACD(const QVector<StandardKLine>& klines, const Pen& pen)
{
    return calculateMACDArea(klines, pen.startKLineIndex, pen.endKLineIndex);
}

bool ChanLunAnalyzer::isPenBroken(const Pen& current, const Pen& previous)
{
    // 简化的笔破坏判断
    if (current.direction == previous.direction) {
        return false;
    }
    
    // 检查是否破坏了前笔的极值
    if (previous.isUp() && current.endValue < previous.startValue) {
        return true;
    }
    if (previous.isDown() && current.endValue > previous.startValue) {
        return true;
    }
    
    return false;
}

// ============================================================================
// IAnalyzer 接口实现
// ============================================================================

Analysis::AnalysisResult ChanLunAnalyzer::analyze(const QVector<Analysis::KLine>& klines)
{
    // 转换K线数据格式
    QVector<RawKLine> rawKlines;
    for (const auto& kline : klines) {
        RawKLine raw;
        raw.time = kline.time;
        raw.open = kline.open;
        raw.high = kline.high;
        raw.low = kline.low;
        raw.close = kline.close;
        raw.volume = kline.volume;
        rawKlines.append(raw);
    }

    // 执行缠论分析
    auto result = analyze(rawKlines);

    // 转换结果
    Analysis::AnalysisResult analysisResult;
    analysisResult.isValid = result.tradeSignals.size() > 0;
    analysisResult.errorMessage = "";

    // 转换信号
    for (const auto& signal : result.tradeSignals) {
        Analysis::UnifiedSignal unifiedSignal;
        unifiedSignal.id = QString::number(signal.index);
        unifiedSignal.source = Analysis::TheoryType::ChanLun;
        unifiedSignal.direction = (signal.type == SignalType::Buy1 || signal.type == SignalType::Buy2 || signal.type == SignalType::Buy3) 
                               ? Analysis::SignalDirection::Bullish :
                           (signal.type == SignalType::Sell1 || signal.type == SignalType::Sell2 || signal.type == SignalType::Sell3) 
                               ? Analysis::SignalDirection::Bearish :
                               Analysis::SignalDirection::Neutral;
        unifiedSignal.strength = Analysis::SignalStrength::Strong;
        unifiedSignal.time = signal.time;
        unifiedSignal.price = signal.price;
        unifiedSignal.confidence = 75.0;
        unifiedSignal.description = signal.description;
        analysisResult.generatedSignals.append(unifiedSignal);
    }

    return analysisResult;
}

QVector<Analysis::UnifiedSignal> ChanLunAnalyzer::currentSignals() const
{
    QVector<Analysis::UnifiedSignal> signalList;
    for (const auto& signal : d->result.tradeSignals) {
        Analysis::UnifiedSignal unifiedSignal;
        unifiedSignal.id = QString::number(signal.index);
        unifiedSignal.source = Analysis::TheoryType::ChanLun;
        unifiedSignal.direction = (signal.type == SignalType::Buy1 || signal.type == SignalType::Buy2 || signal.type == SignalType::Buy3) 
                               ? Analysis::SignalDirection::Bullish :
                           (signal.type == SignalType::Sell1 || signal.type == SignalType::Sell2 || signal.type == SignalType::Sell3) 
                               ? Analysis::SignalDirection::Bearish :
                               Analysis::SignalDirection::Neutral;
        unifiedSignal.strength = Analysis::SignalStrength::Strong;
        unifiedSignal.time = signal.time;
        unifiedSignal.price = signal.price;
        unifiedSignal.confidence = 75.0;
        unifiedSignal.description = signal.description;
        signalList.append(unifiedSignal);
    }
    return signalList;
}

} // namespace ChanLun
} // namespace WealthPilot
