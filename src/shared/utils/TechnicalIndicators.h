/**
 * @file TechnicalIndicators.h
 * @brief 技术指标计算器 - 高性能技术分析
 *
 * @details 功能：
 * - 移动平均线（MA, EMA, SMA）
 * - MACD指标
 * - RSI相对强弱指标
 * - KDJ随机指标
 * - 布林带
 * - 成交量指标
 * - 性能优化：增量计算、缓存
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef TECHNICALINDICATORS_H
#define TECHNICALINDICATORS_H

#include <QVector>
#include <QMap>
#include <QString>
#include <QDateTime>
#include <cmath>

/**
 * @brief 指标结果
 */
struct IndicatorResult {
    QString name;                       // 指标名称
    QMap<QString, QVector<double>> values;  // 指标值（可能有多个值，如MACD有DIF、DEA、MACD）
    QVector<QDateTime> times;          // 时间序列
    bool valid;                        // 是否有效
};

/**
 * @brief 技术指标计算器
 */
class TechnicalIndicators
{
public:
    // ========== 移动平均线 ==========

    /**
     * @brief 简单移动平均（SMA）
     */
    static QVector<double> SMA(const QVector<double>& data, int period);

    /**
     * @brief 指数移动平均（EMA）
     */
    static QVector<double> EMA(const QVector<double>& data, int period);

    /**
     * @brief 加权移动平均（WMA）
     */
    static QVector<double> WMA(const QVector<double>& data, int period);

    /**
     * @brief 成交量加权移动平均（VWMA）
     */
    static QVector<double> VWMA(const QVector<double>& prices, 
                                const QVector<qint64>& volumes, 
                                int period);

    // ========== MACD ==========

    /**
     * @brief MACD指标
     * @return DIF, DEA, MACD柱
     */
    static IndicatorResult MACD(const QVector<double>& data, 
                               int fastPeriod = 12, 
                               int slowPeriod = 26, 
                               int signalPeriod = 9);

    // ========== RSI ==========

    /**
     * @brief RSI相对强弱指标
     */
    static QVector<double> RSI(const QVector<double>& data, int period = 14);

    // ========== KDJ ==========

    /**
     * @brief KDJ随机指标
     * @return K, D, J
     */
    static IndicatorResult KDJ(const QVector<double>& high,
                              const QVector<double>& low,
                              const QVector<double>& close,
                              int n = 9, int m1 = 3, int m2 = 3);

    // ========== 布林带 ==========

    /**
     * @brief 布林带
     * @return 上轨、中轨、下轨
     */
    static IndicatorResult BollingerBands(const QVector<double>& data,
                                         int period = 20,
                                         double stdDev = 2.0);

    // ========== 成交量指标 ==========

    /**
     * @brief 成交量移动平均
     */
    static QVector<double> VolumeMA(const QVector<qint64>& volume, int period = 5);

    /**
     * @brief 能量潮（OBV）
     */
    static QVector<double> OBV(const QVector<double>& close, const QVector<qint64>& volume);

    /**
     * @brief 成交量变异率（VR）
     */
    static QVector<double> VR(const QVector<double>& close, 
                             const QVector<qint64>& volume, 
                             int period = 26);

    // ========== 趋势指标 ==========

    /**
     * @brief 平均趋向指数（ADX）
     */
    static IndicatorResult ADX(const QVector<double>& high,
                              const QVector<double>& low,
                              const QVector<double>& close,
                              int period = 14);

    /**
     * @brief 抛物线转向（SAR）
     */
    static QVector<double> SAR(const QVector<double>& high,
                              const QVector<double>& low,
                              double af = 0.02,
                              double maxAF = 0.2);

    // ========== 动量指标 ==========

    /**
     * @brief 动量指标（MOM）
     */
    static QVector<double> Momentum(const QVector<double>& data, int period = 10);

    /**
     * @brief 变动率（ROC）
     */
    static QVector<double> ROC(const QVector<double>& data, int period = 10);

    /**
     * @brief 威廉指标（Williams %R）
     */
    static QVector<double> WilliamsR(const QVector<double>& high,
                                    const QVector<double>& low,
                                    const QVector<double>& close,
                                    int period = 14);

    // ========== 波动率指标 ==========

    /**
     * @brief 真实波幅（ATR）
     */
    static QVector<double> ATR(const QVector<double>& high,
                              const QVector<double>& low,
                              const QVector<double>& close,
                              int period = 14);

    /**
     * @brief 标准差
     */
    static QVector<double> StandardDeviation(const QVector<double>& data, int period);

    // ========== 支撑阻力 ==========

    /**
     * @brief 支撑阻力位
     */
    static IndicatorResult SupportResistance(const QVector<double>& high,
                                            const QVector<double>& low,
                                            const QVector<double>& close,
                                            int period = 20);

    // ========== 辅助函数 ==========

    /**
     * @brief 计算最高值
     */
    static QVector<double> Highest(const QVector<double>& data, int period);

    /**
     * @brief 计算最低值
     */
    static QVector<double> Lowest(const QVector<double>& data, int period);

    /**
     * @brief 计算差值
     */
    static QVector<double> Diff(const QVector<double>& data, int period = 1);

    /**
     * @brief 计算变化率
     */
    static QVector<double> Change(const QVector<double>& data, int period = 1);

private:
    // 内部辅助函数
    static double mean(const QVector<double>& data, int start, int count);
    static double stddev(const QVector<double>& data, int start, int count, double mean);
};

#endif // TECHNICALINDICATORS_H
