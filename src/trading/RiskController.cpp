/**
 * @file RiskController.cpp
 * @brief 风控系统实现
 *
 * @details 实现：
 * - 多维度风控检查
 * - 实时风险监控
 * - 动态规则管理
 * - 风险报告生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "RiskController.h"
#include "OrderManager.h"
#include "PositionManager.h"
#include "utils/Logger.h"

#include <QMutexLocker>
#include <QDateTime>
#include <QtMath>

// ============================================================================
// PIMPL 实现
// ============================================================================

struct RiskController::Impl {
    // 风控规则
    QHash<QString, RiskRule> rules;

    // 默认规则
    RiskRule defaultRule;

    // 账户信息
    AccountInfo accountInfo;
    QVector<PositionInfo> positions;

    // 风险状态
    int currentRiskLevel = 0;
    double todayPnL = 0.0;
    double maxDrawdown = 0.0;
    double peakValue = 0.0;

    // 定时器
    QTimer* riskCheckTimer = nullptr;

    // 线程安全
    mutable QMutex mutex;

    // 初始化标志
    bool initialized = false;
};

// ============================================================================
// 单例实现
// ============================================================================

RiskController& RiskController::instance()
{
    static RiskController instance;
    return instance;
}

RiskController::RiskController(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    // 初始化默认风控规则
    d->defaultRule.ruleId = "default";
    d->defaultRule.name = QStringLiteral("默认风控规则");
    d->defaultRule.maxPositionSize = 1000000.0;      // 最大持仓100万
    d->defaultRule.maxPositionCount = 20;             // 最多20个持仓
    d->defaultRule.maxDailyLoss = 50000.0;            // 日最大亏损5万
    d->defaultRule.maxSingleLoss = 10000.0;           // 单笔最大亏损1万
    d->defaultRule.maxLeverage = 5.0;                 // 最大杠杆5倍
    d->defaultRule.maxMarginRatio = 80.0;             // 最大保证金比例80%
    d->defaultRule.maxDrawdown = 20.0;                // 最大回撤20%
    d->defaultRule.allowNightTrading = true;
    d->defaultRule.allowReverseTrade = true;
    d->defaultRule.isEnabled = true;

    // 创建风险检查定时器
    d->riskCheckTimer = new QTimer(this);
    d->riskCheckTimer->setInterval(5000); // 5秒检查一次
    connect(d->riskCheckTimer, &QTimer::timeout, this, &RiskController::onRiskCheckTimer);

    LOG_DEBUG("RiskController created");
}

RiskController::~RiskController()
{
    shutdown();
    LOG_DEBUG("RiskController destroyed");
}

// ============================================================================
// 初始化
// ============================================================================

bool RiskController::initialize()
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        LOG_WARNING("RiskController already initialized");
        return true;
    }

    // 加载风控规则
    // TODO: 从配置文件或数据库加载

    // 启动风险检查
    d->riskCheckTimer->start();

    d->initialized = true;
    LOG_INFO("RiskController initialized");
    return true;
}

void RiskController::shutdown()
{
    QMutexLocker locker(&d->mutex);

    if (!d->initialized) {
        return;
    }

    d->riskCheckTimer->stop();
    d->rules.clear();
    d->initialized = false;

    LOG_INFO("RiskController shutdown");
}

// ============================================================================
// 风控规则管理
// ============================================================================

void RiskController::addRule(const RiskRule& rule)
{
    QMutexLocker locker(&d->mutex);

    QString ruleId = rule.ruleId.isEmpty() 
        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
        : rule.ruleId;

    RiskRule newRule = rule;
    newRule.ruleId = ruleId;
    newRule.createTime = QDateTime::currentDateTime();
    newRule.updateTime = QDateTime::currentDateTime();

    d->rules[ruleId] = newRule;

    LOG_INFO(QString("Risk rule added: %1").arg(rule.name));
}

bool RiskController::updateRule(const QString& ruleId, const RiskRule& rule)
{
    QMutexLocker locker(&d->mutex);

    if (!d->rules.contains(ruleId)) {
        return false;
    }

    RiskRule& existing = d->rules[ruleId];
    existing.maxPositionSize = rule.maxPositionSize;
    existing.maxPositionCount = rule.maxPositionCount;
    existing.maxDailyLoss = rule.maxDailyLoss;
    existing.maxSingleLoss = rule.maxSingleLoss;
    existing.maxLeverage = rule.maxLeverage;
    existing.maxMarginRatio = rule.maxMarginRatio;
    existing.maxDrawdown = rule.maxDrawdown;
    existing.allowNightTrading = rule.allowNightTrading;
    existing.allowReverseTrade = rule.allowReverseTrade;
    existing.updateTime = QDateTime::currentDateTime();

    LOG_INFO(QString("Risk rule updated: %1").arg(ruleId));
    return true;
}

bool RiskController::removeRule(const QString& ruleId)
{
    QMutexLocker locker(&d->mutex);

    if (!d->rules.contains(ruleId)) {
        return false;
    }

    d->rules.remove(ruleId);
    LOG_INFO(QString("Risk rule removed: %1").arg(ruleId));
    return true;
}

void RiskController::setRuleEnabled(const QString& ruleId, bool enabled)
{
    QMutexLocker locker(&d->mutex);

    if (d->rules.contains(ruleId)) {
        d->rules[ruleId].isEnabled = enabled;
        LOG_DEBUG(QString("Risk rule %1 enabled: %2").arg(ruleId).arg(enabled));
    }
}

QVector<RiskRule> RiskController::getRules() const
{
    QMutexLocker locker(&d->mutex);
    return d->rules.values().toVector();
}

std::optional<RiskRule> RiskController::getRule(const QString& ruleId) const
{
    QMutexLocker locker(&d->mutex);

    if (d->rules.contains(ruleId)) {
        return d->rules[ruleId];
    }
    return std::nullopt;
}

// ============================================================================
// 风控检查
// ============================================================================

RiskCheckResult RiskController::checkOrder(const OrderRequest& request)
{
    QMutexLocker locker(&d->mutex);

    // 获取活跃规则
    const RiskRule& rule = d->defaultRule;
    // TODO: 根据合约或策略选择对应规则

    // 1. 检查最大持仓金额
    auto result = checkMaxPositionSize(request);
    if (!result.passed) return result;

    // 2. 检查最大持仓数量
    result = checkMaxPositionCount(request);
    if (!result.passed) return result;

    // 3. 检查日最大亏损
    result = checkMaxDailyLoss(request);
    if (!result.passed) return result;

    // 4. 检查单笔最大亏损
    result = checkMaxSingleLoss(request);
    if (!result.passed) return result;

    // 5. 检查最大杠杆
    result = checkMaxLeverage(request);
    if (!result.passed) return result;

    // 6. 检查最大保证金比例
    result = checkMaxMarginRatio(request);
    if (!result.passed) return result;

    // 7. 检查最大回撤
    result = checkMaxDrawdown(request);
    if (!result.passed) return result;

    // 8. 检查夜盘交易
    result = checkNightTrading(request);
    if (!result.passed) return result;

    // 9. 检查反向交易
    result = checkReverseTrade(request);
    if (!result.passed) return result;

    return result;
}

RiskCheckResult RiskController::checkPosition(const PositionInfo& position)
{
    RiskCheckResult result;

    // 检查持仓盈亏是否超过单笔最大亏损
    const RiskRule& rule = d->defaultRule;

    if (position.profit < -rule.maxSingleLoss) {
        result.passed = false;
        result.ruleName = QStringLiteral("单笔最大亏损");
        result.message = QString("持仓 %1 亏损 %2 超过限制 %3")
            .arg(position.instrumentId)
            .arg(-position.profit)
            .arg(rule.maxSingleLoss);
        result.suggestion = QStringLiteral("建议及时止损");
        result.severity = 1;

        emit riskWarning(result.message);
    }

    return result;
}

RiskCheckResult RiskController::checkAccount(const AccountInfo& account)
{
    RiskCheckResult result;

    const RiskRule& rule = d->defaultRule;

    // 检查保证金比例
    double marginRatio = account.riskLevel();
    if (marginRatio > rule.maxMarginRatio) {
        result.passed = false;
        result.ruleName = QStringLiteral("保证金比例过高");
        result.message = QString("保证金比例 %1% 超过限制 %2%")
            .arg(marginRatio).arg(rule.maxMarginRatio);
        result.suggestion = QStringLiteral("建议减少持仓或追加保证金");
        result.severity = 2;

        emit riskError(result.message);
    }

    return result;
}

QVector<RiskCheckResult> RiskController::checkOrders(const QVector<OrderRequest>& requests)
{
    QVector<RiskCheckResult> results;
    for (const auto& request : requests) {
        results.append(checkOrder(request));
    }
    return results;
}

// ============================================================================
// 风险监控
// ============================================================================

int RiskController::getRiskLevel() const
{
    QMutexLocker locker(&d->mutex);
    return d->currentRiskLevel;
}

RiskController::RiskReport RiskController::generateReport() const
{
    QMutexLocker locker(&d->mutex);

    RiskReport report;
    report.reportTime = QDateTime::currentDateTime();

    // 计算各项风险
    report.positionRisk = calculatePositionRisk();
    report.marginRisk = calculateMarginRisk();
    report.drawdownRisk = calculateDrawdownRisk();
    report.leverageRisk = calculateLeverageRisk();

    // 总风险值（加权平均）
    report.totalRisk = report.positionRisk * 0.3 +
                       report.marginRisk * 0.3 +
                       report.drawdownRisk * 0.2 +
                       report.leverageRisk * 0.2;

    // 确定风险等级
    if (report.totalRisk > 80) {
        report.riskLevel = 2; // 高风险
        report.errors.append(QStringLiteral("风险等级过高，请立即降低仓位"));
    } else if (report.totalRisk > 50) {
        report.riskLevel = 1; // 中风险
        report.warnings.append(QStringLiteral("风险等级中等，请注意控制仓位"));
    } else {
        report.riskLevel = 0; // 低风险
    }

    // 添加建议
    if (report.marginRisk > 60) {
        report.suggestions.append(QStringLiteral("保证金占用过高，建议减仓"));
    }
    if (report.drawdownRisk > 50) {
        report.suggestions.append(QStringLiteral("回撤较大，建议控制风险敞口"));
    }
    if (report.leverageRisk > 70) {
        report.suggestions.append(QStringLiteral("杠杆过高，建议降低杠杆"));
    }

    return report;
}

void RiskController::setAccountInfo(const AccountInfo& account)
{
    QMutexLocker locker(&d->mutex);
    d->accountInfo = account;

    // 更新峰值和回撤
    double currentValue = account.totalAssets();
    if (currentValue > d->peakValue) {
        d->peakValue = currentValue;
    }

    if (d->peakValue > 0) {
        double drawdown = (d->peakValue - currentValue) / d->peakValue * 100.0;
        d->maxDrawdown = qMax(d->maxDrawdown, drawdown);
    }
}

void RiskController::setPositions(const QVector<PositionInfo>& positions)
{
    QMutexLocker locker(&d->mutex);
    d->positions = positions;
}

// ============================================================================
// 内部检查方法
// ============================================================================

RiskCheckResult RiskController::checkMaxPositionSize(const OrderRequest& request)
{
    RiskCheckResult result;
    const RiskRule& rule = d->defaultRule;

    // 计算新增持仓金额
    double newPositionValue = request.price * request.volume;

    // 获取当前总持仓金额
    double totalPositionValue = 0.0;
    for (const auto& pos : d->positions) {
        totalPositionValue += pos.marketValue;
    }

    // 检查是否超过限制
    if (totalPositionValue + newPositionValue > rule.maxPositionSize) {
        result.passed = false;
        result.ruleName = QStringLiteral("最大持仓金额");
        result.message = QString("持仓金额 %1 超过限制 %2")
            .arg(totalPositionValue + newPositionValue)
            .arg(rule.maxPositionSize);
        result.suggestion = QStringLiteral("建议减少持仓金额");
        result.severity = 1;
    }

    return result;
}

RiskCheckResult RiskController::checkMaxPositionCount(const OrderRequest& request)
{
    RiskCheckResult result;
    const RiskRule& rule = d->defaultRule;

    // 检查是否已有该合约持仓
    bool hasPosition = false;
    for (const auto& pos : d->positions) {
        if (pos.instrumentId == request.instrumentId) {
            hasPosition = true;
            break;
        }
    }

    // 如果没有持仓，检查持仓数量
    if (!hasPosition && d->positions.size() >= rule.maxPositionCount) {
        result.passed = false;
        result.ruleName = QStringLiteral("最大持仓数量");
        result.message = QString("持仓数量 %1 超过限制 %2")
            .arg(d->positions.size())
            .arg(rule.maxPositionCount);
        result.suggestion = QStringLiteral("建议减少持仓数量");
        result.severity = 1;
    }

    return result;
}

RiskCheckResult RiskController::checkMaxDailyLoss(const OrderRequest& request)
{
    RiskCheckResult result;
    const RiskRule& rule = d->defaultRule;

    // 获取今日盈亏
    double todayPnL = d->todayPnL;

    // 如果已亏损且超过限制
    if (todayPnL < -rule.maxDailyLoss) {
        result.passed = false;
        result.ruleName = QStringLiteral("日最大亏损");
        result.message = QString("今日亏损 %1 超过限制 %2")
            .arg(-todayPnL)
            .arg(rule.maxDailyLoss);
        result.suggestion = QStringLiteral("建议今日停止交易");
        result.severity = 2;
    }

    return result;
}

RiskCheckResult RiskController::checkMaxSingleLoss(const OrderRequest& request)
{
    RiskCheckResult result;
    const RiskRule& rule = d->defaultRule;

    // 检查现有持仓中是否有单笔亏损超过限制
    for (const auto& pos : d->positions) {
        if (pos.profit < -rule.maxSingleLoss) {
            result.passed = false;
            result.ruleName = QStringLiteral("单笔最大亏损");
            result.message = QString("持仓 %1 亏损 %2 超过限制 %3")
                .arg(pos.instrumentId)
                .arg(-pos.profit)
                .arg(rule.maxSingleLoss);
            result.suggestion = QStringLiteral("建议及时止损");
            result.severity = 1;
            break;
        }
    }

    return result;
}

RiskCheckResult RiskController::checkMaxLeverage(const OrderRequest& request)
{
    RiskCheckResult result;
    const RiskRule& rule = d->defaultRule;

    // 计算当前杠杆
    double totalValue = 0.0;
    double totalMargin = 0.0;

    for (const auto& pos : d->positions) {
        totalValue += pos.marketValue;
        totalMargin += pos.margin;
    }

    // 新订单的保证金（假设保证金比例10%）
    double newMargin = request.price * request.volume * 0.1;
    totalMargin += newMargin;

    // 计算杠杆
    double leverage = totalMargin > 0 ? totalValue / totalMargin : 0.0;

    if (leverage > rule.maxLeverage) {
        result.passed = false;
        result.ruleName = QStringLiteral("最大杠杆");
        result.message = QString("杠杆 %1倍 超过限制 %2倍")
            .arg(leverage).arg(rule.maxLeverage);
        result.suggestion = QStringLiteral("建议降低杠杆");
        result.severity = 1;
    }

    return result;
}

RiskCheckResult RiskController::checkMaxMarginRatio(const OrderRequest& request)
{
    RiskCheckResult result;
    const RiskRule& rule = d->defaultRule;

    // 计算当前保证金比例
    double totalMargin = d->accountInfo.margin;
    double balance = d->accountInfo.balance;

    // 新订单的保证金
    double newMargin = request.price * request.volume * 0.1;
    totalMargin += newMargin;

    double marginRatio = balance > 0 ? totalMargin / balance * 100.0 : 0.0;

    if (marginRatio > rule.maxMarginRatio) {
        result.passed = false;
        result.ruleName = QStringLiteral("最大保证金比例");
        result.message = QString("保证金比例 %1% 超过限制 %2%")
            .arg(marginRatio).arg(rule.maxMarginRatio);
        result.suggestion = QStringLiteral("建议追加保证金或减仓");
        result.severity = 2;
    }

    return result;
}

RiskCheckResult RiskController::checkMaxDrawdown(const OrderRequest& request)
{
    RiskCheckResult result;
    const RiskRule& rule = d->defaultRule;

    if (d->maxDrawdown > rule.maxDrawdown) {
        result.passed = false;
        result.ruleName = QStringLiteral("最大回撤");
        result.message = QString("回撤 %1% 超过限制 %2%")
            .arg(d->maxDrawdown).arg(rule.maxDrawdown);
        result.suggestion = QStringLiteral("建议暂停交易，重新评估策略");
        result.severity = 2;
    }

    return result;
}

RiskCheckResult RiskController::checkNightTrading(const OrderRequest& request)
{
    RiskCheckResult result;
    const RiskRule& rule = d->defaultRule;

    if (!rule.allowNightTrading) {
        // 检查是否是夜盘时间（21:00-02:30）
        QTime now = QTime::currentTime();
        bool isNight = now >= QTime(21, 0) || now <= QTime(2, 30);

        if (isNight) {
            result.passed = false;
            result.ruleName = QStringLiteral("夜盘交易限制");
            result.message = QStringLiteral("当前禁止夜盘交易");
            result.suggestion = QStringLiteral("请等待日盘开盘");
            result.severity = 1;
        }
    }

    return result;
}

RiskCheckResult RiskController::checkReverseTrade(const OrderRequest& request)
{
    RiskCheckResult result;
    const RiskRule& rule = d->defaultRule;

    if (!rule.allowReverseTrade) {
        // 检查是否有反向持仓
        for (const auto& pos : d->positions) {
            if (pos.instrumentId == request.instrumentId) {
                // 检查方向是否相反
                bool isReverse = (pos.direction == PositionDirection::Long && 
                                  request.direction == TradeDirection::Sell) ||
                                 (pos.direction == PositionDirection::Short && 
                                  request.direction == TradeDirection::Buy);

                if (isReverse && request.openClose == OpenCloseFlag::Open) {
                    result.passed = false;
                    result.ruleName = QStringLiteral("反向交易限制");
                    result.message = QStringLiteral("当前禁止反向开仓");
                    result.suggestion = QStringLiteral("请先平仓再反向开仓");
                    result.severity = 1;
                    break;
                }
            }
        }
    }

    return result;
}

// ============================================================================
// 风险计算
// ============================================================================

double RiskController::calculatePositionRisk() const
{
    // 基于持仓集中度计算风险
    if (d->positions.isEmpty()) {
        return 0.0;
    }

    double totalValue = 0.0;
    double maxValue = 0.0;

    for (const auto& pos : d->positions) {
        totalValue += pos.marketValue;
        maxValue = qMax(maxValue, pos.marketValue);
    }

    // 集中度风险
    double concentration = totalValue > 0 ? maxValue / totalValue * 100.0 : 0.0;
    return concentration;
}

double RiskController::calculateMarginRisk() const
{
    // 基于保证金占用比例计算风险
    if (d->accountInfo.balance <= 0) {
        return 0.0;
    }

    return d->accountInfo.margin / d->accountInfo.balance * 100.0;
}

double RiskController::calculateDrawdownRisk() const
{
    // 基于回撤计算风险
    return d->maxDrawdown;
}

double RiskController::calculateLeverageRisk() const
{
    // 基于杠杆计算风险
    double totalValue = 0.0;
    double totalMargin = 0.0;

    for (const auto& pos : d->positions) {
        totalValue += pos.marketValue;
        totalMargin += pos.margin;
    }

    if (totalMargin <= 0) {
        return 0.0;
    }

    double leverage = totalValue / totalMargin;
    return leverage * 20.0; // 转换为百分比
}

// ============================================================================
// 槽函数
// ============================================================================

void RiskController::onRiskCheckTimer()
{
    // 定期检查账户和持仓风险
    auto accountResult = checkAccount(d->accountInfo);
    if (!accountResult.passed) {
        emit ruleTriggered(accountResult.ruleName, accountResult.message);
    }

    for (const auto& position : d->positions) {
        auto posResult = checkPosition(position);
        if (!posResult.passed) {
            emit ruleTriggered(posResult.ruleName, posResult.message);
        }
    }

    // 更新风险等级
    auto report = generateReport();
    if (report.riskLevel != d->currentRiskLevel) {
        d->currentRiskLevel = report.riskLevel;
        emit riskLevelChanged(report.riskLevel);
    }
}
