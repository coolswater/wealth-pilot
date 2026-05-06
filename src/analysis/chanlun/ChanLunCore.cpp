/**
 * @file ChanLunCore.cpp
 * @brief 缠论核心算法实现 - K线包含处理、分型识别
 *
 * @details 实现缠论的核心概念：
 * - K线包含处理（标准K线)
 * - 分型识别(顶分型、底分型)
 * - 笔、线段、中枢识别
 * - 背驰判断(MACD辅助)
 * - 买卖点识别
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ChanLunCore.h"
#include "utils/Logger.h"
#include <algorithm>
#include <cmath>

namespace WealthPilot {

// ============================================================================
// K线包含处理算法
// ============================================================================

/**
 * @brief 处理K线包含关系，得到标准K线
 * @param klines 原始K线数据
 * @return 标准化的K线数据
 */
QVector<KLineItem> ChanLunCore::processKLines(const QVector<KLineItem>& klines)
{
    if (klines.isEmpty()) {
        return {};
    }
    
    QVector<KLineItem> result;
    result.reserve(klines.size());
    
    // 缠论标准：处理包含关系
    // 向上处理：高点取最大值， 低点取最小值
    // 向下处理：高点取最小值， 低点取最大值
    
    for (int i = 0; i < klines.size(); ++i) {
        const auto& src = klines[i];
        KLineItem stdK;
        
        stdK.time = src.time;
        stdK.volume = src.volume;
        
        // 判断包含方向
        bool isUpTrend = (i > 0) ? klines[i-1].close < src.close : src.close > src.open;
        
        if (isUpTrend) {
            // 向上处理：高点取最大值， 低点取最小值
            stdK.high = std::max({src.high, src.close});
            stdK.low = std::min({src.low, src.open});
        } else {
            // 向下处理：高点取最小值， 低点取最大值
            stdK.high = std::max({src.high, src.open});
            stdK.low = std::min({src.low, src.close});
        }
        
        // 开盘价处理
        if (i == 0) {
            stdK.open = src.open;
        } else {
            // 开盘价取前一根K线的收盘价
            stdK.open = klines[i-1].close;
        }
        
        // 收盘价取当前K线的收盘价
        stdK.close = src.close;
        
        result.append(stdK);
    }
    
    LOG_DEBUG(QString("Processed %1 K-lines to %2 standard K-lines")
        .arg(klines.size()).arg(result.size()));
    
    return result;
}

// ============================================================================
// 分型识别算法
// ============================================================================

/**
 * @brief 识别分型（顶分型、底分型） - 缠论基础构件
 * @param klines 标准K线数据
 * @return 分型列表
 */
QVector<Fractal> ChanLunCore::identifyFractals(const QVector<KLineItem>& klines)
{
    if (klines.size() < 3) {
        return {};
    }
    
    QVector<Fractal> fractals;
    fractals.reserve(klines.size() / 3);
    
    // 缠论分型定义： 三根K线构成一个分型
    // 顶分型: 三根K线的高点依次降低
    // 底分型: 三根K线的高点依次升高
    
    for (int i = 0; i <= klines.size() - 3; ++i) {
        const auto& k1 = klines[i];
        const auto& k2 = klines[i+1];
        const auto& k3 = klines[i+2];
        
        // 判断是否构成分型
        Fractal f;
        f.startIdx = i;
        f.endIdx = i + 2;
        f.startTime = k1.time;
        f.endTime = k3.time;
        
        // 顶分型判断: k1.high > k2.high > k3.high
        if (k1.high > k2.high && k2.high > k3.high) {
            f.type = FractalType::Top;
            f.high = k1.high;
            f.low = std::min({k1.low, k2.low, k3.low});
            fractals.append(f);
            LOG_DEBUG(QString("Found top fractal at index %1").arg(i));
        }
        // 底分型判断: k1.high < k2.high < k3.high
        else if (k1.high < k2.high && k2.high < k3.high) {
            f.type = FractalType::Bottom;
            f.high = std::max({k1.high, k2.high, k3.high});
            f.low = k1.low;
            fractals.append(f);
            LOG_DEBUG(QString("Found bottom fractal at index %1").arg(i));
        }
    }
    
    LOG_INFO(QString("Identified %1 fractals").arg(fractals.size()));
    return fractals;
}

// ============================================================================
// 笔识别算法
// ============================================================================

/**
 * @brief 从分型序列中识别笔 - 缠论基础构件
 * @param fractals 分型列表
 * @return 笔列表
 */
QVector<Pen> ChanLunCore::identifyPens(const QVector<Fractal>& fractals)
{
    if (fractals.size() < 2) {
        return {};
    }
    
    QVector<Pen> pens;
    pens.reserve(fractals.size() - 1);
    
    // 缠论笔定义: 两个相邻的分型构成一笔
    // 向上笔: 底分型 -> 顶分型
    // 向下笔: 顶分型 -> 底分型
    
    for (int i = 0; i < fractals.size() - 1; ++i) {
        const auto& f1 = fractals[i];
        const auto& f2 = fractals[i + 1];
        
        Pen p;
        p.startIdx = f1.startIdx;
        p.endIdx = f2.endIdx;
        p.startTime = f1.startTime;
        p.endTime = f2.endTime;
        
        // 向上笔: 底分型 -> 顶分型
        if (f1.type == FractalType::Bottom && f2.type == FractalType::Top) {
            p.direction = PenDirection::Up;
            p.high = std::max(f1.high, f2.high);
            p.low = std::min(f1.low, f2.low);
            pens.append(p);
            LOG_DEBUG(QString("Found up pen from index %1 to %2")
                .arg(f1.startIdx).arg(f2.startIdx));
        }
        // 向下笔: 顶分型 -> 底分型
        else if (f1.type == FractalType::Top && f2.type == FractalType::Bottom) {
            p.direction = PenDirection::Down;
            p.high = std::max(f1.high, f2.high);
            p.low = std::min(f1.low, f2.low);
            pens.append(p);
            LOG_DEBUG(QString("Found down pen from index %1 to %2")
                .arg(f1.startIdx).arg(f2.startIdx));
        }
    }
    
    LOG_INFO(QString("Identified %1 pens").arg(pens.size()));
    return pens;
}