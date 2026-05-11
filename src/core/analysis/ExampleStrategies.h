/**
 * @file ExampleStrategies.h
 * @brief 示例策略集合
 *
 * @details 包含多种经典策略：
 * - 双均线策略
 * - RSI策略
 * - MACD策略
 * - 布林带策略
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef EXAMPLESTRATEGIES_H
#define EXAMPLESTRATEGIES_H

#include "../backtest/BacktestEngine.h"
#include <QVector>

/**
 * @brief 双均线策略
 *
 * 策略逻辑：
 * - 短期均线上穿长期均线时买入
 * - 短期均线下穿长期均线时卖出
 */
class DoubleMAStrategy : public IStrategy {
public:
    explicit DoubleMAStrategy(int shortPeriod = 5, int longPeriod = 20)
        : m_shortPeriod(shortPeriod), m_longPeriod(longPeriod) {}

    void initialize() override {
        m_shortMA.clear();
        m_longMA.clear();
        m_prices.clear();
    }

    StrategySignal processData(const QVariantMap& data) override {
        double close = data["close"].toDouble();
        m_prices.append(close);

        // 计算短期均线
        if (m_prices.size() >= m_shortPeriod) {
            double sum = 0;
            for (int i = m_prices.size() - m_shortPeriod; i < m_prices.size(); ++i) {
                sum += m_prices[i];
            }
            m_shortMA.append(sum / m_shortPeriod);
        }

        // 计算长期均线
        if (m_prices.size() >= m_longPeriod) {
            double sum = 0;
            for (int i = m_prices.size() - m_longPeriod; i < m_prices.size(); ++i) {
                sum += m_prices[i];
            }
            m_longMA.append(sum / m_longPeriod);
        }

        StrategySignal signal;
        signal.time = data["time"].toDateTime();
        signal.symbol = data["symbol"].toString();
        signal.price = close;
        signal.action = "hold";

        // 生成信号
        if (m_shortMA.size() >= 2 && m_longMA.size() >= 2) {
            double shortCurr = m_shortMA.last();
            double shortPrev = m_shortMA[m_shortMA.size() - 2];
            double longCurr = m_longMA.last();
            double longPrev = m_longMA[m_longMA.size() - 2];

            // 金叉买入
            if (shortPrev <= longPrev && shortCurr > longCurr) {
                signal.action = "buy";
                signal.quantity = 100;
                signal.reason = QString("金叉: 短期MA%1上穿长期MA%2")
                    .arg(m_shortPeriod).arg(m_longPeriod);
            }
            // 死叉卖出
            else if (shortPrev >= longPrev && shortCurr < longCurr) {
                signal.action = "sell";
                signal.quantity = 100;
                signal.reason = QString("死叉: 短期MA%1下穿长期MA%2")
                    .arg(m_shortPeriod).arg(m_longPeriod);
            }
        }

        return signal;
    }

    QString name() const override {
        return QString("双均线策略(MA%1/MA%2)").arg(m_shortPeriod).arg(m_longPeriod);
    }

    QString description() const override {
        return "短期均线上穿长期均线买入，下穿卖出";
    }

private:
    int m_shortPeriod;
    int m_longPeriod;
    QVector<double> m_prices;
    QVector<double> m_shortMA;
    QVector<double> m_longMA;
};

/**
 * @brief RSI策略
 *
 * 策略逻辑：
 * - RSI < 30 超卖买入
 * - RSI > 70 超买卖出
 */
class RSIStrategy : public IStrategy {
public:
    explicit RSIStrategy(int period = 14, double oversold = 30, double overbought = 70)
        : m_period(period), m_oversold(oversold), m_overbought(overbought) {}

    void initialize() override {
        m_prices.clear();
        m_rsi = 50.0;
    }

    StrategySignal processData(const QVariantMap& data) override {
        double close = data["close"].toDouble();
        m_prices.append(close);

        StrategySignal signal;
        signal.time = data["time"].toDateTime();
        signal.symbol = data["symbol"].toString();
        signal.price = close;
        signal.action = "hold";

        // 计算RSI
        if (m_prices.size() >= m_period + 1) {
            double gain = 0, loss = 0;
            for (int i = m_prices.size() - m_period; i < m_prices.size(); ++i) {
                double change = m_prices[i] - m_prices[i - 1];
                if (change > 0) gain += change;
                else loss -= change;
            }

            double avgGain = gain / m_period;
            double avgLoss = loss / m_period;

            if (avgLoss > 0) {
                double rs = avgGain / avgLoss;
                m_rsi = 100 - 100 / (1 + rs);
            } else {
                m_rsi = 100;
            }

            // 生成信号
            if (m_rsi < m_oversold) {
                signal.action = "buy";
                signal.quantity = 100;
                signal.reason = QString("RSI超卖: %1 < %2").arg(m_rsi, 0, 'f', 1).arg(m_oversold);
            } else if (m_rsi > m_overbought) {
                signal.action = "sell";
                signal.quantity = 100;
                signal.reason = QString("RSI超买: %1 > %2").arg(m_rsi, 0, 'f', 1).arg(m_overbought);
            }
        }

        return signal;
    }

    QString name() const override {
        return QString("RSI策略(%1)").arg(m_period);
    }

    QString description() const override {
        return QString("RSI低于%1买入，高于%2卖出").arg(m_oversold).arg(m_overbought);
    }

private:
    int m_period;
    double m_oversold;
    double m_overbought;
    QVector<double> m_prices;
    double m_rsi;
};

/**
 * @brief MACD策略
 *
 * 策略逻辑：
 * - MACD金叉买入
 * - MACD死叉卖出
 */
class MACDStrategy : public IStrategy {
public:
    explicit MACDStrategy(int fast = 12, int slow = 26, int signal = 9)
        : m_fastPeriod(fast), m_slowPeriod(slow), m_signalPeriod(signal) {}

    void initialize() override {
        m_prices.clear();
        m_ema12 = 0;
        m_ema26 = 0;
        m_macd = 0;
        m_signal = 0;
        m_firstCalc = true;
    }

    StrategySignal processData(const QVariantMap& data) override {
        double close = data["close"].toDouble();
        m_prices.append(close);

        StrategySignal signal;
        signal.time = data["time"].toDateTime();
        signal.symbol = data["symbol"].toString();
        signal.price = close;
        signal.action = "hold";

        // 计算EMA
        if (m_firstCalc && m_prices.size() >= m_slowPeriod) {
            // 初始EMA
            double sum12 = 0, sum26 = 0;
            for (int i = 0; i < m_fastPeriod; ++i) sum12 += m_prices[i];
            for (int i = 0; i < m_slowPeriod; ++i) sum26 += m_prices[i];
            m_ema12 = sum12 / m_fastPeriod;
            m_ema26 = sum26 / m_slowPeriod;
            m_firstCalc = false;
        } else if (!m_firstCalc) {
            // 更新EMA
            double k12 = 2.0 / (m_fastPeriod + 1);
            double k26 = 2.0 / (m_slowPeriod + 1);
            m_ema12 = close * k12 + m_ema12 * (1 - k12);
            m_ema26 = close * k26 + m_ema26 * (1 - k26);
        }

        // 计算MACD
        double macd = m_ema12 - m_ema26;
        double ks = 2.0 / (m_signalPeriod + 1);
        m_signal = macd * ks + m_signal * (1 - ks);
        double prevMacd = m_macd;
        m_macd = macd;

        // 生成信号
        if (m_prices.size() > m_slowPeriod + m_signalPeriod) {
            // 金叉
            if (prevMacd <= m_signal && m_macd > m_signal) {
                signal.action = "buy";
                signal.quantity = 100;
                signal.reason = "MACD金叉";
            }
            // 死叉
            else if (prevMacd >= m_signal && m_macd < m_signal) {
                signal.action = "sell";
                signal.quantity = 100;
                signal.reason = "MACD死叉";
            }
        }

        return signal;
    }

    QString name() const override {
        return QString("MACD策略(%1,%2,%3)").arg(m_fastPeriod).arg(m_slowPeriod).arg(m_signalPeriod);
    }

    QString description() const override {
        return "MACD金叉买入，死叉卖出";
    }

private:
    int m_fastPeriod, m_slowPeriod, m_signalPeriod;
    QVector<double> m_prices;
    double m_ema12, m_ema26, m_macd, m_signal;
    bool m_firstCalc = true;
};

/**
 * @brief 布林带策略
 *
 * 策略逻辑：
 * - 价格触及下轨买入
 * - 价格触及上轨卖出
 */
class BollingerBandsStrategy : public IStrategy {
public:
    explicit BollingerBandsStrategy(int period = 20, double stdDev = 2.0)
        : m_period(period), m_stdDev(stdDev) {}

    void initialize() override {
        m_prices.clear();
    }

    StrategySignal processData(const QVariantMap& data) override {
        double close = data["close"].toDouble();
        m_prices.append(close);

        StrategySignal signal;
        signal.time = data["time"].toDateTime();
        signal.symbol = data["symbol"].toString();
        signal.price = close;
        signal.action = "hold";

        if (m_prices.size() >= m_period) {
            // 计算中轨（MA）
            double sum = 0;
            for (int i = m_prices.size() - m_period; i < m_prices.size(); ++i) {
                sum += m_prices[i];
            }
            double mid = sum / m_period;

            // 计算标准差
            double variance = 0;
            for (int i = m_prices.size() - m_period; i < m_prices.size(); ++i) {
                variance += pow(m_prices[i] - mid, 2);
            }
            double std = sqrt(variance / m_period);

            double upper = mid + m_stdDev * std;
            double lower = mid - m_stdDev * std;

            // 生成信号
            if (close <= lower) {
                signal.action = "buy";
                signal.quantity = 100;
                signal.reason = QString("触及下轨: %1 <= %2").arg(close, 0, 'f', 2).arg(lower, 0, 'f', 2);
            } else if (close >= upper) {
                signal.action = "sell";
                signal.quantity = 100;
                signal.reason = QString("触及上轨: %1 >= %2").arg(close, 0, 'f', 2).arg(upper, 0, 'f', 2);
            }
        }

        return signal;
    }

    QString name() const override {
        return QString("布林带策略(%1,%2)").arg(m_period).arg(m_stdDev);
    }

    QString description() const override {
        return "价格触及下轨买入，触及上轨卖出";
    }

private:
    int m_period;
    double m_stdDev;
    QVector<double> m_prices;
};

#endif // EXAMPLESTRATEGIES_H