/**
 * @file RiskWarningSystem.cpp
 * @brief 风险预警系统实现
 */

#include "RiskWarningSystem.h"
#include "core/trading/PositionManager.h"
#include "shared/utils/Logger.h"
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>

RiskWarningSystem* RiskWarningSystem::instance()
{
    static RiskWarningSystem* inst = new RiskWarningSystem();
    return inst;
}

RiskWarningSystem::RiskWarningSystem(QObject* parent)
    : QObject(parent)
    , m_checkTimer(new QTimer(this))
{
    connect(m_checkTimer, &QTimer::timeout, this, &RiskWarningSystem::onPeriodicCheck);
}

RiskWarningSystem::~RiskWarningSystem()
{
    m_checkTimer->stop();
}

bool RiskWarningSystem::initialize()
{
    if (m_initialized) return true;

    LOG_INFO("Initializing Risk Warning System");

    // 启动定期检查（每30秒）
    m_checkTimer->start(30000);

    m_initialized = true;
    LOG_INFO("Risk Warning System initialized successfully");
    return true;
}

void RiskWarningSystem::setRiskThreshold(const RiskThreshold& threshold)
{
    m_threshold = threshold;
    LOG_INFO(QString("Risk threshold updated: maxLoss=%1%, maxDrawdown=%2%")
        .arg(threshold.maxLossPercent).arg(threshold.maxDrawdownPercent));
}

void RiskWarningSystem::monitorSymbol(const QString& symbol)
{
    if (!m_alerts.contains(symbol)) {
        m_alerts[symbol] = QVector<RiskAlert>();
        m_currentRiskLevel[symbol] = RiskLevel::Low;
        LOG_INFO(QString("Started monitoring: %1").arg(symbol));
    }
}

void RiskWarningSystem::stopMonitoring(const QString& symbol)
{
    m_alerts.remove(symbol);
    m_latestQuotes.remove(symbol);
    m_priceHistory.remove(symbol);
    m_currentRiskLevel.remove(symbol);
    LOG_INFO(QString("Stopped monitoring: %1").arg(symbol));
}

QVector<QString> RiskWarningSystem::monitoredSymbols() const
{
    return m_alerts.keys().toVector();
}

void RiskWarningSystem::assessRisk(const QString& symbol)
{
    if (!m_latestQuotes.contains(symbol)) {
        LOG_WARNING(QString("No data available for risk assessment: %1").arg(symbol));
        return;
    }

    const auto& quote = m_latestQuotes[symbol];

    // 评估各类风险
    RiskLevel priceRisk = assessPriceDropRisk(symbol, quote);
    RiskLevel volumeRisk = assessVolumeRisk(symbol, quote);
    RiskLevel volatilityRisk = assessVolatilityRisk(symbol);
    RiskLevel drawdownRisk = assessDrawdownRisk(symbol);
    RiskLevel concentrationRisk = assessConcentrationRisk(symbol);

    // 计算综合风险等级
    RiskLevel overallRisk = std::max({priceRisk, volumeRisk, volatilityRisk,
                                      drawdownRisk, concentrationRisk});

    // 检查风险等级是否变化
    if (m_currentRiskLevel[symbol] != overallRisk) {
        RiskLevel oldLevel = m_currentRiskLevel[symbol];
        m_currentRiskLevel[symbol] = overallRisk;
        emit riskLevelChanged(symbol, overallRisk);

        LOG_INFO(QString("Risk level changed for %1: %2 -> %3")
            .arg(symbol)
            .arg(riskLevelToString(oldLevel))
            .arg(riskLevelToString(overallRisk)));
    }
}

QVector<RiskAlert> RiskWarningSystem::getAlerts(const QString& symbol) const
{
    if (symbol.isEmpty()) {
        QVector<RiskAlert> allAlerts;
        for (const auto& alerts : m_alerts) {
            allAlerts.append(alerts);
        }
        return allAlerts;
    }
    return m_alerts.value(symbol);
}

QVector<RiskAlert> RiskWarningSystem::getUnacknowledgedAlerts() const
{
    QVector<RiskAlert> unacknowledged;
    for (const auto& alerts : m_alerts) {
        for (const auto& alert : alerts) {
            if (!alert.acknowledged) {
                unacknowledged.append(alert);
            }
        }
    }
    return unacknowledged;
}

void RiskWarningSystem::acknowledgeAlert(const QString& alertId)
{
    for (auto& alerts : m_alerts) {
        for (auto& alert : alerts) {
            if (alert.id == alertId) {
                alert.acknowledged = true;
                LOG_DEBUG(QString("Alert acknowledged: %1").arg(alertId));
                return;
            }
        }
    }
}

RiskStatistics RiskWarningSystem::getStatistics() const
{
    return m_statistics;
}

QString RiskWarningSystem::generateRiskReport(const QString& symbol)
{
    QString report;
    QTextStream stream(&report);

    stream << QStringLiteral("=== 风险分析报告 ===\n\n");

    if (symbol.isEmpty()) {
        // 整体风险报告
        stream << QStringLiteral("监控股票数: %1\n").arg(m_alerts.size());
        stream << QStringLiteral("总预警次数: %1\n").arg(m_statistics.totalAlerts);
        stream << QStringLiteral("极高风险: %1 次\n").arg(m_statistics.criticalAlerts);
        stream << QStringLiteral("高风险: %1 次\n").arg(m_statistics.highAlerts);
        stream << QStringLiteral("中风险: %1 次\n").arg(m_statistics.mediumAlerts);
        stream << QStringLiteral("低风险: %1 次\n").arg(m_statistics.lowAlerts);
        stream << QStringLiteral("平均风险分数: %1\n").arg(m_statistics.avgRiskScore, 0, 'f', 2);
        stream << QStringLiteral("最后更新: %1\n\n").arg(m_statistics.lastUpdateTime.toString("yyyy-MM-dd hh:mm:ss"));

        // 各股票风险等级
        stream << QStringLiteral("=== 各股票风险等级 ===\n");
        for (auto it = m_currentRiskLevel.begin(); it != m_currentRiskLevel.end(); ++it) {
            stream << QString("%1: %2\n").arg(it.key(), riskLevelToString(it.value()));
        }
    } else {
        // 单个股票风险报告
        stream << QStringLiteral("股票代码: %1\n").arg(symbol);
        stream << QStringLiteral("当前风险等级: %1\n").arg(riskLevelToString(m_currentRiskLevel.value(symbol)));

        if (m_latestQuotes.contains(symbol)) {
            const auto& quote = m_latestQuotes[symbol];
            stream << QStringLiteral("\n=== 行情数据 ===\n");
            stream << QStringLiteral("最新价: %1\n").arg(quote.lastPrice, 0, 'f', 2);
            stream << QStringLiteral("涨跌幅: %1%%\n").arg(quote.changePercent, 0, 'f', 2);
            stream << QStringLiteral("成交量: %1\n").arg(quote.volume);
        }

        // 风险指标
        stream << QStringLiteral("\n=== 风险指标 ===\n");
        stream << QStringLiteral("波动率: %1%%\n").arg(calculateVolatility(symbol), 0, 'f', 2);
        stream << QStringLiteral("最大回撤: %1%%\n").arg(calculateDrawdown(symbol), 0, 'f', 2);

        // 历史预警
        if (m_alerts.contains(symbol)) {
            stream << QStringLiteral("\n=== 历史预警 (%1条) ===\n").arg(m_alerts[symbol].size());
            for (const auto& alert : m_alerts[symbol]) {
                stream << QString("[%1] %2: %3\n")
                    .arg(alert.timestamp.toString("MM-dd hh:mm"))
                    .arg(riskTypeToString(alert.type))
                    .arg(alert.description);
            }
        }
    }

    return report;
}

QString RiskWarningSystem::getRiskSuggestion(RiskType type, RiskLevel level)
{
    if (level == RiskLevel::Low) {
        return QStringLiteral("风险可控，可继续持有观察。");
    }

    QString suggestion;

    switch (type) {
    case RiskType::PriceDrop:
        if (level == RiskLevel::Critical) {
            suggestion = QStringLiteral("建议立即止损，控制损失在可承受范围内。");
        } else if (level == RiskLevel::High) {
            suggestion = QStringLiteral("建议考虑减仓或设置止损位，防范进一步下跌风险。");
        } else {
            suggestion = QStringLiteral("密切关注价格走势，做好风险应对准备。");
        }
        break;

    case RiskType::VolumeSpike:
        suggestion = QStringLiteral("成交量异常放大，注意市场情绪变化，谨慎操作。");
        break;

    case RiskType::VolatilityHigh:
        suggestion = QStringLiteral("波动率较高，建议降低仓位或使用对冲策略。");
        break;

    case RiskType::DrawdownExceed:
        suggestion = QStringLiteral("回撤超过阈值，建议重新评估持仓策略，考虑止损或调仓。");
        break;

    case RiskType::ConcentrationRisk:
        suggestion = QStringLiteral("持仓过于集中，建议分散投资降低单一股票风险。");
        break;

    case RiskType::LiquidityRisk:
        suggestion = QStringLiteral("流动性风险上升，注意交易成本和滑点影响。");
        break;

    case RiskType::TrendReversal:
        suggestion = QStringLiteral("趋势可能反转，建议关注技术指标确认，及时调整策略。");
        break;
    }

    return suggestion;
}

void RiskWarningSystem::onMarketDataUpdated(const QString& symbol, const StockQuote& quote)
{
    m_latestQuotes[symbol] = quote;

    // 更新价格历史
    if (!m_priceHistory.contains(symbol)) {
        m_priceHistory[symbol] = QVector<double>();
    }
    m_priceHistory[symbol].append(quote.lastPrice);

    // 保留最近100个价格点
    if (m_priceHistory[symbol].size() > 100) {
        m_priceHistory[symbol].removeFirst();
    }

    // 触发风险评估
    assessRisk(symbol);
}

void RiskWarningSystem::onPeriodicCheck()
{
    // 定期检查所有监控股票的风险
    for (const QString& symbol : m_alerts.keys()) {
        assessRisk(symbol);
    }

    // 更新统计信息
    m_statistics.lastUpdateTime = QDateTime::currentDateTime();
    emit statisticsUpdated(m_statistics);
}

RiskLevel RiskWarningSystem::assessPriceDropRisk(const QString& symbol, const StockQuote& quote)
{
    if (quote.preClose <= 0) return RiskLevel::Low;

    double dropPercent = -quote.changePercent;

    if (dropPercent >= m_threshold.maxLossPercent) {
        generateAlert(symbol, RiskType::PriceDrop, RiskLevel::Critical,
                    dropPercent, m_threshold.maxLossPercent);
        return RiskLevel::Critical;
    } else if (dropPercent >= m_threshold.maxLossPercent * 0.7) {
        generateAlert(symbol, RiskType::PriceDrop, RiskLevel::High,
                    dropPercent, m_threshold.maxLossPercent);
        return RiskLevel::High;
    } else if (dropPercent >= m_threshold.maxLossPercent * 0.5) {
        return RiskLevel::Medium;
    }

    return RiskLevel::Low;
}

RiskLevel RiskWarningSystem::assessVolumeRisk(const QString& symbol, const StockQuote& quote)
{
    // 简化的成交量风险评估
    // 实际应该与历史平均成交量比较
    return RiskLevel::Low;
}

RiskLevel RiskWarningSystem::assessVolatilityRisk(const QString& symbol)
{
    double volatility = calculateVolatility(symbol);

    if (volatility >= m_threshold.maxVolatility) {
        generateAlert(symbol, RiskType::VolatilityHigh, RiskLevel::High,
                    volatility, m_threshold.maxVolatility);
        return RiskLevel::High;
    } else if (volatility >= m_threshold.maxVolatility * 0.7) {
        return RiskLevel::Medium;
    }

    return RiskLevel::Low;
}

RiskLevel RiskWarningSystem::assessDrawdownRisk(const QString& symbol)
{
    double drawdown = calculateDrawdown(symbol);

    if (drawdown >= m_threshold.maxDrawdownPercent) {
        generateAlert(symbol, RiskType::DrawdownExceed, RiskLevel::Critical,
                    drawdown, m_threshold.maxDrawdownPercent);
        return RiskLevel::Critical;
    } else if (drawdown >= m_threshold.maxDrawdownPercent * 0.7) {
        generateAlert(symbol, RiskType::DrawdownExceed, RiskLevel::High,
                    drawdown, m_threshold.maxDrawdownPercent);
        return RiskLevel::High;
    } else if (drawdown >= m_threshold.maxDrawdownPercent * 0.5) {
        return RiskLevel::Medium;
    }

    return RiskLevel::Low;
}

RiskLevel RiskWarningSystem::assessConcentrationRisk(const QString& symbol)
{
    // 简化的集中度风险评估
    // 实际应该考虑整个投资组合
    return RiskLevel::Low;
}

double RiskWarningSystem::calculateVolatility(const QString& symbol)
{
    if (!m_priceHistory.contains(symbol) || m_priceHistory[symbol].size() < 2) {
        return 0.0;
    }

    const auto& prices = m_priceHistory[symbol];
    int n = prices.size();

    // 计算收益率
    QVector<double> returns;
    for (int i = 1; i < n; ++i) {
        if (prices[i-1] > 0) {
            returns.append((prices[i] - prices[i-1]) / prices[i-1]);
        }
    }

    if (returns.isEmpty()) return 0.0;

    // 计算标准差（波动率）
    double mean = 0.0;
    for (double r : returns) {
        mean += r;
    }
    mean /= returns.size();

    double variance = 0.0;
    for (double r : returns) {
        variance += (r - mean) * (r - mean);
    }
    variance /= returns.size();

    // 年化波动率（假设数据频率）
    return std::sqrt(variance) * 100.0;
}

double RiskWarningSystem::calculateDrawdown(const QString& symbol)
{
    if (!m_priceHistory.contains(symbol) || m_priceHistory[symbol].isEmpty()) {
        return 0.0;
    }

    const auto& prices = m_priceHistory[symbol];
    double maxPrice = prices.first();
    double maxDrawdown = 0.0;

    for (double price : prices) {
        if (price > maxPrice) {
            maxPrice = price;
        }
        double drawdown = (maxPrice - price) / maxPrice * 100.0;
        if (drawdown > maxDrawdown) {
            maxDrawdown = drawdown;
        }
    }

    return maxDrawdown;
}

double RiskWarningSystem::calculatePositionConcentration(const QString& symbol)
{
    // 计算单个股票在总资产中的占比
    double totalValue = 0.0;
    double symbolValue = 0.0;

    // 获取所有持仓
    QVector<PositionInfo> positions = PositionManager::instance().getPositions();

    for (const auto& pos : positions)
    {
        double value = pos.volume * pos.marketPrice;
        totalValue += value;

        if (pos.instrumentId == symbol)
        {
            symbolValue = value;
        }
    }

    // 返回集中度（百分比）
    if (totalValue > 0)
    {
        return (symbolValue / totalValue) * 100.0;
    }

    return 0.0;
}

void RiskWarningSystem::generateAlert(const QString& symbol, RiskType type, RiskLevel level,
                                      double value, double threshold)
{
    RiskAlert alert;
    alert.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    alert.symbol = symbol;
    alert.type = type;
    alert.level = level;
    alert.value = value;
    alert.threshold = threshold;
    alert.timestamp = QDateTime::currentDateTime();
    alert.description = QString("%1: 当前值 %2, 阈值 %3")
        .arg(riskTypeToString(type))
        .arg(value, 0, 'f', 2)
        .arg(threshold, 0, 'f', 2);
    alert.suggestion = getRiskSuggestion(type, level);

    m_alerts[symbol].append(alert);

    // 更新统计
    m_statistics.totalAlerts++;
    switch (level) {
    case RiskLevel::Critical: m_statistics.criticalAlerts++; break;
    case RiskLevel::High: m_statistics.highAlerts++; break;
    case RiskLevel::Medium: m_statistics.mediumAlerts++; break;
    case RiskLevel::Low: m_statistics.lowAlerts++; break;
    }

    LOG_WARNING(QString("Risk alert generated: %1 - %2 (%3)")
        .arg(symbol, riskTypeToString(type), riskLevelToString(level)));

    emit riskAlertTriggered(alert);
}

QString RiskWarningSystem::riskLevelToString(RiskLevel level) const
{
    switch (level) {
    case RiskLevel::Low: return QStringLiteral("低风险");
    case RiskLevel::Medium: return QStringLiteral("中风险");
    case RiskLevel::High: return QStringLiteral("高风险");
    case RiskLevel::Critical: return QStringLiteral("极高风险");
    default: return QStringLiteral("未知");
    }
}

QString RiskWarningSystem::riskTypeToString(RiskType type) const
{
    switch (type) {
    case RiskType::PriceDrop: return QStringLiteral("价格下跌");
    case RiskType::VolumeSpike: return QStringLiteral("成交量异常");
    case RiskType::VolatilityHigh: return QStringLiteral("波动率过高");
    case RiskType::DrawdownExceed: return QStringLiteral("回撤超标");
    case RiskType::ConcentrationRisk: return QStringLiteral("持仓集中风险");
    case RiskType::LiquidityRisk: return QStringLiteral("流动性风险");
    case RiskType::TrendReversal: return QStringLiteral("趋势反转");
    default: return QStringLiteral("未知风险");
    }
}

RiskLevel RiskWarningSystem::calculateOverallRisk(const QString& symbol)
{
    return m_currentRiskLevel.value(symbol, RiskLevel::Low);
}
