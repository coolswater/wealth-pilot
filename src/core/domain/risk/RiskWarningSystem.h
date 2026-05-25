/**
 * @file RiskWarningSystem.h
 * @brief 风险预警系统 - 实时监控投资风险
 *
 * @details 提供全面的风险管理功能：
 * - 实时风险监控
 * - 风险阈值设置
 * - 风险预警通知
 * - 风险历史记录
 * - 风险报告生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef RISKWARNINGSYSTEM_H
#define RISKWARNINGSYSTEM_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <QTimer>
#include "data/market/StockDataSource.h"

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::StockQuote;

/**
 * @brief 风险等级
 */
enum class RiskLevel {
    Low = 0,        ///< 低风险
    Medium = 1,     ///< 中风险
    High = 2,       ///< 高风险
    Critical = 3    ///< 极高风险
};

/**
 * @brief 风险类型
 */
enum class RiskType {
    PriceDrop,          ///< 价格下跌
    VolumeSpike,        ///< 成交量异常
    VolatilityHigh,     ///< 波动率过高
    DrawdownExceed,     ///< 回撤超标
    ConcentrationRisk,  ///< 持仓集中风险
    LiquidityRisk,      ///< 流动性风险
    TrendReversal       ///< 趋势反转
};

/**
 * @brief 风险预警记录
 */
struct RiskAlert {
    QString id;                     ///< 预警ID
    QString symbol;                 ///< 股票代码
    RiskType type;                  ///< 风险类型
    RiskLevel level;                ///< 风险等级
    double value = 0.0;             ///< 风险值
    double threshold = 0.0;         ///< 阈值
    QString description;            ///< 描述
    QDateTime timestamp;            ///< 时间戳
    bool acknowledged = false;      ///< 是否已确认
    QString suggestion;             ///< 建议措施
};

/**
 * @brief 风险阈值配置
 */
struct RiskThreshold {
    double maxLossPercent = 10.0;           ///< 最大亏损百分比
    double maxDrawdownPercent = 20.0;       ///< 最大回撤百分比
    double maxVolatility = 50.0;            ///< 最大波动率
    double maxPositionConcentration = 30.0; ///< 最大持仓集中度
    double volumeSpikeMultiplier = 3.0;     ///< 成交量异常倍数
    int priceDropConsecutiveDays = 3;       ///< 连续下跌天数
};

/**
 * @brief 风险统计
 */
struct RiskStatistics {
    int totalAlerts = 0;            ///< 总预警次数
    int criticalAlerts = 0;         ///< 极高风险次数
    int highAlerts = 0;             ///< 高风险次数
    int mediumAlerts = 0;           ///< 中风险次数
    int lowAlerts = 0;              ///< 低风险次数
    double avgRiskScore = 0.0;      ///< 平均风险分数
    QDateTime lastUpdateTime;       ///< 最后更新时间
};

/**
 * @brief 风险预警系统
 */
class RiskWarningSystem : public QObject
{
    Q_OBJECT

public:
    static RiskWarningSystem* instance();

    /**
     * @brief 初始化系统
     */
    bool initialize();

    /**
     * @brief 设置风险阈值
     */
    void setRiskThreshold(const RiskThreshold& threshold);
    RiskThreshold riskThreshold() const { return m_threshold; }

    /**
     * @brief 监控股票风险
     */
    void monitorSymbol(const QString& symbol);
    void stopMonitoring(const QString& symbol);
    QVector<QString> monitoredSymbols() const;

    /**
     * @brief 手动触发风险评估
     */
    void assessRisk(const QString& symbol);

    /**
     * @brief 获取风险预警
     */
    QVector<RiskAlert> getAlerts(const QString& symbol = QString()) const;
    QVector<RiskAlert> getUnacknowledgedAlerts() const;

    /**
     * @brief 确认预警
     */
    void acknowledgeAlert(const QString& alertId);

    /**
     * @brief 获取风险统计
     */
    RiskStatistics getStatistics() const;

    /**
     * @brief 生成风险报告
     */
    QString generateRiskReport(const QString& symbol = QString());

    /**
     * @brief 获取风险建议
     */
    QString getRiskSuggestion(RiskType type, RiskLevel level);

signals:
    /**
     * @brief 风险预警信号
     */
    void riskAlertTriggered(const RiskAlert& alert);

    /**
     * @brief 风险等级变化
     */
    void riskLevelChanged(const QString& symbol, RiskLevel level);

    /**
     * @brief 风险统计更新
     */
    void statisticsUpdated(const RiskStatistics& stats);

private slots:
    void onMarketDataUpdated(const QString& symbol, const StockQuote& quote);
    void onPeriodicCheck();

private:
    explicit RiskWarningSystem(QObject* parent = nullptr);
    ~RiskWarningSystem() override;

    // 风险评估方法
    RiskLevel assessPriceDropRisk(const QString& symbol, const StockQuote& quote);
    RiskLevel assessVolumeRisk(const QString& symbol, const StockQuote& quote);
    RiskLevel assessVolatilityRisk(const QString& symbol);
    RiskLevel assessDrawdownRisk(const QString& symbol);
    RiskLevel assessConcentrationRisk(const QString& symbol);

    // 风险计算
    double calculateVolatility(const QString& symbol);
    double calculateDrawdown(const QString& symbol);
    double calculatePositionConcentration(const QString& symbol);

    // 预警生成
    void generateAlert(const QString& symbol, RiskType type, RiskLevel level,
                      double value, double threshold);

    // 辅助方法
    QString riskLevelToString(RiskLevel level) const;
    QString riskTypeToString(RiskType type) const;
    RiskLevel calculateOverallRisk(const QString& symbol);

    // 数据成员
    RiskThreshold m_threshold;
    QMap<QString, QVector<RiskAlert>> m_alerts;  // symbol -> alerts
    QMap<QString, StockQuote> m_latestQuotes;
    QMap<QString, QVector<double>> m_priceHistory;
    QMap<QString, RiskLevel> m_currentRiskLevel;
    QTimer* m_checkTimer = nullptr;
    RiskStatistics m_statistics;

    bool m_initialized = false;
};

#endif // RISKWARNINGSYSTEM_H
