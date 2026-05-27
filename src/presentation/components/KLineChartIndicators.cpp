/**
 * @file KLineChartIndicators.cpp
 * @brief K线图技术指标计算实现
 *
 * @details 实现功能：
 * - 主图指标：MA、EMA、BOLL、DMI、ENE
 * - 副图指标：MACD、KDJ、RSI、EXPMA
 * - 指标数据管理和颜色配置
 *
 * @author WealthPilot Team
 * @version 2.1.0
 */

#include "KLineChartImpl.h"
#include "infrastructure/config/Tokens.h"
#include <QtMath>

// ========== 主图指标 ==========

/**
 * @brief 设置主图指标
 */
void KLineChart::setMainIndicator(MainIndicator indicator)
{
    d->currentMainIndicator = indicator;
    
    // 清除现有主图指标
    d->mainIndicatorLines.clear();
    d->mainIndicatorColors.clear();
    
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
        case MainIndicator::DMI:
            calculateDMI(14);
            break;
        case MainIndicator::ENE:
            calculateENE(10);
            break;
        case MainIndicator::None:
        default:
            // 不显示任何主图指标
            break;
    }
    
    update();
}

/**
 * @brief 获取当前主图指标
 */
MainIndicator KLineChart::mainIndicator() const
{
    return d->currentMainIndicator;
}

/**
 * @brief 计算MA移动平均线
 * @param period 周期
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
        case 5: color = QColor(Tokens::Colors::TextPrimary); break;
        case 10: color = QColor(Tokens::Colors::ChartYellow); break;
        case 20: color = QColor(Tokens::Colors::ChartPurple); break;
        default: color = QColor(Tokens::Colors::TextPrimary); break;
    }
    
    // 添加到主图指标
    d->mainIndicatorLines[QString("MA%1").arg(period)] = maValues;
    d->mainIndicatorColors[QString("MA%1").arg(period)] = color;
}

/**
 * @brief 计算EMA指数移动平均
 * @param period 周期
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
    
    QColor color = (period == 12) ? QColor(Tokens::Colors::ChartGreen) : QColor(Tokens::Colors::ChartOrange);
    
    // 添加到主图指标
    d->mainIndicatorLines[QString("EMA%1").arg(period)] = emaValues;
    d->mainIndicatorColors[QString("EMA%1").arg(period)] = color;
}

/**
 * @brief 计算BOLL布林带
 * @param period 周期，默认20
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
    
    // 添加到主图指标
    d->mainIndicatorLines["BOLL_MID"] = midValues;
    d->mainIndicatorColors["BOLL_MID"] = QColor(Tokens::Colors::TextPrimary);
    d->mainIndicatorLines["BOLL_UP"] = upValues;
    d->mainIndicatorColors["BOLL_UP"] = QColor(Tokens::Colors::Danger);
    d->mainIndicatorLines["BOLL_LOW"] = lowValues;
    d->mainIndicatorColors["BOLL_LOW"] = QColor(Tokens::Colors::Success);
}

/**
 * @brief 计算DMI趋向指标（主图指标）
 * @param period 周期，默认14
 */
void KLineChart::calculateDMI(int period)
{
    if (d->data.size() < period + 1) return;
    
    QVector<double> pdmValues, mdmValues, trValues;
    QVector<double> pdiValues, mdiValues, adxValues;
    pdmValues.resize(d->data.size());
    mdmValues.resize(d->data.size());
    trValues.resize(d->data.size());
    pdiValues.resize(d->data.size());
    mdiValues.resize(d->data.size());
    adxValues.resize(d->data.size());
    
    // 计算PDM、MDM、TR
    for (int i = 1; i < d->data.size(); ++i) {
        double upMove = d->data[i].high - d->data[i-1].high;
        double downMove = d->data[i-1].low - d->data[i].low;
        
        double pdm = (upMove > downMove && upMove > 0) ? upMove : 0;
        double mdm = (downMove > upMove && downMove > 0) ? downMove : 0;
        
        double tr = qMax(qMax(d->data[i].high - d->data[i].low,
                             d->data[i].high - d->data[i-1].close),
                        d->data[i-1].close - d->data[i].low);
        
        pdmValues[i] = pdm;
        mdmValues[i] = mdm;
        trValues[i] = tr;
    }
    
    // 计算PDI、MDI（使用EMA平滑）
    double smoothPDM = 0, smoothMDM = 0, smoothTR = 0;
    for (int i = 1; i < d->data.size(); ++i) {
        if (i <= period) {
            smoothPDM += pdmValues[i];
            smoothMDM += mdmValues[i];
            smoothTR += trValues[i];
            pdiValues[i] = 0;
            mdiValues[i] = 0;
        } else {
            smoothPDM = smoothPDM - smoothPDM / period + pdmValues[i];
            smoothMDM = smoothMDM - smoothMDM / period + mdmValues[i];
            smoothTR = smoothTR - smoothTR / period + trValues[i];
            
            pdiValues[i] = (smoothTR > 0) ? (smoothPDM / smoothTR * 100) : 0;
            mdiValues[i] = (smoothTR > 0) ? (smoothMDM / smoothTR * 100) : 0;
        }
    }
    
    // 计算ADX
    double smoothDX = 0;
    for (int i = period + 1; i < d->data.size(); ++i) {
        double diSum = pdiValues[i] + mdiValues[i];
        double dx = (diSum > 0) ? (qAbs(pdiValues[i] - mdiValues[i]) / diSum * 100) : 0;
        
        if (i <= period * 2) {
            smoothDX += dx;
            adxValues[i] = 0;
        } else {
            smoothDX = smoothDX - smoothDX / period + dx;
            adxValues[i] = smoothDX;
        }
    }
    
    // 添加到主图指标（PDI、MDI、ADX）
    d->mainIndicatorLines["PDI"] = pdiValues;
    d->mainIndicatorColors["PDI"] = QColor(Tokens::Colors::Success);
    d->mainIndicatorLines["MDI"] = mdiValues;
    d->mainIndicatorColors["MDI"] = QColor(Tokens::Colors::Danger);
    d->mainIndicatorLines["ADX"] = adxValues;
    d->mainIndicatorColors["ADX"] = QColor(Tokens::Colors::ChartYellow);
}

/**
 * @brief 计算ENE轨道线（主图指标）
 * @param period 周期，默认10
 */
void KLineChart::calculateENE(int period)
{
    if (d->data.size() < period) return;
    
    QVector<double> upperValues, midValues, lowerValues;
    upperValues.resize(d->data.size());
    midValues.resize(d->data.size());
    lowerValues.resize(d->data.size());
    
    // ENE参数
    double n1 = 11.0 / 100.0;  // 上轨系数
    double n2 = 9.0 / 100.0;   // 下轨系数
    
    for (int i = 0; i < d->data.size(); ++i) {
        if (i < period - 1) {
            upperValues[i] = midValues[i] = lowerValues[i] = 0;
        } else {
            // 计算中间价
            double sum = 0;
            for (int j = 0; j < period; ++j) {
                sum += (d->data[i - j].high + d->data[i - j].low) / 2.0;
            }
            double mid = sum / period;
            
            midValues[i] = mid;
            upperValues[i] = mid * (1 + n1);
            lowerValues[i] = mid * (1 - n2);
        }
    }
    
    // 添加到主图指标
    d->mainIndicatorLines["ENE_UPPER"] = upperValues;
    d->mainIndicatorColors["ENE_UPPER"] = QColor(Tokens::Colors::Danger);
    d->mainIndicatorLines["ENE_MID"] = midValues;
    d->mainIndicatorColors["ENE_MID"] = QColor(Tokens::Colors::TextPrimary);
    d->mainIndicatorLines["ENE_LOWER"] = lowerValues;
    d->mainIndicatorColors["ENE_LOWER"] = QColor(Tokens::Colors::Success);
}

// ========== 副图指标 ==========

/**
 * @brief 设置副图指标
 */
void KLineChart::setSubIndicator(SubIndicator indicator)
{
    d->currentSubIndicator = indicator;
    
    // 清除现有副图指标
    d->subIndicatorLines.clear();
    d->subIndicatorColors.clear();
    
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
        case SubIndicator::EXPMA:
            calculateEXPMA(12);
            break;
        case SubIndicator::None:
        default:
            // 不显示任何副图指标
            break;
    }
    
    // 重新计算副图范围
    d->calculateSubChartRange();
    update();
}

/**
 * @brief 获取当前副图指标
 */
SubIndicator KLineChart::subIndicator() const
{
    return d->currentSubIndicator;
}

/**
 * @brief 计算MACD指标
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
    
    // 添加到副图指标
    d->subIndicatorLines["MACD_DIF"] = difValues;
    d->subIndicatorColors["MACD_DIF"] = QColor(Tokens::Colors::TextPrimary);
    d->subIndicatorLines["MACD_DEA"] = deaValues;
    d->subIndicatorColors["MACD_DEA"] = QColor(Tokens::Colors::ChartYellow);
}

/**
 * @brief 计算KDJ指标
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
    
    // 添加到副图指标
    d->subIndicatorLines["KDJ_K"] = kValues;
    d->subIndicatorColors["KDJ_K"] = QColor(Tokens::Colors::TextPrimary);
    d->subIndicatorLines["KDJ_D"] = dValues;
    d->subIndicatorColors["KDJ_D"] = QColor(Tokens::Colors::ChartYellow);
    d->subIndicatorLines["KDJ_J"] = jValues;
    d->subIndicatorColors["KDJ_J"] = QColor(Tokens::Colors::Danger);
}

/**
 * @brief 计算RSI指标
 * @param period 周期，默认14
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
    
    // 添加到副图指标
    d->subIndicatorLines["RSI"] = rsiValues;
    d->subIndicatorColors["RSI"] = QColor(Tokens::Colors::ChartOrange);
}

/**
 * @brief 计算EXPMA指数平均数（副图指标）
 * @param period 周期，默认12
 */
void KLineChart::calculateEXPMA(int period)
{
    if (d->data.isEmpty()) return;
    
    QVector<double> expmaValues;
    expmaValues.resize(d->data.size());
    
    // EXPMA计算公式：EXPMA = (C - EXPMA_prev) * K + EXPMA_prev
    // K = 2 / (N + 1)
    double k = 2.0 / (period + 1);
    expmaValues[0] = d->data[0].close;
    
    for (int i = 1; i < d->data.size(); ++i) {
        expmaValues[i] = (d->data[i].close - expmaValues[i-1]) * k + expmaValues[i-1];
    }
    
    // 添加到副图指标
    d->subIndicatorLines["EXPMA"] = expmaValues;
    d->subIndicatorColors["EXPMA"] = QColor(Tokens::Colors::Success);
}

// ========== 指标管理 ==========

/**
 * @brief 添加技术指标（已弃用，建议使用setMainIndicator/setSubIndicator）
 */
void KLineChart::addIndicator(const QString& name, 
                             const QVector<double>& values,
                             const QColor& color)
{
    // 根据指标名称判断是主图还是副图指标
    if (name.startsWith("MA") || name.startsWith("EMA") || name.startsWith("BOLL")) {
        d->mainIndicatorLines[name] = values;
        d->mainIndicatorColors[name] = color;
    } else {
        d->subIndicatorLines[name] = values;
        d->subIndicatorColors[name] = color;
    }
    update();
}

/**
 * @brief 移除技术指标
 */
void KLineChart::removeIndicator(const QString& name)
{
    d->mainIndicatorLines.remove(name);
    d->mainIndicatorColors.remove(name);
    d->subIndicatorLines.remove(name);
    d->subIndicatorColors.remove(name);
    update();
}

/**
 * @brief 清空所有指标
 */
void KLineChart::clearIndicators()
{
    d->mainIndicatorLines.clear();
    d->mainIndicatorColors.clear();
    d->subIndicatorLines.clear();
    d->subIndicatorColors.clear();
    update();
}