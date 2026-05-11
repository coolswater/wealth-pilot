/**
 * @file QuantTradingEngine.cpp
 * @brief 量化交易引擎实现
 */

#include "QuantTradingEngine.h"
#include "../utils/Logger.h"
#include <QUuid>
#include <QDateTime>

QuantTradingEngine* QuantTradingEngine::instance()
{
    static QuantTradingEngine* inst = new QuantTradingEngine();
    return inst;
}

QuantTradingEngine::QuantTradingEngine(QObject* parent)
    : QObject(parent)
{
    // 启动风控检查定时器
    m_riskCheckTimer = new QTimer(this);
    connect(m_riskCheckTimer, &QTimer::timeout, this, &QuantTradingEngine::checkAllRiskRules);
    m_riskCheckTimer->start(5000); // 每5秒检查一次

    LOG_INFO("QuantTradingEngine initialized");
}

QuantTradingEngine::~QuantTradingEngine()
{
    // 停止所有策略
    for (auto it = m_strategies.begin(); it != m_strategies.end(); ++it) {
        stopStrategy(it.key());
    }
}

void QuantTradingEngine::addStrategy(std::shared_ptr<IStrategy> strategy, const QString& strategyId)
{
    m_strategies[strategyId] = strategy;

    StrategyRuntime status;
    status.strategyId = strategyId;
    status.name = strategy->name();
    status.state = TradingState::Idle;
    m_strategyStatus[strategyId] = status;

    LOG_INFO(QString("Strategy added: %1 (%2)").arg(strategy->name()).arg(strategyId));
}

void QuantTradingEngine::removeStrategy(const QString& strategyId)
{
    stopStrategy(strategyId);
    m_strategies.remove(strategyId);
    m_strategyStatus.remove(strategyId);
    m_strategySymbols.remove(strategyId);
    m_positions.remove(strategyId);

    LOG_INFO(QString("Strategy removed: %1").arg(strategyId));
}

bool QuantTradingEngine::startStrategy(const QString& strategyId)
{
    if (!m_strategies.contains(strategyId)) {
        LOG_ERROR(QString("Strategy not found: %1").arg(strategyId));
        return false;
    }

    auto& status = m_strategyStatus[strategyId];
    status.state = TradingState::Running;
    status.startTime = QDateTime::currentDateTime();

    // 初始化策略
    m_strategies[strategyId]->initialize();

    emit strategyStateChanged(strategyId, TradingState::Running);
    LOG_INFO(QString("Strategy started: %1").arg(strategyId));

    return true;
}

bool QuantTradingEngine::stopStrategy(const QString& strategyId)
{
    if (!m_strategies.contains(strategyId)) {
        return false;
    }

    auto& status = m_strategyStatus[strategyId];
    status.state = TradingState::Idle;

    emit strategyStateChanged(strategyId, TradingState::Idle);
    LOG_INFO(QString("Strategy stopped: %1").arg(strategyId));

    return true;
}

bool QuantTradingEngine::pauseStrategy(const QString& strategyId)
{
    if (!m_strategies.contains(strategyId)) {
        return false;
    }

    m_strategyStatus[strategyId].state = TradingState::Paused;
    emit strategyStateChanged(strategyId, TradingState::Paused);
    LOG_INFO(QString("Strategy paused: %1").arg(strategyId));

    return true;
}

bool QuantTradingEngine::resumeStrategy(const QString& strategyId)
{
    if (!m_strategies.contains(strategyId)) {
        return false;
    }

    m_strategyStatus[strategyId].state = TradingState::Running;
    emit strategyStateChanged(strategyId, TradingState::Running);
    LOG_INFO(QString("Strategy resumed: %1").arg(strategyId));

    return true;
}

StrategyRuntime QuantTradingEngine::getStrategyStatus(const QString& strategyId) const
{
    return m_strategyStatus.value(strategyId);
}

QVector<StrategyRuntime> QuantTradingEngine::getAllStrategyStatus() const
{
    QVector<StrategyRuntime> result;
    for (const auto& status : m_strategyStatus) {
        result.append(status);
    }
    return result;
}

void QuantTradingEngine::setTradingMode(TradingMode mode)
{
    m_mode = mode;
    LOG_INFO(QString("Trading mode set to: %1").arg(mode == TradingMode::Live ? "Live" : "Simulation"));
}

void QuantTradingEngine::onMarketData(const QString& symbol, const QVariantMap& data)
{
    // 分发数据到订阅该标的的策略
    for (auto it = m_strategySymbols.begin(); it != m_strategySymbols.end(); ++it) {
        const QString& strategyId = it.key();
        const QStringList& symbols = it.value();

        if (symbols.contains(symbol) &&
            m_strategyStatus[strategyId].state == TradingState::Running) {

            // 处理数据
            StrategySignal signal = m_strategies[strategyId]->processData(data);

            if (signal.action != "hold") {
                processSignal(strategyId, signal);
            }
        }
    }
}

void QuantTradingEngine::subscribeSymbols(const QString& strategyId, const QStringList& symbols)
{
    m_strategySymbols[strategyId] = symbols;
    m_strategyStatus[strategyId].symbols = symbols;

    LOG_INFO(QString("Symbols subscribed for %1: %2").arg(strategyId).arg(symbols.join(",")));
}

void QuantTradingEngine::addRiskRule(const RiskRule& rule)
{
    m_riskRules.append(rule);
    LOG_INFO(QString("Risk rule added: %1 (%2)").arg(rule.name).arg(rule.id));
}

void QuantTradingEngine::removeRiskRule(const QString& ruleId)
{
    for (int i = 0; i < m_riskRules.size(); ++i) {
        if (m_riskRules[i].id == ruleId) {
            m_riskRules.removeAt(i);
            LOG_INFO(QString("Risk rule removed: %1").arg(ruleId));
            break;
        }
    }
}

bool QuantTradingEngine::checkRiskControl(const QString& strategyId, const StrategySignal& signal)
{
    for (const RiskRule& rule : m_riskRules) {
        if (!rule.enabled) continue;

        // 检查单笔交易限额
        if (rule.type == "single_trade_limit") {
            double tradeValue = signal.price * signal.quantity;
            if (tradeValue > rule.threshold) {
                emit riskTriggered(rule.id, QString("单笔交易超过限额: %1 > %2")
                    .arg(tradeValue).arg(rule.threshold));
                return false;
            }
        }

        // 检查持仓限额
        if (rule.type == "position_limit") {
            int totalPosition = 0;
            for (const auto& pos : m_positions[strategyId]) {
                totalPosition += pos;
            }
            if (totalPosition + signal.quantity > rule.threshold) {
                emit riskTriggered(rule.id, QString("持仓超过限额"));
                return false;
            }
        }

        // 检查亏损限额
        if (rule.type == "loss_limit") {
            double totalProfit = m_strategyStatus[strategyId].totalProfit;
            if (totalProfit < -rule.threshold) {
                emit riskTriggered(rule.id, QString("亏损超过限额: %1").arg(-totalProfit));
                return false;
            }
        }
    }

    return true;
}

QVector<LiveTrade> QuantTradingEngine::getTradeHistory(const QString& strategyId) const
{
    if (strategyId.isEmpty()) {
        return m_tradeHistory;
    }

    QVector<LiveTrade> result;
    for (const LiveTrade& trade : m_tradeHistory) {
        if (trade.strategyId == strategyId) {
            result.append(trade);
        }
    }
    return result;
}

QHash<QString, int> QuantTradingEngine::getPositions(const QString& strategyId) const
{
    if (strategyId.isEmpty()) {
        QHash<QString, int> allPositions;
        for (const auto& pos : m_positions) {
            for (auto it = pos.begin(); it != pos.end(); ++it) {
                allPositions[it.key()] += it.value();
            }
        }
        return allPositions;
    }

    return m_positions.value(strategyId);
}

QuantTradingEngine::OverallStats QuantTradingEngine::getOverallStats() const
{
    OverallStats stats;

    for (const auto& status : m_strategyStatus) {
        stats.totalProfit += status.totalProfit;
        stats.totalTrades += status.tradeCount;

        if (status.state == TradingState::Running) {
            stats.activeStrategies++;
        }
    }

    // 计算胜率
    int winCount = 0;
    for (const LiveTrade& trade : m_tradeHistory) {
        if (trade.profit > 0) winCount++;
    }
    stats.winRate = m_tradeHistory.isEmpty() ? 0 :
        static_cast<double>(winCount) / m_tradeHistory.size();

    return stats;
}

void QuantTradingEngine::processSignal(const QString& strategyId, const StrategySignal& signal)
{
    // 风控检查
    if (!checkRiskControl(strategyId, signal)) {
        LOG_WARNING(QString("Signal blocked by risk control: %1 %2")
            .arg(signal.symbol).arg(signal.action));
        return;
    }

    // 执行订单
    executeOrder(strategyId, signal);
}

void QuantTradingEngine::executeOrder(const QString& strategyId, const StrategySignal& signal)
{
    LiveTrade trade;
    trade.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    trade.strategyId = strategyId;
    trade.symbol = signal.symbol;
    trade.direction = signal.action;
    trade.price = signal.price;
    trade.quantity = signal.quantity;
    trade.time = QDateTime::currentDateTime();
    trade.status = m_mode == TradingMode::Simulation ? "filled" : "pending";
    trade.orderId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // 更新持仓
    auto& positions = m_positions[strategyId];
    if (signal.action == "buy") {
        positions[signal.symbol] += signal.quantity;
    } else if (signal.action == "sell") {
        positions[signal.symbol] -= signal.quantity;
        if (positions[signal.symbol] <= 0) {
            positions.remove(signal.symbol);
        }
    }

    // 更新统计
    auto& status = m_strategyStatus[strategyId];
    status.tradeCount++;

    m_tradeHistory.append(trade);

    emit tradeExecuted(trade);
    LOG_INFO(QString("Trade executed: %1 %2 %3@%4 (%5)")
        .arg(strategyId).arg(signal.symbol).arg(signal.action)
        .arg(signal.price).arg(signal.quantity));
}

void QuantTradingEngine::updatePositions(const QString& symbol, double price)
{
    // 更新所有策略的持仓市值
    for (auto it = m_positions.begin(); it != m_positions.end(); ++it) {
        if (it.value().contains(symbol)) {
            // 计算盈亏
            // TODO: 实现详细的盈亏计算
        }
    }
}

void QuantTradingEngine::checkAllRiskRules()
{
    // 定期检查风控规则
    for (const auto& status : m_strategyStatus) {
        if (status.state != TradingState::Running) continue;

        // 检查亏损限额
        for (const RiskRule& rule : m_riskRules) {
            if (rule.type == "loss_limit" && rule.enabled) {
                if (status.totalProfit < -rule.threshold) {
                    emit riskTriggered(rule.id, QString("策略 %1 亏损超过限额")
                        .arg(status.name));

                    // 自动停止策略
                    if (rule.action == "stop") {
                        stopStrategy(status.strategyId);
                    }
                }
            }
        }
    }
}