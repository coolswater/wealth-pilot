/**
 * @file TechnicalIndicators.cpp
 * @brief 技术指标计算器实现 - 高性能技术分析
 */

#include "TechnicalIndicators.h"
#include <QDebug>
#include <qmath.h>
#include <algorithm>

// ========== 移动平均线 ==========

QVector<double> TechnicalIndicators::SMA(const QVector<double>& data, int period)
{
    if (data.size() < period || period <= 0) {
        return QVector<double>();
    }
    
    QVector<double> result(data.size(), 0.0);
    
    // 计算第一个SMA
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += data[i];
    }
    result[period - 1] = sum / period;
    
    // 滑动窗口计算后续SMA（性能优化）
    for (int i = period; i < data.size(); ++i) {
        sum = sum - data[i - period] + data[i];
        result[i] = sum / period;
    }
    
    return result;
}

QVector<double> TechnicalIndicators::EMA(const QVector<double>& data, int period)
{
    if (data.isEmpty() || period <= 0) {
        return QVector<double>();
    }
    
    QVector<double> result(data.size(), 0.0);
    
    // EMA系数
    double k = 2.0 / (period + 1);
    
    // 第一个EMA使用SMA
    double sum = 0.0;
    for (int i = 0; i < qMin(period, data.size()); ++i) {
        sum += data[i];
    }
    result[qMin(period - 1, data.size() - 1)] = sum / qMin(period, data.size());
    
    // 计算后续EMA
    for (int i = period; i < data.size(); ++i) {
        result[i] = data[i] * k + result[i - 1] * (1 - k);
    }
    
    return result;
}

QVector<double> TechnicalIndicators::WMA(const QVector<double>& data, int period)
{
    if (data.size() < period || period <= 0) {
        return QVector<double>();
    }
    
    QVector<double> result(data.size(), 0.0);
    
    // 权重总和
    double weightSum = period * (period + 1) / 2.0;
    
    for (int i = period - 1; i < data.size(); ++i) {
        double sum = 0.0;
        for (int j = 0; j < period; ++j) {
            sum += data[i - period + 1 + j] * (j + 1);
        }
        result[i] = sum / weightSum;
    }
    
    return result;
}

QVector<double> TechnicalIndicators::VWMA(const QVector<double>& prices, 
                                         const QVector<qint64>& volumes, 
                                         int period)
{
    if (prices.size() < period || volumes.size() < period || period <= 0) {
        return QVector<double>();
    }
    
    QVector<double> result(prices.size(), 0.0);
    
    for (int i = period - 1; i < prices.size(); ++i) {
        double priceVolumeSum = 0.0;
        qint64 volumeSum = 0;
        
        for (int j = 0; j < period; ++j) {
            priceVolumeSum += prices[i - period + 1 + j] * volumes[i - period + 1 + j];
            volumeSum += volumes[i - period + 1 + j];
        }
        
        result[i] = volumeSum > 0 ? priceVolumeSum / volumeSum : 0.0;
    }
    
    return result;
}

// ========== MACD ==========

IndicatorResult TechnicalIndicators::MACD(const QVector<double>& data, 
                                         int fastPeriod, 
                                         int slowPeriod, 
                                         int signalPeriod)
{
    IndicatorResult result;
    result.name = "MACD";
    result.valid = false;
    
    if (data.size() < slowPeriod + signalPeriod) {
        return result;
    }
    
    // 计算快速EMA和慢速EMA
    QVector<double> fastEMA = EMA(data, fastPeriod);
    QVector<double> slowEMA = EMA(data, slowPeriod);
    
    // 计算DIF
    QVector<double> dif(data.size(), 0.0);
    for (int i = 0; i < data.size(); ++i) {
        dif[i] = fastEMA[i] - slowEMA[i];
    }
    
    // 计算DEA（DIF的EMA）
    QVector<double> dea = EMA(dif, signalPeriod);
    
    // 计算MACD柱
    QVector<double> macd(data.size(), 0.0);
    for (int i = 0; i < data.size(); ++i) {
        macd[i] = (dif[i] - dea[i]) * 2;
    }
    
    result.values["DIF"] = dif;
    result.values["DEA"] = dea;
    result.values["MACD"] = macd;
    result.valid = true;
    
    return result;
}

// ========== RSI ==========

QVector<double> TechnicalIndicators::RSI(const QVector<double>& data, int period)
{
    if (data.size() < period + 1 || period <= 0) {
        return QVector<double>();
    }
    
    QVector<double> result(data.size(), 0.0);
    
    // 计算价格变化
    QVector<double> changes(data.size() - 1);
    for (int i = 1; i < data.size(); ++i) {
        changes[i - 1] = data[i] - data[i - 1];
    }
    
    // 计算上涨和下跌
    QVector<double> gains(changes.size(), 0.0);
    QVector<double> losses(changes.size(), 0.0);
    
    for (int i = 0; i < changes.size(); ++i) {
        if (changes[i] > 0) {
            gains[i] = changes[i];
        } else {
            losses[i] = -changes[i];
        }
    }
    
    // 计算平均上涨和下跌（使用EMA）
    QVector<double> avgGains = EMA(gains, period);
    QVector<double> avgLosses = EMA(losses, period);
    
    // 计算RSI
    for (int i = period; i < changes.size(); ++i) {
        if (avgLosses[i] == 0) {
            result[i + 1] = 100.0;
        } else {
            double rs = avgGains[i] / avgLosses[i];
            result[i + 1] = 100.0 - (100.0 / (1.0 + rs));
        }
    }
    
    return result;
}

// ========== KDJ ==========

IndicatorResult TechnicalIndicators::KDJ(const QVector<double>& high,
                                        const QVector<double>& low,
                                        const QVector<double>& close,
                                        int n, int m1, int m2)
{
    IndicatorResult result;
    result.name = "KDJ";
    result.valid = false;
    
    if (high.size() < n || low.size() < n || close.size() < n) {
        return result;
    }
    
    int size = close.size();
    QVector<double> k(size, 0.0);
    QVector<double> d(size, 0.0);
    QVector<double> j(size, 0.0);
    
    // 计算RSV
    QVector<double> rsv(size, 0.0);
    for (int i = n - 1; i < size; ++i) {
        double highestN = high[i];
        double lowestN = low[i];
        
        for (int j = 0; j < n; ++j) {
            highestN = qMax(highestN, high[i - j]);
            lowestN = qMin(lowestN, low[i - j]);
        }
        
        if (highestN != lowestN) {
            rsv[i] = (close[i] - lowestN) / (highestN - lowestN) * 100.0;
        }
    }
    
    // 计算K值（RSV的EMA）
    k = EMA(rsv, m1);
    
    // 计算D值（K的EMA）
    d = EMA(k, m2);
    
    // 计算J值
    for (int i = 0; i < size; ++i) {
        j[i] = 3 * k[i] - 2 * d[i];
    }
    
    result.values["K"] = k;
    result.values["D"] = d;
    result.values["J"] = j;
    result.valid = true;
    
    return result;
}

// ========== 布林带 ==========

IndicatorResult TechnicalIndicators::BollingerBands(const QVector<double>& data,
                                                   int period,
                                                   double stdDev)
{
    IndicatorResult result;
    result.name = "BollingerBands";
    result.valid = false;
    
    if (data.size() < period) {
        return result;
    }
    
    int size = data.size();
    QVector<double> upper(size, 0.0);
    QVector<double> middle(size, 0.0);
    QVector<double> lower(size, 0.0);
    
    // 计算中轨（SMA）
    middle = SMA(data, period);
    
    // 计算上轨和下轨
    for (int i = period - 1; i < size; ++i) {
        double mean = middle[i];
        double std = stddev(data, i - period + 1, period, mean);
        
        upper[i] = mean + stdDev * std;
        lower[i] = mean - stdDev * std;
    }
    
    result.values["Upper"] = upper;
    result.values["Middle"] = middle;
    result.values["Lower"] = lower;
    result.valid = true;
    
    return result;
}

// ========== 成交量指标 ==========

QVector<double> TechnicalIndicators::VolumeMA(const QVector<qint64>& volume, int period)
{
    if (volume.size() < period || period <= 0) {
        return QVector<double>();
    }
    
    QVector<double> result(volume.size(), 0.0);
    
    double sum = 0.0;
    for (int i = 0; i < period; ++i) {
        sum += volume[i];
    }
    result[period - 1] = sum / period;
    
    for (int i = period; i < volume.size(); ++i) {
        sum = sum - volume[i - period] + volume[i];
        result[i] = sum / period;
    }
    
    return result;
}

QVector<double> TechnicalIndicators::OBV(const QVector<double>& close, const QVector<qint64>& volume)
{
    if (close.size() != volume.size() || close.isEmpty()) {
        return QVector<double>();
    }
    
    QVector<double> result(close.size(), 0.0);
    result[0] = volume[0];
    
    for (int i = 1; i < close.size(); ++i) {
        if (close[i] > close[i - 1]) {
            result[i] = result[i - 1] + volume[i];
        } else if (close[i] < close[i - 1]) {
            result[i] = result[i - 1] - volume[i];
        } else {
            result[i] = result[i - 1];
        }
    }
    
    return result;
}

// ========== 辅助函数 ==========

double TechnicalIndicators::mean(const QVector<double>& data, int start, int count)
{
    if (start < 0 || count <= 0 || start + count > data.size()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (int i = start; i < start + count; ++i) {
        sum += data[i];
    }
    
    return sum / count;
}

double TechnicalIndicators::stddev(const QVector<double>& data, int start, int count, double mean)
{
    if (start < 0 || count <= 0 || start + count > data.size()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (int i = start; i < start + count; ++i) {
        double diff = data[i] - mean;
        sum += diff * diff;
    }
    
    return qSqrt(sum / count);
}

QVector<double> TechnicalIndicators::Highest(const QVector<double>& data, int period)
{
    if (data.size() < period || period <= 0) {
        return QVector<double>();
    }
    
    QVector<double> result(data.size(), 0.0);
    
    for (int i = period - 1; i < data.size(); ++i) {
        double highest = data[i];
        for (int j = 0; j < period; ++j) {
            highest = qMax(highest, data[i - j]);
        }
        result[i] = highest;
    }
    
    return result;
}

QVector<double> TechnicalIndicators::Lowest(const QVector<double>& data, int period)
{
    if (data.size() < period || period <= 0) {
        return QVector<double>();
    }
    
    QVector<double> result(data.size(), 0.0);
    
    for (int i = period - 1; i < data.size(); ++i) {
        double lowest = data[i];
        for (int j = 0; j < period; ++j) {
            lowest = qMin(lowest, data[i - j]);
        }
        result[i] = lowest;
    }
    
    return result;
}

QVector<double> TechnicalIndicators::Diff(const QVector<double>& data, int period)
{
    if (data.size() <= period || period < 0) {
        return QVector<double>();
    }
    
    QVector<double> result(data.size() - period);
    for (int i = period; i < data.size(); ++i) {
        result[i - period] = data[i] - data[i - period];
    }
    
    return result;
}

QVector<double> TechnicalIndicators::Change(const QVector<double>& data, int period)
{
    if (data.size() <= period || period < 0) {
        return QVector<double>();
    }
    
    QVector<double> result(data.size() - period);
    for (int i = period; i < data.size(); ++i) {
        if (data[i - period] != 0) {
            result[i - period] = (data[i] - data[i - period]) / data[i - period] * 100.0;
        } else {
            result[i - period] = 0.0;
        }
    }
    
    return result;
}
