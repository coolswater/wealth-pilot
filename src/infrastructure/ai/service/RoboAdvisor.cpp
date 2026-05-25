/**
 * @file RoboAdvisor.cpp
 * @brief 智能投顾服务实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "RoboAdvisor.h"
#include "AIService.h"
#include "shared/utils/Logger.h"

#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QRandomGenerator>

namespace WealthPilot
{
    // ============================================================================
    // 数据结构实现
    // ============================================================================

    UserProfile UserProfile::fromJson(const QJsonObject& json)
    {
        UserProfile profile;
        profile.userId = json["userId"].toString();
        profile.nickname = json["nickname"].toString();
        profile.riskTolerance = json["riskTolerance"].toInt(5);
        profile.riskLevel = static_cast<RiskLevel>(json["riskLevel"].toInt(3));
        profile.investmentHorizon = json["investmentHorizon"].toInt(12);
        profile.isShortTerm = json["isShortTerm"].toBool();
        profile.isMediumTerm = json["isMediumTerm"].toBool(true);
        profile.isLongTerm = json["isLongTerm"].toBool();
        profile.totalAssets = json["totalAssets"].toDouble();
        profile.investableAssets = json["investableAssets"].toDouble();
        profile.monthlyIncome = json["monthlyIncome"].toDouble();
        profile.monthlyExpense = json["monthlyExpense"].toDouble();
        profile.goal = static_cast<InvestmentGoal>(json["goal"].toInt());
        profile.preferDividend = json["preferDividend"].toBool();
        profile.preferGrowth = json["preferGrowth"].toBool(true);
        profile.investmentExperience = json["investmentExperience"].toInt();
        profile.hasStockExperience = json["hasStockExperience"].toBool();
        profile.hasFundExperience = json["hasFundExperience"].toBool();
        profile.hasFuturesExperience = json["hasFuturesExperience"].toBool();
        profile.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
        profile.updatedAt = QDateTime::fromString(json["updatedAt"].toString(), Qt::ISODate);

        // 解析列表
        QJsonArray sectorsArray = json["preferredSectors"].toArray();
        for (const auto& s : sectorsArray)
        {
            profile.preferredSectors.append(s.toString());
        }

        QJsonArray excludedArray = json["excludedStocks"].toArray();
        for (const auto& s : excludedArray)
        {
            profile.excludedStocks.append(s.toString());
        }

        return profile;
    }

    QJsonObject UserProfile::toJson() const
    {
        QJsonObject json;
        json["userId"] = userId;
        json["nickname"] = nickname;
        json["riskTolerance"] = riskTolerance;
        json["riskLevel"] = static_cast<int>(riskLevel);
        json["investmentHorizon"] = investmentHorizon;
        json["isShortTerm"] = isShortTerm;
        json["isMediumTerm"] = isMediumTerm;
        json["isLongTerm"] = isLongTerm;
        json["totalAssets"] = totalAssets;
        json["investableAssets"] = investableAssets;
        json["monthlyIncome"] = monthlyIncome;
        json["monthlyExpense"] = monthlyExpense;
        json["goal"] = static_cast<int>(goal);
        json["preferDividend"] = preferDividend;
        json["preferGrowth"] = preferGrowth;
        json["investmentExperience"] = investmentExperience;
        json["hasStockExperience"] = hasStockExperience;
        json["hasFundExperience"] = hasFundExperience;
        json["hasFuturesExperience"] = hasFuturesExperience;
        json["createdAt"] = createdAt.toString(Qt::ISODate);
        json["updatedAt"] = updatedAt.toString(Qt::ISODate);

        QJsonArray sectorsArray;
        for (const auto& s : preferredSectors)
        {
            sectorsArray.append(s);
        }
        json["preferredSectors"] = sectorsArray;

        QJsonArray excludedArray;
        for (const auto& s : excludedStocks)
        {
            excludedArray.append(s);
        }
        json["excludedStocks"] = excludedArray;

        return json;
    }

    RiskLevel UserProfile::calculateRiskLevel() const
    {
        return RoboAdvisor::calculateRiskLevel(riskTolerance);
    }

    RiskAssessmentResult RiskAssessmentResult::fromJson(const QJsonObject& json)
    {
        RiskAssessmentResult result;
        result.totalScore = json["totalScore"].toInt();
        result.riskLevel = static_cast<RiskLevel>(json["riskLevel"].toInt());
        result.riskDescription = json["riskDescription"].toString();
        result.assessedAt = QDateTime::fromString(json["assessedAt"].toString(), Qt::ISODate);

        QJsonArray recsArray = json["recommendations"].toArray();
        for (const auto& r : recsArray)
        {
            result.recommendations.append(r.toString());
        }

        return result;
    }

    QJsonObject RiskAssessmentResult::toJson() const
    {
        QJsonObject json;
        json["totalScore"] = totalScore;
        json["riskLevel"] = static_cast<int>(riskLevel);
        json["riskDescription"] = riskDescription;
        json["assessedAt"] = assessedAt.toString(Qt::ISODate);

        QJsonArray recsArray;
        for (const auto& r : recommendations)
        {
            recsArray.append(r);
        }
        json["recommendations"] = recsArray;

        return json;
    }

    AssetAllocation AssetAllocation::fromJson(const QJsonObject& json)
    {
        AssetAllocation alloc;
        alloc.assetClass = json["assetClass"].toString();
        alloc.percentage = json["percentage"].toDouble();
        alloc.amount = json["amount"].toDouble();
        alloc.reason = json["reason"].toString();

        QJsonArray productsArray = json["suggestedProducts"].toArray();
        for (const auto& p : productsArray)
        {
            alloc.suggestedProducts.append(p.toString());
        }

        return alloc;
    }

    QJsonObject AssetAllocation::toJson() const
    {
        QJsonObject json;
        json["assetClass"] = assetClass;
        json["percentage"] = percentage;
        json["amount"] = amount;
        json["reason"] = reason;

        QJsonArray productsArray;
        for (const auto& p : suggestedProducts)
        {
            productsArray.append(p);
        }
        json["suggestedProducts"] = productsArray;

        return json;
    }

    InvestmentAdvice InvestmentAdvice::fromJson(const QJsonObject& json)
    {
        InvestmentAdvice advice;
        advice.id = json["id"].toString();
        advice.title = json["title"].toString();
        advice.summary = json["summary"].toString();
        advice.detailedAnalysis = json["detailedAnalysis"].toString();
        advice.confidence = json["confidence"].toDouble();
        advice.generatedAt = QDateTime::fromString(json["generatedAt"].toString(), Qt::ISODate);

        QJsonArray allocsArray = json["allocations"].toArray();
        for (const auto& a : allocsArray)
        {
            advice.allocations.append(AssetAllocation::fromJson(a.toObject()));
        }

        QJsonArray actionsArray = json["actionItems"].toArray();
        for (const auto& a : actionsArray)
        {
            advice.actionItems.append(a.toString());
        }

        QJsonArray warningsArray = json["riskWarnings"].toArray();
        for (const auto& w : warningsArray)
        {
            advice.riskWarnings.append(w.toString());
        }

        return advice;
    }

    QJsonObject InvestmentAdvice::toJson() const
    {
        QJsonObject json;
        json["id"] = id;
        json["title"] = title;
        json["summary"] = summary;
        json["detailedAnalysis"] = detailedAnalysis;
        json["confidence"] = confidence;
        json["generatedAt"] = generatedAt.toString(Qt::ISODate);

        QJsonArray allocsArray;
        for (const auto& a : allocations)
        {
            allocsArray.append(a.toJson());
        }
        json["allocations"] = allocsArray;

        QJsonArray actionsArray;
        for (const auto& a : actionItems)
        {
            actionsArray.append(a);
        }
        json["actionItems"] = actionsArray;

        QJsonArray warningsArray;
        for (const auto& w : riskWarnings)
        {
            warningsArray.append(w);
        }
        json["riskWarnings"] = warningsArray;

        return json;
    }

    SIPPlan SIPPlan::fromJson(const QJsonObject& json)
    {
        SIPPlan plan;
        plan.id = json["id"].toString();
        plan.name = json["name"].toString();
        plan.monthlyAmount = json["monthlyAmount"].toDouble();
        plan.totalMonths = json["totalMonths"].toInt();
        plan.expectedReturn = json["expectedReturn"].toDouble();
        plan.expectedRisk = json["expectedRisk"].toDouble();
        plan.startDate = QDateTime::fromString(json["startDate"].toString(), Qt::ISODate);
        plan.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);

        QJsonArray allocsArray = json["allocations"].toArray();
        for (const auto& a : allocsArray)
        {
            plan.allocations.append(AssetAllocation::fromJson(a.toObject()));
        }

        return plan;
    }

    QJsonObject SIPPlan::toJson() const
    {
        QJsonObject json;
        json["id"] = id;
        json["name"] = name;
        json["monthlyAmount"] = monthlyAmount;
        json["totalMonths"] = totalMonths;
        json["expectedReturn"] = expectedReturn;
        json["expectedRisk"] = expectedRisk;
        json["startDate"] = startDate.toString(Qt::ISODate);
        json["createdAt"] = createdAt.toString(Qt::ISODate);

        QJsonArray allocsArray;
        for (const auto& a : allocations)
        {
            allocsArray.append(a.toJson());
        }
        json["allocations"] = allocsArray;

        return json;
    }

    // ============================================================================
    // RoboAdvisor 实现
    // ============================================================================

    RoboAdvisor::RoboAdvisor(QObject* parent)
        : QObject(parent)
    {
        initializeQuestionnaire();

        // 设置存储路径
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        m_storagePath = appDataPath + "/robo_advisor";
        QDir dir(m_storagePath);
        if (!dir.exists())
        {
            dir.mkpath(".");
        }

        LOG_DEBUG("RoboAdvisor created");
    }

    RoboAdvisor::~RoboAdvisor()
    {
        LOG_DEBUG("RoboAdvisor destroyed");
    }

    UserProfile RoboAdvisor::createUserProfile(const QString& userId)
    {
        UserProfile profile;
        profile.userId = userId;
        profile.createdAt = QDateTime::currentDateTime();
        profile.updatedAt = profile.createdAt;

        m_profiles[userId] = profile;
        saveUserProfile(profile);

        LOG_INFO("Created user profile: " + userId);
        return profile;
    }

    UserProfile RoboAdvisor::getUserProfile(const QString& userId) const
    {
        if (m_profiles.contains(userId))
        {
            return m_profiles[userId];
        }
        return loadUserProfile(userId);
    }

    bool RoboAdvisor::updateUserProfile(const UserProfile& profile)
    {
        UserProfile updatedProfile = profile;
        updatedProfile.updatedAt = QDateTime::currentDateTime();

        m_profiles[profile.userId] = updatedProfile;
        bool success = saveUserProfile(updatedProfile);

        if (success)
        {
            emit userProfileUpdated(profile.userId);
            LOG_INFO("Updated user profile: " + profile.userId);
        }

        return success;
    }

    bool RoboAdvisor::saveUserProfile(const UserProfile& profile)
    {
        QString filePath = m_storagePath + "/" + profile.userId + "_profile.json";

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
        {
            LOG_ERROR("Failed to save user profile: " + filePath);
            return false;
        }

        QJsonDocument doc(profile.toJson());
        file.write(doc.toJson());
        file.close();

        return true;
    }

    QList<RiskQuestion> RoboAdvisor::getRiskQuestionnaire() const
    {
        return m_questionnaire;
    }

    RiskAssessmentResult RoboAdvisor::submitRiskAssessment(
        const QString& userId,
        const QMap<QString, int>& answers)
    {
        // 计算总分
        int totalScore = 0;
        for (const auto& question : m_questionnaire)
        {
            if (answers.contains(question.id))
            {
                int optionIndex = answers[question.id];
                if (optionIndex >= 0 && optionIndex < question.scores.size())
                {
                    totalScore += question.scores[optionIndex];
                }
            }
        }

        // 确定风险等级
        RiskLevel riskLevel = calculateRiskLevel(totalScore);

        // 生成评估结果
        RiskAssessmentResult result;
        result.totalScore = totalScore;
        result.riskLevel = riskLevel;
        result.riskDescription = getRiskLevelDescription(riskLevel);
        result.assessedAt = QDateTime::currentDateTime();

        // 添加建议
        switch (riskLevel)
        {
        case RiskLevel::VeryLow:
            result.recommendations.append(QStringLiteral("建议以货币基金、银行理财为主"));
            result.recommendations.append(QStringLiteral("可考虑定期存款和国债"));
            break;
        case RiskLevel::Low:
            result.recommendations.append(QStringLiteral("建议配置债券型基金"));
            result.recommendations.append(QStringLiteral("可少量配置蓝筹股"));
            break;
        case RiskLevel::Medium:
            result.recommendations.append(QStringLiteral("建议股债平衡配置"));
            result.recommendations.append(QStringLiteral("可配置混合型基金"));
            break;
        case RiskLevel::High:
            result.recommendations.append(QStringLiteral("可增加股票配置比例"));
            result.recommendations.append(QStringLiteral("可考虑成长型基金"));
            break;
        case RiskLevel::VeryHigh:
            result.recommendations.append(QStringLiteral("可积极配置股票"));
            result.recommendations.append(QStringLiteral("可考虑主题基金和行业基金"));
            break;
        }

        // 保存评估结果
        m_assessments[userId] = result;

        // 更新用户画像
        if (m_profiles.contains(userId))
        {
            UserProfile profile = m_profiles[userId];
            profile.riskTolerance = totalScore;
            profile.riskLevel = riskLevel;
            updateUserProfile(profile);
        }

        emit riskAssessmentCompleted(userId, result);
        LOG_INFO("Risk assessment completed for user: " + userId + ", score: " + QString::number(totalScore));

        return result;
    }

    RiskAssessmentResult RoboAdvisor::getRiskAssessment(const QString& userId) const
    {
        return m_assessments.value(userId);
    }

    void RoboAdvisor::getInvestmentAdvice(
        const QString& userId,
        std::function<void(const InvestmentAdvice &)> callback)
    {
        UserProfile profile = getUserProfile(userId);
        generateAIAdvice(profile, callback);
    }

    InvestmentAdvice RoboAdvisor::getInvestmentAdviceSync(const QString& userId)
    {
        UserProfile profile = getUserProfile(userId);

        InvestmentAdvice advice;
        advice.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        advice.title = QStringLiteral("投资组合建议");
        advice.generatedAt = QDateTime::currentDateTime();

        // 基于风险等级生成资产配置
        advice.allocations = generateAllocation(profile, profile.investableAssets);

        // 生成摘要
        advice.summary = QString(QStringLiteral("根据您的风险承受能力（%1）和投资期限（%2个月），")
                                 .arg(getRiskLevelDescription(profile.riskLevel))
                                 .arg(profile.investmentHorizon));
        advice.summary += QStringLiteral("我们建议您采用以下资产配置方案。");

        // 添加行动项
        advice.actionItems.append(QStringLiteral("1. 按建议比例配置资产"));
        advice.actionItems.append(QStringLiteral("2. 定期（如每季度）再平衡"));
        advice.actionItems.append(QStringLiteral("3. 关注市场变化，适时调整"));

        // 添加风险提示
        advice.riskWarnings.append(QStringLiteral("投资有风险，入市需谨慎"));
        advice.riskWarnings.append(QStringLiteral("过往业绩不代表未来表现"));
        advice.riskWarnings.append(QStringLiteral("建议分散投资降低风险"));

        advice.confidence = 0.75;

        emit investmentAdviceGenerated(userId, advice);
        return advice;
    }

    QList<AssetAllocation> RoboAdvisor::getAssetAllocation(
        const QString& userId,
        double totalAmount)
    {
        UserProfile profile = getUserProfile(userId);
        return generateAllocation(profile, totalAmount);
    }

    SIPPlan RoboAdvisor::generateSIPPlan(
        const QString& userId,
        double monthlyAmount,
        int totalMonths)
    {
        UserProfile profile = getUserProfile(userId);

        SIPPlan plan;
        plan.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        plan.name = QStringLiteral("智能定投计划");
        plan.monthlyAmount = monthlyAmount;
        plan.totalMonths = totalMonths;
        plan.startDate = QDateTime::currentDateTime();
        plan.createdAt = QDateTime::currentDateTime();

        // 生成配置
        plan.allocations = generateAllocation(profile, monthlyAmount);

        // 计算预期收益和风险（简化估算）
        double baseReturn = 0.05; // 基础年化收益 5%
        switch (profile.riskLevel)
        {
        case RiskLevel::VeryLow:
            plan.expectedReturn = baseReturn * 0.5;
            plan.expectedRisk = 0.02;
            break;
        case RiskLevel::Low:
            plan.expectedReturn = baseReturn * 0.8;
            plan.expectedRisk = 0.05;
            break;
        case RiskLevel::Medium:
            plan.expectedReturn = baseReturn * 1.0;
            plan.expectedRisk = 0.10;
            break;
        case RiskLevel::High:
            plan.expectedReturn = baseReturn * 1.3;
            plan.expectedRisk = 0.15;
            break;
        case RiskLevel::VeryHigh:
            plan.expectedReturn = baseReturn * 1.6;
            plan.expectedRisk = 0.20;
            break;
        }

        LOG_INFO("Generated SIP plan for user: " + userId);
        return plan;
    }

    QList<SIPPlan> RoboAdvisor::getSIPPlans(const QString& userId) const
    {
        // TODO: 从存储加载
        Q_UNUSED(userId)
        return QList<SIPPlan>();
    }

    RiskLevel RoboAdvisor::calculateRiskLevel(int score)
    {
        if (score <= 10) return RiskLevel::VeryLow;
        if (score <= 20) return RiskLevel::Low;
        if (score <= 30) return RiskLevel::Medium;
        if (score <= 40) return RiskLevel::High;
        return RiskLevel::VeryHigh;
    }

    QString RoboAdvisor::getRiskLevelDescription(RiskLevel level)
    {
        switch (level)
        {
        case RiskLevel::VeryLow:
            return QStringLiteral("极低风险");
        case RiskLevel::Low:
            return QStringLiteral("低风险");
        case RiskLevel::Medium:
            return QStringLiteral("中等风险");
        case RiskLevel::High:
            return QStringLiteral("高风险");
        case RiskLevel::VeryHigh:
            return QStringLiteral("极高风险");
        default:
            return QStringLiteral("未知");
        }
    }

    QString RoboAdvisor::getInvestmentGoalDescription(InvestmentGoal goal)
    {
        switch (goal)
        {
        case InvestmentGoal::CapitalPreservation:
            return QStringLiteral("资产保值");
        case InvestmentGoal::SteadyGrowth:
            return QStringLiteral("稳健增值");
        case InvestmentGoal::BalancedGrowth:
            return QStringLiteral("平衡增长");
        case InvestmentGoal::AggressiveGrowth:
            return QStringLiteral("积极增长");
        case InvestmentGoal::HighReturn:
            return QStringLiteral("高收益");
        default:
            return QStringLiteral("未知");
        }
    }

    void RoboAdvisor::initializeQuestionnaire()
    {
        m_questionnaire.clear();

        // 问题 1：投资经验
        RiskQuestion q1;
        q1.id = "q1";
        q1.text = QStringLiteral("您的投资经验如何？");
        q1.options = {
            QStringLiteral("无投资经验"),
            QStringLiteral("1-3年投资经验"),
            QStringLiteral("3-5年投资经验"),
            QStringLiteral("5年以上投资经验")
        };
        q1.scores = {2, 5, 8, 10};
        m_questionnaire.append(q1);

        // 问题 2：风险承受
        RiskQuestion q2;
        q2.id = "q2";
        q2.text = QStringLiteral("如果您的投资亏损20%，您会怎么做？");
        q2.options = {
            QStringLiteral("立即全部卖出"),
            QStringLiteral("卖出部分止损"),
            QStringLiteral("持有等待反弹"),
            QStringLiteral("加仓摊低成本")
        };
        q2.scores = {2, 5, 8, 10};
        m_questionnaire.append(q2);

        // 问题 3：投资期限
        RiskQuestion q3;
        q3.id = "q3";
        q3.text = QStringLiteral("您的投资期限是多久？");
        q3.options = {
            QStringLiteral("1年以内"),
            QStringLiteral("1-3年"),
            QStringLiteral("3-5年"),
            QStringLiteral("5年以上")
        };
        q3.scores = {2, 5, 8, 10};
        m_questionnaire.append(q3);

        // 问题 4：收入稳定性
        RiskQuestion q4;
        q4.id = "q4";
        q4.text = QStringLiteral("您的收入稳定性如何？");
        q4.options = {
            QStringLiteral("不稳定"),
            QStringLiteral("基本稳定"),
            QStringLiteral("稳定且有结余"),
            QStringLiteral("非常稳定且结余较多")
        };
        q4.scores = {2, 5, 8, 10};
        m_questionnaire.append(q4);

        // 问题 5：投资目标
        RiskQuestion q5;
        q5.id = "q5";
        q5.text = QStringLiteral("您的主要投资目标是什么？");
        q5.options = {
            QStringLiteral("资产保值，不愿承担风险"),
            QStringLiteral("稳健增值，接受小幅波动"),
            QStringLiteral("追求增长，可承受一定波动"),
            QStringLiteral("追求高收益，可承受较大波动")
        };
        q5.scores = {2, 5, 8, 10};
        m_questionnaire.append(q5);
    }

    QList<AssetAllocation> RoboAdvisor::generateAllocation(
        const UserProfile& profile,
        double totalAmount)
    {
        QList<AssetAllocation> allocations;

        // 根据风险等级确定配置比例
        double stockRatio = 0.0;
        double bondRatio = 0.0;
        double cashRatio = 0.0;
        double fundRatio = 0.0;

        switch (profile.riskLevel)
        {
        case RiskLevel::VeryLow:
            stockRatio = 0.0;
            bondRatio = 0.4;
            cashRatio = 0.5;
            fundRatio = 0.1;
            break;
        case RiskLevel::Low:
            stockRatio = 0.1;
            bondRatio = 0.4;
            cashRatio = 0.3;
            fundRatio = 0.2;
            break;
        case RiskLevel::Medium:
            stockRatio = 0.3;
            bondRatio = 0.3;
            cashRatio = 0.1;
            fundRatio = 0.3;
            break;
        case RiskLevel::High:
            stockRatio = 0.5;
            bondRatio = 0.2;
            cashRatio = 0.05;
            fundRatio = 0.25;
            break;
        case RiskLevel::VeryHigh:
            stockRatio = 0.7;
            bondRatio = 0.1;
            cashRatio = 0.0;
            fundRatio = 0.2;
            break;
        }

        // 股票配置
        if (stockRatio > 0)
        {
            AssetAllocation stockAlloc;
            stockAlloc.assetClass = QStringLiteral("股票");
            stockAlloc.percentage = stockRatio * 100;
            stockAlloc.amount = totalAmount * stockRatio;
            stockAlloc.reason = QStringLiteral("根据您的风险承受能力配置");
            stockAlloc.suggestedProducts = {
                QStringLiteral("沪深300ETF"),
                QStringLiteral("中证500ETF")
            };
            allocations.append(stockAlloc);
        }

        // 债券配置
        if (bondRatio > 0)
        {
            AssetAllocation bondAlloc;
            bondAlloc.assetClass = QStringLiteral("债券");
            bondAlloc.percentage = bondRatio * 100;
            bondAlloc.amount = totalAmount * bondRatio;
            bondAlloc.reason = QStringLiteral("提供稳定收益，降低组合波动");
            bondAlloc.suggestedProducts = {
                QStringLiteral("国债ETF"),
                QStringLiteral("信用债基金")
            };
            allocations.append(bondAlloc);
        }

        // 现金配置
        if (cashRatio > 0)
        {
            AssetAllocation cashAlloc;
            cashAlloc.assetClass = QStringLiteral("现金及货币基金");
            cashAlloc.percentage = cashRatio * 100;
            cashAlloc.amount = totalAmount * cashRatio;
            cashAlloc.reason = QStringLiteral("保持流动性，应对短期需求");
            cashAlloc.suggestedProducts = {
                QStringLiteral("货币基金"),
                QStringLiteral("银行理财")
            };
            allocations.append(cashAlloc);
        }

        // 基金配置
        if (fundRatio > 0)
        {
            AssetAllocation fundAlloc;
            fundAlloc.assetClass = QStringLiteral("基金");
            fundAlloc.percentage = fundRatio * 100;
            fundAlloc.amount = totalAmount * fundRatio;
            fundAlloc.reason = QStringLiteral("分散投资，专业管理");
            fundAlloc.suggestedProducts = {
                QStringLiteral("混合型基金"),
                QStringLiteral("指数增强基金")
            };
            allocations.append(fundAlloc);
        }

        return allocations;
    }

    UserProfile RoboAdvisor::loadUserProfile(const QString& userId) const
    {
        QString filePath = m_storagePath + "/" + userId + "_profile.json";

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return UserProfile();
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        return UserProfile::fromJson(doc.object());
    }

    void RoboAdvisor::generateAIAdvice(
        const UserProfile& profile,
        std::function<void(const InvestmentAdvice &)> callback)
    {
        // 构建提示词
        QString prompt = QString(QStringLiteral(
                             "请根据以下用户画像，提供个性化的投资建议：\n\n"
                             "用户画像：\n"
                             "- 风险承受能力：%1 (1-10)\n"
                             "- 风险等级：%2\n"
                             "- 投资期限：%3 个月\n"
                             "- 可投资资产：%4 元\n"
                             "- 投资目标：%5\n"
                             "- 投资经验：%6 年\n\n"
                             "请提供：\n"
                             "1. 资产配置建议\n"
                             "2. 具体投资建议\n"
                             "3. 风险提示\n"
                         ))
                         .arg(profile.riskTolerance)
                         .arg(getRiskLevelDescription(profile.riskLevel))
                         .arg(profile.investmentHorizon)
                         .arg(profile.investableAssets)
                         .arg(getInvestmentGoalDescription(profile.goal))
                         .arg(profile.investmentExperience);

        // 调用 AI 服务
        AIService::instance()->chat(prompt, [this, profile, callback](Result<QString> result)
        {
            if (result.isError())
            {
                emit errorOccurred(result.errorMessage());
                return;
            }

            // 解析 AI 响应
            InvestmentAdvice advice;
            advice.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            advice.title = QStringLiteral("AI 投资建议");
            advice.detailedAnalysis = result.value();
            advice.generatedAt = QDateTime::currentDateTime();
            advice.confidence = 0.8;

            // 生成配置
            advice.allocations = generateAllocation(profile, profile.investableAssets);

            emit investmentAdviceGenerated(profile.userId, advice);
            callback(advice);
        });
    }
} // namespace WealthPilot