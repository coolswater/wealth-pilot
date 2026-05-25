/**
 * @file PortfolioOptimizer.h
 * @brief 投资组合优化 - 资产配置与风险收益分析
 *
 * @details 提供投资组合优化功能：
 * - 资产配置建议
 * - 风险收益分析
 * - 组合优化算法
 * - 回测验证
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PORTFOLIOOPTIMIZER_H
#define PORTFOLIOOPTIMIZER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include "core/domain/recommendation/PersonalizedRecommendation.h"  // 包含InvestmentStyle定义

/**
 * @brief 资产类型
 */
enum class AssetType {
    Stock,          ///< 股票
    Bond,           ///< 债券
    Fund,           ///< 基金
    Cash,           ///< 现金
    Crypto          ///< 数字货币
};

/**
 * @brief 组合资产配置
 */
struct PortfolioAssetAllocation {
    QString symbol;         ///< 资产代码
    AssetType type;         ///< 资产类型
    double weight = 0.0;    ///< 权重（百分比）
    double amount = 0.0;    ///< 金额
    QString reason;         ///< 配置原因
};

/**
 * @brief 组合风险指标
 */
struct PortfolioRiskMetrics {
    double volatility = 0.0;        ///< 波动率
    double maxDrawdown = 0.0;        ///< 最大回撤
    double sharpeRatio = 0.0;       ///< 夏普比率
    double beta = 0.0;              ///< Beta系数
    double var95 = 0.0;             ///< 95% VaR
    double expectedShortfall = 0.0;  ///< 预期损失
};

/**
 * @brief 组合收益指标
 */
struct PortfolioReturnMetrics {
    double totalReturn = 0.0;       ///< 总收益
    double annualizedReturn = 0.0;  ///< 年化收益
    double dividendYield = 0.0;     ///< 股息率
    double alpha = 0.0;             ///< Alpha
};

/**
 * @brief 投资组合
 */
struct Portfolio {
    QString id;                             ///< 组合ID
    QString name;                           ///< 组合名称
    QString description;                    ///< 描述
    QVector<PortfolioAssetAllocation> allocations;   ///< 资产配置
    PortfolioRiskMetrics riskMetrics;       ///< 风险指标
    PortfolioReturnMetrics returnMetrics;   ///< 收益指标
    QDateTime createTime;                   ///< 创建时间
    QDateTime updateTime;                   ///< 更新时间
    double totalValue = 0.0;                ///< 总价值
};

/**
 * @brief 优化目标
 */
enum class OptimizationObjective {
    MaxReturn,          ///< 最大收益
    MinRisk,            ///< 最小风险
    MaxSharpeRatio,     ///< 最大夏普比率
    RiskParity          ///< 风险平价
};

/**
 * @brief 约束条件
 */
struct OptimizationConstraint {
    double minWeight = 0.0;         ///< 最小权重
    double maxWeight = 100.0;       ///< 最大权重
    int maxAssets = 20;             ///< 最大资产数量
    double targetReturn = 0.0;      ///< 目标收益
    double maxRisk = 100.0;         ///< 最大风险
};

/**
 * @brief 组合回测结果
 */
struct PortfolioBacktestResult {
    double totalReturn = 0.0;       ///< 总收益
    double annualizedReturn = 0.0;  ///< 年化收益
    double maxDrawdown = 0.0;       ///< 最大回撤
    double sharpeRatio = 0.0;       ///< 夏普比率
    double winRate = 0.0;           ///< 胜率
    int totalTrades = 0;            ///< 总交易次数
    QVector<double> dailyReturns;   ///< 每日收益
    QVector<double> cumulativeReturns; ///< 累计收益
};

/**
 * @brief 投资组合优化器
 */
class PortfolioOptimizer : public QObject
{
    Q_OBJECT

public:
    static PortfolioOptimizer* instance();

    /**
     * @brief 初始化优化器
     */
    bool initialize();

    /**
     * @brief 创建投资组合
     */
    Portfolio createPortfolio(const QString& name, const QString& description = QString());

    /**
     * @brief 优化投资组合
     */
    Portfolio optimize(const QVector<QString>& assets,
                      OptimizationObjective objective,
                      const OptimizationConstraint& constraint);

    /**
     * @brief 计算风险指标
     */
    PortfolioRiskMetrics calculateRiskMetrics(const Portfolio& portfolio);

    /**
     * @brief 计算收益指标
     */
    PortfolioReturnMetrics calculateReturnMetrics(const Portfolio& portfolio);

    /**
     * @brief 回测投资组合
     */
    PortfolioBacktestResult backtest(const Portfolio& portfolio,
                           const QDateTime& startDate,
                           const QDateTime& endDate);

    /**
     * @brief 获取资产配置建议
     */
    QVector<PortfolioAssetAllocation> getAllocationSuggestion(double totalAmount,
                                                    InvestmentStyle style);

    /**
     * @brief 再平衡建议
     */
    QVector<PortfolioAssetAllocation> rebalanceSuggestion(const Portfolio& current,
                                                const Portfolio& target);

signals:
    void optimizationCompleted(const Portfolio& portfolio);
    void backtestCompleted(const PortfolioBacktestResult& result);

private:
    explicit PortfolioOptimizer(QObject* parent = nullptr);
    ~PortfolioOptimizer() override = default;

    // 优化算法
    Portfolio optimizeMaxReturn(const QVector<QString>& assets, const OptimizationConstraint& constraint);
    Portfolio optimizeMinRisk(const QVector<QString>& assets, const OptimizationConstraint& constraint);
    Portfolio optimizeMaxSharpe(const QVector<QString>& assets, const OptimizationConstraint& constraint);
    Portfolio optimizeRiskParity(const QVector<QString>& assets, const OptimizationConstraint& constraint);

    // 辅助方法
    double calculateCovariance(const QString& asset1, const QString& asset2);
    double calculateVariance(const QString& asset);
    QVector<double> getHistoricalReturns(const QString& asset, int days = 252);

    QString generatePortfolioId() const;

    // 数据成员
    QMap<QString, Portfolio> m_portfolios;
    bool m_initialized = false;
};

#endif // PORTFOLIOOPTIMIZER_H
