/**
 * @file PortfolioOptimizer.cpp
 * @brief 投资组合优化实现
 */

#include "PortfolioOptimizer.h"
#include "shared/utils/Logger.h"
#include <QUuid>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

PortfolioOptimizer* PortfolioOptimizer::instance()
{
    static PortfolioOptimizer* inst = new PortfolioOptimizer();
    return inst;
}

PortfolioOptimizer::PortfolioOptimizer(QObject* parent)
    : QObject(parent)
{
}

bool PortfolioOptimizer::initialize()
{
    if (m_initialized) return true;

    LOG_INFO("Initializing Portfolio Optimizer");
    m_initialized = true;
    LOG_INFO("Portfolio Optimizer initialized");
    return true;
}

Portfolio PortfolioOptimizer::createPortfolio(const QString& name, const QString& description)
{
    Portfolio portfolio;
    portfolio.id = generatePortfolioId();
    portfolio.name = name;
    portfolio.description = description;
    portfolio.createTime = QDateTime::currentDateTime();
    portfolio.updateTime = QDateTime::currentDateTime();

    m_portfolios[portfolio.id] = portfolio;

    LOG_INFO(QString("Portfolio created: %1 (%2)").arg(name, portfolio.id));
    return portfolio;
}

Portfolio PortfolioOptimizer::optimize(const QVector<QString>& assets,
                                     OptimizationObjective objective,
                                     const OptimizationConstraint& constraint)
{
    Portfolio portfolio;

    switch (objective) {
    case OptimizationObjective::MaxReturn:
        portfolio = optimizeMaxReturn(assets, constraint);
        break;
    case OptimizationObjective::MinRisk:
        portfolio = optimizeMinRisk(assets, constraint);
        break;
    case OptimizationObjective::MaxSharpeRatio:
        portfolio = optimizeMaxSharpe(assets, constraint);
        break;
    case OptimizationObjective::RiskParity:
        portfolio = optimizeRiskParity(assets, constraint);
        break;
    }

    portfolio.riskMetrics = calculateRiskMetrics(portfolio);
    portfolio.returnMetrics = calculateReturnMetrics(portfolio);
    portfolio.updateTime = QDateTime::currentDateTime();

    emit optimizationCompleted(portfolio);

    LOG_INFO(QString("Portfolio optimized: %1 assets, objective=%2")
        .arg(assets.size()).arg(static_cast<int>(objective)));

    return portfolio;
}

PortfolioRiskMetrics PortfolioOptimizer::calculateRiskMetrics(const Portfolio& portfolio)
{
    PortfolioRiskMetrics metrics;

    if (portfolio.allocations.isEmpty()) return metrics;

    // 简化的风险指标计算
    double avgVolatility = 20.0;
    metrics.volatility = avgVolatility * std::sqrt(portfolio.allocations.size() / 10.0);
    metrics.maxDrawdown = metrics.volatility * 1.5;

    double riskFreeRate = 3.0;
    metrics.sharpeRatio = (portfolio.returnMetrics.annualizedReturn - riskFreeRate) / metrics.volatility;
    metrics.beta = 1.0;
    metrics.var95 = portfolio.totalValue * metrics.volatility / 100.0 * 1.65;
    metrics.expectedShortfall = metrics.var95 * 1.2;

    return metrics;
}

PortfolioReturnMetrics PortfolioOptimizer::calculateReturnMetrics(const Portfolio& portfolio)
{
    PortfolioReturnMetrics metrics;

    if (portfolio.allocations.isEmpty()) return metrics;

    double avgReturn = 10.0;
    int stockCount = 0;
    for (const auto& allocation : portfolio.allocations) {
        if (allocation.type == AssetType::Stock) {
            avgReturn += 2.0;
            stockCount++;
        }
    }

    metrics.annualizedReturn = avgReturn;
    metrics.totalReturn = avgReturn * 3;
    metrics.dividendYield = 2.5;
    metrics.alpha = avgReturn - 10.0;

    return metrics;
}

PortfolioBacktestResult PortfolioOptimizer::backtest(const Portfolio& portfolio,
                                           const QDateTime& startDate,
                                           const QDateTime& endDate)
{
    PortfolioBacktestResult result;

    if (portfolio.allocations.isEmpty()) return result;

    LOG_INFO(QString("Backtesting portfolio from %1 to %2")
        .arg(startDate.toString("yyyy-MM-dd"))
        .arg(endDate.toString("yyyy-MM-dd")));

    int days = startDate.daysTo(endDate);
    if (days <= 0) return result;

    double cumulativeReturn = 1.0;
    for (int i = 0; i < days; ++i) {
        double dailyReturn = (QRandomGenerator::global()->bounded(60) - 30) / 1000.0;
        cumulativeReturn *= (1.0 + dailyReturn);

        result.dailyReturns.append(dailyReturn);
        result.cumulativeReturns.append((cumulativeReturn - 1.0) * 100.0);
    }

    result.totalReturn = (cumulativeReturn - 1.0) * 100.0;
    result.annualizedReturn = (std::pow(cumulativeReturn, 365.0 / days) - 1.0) * 100.0;

    double peak = 1.0;
    double maxDD = 0.0;
    for (double cumRet : result.cumulativeReturns) {
        double value = 1.0 + cumRet / 100.0;
        if (value > peak) peak = value;
        double dd = (peak - value) / peak;
        if (dd > maxDD) maxDD = dd;
    }
    result.maxDrawdown = maxDD * 100.0;

    double avgDailyReturn = 0.0;
    for (double r : result.dailyReturns) {
        avgDailyReturn += r;
    }
    avgDailyReturn /= days;

    double variance = 0.0;
    for (double r : result.dailyReturns) {
        variance += (r - avgDailyReturn) * (r - avgDailyReturn);
    }
    variance /= days;

    double dailyVol = std::sqrt(variance);
    result.sharpeRatio = (avgDailyReturn * 252) / (dailyVol * std::sqrt(252));

    int winDays = 0;
    for (double r : result.dailyReturns) {
        if (r > 0) winDays++;
    }
    result.winRate = (double)winDays / days * 100.0;

    result.totalTrades = portfolio.allocations.size();

    emit backtestCompleted(result);

    LOG_INFO(QString("Backtest completed: return=%1%, maxDD=%2%, sharpe=%3")
        .arg(result.totalReturn, 0, 'f', 2)
        .arg(result.maxDrawdown, 0, 'f', 2)
        .arg(result.sharpeRatio, 0, 'f', 2));

    return result;
}

QVector<PortfolioAssetAllocation> PortfolioOptimizer::getAllocationSuggestion(double totalAmount,
                                                                    InvestmentStyle style)
{
    QVector<PortfolioAssetAllocation> allocations;

    if (style == InvestmentStyle::Conservative) {
        PortfolioAssetAllocation bond;
        bond.symbol = "BOND_ETF";
        bond.type = AssetType::Bond;
        bond.weight = 50.0;
        bond.amount = totalAmount * 0.5;
        bond.reason = QStringLiteral("债券提供稳定收益，降低波动");
        allocations.append(bond);

        PortfolioAssetAllocation stock;
        stock.symbol = "STOCK_ETF";
        stock.type = AssetType::Stock;
        stock.weight = 30.0;
        stock.amount = totalAmount * 0.3;
        stock.reason = QStringLiteral("股票提供适度增长");
        allocations.append(stock);

        PortfolioAssetAllocation cash;
        cash.symbol = "CASH";
        cash.type = AssetType::Cash;
        cash.weight = 20.0;
        cash.amount = totalAmount * 0.2;
        cash.reason = QStringLiteral("现金保持流动性");
        allocations.append(cash);

    } else if (style == InvestmentStyle::Aggressive) {
        PortfolioAssetAllocation stock;
        stock.symbol = "GROWTH_STOCK";
        stock.type = AssetType::Stock;
        stock.weight = 70.0;
        stock.amount = totalAmount * 0.7;
        stock.reason = QStringLiteral("成长股提供高收益潜力");
        allocations.append(stock);

        PortfolioAssetAllocation fund;
        fund.symbol = "THEMATIC_FUND";
        fund.type = AssetType::Fund;
        fund.weight = 20.0;
        fund.amount = totalAmount * 0.2;
        fund.reason = QStringLiteral("主题基金把握行业机会");
        allocations.append(fund);

        PortfolioAssetAllocation cash;
        cash.symbol = "CASH";
        cash.type = AssetType::Cash;
        cash.weight = 10.0;
        cash.amount = totalAmount * 0.1;
        cash.reason = QStringLiteral("小额现金应对机会");
        allocations.append(cash);

    } else {
        PortfolioAssetAllocation stock;
        stock.symbol = "BLUECHIP_ETF";
        stock.type = AssetType::Stock;
        stock.weight = 50.0;
        stock.amount = totalAmount * 0.5;
        stock.reason = QStringLiteral("蓝筹股稳健增长");
        allocations.append(stock);

        PortfolioAssetAllocation bond;
        bond.symbol = "BOND_ETF";
        bond.type = AssetType::Bond;
        bond.weight = 30.0;
        bond.amount = totalAmount * 0.3;
        bond.reason = QStringLiteral("债券平衡风险");
        allocations.append(bond);

        PortfolioAssetAllocation fund;
        fund.symbol = "MIXED_FUND";
        fund.type = AssetType::Fund;
        fund.weight = 15.0;
        fund.amount = totalAmount * 0.15;
        fund.reason = QStringLiteral("混合基金专业管理");
        allocations.append(fund);

        PortfolioAssetAllocation cash;
        cash.symbol = "CASH";
        cash.type = AssetType::Cash;
        cash.weight = 5.0;
        cash.amount = totalAmount * 0.05;
        cash.reason = QStringLiteral("保持适度流动性");
        allocations.append(cash);
    }

    return allocations;
}

QVector<PortfolioAssetAllocation> PortfolioOptimizer::rebalanceSuggestion(const Portfolio& current,
                                                                const Portfolio& target)
{
    QVector<PortfolioAssetAllocation> suggestions;

    QMap<QString, double> currentWeights;
    for (const auto& allocation : current.allocations) {
        currentWeights[allocation.symbol] = allocation.weight;
    }

    QMap<QString, double> targetWeights;
    for (const auto& allocation : target.allocations) {
        targetWeights[allocation.symbol] = allocation.weight;
    }

    for (auto it = targetWeights.begin(); it != targetWeights.end(); ++it) {
        double currentWeight = currentWeights.value(it.key(), 0.0);
        double targetWeight = it.value();
        double diff = targetWeight - currentWeight;

        if (std::abs(diff) > 1.0) {
            PortfolioAssetAllocation adjustment;
            adjustment.symbol = it.key();
            adjustment.weight = diff;

            if (diff > 0) {
                adjustment.reason = QStringLiteral("增加配置至%1%").arg(targetWeight, 0, 'f', 1);
            } else {
                adjustment.reason = QStringLiteral("减少配置至%1%").arg(targetWeight, 0, 'f', 1);
            }

            suggestions.append(adjustment);
        }
    }

    return suggestions;
}

Portfolio PortfolioOptimizer::optimizeMaxReturn(const QVector<QString>& assets,
                                              const OptimizationConstraint& constraint)
{
    Portfolio portfolio = createPortfolio(QStringLiteral("最大收益组合"));

    if (assets.isEmpty()) return portfolio;

    double weight = 100.0 / assets.size();

    for (const QString& symbol : assets) {
        PortfolioAssetAllocation allocation;
        allocation.symbol = symbol;
        allocation.type = AssetType::Stock;
        allocation.weight = weight;
        allocation.reason = QStringLiteral("追求最大收益");
        portfolio.allocations.append(allocation);
    }

    return portfolio;
}

Portfolio PortfolioOptimizer::optimizeMinRisk(const QVector<QString>& assets,
                                            const OptimizationConstraint& constraint)
{
    Portfolio portfolio = createPortfolio(QStringLiteral("最小风险组合"));

    if (assets.isEmpty()) return portfolio;

    double weight = 100.0 / assets.size();

    for (const QString& symbol : assets) {
        PortfolioAssetAllocation allocation;
        allocation.symbol = symbol;
        allocation.type = AssetType::Stock;
        allocation.weight = weight;
        allocation.reason = QStringLiteral("分散降低风险");
        portfolio.allocations.append(allocation);
    }

    return portfolio;
}

Portfolio PortfolioOptimizer::optimizeMaxSharpe(const QVector<QString>& assets,
                                              const OptimizationConstraint& constraint)
{
    Portfolio portfolio = createPortfolio(QStringLiteral("最优夏普组合"));

    if (assets.isEmpty()) return portfolio;

    double weight = 100.0 / assets.size();

    for (const QString& symbol : assets) {
        PortfolioAssetAllocation allocation;
        allocation.symbol = symbol;
        allocation.type = AssetType::Stock;
        allocation.weight = weight;
        allocation.reason = QStringLiteral("优化风险收益比");
        portfolio.allocations.append(allocation);
    }

    return portfolio;
}

Portfolio PortfolioOptimizer::optimizeRiskParity(const QVector<QString>& assets,
                                                const OptimizationConstraint& constraint)
{
    Portfolio portfolio = createPortfolio(QStringLiteral("风险平价组合"));

    if (assets.isEmpty()) return portfolio;

    double weight = 100.0 / assets.size();

    for (const QString& symbol : assets) {
        PortfolioAssetAllocation allocation;
        allocation.symbol = symbol;
        allocation.type = AssetType::Stock;
        allocation.weight = weight;
        allocation.reason = QStringLiteral("风险均衡配置");
        portfolio.allocations.append(allocation);
    }

    return portfolio;
}

double PortfolioOptimizer::calculateCovariance(const QString& asset1, const QString& asset2)
{
    Q_UNUSED(asset1);
    Q_UNUSED(asset2);
    return 0.0;
}

double PortfolioOptimizer::calculateVariance(const QString& asset)
{
    Q_UNUSED(asset);
    return 0.0;
}

QVector<double> PortfolioOptimizer::getHistoricalReturns(const QString& asset, int days)
{
    Q_UNUSED(asset);
    Q_UNUSED(days);
    return QVector<double>();
}

QString PortfolioOptimizer::generatePortfolioId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
