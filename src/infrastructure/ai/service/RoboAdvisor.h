/**
 * @file RoboAdvisor.h
 * @brief 智能投顾服务
 *
 * @details 功能：
 * - 用户画像管理
 * - 风险评估
 * - 投资组合建议
 * - 资产配置建议
 * - 定投计划生成
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ROBOADVISOR_H
#define ROBOADVISOR_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QDateTime>
#include <functional>

namespace WealthPilot
{
    /**
 * @brief 投资目标枚举
 */
    enum class InvestmentGoal
    {
        CapitalPreservation, ///< 资产保值
        SteadyGrowth, ///< 稳健增值
        BalancedGrowth, ///< 平衡增长
        AggressiveGrowth, ///< 积极增长
        HighReturn ///< 高收益
    };

    /**
 * @brief 风险等级枚举
 */
    enum class RiskLevel
    {
        VeryLow = 1, ///< 极低风险
        Low = 2, ///< 低风险
        Medium = 3, ///< 中等风险
        High = 4, ///< 高风险
        VeryHigh = 5 ///< 极高风险
    };

    /**
 * @brief 用户画像结构
 */
    struct UserProfile
    {
        QString userId; ///< 用户 ID
        QString nickname; ///< 昵称

        // 风险偏好
        int riskTolerance = 5; ///< 风险承受能力 (1-10)
        RiskLevel riskLevel = RiskLevel::Medium; ///< 风险等级

        // 投资期限
        int investmentHorizon = 12; ///< 投资期限（月）
        bool isShortTerm = false; ///< 短期投资
        bool isMediumTerm = true; ///< 中期投资
        bool isLongTerm = false; ///< 长期投资

        // 资产状况
        double totalAssets = 0; ///< 总资产
        double investableAssets = 0; ///< 可投资资产
        double monthlyIncome = 0; ///< 月收入
        double monthlyExpense = 0; ///< 月支出

        // 投资偏好
        InvestmentGoal goal = InvestmentGoal::BalancedGrowth; ///< 投资目标
        QStringList preferredSectors; ///< 偏好板块
        QStringList excludedStocks; ///< 排除股票
        bool preferDividend = false; ///< 偏好分红
        bool preferGrowth = true; ///< 偏好成长

        // 投资经验
        int investmentExperience = 0; ///< 投资经验（年）
        bool hasStockExperience = false; ///< 股票经验
        bool hasFundExperience = false; ///< 基金经验
        bool hasFuturesExperience = false; ///< 期货经验

        // 时间戳
        QDateTime createdAt; ///< 创建时间
        QDateTime updatedAt; ///< 更新时间

        /**
     * @brief 从 JSON 解析
     */
        static UserProfile fromJson(const QJsonObject& json);

        /**
     * @brief 转换为 JSON
     */
        QJsonObject toJson() const;

        /**
     * @brief 计算风险等级
     */
        RiskLevel calculateRiskLevel() const;
    };

    /**
 * @brief 风险评估问题
 */
    struct RiskQuestion
    {
        QString id; ///< 问题 ID
        QString text; ///< 问题文本
        QStringList options; ///< 选项列表
        QList<int> scores; ///< 各选项分数
        int selectedOption = -1; ///< 选中选项
    };

    /**
 * @brief 风险评估结果
 */
    struct RiskAssessmentResult
    {
        int totalScore = 0; ///< 总分
        RiskLevel riskLevel = RiskLevel::Medium; ///< 风险等级
        QString riskDescription; ///< 风险描述
        QStringList recommendations; ///< 建议
        QDateTime assessedAt; ///< 评估时间

        /**
     * @brief 从 JSON 解析
     */
        static RiskAssessmentResult fromJson(const QJsonObject& json);

        /**
     * @brief 转换为 JSON
     */
        QJsonObject toJson() const;
    };

    /**
 * @brief 资产配置建议
 */
    struct AssetAllocation
    {
        QString assetClass; ///< 资产类别
        double percentage; ///< 配置比例
        double amount; ///< 配置金额
        QString reason; ///< 配置原因
        QStringList suggestedProducts; ///< 推荐产品

        /**
     * @brief 从 JSON 解析
     */
        static AssetAllocation fromJson(const QJsonObject& json);

        /**
     * @brief 转换为 JSON
     */
        QJsonObject toJson() const;
    };

    /**
 * @brief 投资建议
 */
    struct InvestmentAdvice
    {
        QString id; ///< 建议 ID
        QString title; ///< 标题
        QString summary; ///< 摘要
        QString detailedAnalysis; ///< 详细分析
        QList<AssetAllocation> allocations; ///< 资产配置
        QStringList actionItems; ///< 行动项
        QStringList riskWarnings; ///< 风险提示
        QDateTime generatedAt; ///< 生成时间
        double confidence = 0.0; ///< 置信度

        /**
     * @brief 从 JSON 解析
     */
        static InvestmentAdvice fromJson(const QJsonObject& json);

        /**
     * @brief 转换为 JSON
     */
        QJsonObject toJson() const;
    };

    /**
 * @brief 定投计划
 */
    struct SIPPlan
    {
        QString id; ///< 计划 ID
        QString name; ///< 计划名称
        double monthlyAmount; ///< 月投金额
        int totalMonths; ///< 总月数
        QList<AssetAllocation> allocations; ///< 资产配置
        double expectedReturn; ///< 预期收益
        double expectedRisk; ///< 预期风险
        QDateTime startDate; ///< 开始日期
        QDateTime createdAt; ///< 创建时间

        /**
     * @brief 从 JSON 解析
     */
        static SIPPlan fromJson(const QJsonObject& json);

        /**
     * @brief 转换为 JSON
     */
        QJsonObject toJson() const;
    };

    /**
 * @brief 智能投顾服务
 *
 * @details 提供智能投资建议服务：
 * - 用户画像管理
 * - 风险评估
 * - 投资组合建议
 * - 资产配置建议
 * - 定投计划生成
 */
    class RoboAdvisor : public QObject
    {
        Q_OBJECT

    public:
        /**
     * @brief 构造函数
     * @param parent 父对象
     */
        explicit RoboAdvisor(QObject* parent = nullptr);

        /**
     * @brief 析构函数
     */
        ~RoboAdvisor() override;

        // ========== 用户画像管理 ==========

        /**
     * @brief 创建用户画像
     * @param userId 用户 ID
     * @return 用户画像
     */
        UserProfile createUserProfile(const QString& userId);

        /**
     * @brief 获取用户画像
     * @param userId 用户 ID
     * @return 用户画像
     */
        UserProfile getUserProfile(const QString& userId) const;

        /**
     * @brief 更新用户画像
     * @param profile 用户画像
     * @return 是否成功
     */
        bool updateUserProfile(const UserProfile& profile);

        /**
     * @brief 保存用户画像
     * @param profile 用户画像
     * @return 是否成功
     */
        bool saveUserProfile(const UserProfile& profile);

        // ========== 风险评估 ==========

        /**
     * @brief 获取风险评估问卷
     * @return 问题列表
     */
        QList<RiskQuestion> getRiskQuestionnaire() const;

        /**
     * @brief 提交风险评估答案
     * @param userId 用户 ID
     * @param answers 答案（问题ID -> 选项索引）
     * @return 评估结果
     */
        RiskAssessmentResult submitRiskAssessment(
            const QString& userId,
            const QMap<QString, int>& answers);

        /**
     * @brief 获取风险评估结果
     * @param userId 用户 ID
     * @return 评估结果
     */
        RiskAssessmentResult getRiskAssessment(const QString& userId) const;

        // ========== 投资建议 ==========

        /**
     * @brief 获取投资建议
     * @param userId 用户 ID
     * @param callback 回调函数
     */
        void getInvestmentAdvice(
            const QString& userId,
            std::function<void(const InvestmentAdvice&)> callback);

        /**
     * @brief 获取投资建议（同步）
     * @param userId 用户 ID
     * @return 投资建议
     */
        InvestmentAdvice getInvestmentAdviceSync(const QString& userId);

        /**
     * @brief 获取资产配置建议
     * @param userId 用户 ID
     * @param totalAmount 总金额
     * @return 资产配置列表
     */
        QList<AssetAllocation> getAssetAllocation(
            const QString& userId,
            double totalAmount);

        // ========== 定投计划 ==========

        /**
     * @brief 生成定投计划
     * @param userId 用户 ID
     * @param monthlyAmount 月投金额
     * @param totalMonths 总月数
     * @return 定投计划
     */
        SIPPlan generateSIPPlan(
            const QString& userId,
            double monthlyAmount,
            int totalMonths);

        /**
     * @brief 获取定投计划列表
     * @param userId 用户 ID
     * @return 定投计划列表
     */
        QList<SIPPlan> getSIPPlans(const QString& userId) const;

        // ========== 工具方法 ==========

        /**
     * @brief 计算风险等级
     * @param score 分数
     * @return 风险等级
     */
        static RiskLevel calculateRiskLevel(int score);

        /**
     * @brief 获取风险等级描述
     * @param level 风险等级
     * @return 描述
     */
        static QString getRiskLevelDescription(RiskLevel level);

        /**
     * @brief 获取投资目标描述
     * @param goal 投资目标
     * @return 描述
     */
        static QString getInvestmentGoalDescription(InvestmentGoal goal);

        signals :
        /**
     * @brief 用户画像更新信号
     */

        void userProfileUpdated(const QString& userId);

        /**
     * @brief 风险评估完成信号
     */
        void riskAssessmentCompleted(const QString& userId, const RiskAssessmentResult& result);

        /**
     * @brief 投资建议生成信号
     */
        void investmentAdviceGenerated(const QString& userId, const InvestmentAdvice& advice);

        /**
     * @brief 错误信号
     */
        void errorOccurred(const QString& error);

    private:
        /**
     * @brief 初始化风险评估问卷
     */
        void initializeQuestionnaire();

        /**
     * @brief 生成资产配置
     * @param profile 用户画像
     * @param totalAmount 总金额
     * @return 资产配置列表
     */
        QList<AssetAllocation> generateAllocation(
            const UserProfile& profile,
            double totalAmount);

        /**
     * @brief 加载用户画像
     * @param userId 用户 ID
     * @return 用户画像
     */
        UserProfile loadUserProfile(const QString& userId) const;

        /**
     * @brief 调用 AI 生成投资建议
     * @param profile 用户画像
     * @param callback 回调函数
     */
        void generateAIAdvice(
            const UserProfile& profile,
            std::function<void(const InvestmentAdvice&)> callback);

    private:
        QMap<QString, UserProfile> m_profiles; ///< 用户画像缓存
        QMap<QString, RiskAssessmentResult> m_assessments; ///< 风险评估结果
        QList<RiskQuestion> m_questionnaire; ///< 风险评估问卷
        QString m_storagePath; ///< 存储路径
    };
} // namespace WealthPilot

#endif // ROBOADVISOR_H