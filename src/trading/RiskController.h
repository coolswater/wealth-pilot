/**
 * @file RiskController.h
 * @brief 风控系统 - 交易风险控制
 *
 * @details 功能：
 * - 交易前风控检查
 * - 持仓风险监控
 * - 资金风险预警
 * - 风控规则管理
 *
 * @details 风控规则：
 * - 最大持仓限制
 * - 日最大亏损限制
 * - 单笔最大亏损限制
 * - 最大杠杆限制
 * - 最大回撤限制
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef RISKCONTROLLER_H
#define RISKCONTROLLER_H

#include <QObject>
#include <QHash>
#include <QVector>
#include <QTimer>
#include <QMutex>
#include <memory>
#include "TradingTypes.h"

/**
 * @brief 风控检查结果
 */
struct RiskCheckResult {
    bool passed = true;             ///< 是否通过
    QString ruleName;               ///< 触发的规则名称
    QString message;                ///< 提示信息
    QString suggestion;             ///< 建议操作
    int severity = 0;               ///< 严重程度（0-警告，1-错误，2-严重）
};

/**
 * @brief 风控系统
 * @details 单例模式，统一管理所有风控规则
 * 
 * @example
 * @code
 * // 检查订单
 * OrderRequest request;
 * RiskCheckResult result = RiskController::instance().checkOrder(request);
 * if (!result.passed) {
 *     QMessageBox::warning(this, "风控警告", result.message);
 * }
 * @endcode
 */
class RiskController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static RiskController& instance();

    /**
     * @brief 初始化风控系统
     */
    bool initialize();

    /**
     * @brief 关闭风控系统
     */
    void shutdown();

private:
    /**
     * @brief 加载默认风控规则
     */
    void loadDefaultRules();

public:
    /**
     * @brief 添加风控规则
     * @param rule 风控规则
     */
    void addRule(const RiskRule& rule);

    /**
     * @brief 更新风控规则
     * @param ruleId 规则ID
     * @param rule 新的规则
     */
    bool updateRule(const QString& ruleId, const RiskRule& rule);

    /**
     * @brief 删除风控规则
     * @param ruleId 规则ID
     */
    bool removeRule(const QString& ruleId);

    /**
     * @brief 启用/禁用规则
     * @param ruleId 规则ID
     * @param enabled 是否启用
     */
    void setRuleEnabled(const QString& ruleId, bool enabled);

    /**
     * @brief 获取所有规则
     */
    QVector<RiskRule> getRules() const;

    /**
     * @brief 获取规则
     * @param ruleId 规则ID
     */
    std::optional<RiskRule> getRule(const QString& ruleId) const;

    // ========== 风控检查 ==========

    /**
     * @brief 检查订单（交易前风控）
     * @param request 订单请求
     * @return 检查结果
     */
    RiskCheckResult checkOrder(const OrderRequest& request);

    /**
     * @brief 检查持仓
     * @param position 持仓信息
     * @return 检查结果
     */
    RiskCheckResult checkPosition(const PositionInfo& position);

    /**
     * @brief 检查账户
     * @param account 账户信息
     * @return 检查结果
     */
    RiskCheckResult checkAccount(const AccountInfo& account);

    /**
     * @brief 批量检查
     * @param requests 订单请求列表
     * @return 检查结果列表
     */
    QVector<RiskCheckResult> checkOrders(const QVector<OrderRequest>& requests);

    // ========== 风险监控 ==========

    /**
     * @brief 获取当前风险等级
     * @return 0-低风险，1-中风险，2-高风险
     */
    int getRiskLevel() const;

    /**
     * @brief 获取风险报告
     */
    struct RiskReport {
        int riskLevel = 0;              ///< 风险等级
        double totalRisk = 0.0;         ///< 总风险值
        double positionRisk = 0.0;      ///< 持仓风险
        double marginRisk = 0.0;        ///< 保证金风险
        double drawdownRisk = 0.0;      ///< 回撤风险
        double leverageRisk = 0.0;      ///< 杠杆风险
        
        QVector<QString> warnings;      ///< 警告信息
        QVector<QString> errors;        ///< 错误信息
        QVector<QString> suggestions;   ///< 建议信息
        
        QDateTime reportTime;           ///< 报告时间
    };
    RiskReport generateReport() const;

    /**
     * @brief 设置账户信息（用于风控计算）
     */
    void setAccountInfo(const AccountInfo& account);

    /**
     * @brief 设置持仓信息（用于风控计算）
     */
    void setPositions(const QVector<PositionInfo>& positions);

signals:
    /**
     * @brief 风控警告
     * @param warning 警告信息
     */
    void riskWarning(const QString& warning);

    /**
     * @brief 风控错误
     * @param error 错误信息
     */
    void riskError(const QString& error);

    /**
     * @brief 风险等级变化
     * @param level 新的风险等级
     */
    void riskLevelChanged(int level);

    /**
     * @brief 规则触发
     * @param ruleId 规则ID
     * @param message 触发信息
     */
    void ruleTriggered(const QString& ruleId, const QString& message);

private slots:
    /**
     * @brief 定期风险检查
     */
    void onRiskCheckTimer();

private:
    // 私有构造函数（单例）
    RiskController(QObject* parent = nullptr);
    ~RiskController() override;
    Q_DISABLE_COPY(RiskController)

    // 内部检查方法
    RiskCheckResult checkMaxPositionSize(const OrderRequest& request);
    RiskCheckResult checkMaxPositionCount(const OrderRequest& request);
    RiskCheckResult checkMaxDailyLoss(const OrderRequest& request);
    RiskCheckResult checkMaxSingleLoss(const OrderRequest& request);
    RiskCheckResult checkMaxLeverage(const OrderRequest& request);
    RiskCheckResult checkMaxMarginRatio(const OrderRequest& request);
    RiskCheckResult checkMaxDrawdown(const OrderRequest& request);
    RiskCheckResult checkNightTrading(const OrderRequest& request);
    RiskCheckResult checkReverseTrade(const OrderRequest& request);

    // 计算风险值
    double calculatePositionRisk() const;
    double calculateMarginRisk() const;
    double calculateDrawdownRisk() const;
    double calculateLeverageRisk() const;

    // PIMPL 实现
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // RISKCONTROLLER_H
