/**
 * @file RiskAnalyzer.h
 * @brief 风险分析器 - 持仓风险评估
 *
 * @details 功能：
 * - 持仓风险分析
 * - VaR 计算
 * - 风险预警
 * - 风险报告
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef RISKANALYZER_H
#define RISKANALYZER_H

#include <QDateTime>
#include <QObject>
#include <QVector>
#include <QString>
#include <QHash>

/**
 * @brief 持仓信息（风险分析）
 */
struct RiskPositionInfo {
    QString symbol;         ///< 股票代码
    QString name;           ///< 股票名称
    int quantity = 0;       ///< 持仓数量
    double cost = 0.0;      ///< 成本价
    double currentPrice = 0.0; ///< 当前价
    double marketValue = 0.0;  ///< 市值
    double profit = 0.0;    ///< 盈亏
    double profitPercent = 0.0; ///< 盈亏比例
    double weight = 0.0;    ///< 权重
};

/**
 * @brief 风险指标
 */
struct RiskMetrics {
    double totalValue = 0.0;        ///< 总市值
    double totalProfit = 0.0;       ///< 总盈亏
    double totalProfitPercent = 0.0; ///< 总盈亏比例
    double maxDrawdown = 0.0;       ///< 最大回撤
    double volatility = 0.0;        ///< 波动率
    double sharpeRatio = 0.0;       ///< 夏普比率
    double var95 = 0.0;             ///< VaR 95%
    double var99 = 0.0;             ///< VaR 99%
    double beta = 0.0;              ///< Beta 系数
    double concentrationRisk = 0.0; ///< 集中度风险
    int positionCount = 0;          ///< 持仓数量
};

/**
 * @brief 风险预警（分析模块）
 */
struct RiskAnalyzerAlert {
    QString type;           ///< 预警类型
    QString level;          ///< 预警级别 (info/warning/danger)
    QString message;        ///< 预警消息
    QString symbol;         ///< 相关股票
    double value = 0.0;     ///< 相关值
    QDateTime time;         ///< 时间
};

/**
 * @brief 风险分析器
 *
 * 提供全面的风险分析：
 * - 持仓风险
 * - VaR 计算
 * - 风险预警
 * - 风险报告
 */
class RiskAnalyzer : public QObject {
    Q_OBJECT

public:
    static RiskAnalyzer* instance();

    /**
     * @brief 设置持仓数据
     */
    void setPositions(const QVector<PositionInfo>& positions);

    /**
     * @brief 添加持仓
     */
    void addPosition(const PositionInfo& position);

    /**
     * @brief 清除持仓
     */
    void clearPositions();

    /**
     * @brief 计算风险指标
     */
    RiskMetrics calculateRiskMetrics();

    /**
     * @brief 计算 VaR
     * @param confidence 置信度 (0.95 或 0.99)
     * @param days 持有天数
     */
    double calculateVaR(double confidence = 0.95, int days = 1);

    /**
     * @brief 计算最大回撤
     */
    double calculateMaxDrawdown();

    /**
     * @brief 计算波动率
     */
    double calculateVolatility();

    /**
     * @brief 计算集中度风险
     */
    double calculateConcentrationRisk();

    /**
     * @brief 检查风险预警
     */
    QVector<RiskAlert> checkRiskAlerts();

    /**
     * @brief 生成风险报告
     */
    QString generateRiskReport();

    /**
     * @brief 设置风险阈值
     */
    void setRiskThresholds(double maxDrawdownLimit = 0.2,
                          double concentrationLimit = 0.3,
                          double varLimit = 0.1);

signals:
    /**
     * @brief 风险预警信号
     */
    void riskAlert(const RiskAlert& alert);

    /**
     * @brief 风险指标更新
     */
    void metricsUpdated(const RiskMetrics& metrics);

private:
    explicit RiskAnalyzer(QObject* parent = nullptr);
    ~RiskAnalyzer() override = default;

    QVector<double> getHistoricalReturns(const QString& symbol);
    double calculateBeta(const QString& symbol);

    QVector<RiskPositionInfo> m_positions;
    RiskMetrics m_metrics;

    // 风险阈值
    double m_maxDrawdownLimit = 0.2;
    double m_concentrationLimit = 0.3;
    double m_varLimit = 0.1;
};

#endif // RISKANALYZER_H