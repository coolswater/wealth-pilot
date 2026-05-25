/**
 * @file PersonalizedRecommendation.h
 * @brief 个性化推荐系统 - 智能投资建议
 *
 * @details 提供个性化投资建议：
 * - 用户偏好分析
 * - 个性化股票推荐
 * - 投资组合建议
 * - 智能提醒
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef PERSONALIZEDRECOMMENDATION_H
#define PERSONALIZEDRECOMMENDATION_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include "data/market/StockDataSource.h"
#include "core/risk/RiskWarningSystem.h"  // 包含RiskLevel定义

/**
 * @brief 用户偏好类型
 */
enum class PreferenceType {
    RiskTolerance,      ///< 风险承受能力
    InvestmentStyle,    ///< 投资风格
    IndustryPreference, ///< 行业偏好
    MarketCapPreference,///< 市值偏好
    TradingFrequency    ///< 交易频率
};

/**
 * @brief 投资风格
 */
enum class InvestmentStyle {
    Conservative,       ///< 保守型
    Balanced,           ///< 平衡型
    Aggressive          ///< 进取型
};

/**
 * @brief 推荐理由
 */
struct RecommendationReason {
    QString factor;     ///< 推荐因素
    double weight = 0.0;///< 权重
    QString description;///< 描述
};

/**
 * @brief 股票推荐
 */
struct StockRecommendation {
    QString symbol;                     ///< 股票代码
    QString name;                       ///< 股票名称
    double score = 0.0;                 ///< 推荐分数（0-100）
    RiskLevel riskLevel;                ///< 风险等级
    QVector<RecommendationReason> reasons; ///< 推荐理由
    QString suggestion;                 ///< 投资建议
    QDateTime timestamp;                ///< 推荐时间
    int expireDays = 7;                 ///< 有效期（天）
};

/**
 * @brief 投资组合建议
 */
struct PortfolioSuggestion {
    QString name;                       ///< 组合名称
    QString description;                ///< 组合描述
    QMap<QString, double> allocations;  ///< 资产配置（symbol -> 权重）
    double expectedReturn = 0.0;        ///< 预期收益
    double riskScore = 0.0;             ///< 风险分数
    QString strategy;                   ///< 投资策略
    QVector<QString> advantages;        ///< 优势
    QVector<QString> risks;             ///< 风险提示
};

/**
 * @brief 用户偏好配置
 */
struct UserPreference {
    InvestmentStyle style = InvestmentStyle::Balanced;
    double riskTolerance = 50.0;        ///< 风险承受能力（0-100）
    QVector<QString> preferredIndustries; ///< 倾向行业
    QString marketCapRange;             ///< 市值范围（大盘/中盘/小盘）
    int tradingFrequency = 2;           ///< 交易频率（1=低频，2=中频，3=高频）
    double maxPositionPercent = 20.0;   ///< 单只股票最大仓位
    int holdingPeriodDays = 30;         ///< 持仓周期（天）
};

/**
 * @brief 个性化推荐系统
 */
class PersonalizedRecommendation : public QObject
{
    Q_OBJECT

public:
    static PersonalizedRecommendation* instance();

    /**
     * @brief 初始化系统
     */
    bool initialize();

    /**
     * @brief 设置用户偏好
     */
    void setUserPreference(const UserPreference& preference);
    UserPreference userPreference() const { return m_preference; }

    /**
     * @brief 分析用户偏好
     */
    void analyzePreferenceFromHistory();

    /**
     * @brief 获取个性化推荐
     */
    QVector<StockRecommendation> getRecommendations(int count = 10);

    /**
     * @brief 获取投资组合建议
     */
    QVector<PortfolioSuggestion> getPortfolioSuggestions();

    /**
     * @brief 获取智能提醒
     */
    QString getSmartReminder();

    /**
     * @brief 更新推荐分数
     */
    void updateRecommendationScores();

signals:
    void recommendationsUpdated(const QVector<StockRecommendation>& recommendations);
    void portfolioSuggestionReady(const PortfolioSuggestion& suggestion);

private:
    explicit PersonalizedRecommendation(QObject* parent = nullptr);
    ~PersonalizedRecommendation() override = default;

    // 推荐算法
    double calculateRecommendationScore(const QString& symbol);
    QVector<RecommendationReason> generateReasons(const QString& symbol, double score);
    QString generateSuggestion(const QString& symbol, double score, RiskLevel risk);

    // 组合优化
    PortfolioSuggestion optimizePortfolio(const QVector<QString>& symbols);
    double calculateExpectedReturn(const QMap<QString, double>& allocations);
    double calculateRiskScore(const QMap<QString, double>& allocations);

    // 辅助方法
    InvestmentStyle determineStyleFromHistory();
    QVector<QString> filterByPreference(const QVector<QString>& candidates);
    QString styleToString(InvestmentStyle style) const;
    QString getStockIndustry(const QString& symbol);
    QString getStockName(const QString& symbol);
    double getStockMarketCap(const QString& symbol);

    // 数据成员
    UserPreference m_preference;
    QMap<QString, double> m_recommendationScores;
    QVector<StockRecommendation> m_currentRecommendations;
    QVector<PortfolioSuggestion> m_portfolioSuggestions;

    bool m_initialized = false;
};

#endif // PERSONALIZEDRECOMMENDATION_H