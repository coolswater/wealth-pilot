/**
 * @file RiskAnalyzer.cpp
 * @brief 风险分析器实现
 */

#include "RiskAnalyzer.h"
#include "utils/Logger.h"
#include <QDateTime>
#include <cmath>
#include <algorithm>

RiskAnalyzer* RiskAnalyzer::instance()
{
    static RiskAnalyzer* inst = new RiskAnalyzer();
    return inst;
}

RiskAnalyzer::RiskAnalyzer(QObject* parent)
    : QObject(parent)
{
    LOG_INFO("RiskAnalyzer initialized");
}

void RiskAnalyzer::setPositions(const QVector<RiskPositionInfo>& positions)
{
    m_positions = positions;
    LOG_DEBUG(QString("Positions set: %1 items").arg(positions.size()));
}

void RiskAnalyzer::addPosition(const RiskPositionInfo& position)
{
    m_positions.append(position);
    LOG_DEBUG(QString("Position added: %1").arg(position.symbol));
}

void RiskAnalyzer::clearPositions()
{
    m_positions.clear();
    LOG_DEBUG("Positions cleared");
}

RiskMetrics RiskAnalyzer::calculateRiskMetrics()
{
    m_metrics = RiskMetrics();
    m_metrics.positionCount = m_positions.size();

    if (m_positions.isEmpty()) {
        return m_metrics;
    }

    // 计算总市值和盈亏
    for (const RiskPositionInfo& pos : m_positions) {
        m_metrics.totalValue += pos.marketValue;
        m_metrics.totalProfit += pos.profit;
    }

    // 计算总盈亏比例
    double totalCost = 0;
    for (const RiskPositionInfo& pos : m_positions) {
        totalCost += pos.cost * pos.quantity;
    }
    m_metrics.totalProfitPercent = totalCost > 0 ? m_metrics.totalProfit / totalCost : 0;

    // 计算权重
    for (RiskPositionInfo& pos : m_positions) {
        pos.weight = m_metrics.totalValue > 0 ? pos.marketValue / m_metrics.totalValue : 0;
    }

    // 计算风险指标
    m_metrics.maxDrawdown = calculateMaxDrawdown();
    m_metrics.volatility = calculateVolatility();
    m_metrics.var95 = calculateVaR(0.95);
    m_metrics.var99 = calculateVaR(0.99);
    m_metrics.concentrationRisk = calculateConcentrationRisk();

    // 计算夏普比率（简化）
    if (m_metrics.volatility > 0) {
        double riskFreeRate = 0.03; // 无风险利率 3%
        double excessReturn = m_metrics.totalProfitPercent - riskFreeRate;
        m_metrics.sharpeRatio = excessReturn / m_metrics.volatility;
    }

    emit metricsUpdated(m_metrics);
    LOG_INFO(QString("Risk metrics calculated: value=%1, profit=%2%, VaR95=%3%")
        .arg(m_metrics.totalValue, 0, 'f', 2)
        .arg(m_metrics.totalProfitPercent * 100, 0, 'f', 2)
        .arg(m_metrics.var95 * 100, 0, 'f', 2));

    return m_metrics;
}

double RiskAnalyzer::calculateVaR(double confidence, int days)
{
    if (m_positions.isEmpty()) return 0;

    // 收集所有持仓的历史收益率
    QVector<double> allReturns;
    for (const RiskPositionInfo& pos : m_positions) {
        QVector<double> returns = getHistoricalReturns(pos.symbol);
        allReturns.append(returns);
    }

    if (allReturns.isEmpty()) {
        // 使用简化方法：假设正态分布
        double avgReturn = m_metrics.totalProfitPercent;
        double stdDev = m_metrics.volatility;

        // VaR = -μ + z * σ * sqrt(days)
        double z = confidence == 0.99 ? 2.33 : 1.65;
        return -avgReturn + z * stdDev * sqrt(days);
    }

    // 历史模拟法
    std::sort(allReturns.begin(), allReturns.end());
    int index = static_cast<int>((1 - confidence) * allReturns.size());
    double varReturn = -allReturns[index];

    // 调整持有期
    return varReturn * sqrt(days);
}

double RiskAnalyzer::calculateMaxDrawdown()
{
    if (m_positions.isEmpty()) return 0;

    // 简化计算：使用各持仓的最大回撤加权平均
    double totalDrawdown = 0;
    for (const RiskPositionInfo& pos : m_positions) {
        // 假设最大回撤为从成本到当前最低点的回撤
        double drawdown = pos.cost > 0 ?
            qMax(0.0, (pos.cost - pos.currentPrice) / pos.cost) : 0;
        totalDrawdown += drawdown * pos.weight;
    }

    return totalDrawdown;
}

double RiskAnalyzer::calculateVolatility()
{
    if (m_positions.isEmpty()) return 0;

    // 计算组合波动率
    double totalVariance = 0;

    for (const RiskPositionInfo& pos : m_positions) {
        QVector<double> returns = getHistoricalReturns(pos.symbol);
        if (returns.isEmpty()) continue;

        // 计算单个资产的波动率
        double mean = 0;
        for (double r : returns) mean += r;
        mean /= returns.size();

        double variance = 0;
        for (double r : returns) {
            variance += pow(r - mean, 2);
        }
        variance /= returns.size();

        // 加权方差
        totalVariance += pow(pos.weight, 2) * variance;
    }

    return sqrt(totalVariance);
}

double RiskAnalyzer::calculateConcentrationRisk()
{
    if (m_positions.isEmpty()) return 0;

    // 使用赫芬达尔指数计算集中度
    double hhi = 0;
    for (const RiskPositionInfo& pos : m_positions) {
        hhi += pow(pos.weight * 100, 2);
    }

    // 归一化到 0-1
    double maxHHI = 10000; // 完全集中
    return hhi / maxHHI;
}

QVector<RiskAnalyzerAlert> RiskAnalyzer::checkRiskAlerts()
{
    QVector<RiskAnalyzerAlert> alerts;

    // 检查最大回撤
    if (m_metrics.maxDrawdown > m_maxDrawdownLimit) {
        RiskAnalyzerAlert alert;
        alert.type = "max_drawdown";
        alert.level = m_metrics.maxDrawdown > m_maxDrawdownLimit * 1.5 ? "danger" : "warning";
        alert.message = QString("最大回撤超过阈值: %1% > %2%")
            .arg(m_metrics.maxDrawdown * 100, 0, 'f', 1)
            .arg(m_maxDrawdownLimit * 100, 0, 'f', 1);
        alert.value = m_metrics.maxDrawdown;
        alert.time = QDateTime::currentDateTime();
        alerts.append(alert);
        emit riskAlert(alert);
    }

    // 检查集中度风险
    if (m_metrics.concentrationRisk > m_concentrationLimit) {
        RiskAnalyzerAlert alert;
        alert.type = "concentration";
        alert.level = "warning";
        alert.message = QString("持仓集中度过高: %1%")
            .arg(m_metrics.concentrationRisk * 100, 0, 'f', 1);
        alert.value = m_metrics.concentrationRisk;
        alert.time = QDateTime::currentDateTime();
        alerts.append(alert);
        emit riskAlert(alert);
    }

    // 检查 VaR
    if (m_metrics.var95 > m_varLimit) {
        RiskAnalyzerAlert alert;
        alert.type = "var";
        alert.level = "warning";
        alert.message = QString("VaR(95%)超过阈值: %1% > %2%")
            .arg(m_metrics.var95 * 100, 0, 'f', 1)
            .arg(m_varLimit * 100, 0, 'f', 1);
        alert.value = m_metrics.var95;
        alert.time = QDateTime::currentDateTime();
        alerts.append(alert);
        emit riskAlert(alert);
    }

    // 检查单个持仓风险
    for (const RiskPositionInfo& pos : m_positions) {
        // 单只股票权重过高
        if (pos.weight > 0.3) {
            RiskAnalyzerAlert alert;
            alert.type = "single_position";
            alert.level = "warning";
            alert.symbol = pos.symbol;
            alert.message = QString("%1 权重过高: %2%")
                .arg(pos.symbol)
                .arg(pos.weight * 100, 0, 'f', 1);
            alert.value = pos.weight;
            alert.time = QDateTime::currentDateTime();
            alerts.append(alert);
            emit riskAlert(alert);
        }

        // 单只股票亏损过大
        if (pos.profitPercent < -0.2) {
            RiskAnalyzerAlert alert;
            alert.type = "large_loss";
            alert.level = "danger";
            alert.symbol = pos.symbol;
            alert.message = QString("%1 亏损严重: %2%")
                .arg(pos.symbol)
                .arg(pos.profitPercent * 100, 0, 'f', 1);
            alert.value = pos.profitPercent;
            alert.time = QDateTime::currentDateTime();
            alerts.append(alert);
            emit riskAlert(alert);
        }
    }

    LOG_INFO(QString("Risk alerts checked: %1 alerts").arg(alerts.size()));
    return alerts;
}

QString RiskAnalyzer::generateRiskReport()
{
    QString report;
    report += QStringLiteral("========== 风险分析报告 ==========\n\n");

    report += QStringLiteral("========== 持仓概览 ==========\n");
    report += QString("持仓数量: %1\n").arg(m_metrics.positionCount);
    report += QString("总市值: %1\n").arg(m_metrics.totalValue, 0, 'f', 2);
    report += QString("总盈亏: %1 (%2%)\n\n")
        .arg(m_metrics.totalProfit, 0, 'f', 2)
        .arg(m_metrics.totalProfitPercent * 100, 0, 'f', 2);

    report += QStringLiteral("========== 风险指标 ==========\n");
    report += QString("最大回撤: %1%\n").arg(m_metrics.maxDrawdown * 100, 0, 'f', 2);
    report += QString("波动率: %1%\n").arg(m_metrics.volatility * 100, 0, 'f', 2);
    report += QString("夏普比率: %1\n").arg(m_metrics.sharpeRatio, 0, 'f', 2);
    report += QString("VaR(95%): %1%\n").arg(m_metrics.var95 * 100, 0, 'f', 2);
    report += QString("VaR(99%): %1%\n").arg(m_metrics.var99 * 100, 0, 'f', 2);
    report += QString("集中度风险: %1%\n\n").arg(m_metrics.concentrationRisk * 100, 0, 'f', 2);

    report += QStringLiteral("========== 持仓明细 ==========\n");
    report += QString("%-10s %8s %10s %10s %8s\n")
        .arg("代码").arg("数量").arg("市值").arg("盈亏").arg("权重");
    report += QStringLiteral("------------------------------------------------\n");

    for (const RiskPositionInfo& pos : m_positions) {
        report += QString("%-10s %8d %10.2f %10.2f %7.1f%%\n")
            .arg(pos.symbol)
            .arg(pos.quantity)
            .arg(pos.marketValue)
            .arg(pos.profit)
            .arg(pos.weight * 100);
    }

    // 风险预警
    QVector<RiskAnalyzerAlert> alerts = checkRiskAlerts();
    if (!alerts.isEmpty()) {
        report += QStringLiteral("\n========== 风险预警 ==========\n");
        for (const RiskAnalyzerAlert& alert : alerts) {
            report += QString("[%1] %2\n").arg(alert.level.toUpper()).arg(alert.message);
        }
    }

    return report;
}

void RiskAnalyzer::setRiskThresholds(double maxDrawdownLimit,
                                     double concentrationLimit,
                                     double varLimit)
{
    m_maxDrawdownLimit = maxDrawdownLimit;
    m_concentrationLimit = concentrationLimit;
    m_varLimit = varLimit;

    LOG_INFO(QString("Risk thresholds set: drawdown=%1, concentration=%2, var=%3")
        .arg(maxDrawdownLimit).arg(concentrationLimit).arg(varLimit));
}

QVector<double> RiskAnalyzer::getHistoricalReturns(const QString& symbol)
{
    // TODO: 从数据源获取历史收益率
    Q_UNUSED(symbol)
    return {};
}

double RiskAnalyzer::calculateBeta(const QString& symbol)
{
    // TODO: 计算 Beta 系数
    Q_UNUSED(symbol)
    return 1.0;
}