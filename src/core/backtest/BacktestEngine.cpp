/**
 * @file BacktestEngine.cpp
 * @brief 策略回测引擎实现
 */

#include "BacktestEngine.h"
#include "utils/Logger.h"
#include <QtConcurrent>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <cmath>

BacktestEngine* BacktestEngine::instance()
{
    static BacktestEngine* inst = new BacktestEngine();
    return inst;
}

BacktestEngine::BacktestEngine(QObject* parent)
    : QObject(parent)
{
    LOG_INFO("BacktestEngine initialized");
}

void BacktestEngine::setStrategy(std::shared_ptr<IStrategy> strategy)
{
    m_strategy = strategy;
    if (m_strategy) {
        m_strategy->initialize();
        LOG_INFO(QString("Strategy set: %1").arg(strategy->name()));
    }
}

void BacktestEngine::setParameters(double initialCapital, double commissionRate, double slippage)
{
    m_initialCapital = initialCapital;
    m_commissionRate = commissionRate;
    m_slippage = slippage;
    LOG_INFO(QString("Parameters set: capital=%1, commission=%2, slippage=%3")
        .arg(initialCapital).arg(commissionRate).arg(slippage));
}

void BacktestEngine::setData(const QString& symbol, const QVector<QVariantMap>& data)
{
    m_symbol = symbol;
    m_data = data;
    LOG_INFO(QString("Data set: %1 (%2 bars)").arg(symbol).arg(data.size()));
}

void BacktestEngine::run()
{
    if (!m_strategy || m_data.isEmpty()) {
        LOG_ERROR("Strategy or data not set");
        return;
    }

    // 重置状态
    m_trades.clear();
    m_positions.clear();
    m_equityCurve.clear();
    m_currentCash = m_initialCapital;

    int total = m_data.size();
    int current = 0;

    for (const QVariantMap& bar : m_data) {
        current++;
        if (current % 100 == 0) {
            emit progressChanged(current, total);
        }

        // 处理数据，获取信号
        StrategySignal signal = m_strategy->processData(bar);

        // 执行交易
        if (signal.action != "hold") {
            double price = bar["close"].toDouble();
            executeTrade(signal, bar["time"].toDateTime(), price);
        }

        // 更新持仓市值
        updatePositions(bar["close"].toDouble());

        // 记录权益
        recordEquity(bar["time"].toDateTime(), m_currentCash);
    }

    // 计算统计指标
    calculateStats();

    emit backtestCompleted(m_stats);
    LOG_INFO(QString("Backtest completed: %1 trades, return=%2%")
        .arg(m_trades.size()).arg(m_stats.totalReturn * 100, 0, 'f', 2));
}

void BacktestEngine::runAsync()
{
    QtConcurrent::run([this]() {
        run();
    });
}

void BacktestEngine::executeTrade(const StrategySignal& signal, const QDateTime& time, double price)
{
    BacktestTrade trade;
    trade.time = time;
    trade.symbol = signal.symbol.isEmpty() ? m_symbol : signal.symbol;
    trade.direction = signal.action;
    trade.price = price * (1 + (signal.action == "buy" ? m_slippage : -m_slippage));
    trade.quantity = signal.quantity;

    if (signal.action == "buy") {
        // 买入
        trade.amount = trade.price * trade.quantity;
        trade.commission = trade.amount * m_commissionRate;

        if (m_currentCash >= trade.amount + trade.commission) {
            m_currentCash -= (trade.amount + trade.commission);

            // 更新持仓
            BacktestPosition& pos = m_positions[trade.symbol];
            double totalCost = pos.cost * pos.quantity + trade.amount;
            pos.quantity += trade.quantity;
            pos.cost = totalCost / pos.quantity;
            pos.symbol = trade.symbol;

            m_trades.append(trade);
            emit tradeOccurred(trade);

            LOG_DEBUG(QString("BUY %1 %2@%3, commission=%4")
                .arg(trade.symbol).arg(trade.quantity).arg(trade.price).arg(trade.commission));
        }
    } else if (signal.action == "sell") {
        // 卖出
        BacktestPosition& pos = m_positions[trade.symbol];
        if (pos.quantity >= trade.quantity) {
            trade.amount = trade.price * trade.quantity;
            trade.commission = trade.amount * m_commissionRate;

            // 计算盈亏
            trade.profit = (trade.price - pos.cost) * trade.quantity - trade.commission;

            m_currentCash += (trade.amount - trade.commission);

            pos.quantity -= trade.quantity;
            if (pos.quantity == 0) {
                m_positions.remove(trade.symbol);
            }

            m_trades.append(trade);
            emit tradeOccurred(trade);

            LOG_DEBUG(QString("SELL %1 %2@%3, profit=%4")
                .arg(trade.symbol).arg(trade.quantity).arg(trade.price).arg(trade.profit));
        }
    }
}

void BacktestEngine::updatePositions(double currentPrice)
{
    for (auto it = m_positions.begin(); it != m_positions.end(); ++it) {
        BacktestPosition& pos = it.value();
        pos.marketValue = currentPrice * pos.quantity;
        pos.profit = (currentPrice - pos.cost) * pos.quantity;
    }
}

void BacktestEngine::calculateStats()
{
    if (m_trades.isEmpty()) {
        return;
    }

    m_stats.totalTrades = m_trades.size();

    double totalProfit = 0.0;
    double totalLoss = 0.0;
    double maxEquity = m_initialCapital;
    double maxDrawdown = 0.0;
    double peakEquity = m_initialCapital;

    QVector<double> returns;

    for (const BacktestTrade& trade : m_trades) {
        if (trade.profit > 0) {
            m_stats.winTrades++;
            totalProfit += trade.profit;
            m_stats.maxProfit = qMax(m_stats.maxProfit, trade.profit);
        } else {
            m_stats.lossTrades++;
            totalLoss += qAbs(trade.profit);
            m_stats.maxLoss = qMin(m_stats.maxLoss, trade.profit);
        }
    }

    // 计算收益率
    double finalEquity = m_currentCash;
    for (const auto& pos : m_positions) {
        finalEquity += pos.marketValue;
    }

    m_stats.totalReturn = (finalEquity - m_initialCapital) / m_initialCapital;

    // 年化收益率（假设回测期为1年）
    if (!m_equityCurve.isEmpty()) {
        int days = m_equityCurve.first().first.daysTo(m_equityCurve.last().first);
        if (days > 0) {
            double years = days / 365.0;
            m_stats.annualizedReturn = pow(1 + m_stats.totalReturn, 1.0 / years) - 1;
        }
    }

    // 最大回撤
    for (const auto& point : m_equityCurve) {
        peakEquity = qMax(peakEquity, point.second);
        double drawdown = (peakEquity - point.second) / peakEquity;
        maxDrawdown = qMax(maxDrawdown, drawdown);
    }
    m_stats.maxDrawdown = maxDrawdown;

    // 胜率
    m_stats.winRate = static_cast<double>(m_stats.winTrades) / m_stats.totalTrades;

    // 盈亏比
    m_stats.profitFactor = totalLoss > 0 ? totalProfit / totalLoss : totalProfit;

    // 平均盈亏
    m_stats.avgProfit = m_stats.winTrades > 0 ? totalProfit / m_stats.winTrades : 0;
    m_stats.avgLoss = m_stats.lossTrades > 0 ? totalLoss / m_stats.lossTrades : 0;

    // 夏普比率（简化计算）
    if (!returns.isEmpty()) {
        double avgReturn = 0;
        for (double r : returns) avgReturn += r;
        avgReturn /= returns.size();

        double stdDev = 0;
        for (double r : returns) {
            stdDev += pow(r - avgReturn, 2);
        }
        stdDev = sqrt(stdDev / returns.size());

        m_stats.sharpeRatio = stdDev > 0 ? avgReturn / stdDev * sqrt(252) : 0;
    }
}

void BacktestEngine::recordEquity(const QDateTime& time, double equity)
{
    double totalEquity = equity;
    for (const auto& pos : m_positions) {
        totalEquity += pos.marketValue;
    }
    m_equityCurve.append({time, totalEquity});
}

QString BacktestEngine::generateReport() const
{
    QString report;
    report += QStringLiteral("========== 回测报告 ==========\n\n");

    if (m_strategy) {
        report += QString("策略: %1\n").arg(m_strategy->name());
        report += QString("描述: %1\n\n").arg(m_strategy->description());
    }

    report += QString("标的: %1\n").arg(m_symbol);
    report += QString("初始资金: %1\n").arg(m_initialCapital, 0, 'f', 2);
    report += QString("手续费率: %1%\n").arg(m_commissionRate * 100);
    report += QString("滑点: %1%\n\n").arg(m_slippage * 100);

    report += QStringLiteral("========== 绩效指标 ==========\n");
    report += QString("总收益率: %1%\n").arg(m_stats.totalReturn * 100, 0, 'f', 2);
    report += QString("年化收益率: %1%\n").arg(m_stats.annualizedReturn * 100, 0, 'f', 2);
    report += QString("最大回撤: %1%\n").arg(m_stats.maxDrawdown * 100, 0, 'f', 2);
    report += QString("夏普比率: %1\n").arg(m_stats.sharpeRatio, 0, 'f', 2);
    report += QString("胜率: %1%\n").arg(m_stats.winRate * 100, 0, 'f', 2);
    report += QString("盈亏比: %1\n\n").arg(m_stats.profitFactor, 0, 'f', 2);

    report += QStringLiteral("========== 交易统计 ==========\n");
    report += QString("总交易次数: %1\n").arg(m_stats.totalTrades);
    report += QString("盈利次数: %1\n").arg(m_stats.winTrades);
    report += QString("亏损次数: %1\n").arg(m_stats.lossTrades);
    report += QString("平均盈利: %1\n").arg(m_stats.avgProfit, 0, 'f', 2);
    report += QString("平均亏损: %1\n").arg(m_stats.avgLoss, 0, 'f', 2);
    report += QString("最大盈利: %1\n").arg(m_stats.maxProfit, 0, 'f', 2);
    report += QString("最大亏损: %1\n").arg(m_stats.maxLoss, 0, 'f', 2);

    return report;
}

bool BacktestEngine::exportTrades(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    out << "Time,Symbol,Direction,Price,Quantity,Amount,Commission,Profit\n";

    for (const BacktestTrade& trade : m_trades) {
        out << QString("%1,%2,%3,%4,%5,%6,%7,%8\n")
            .arg(trade.time.toString("yyyy-MM-dd hh:mm:ss"))
            .arg(trade.symbol)
            .arg(trade.direction)
            .arg(trade.price, 0, 'f', 2)
            .arg(trade.quantity)
            .arg(trade.amount, 0, 'f', 2)
            .arg(trade.commission, 0, 'f', 2)
            .arg(trade.profit, 0, 'f', 2);
    }

    file.close();
    LOG_INFO(QString("Trades exported: %1").arg(filePath));
    return true;
}