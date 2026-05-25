/**
 * @file PersonalizedRecommendation.cpp
 * @brief 个性化推荐系统实现
 */

#include "PersonalizedRecommendation.h"
#include "core/domain/risk/RiskWarningSystem.h"
#include "shared/utils/Logger.h"
#include <QRandomGenerator>
#include <algorithm>

PersonalizedRecommendation* PersonalizedRecommendation::instance()
{
    static PersonalizedRecommendation* inst = new PersonalizedRecommendation();
    return inst;
}

PersonalizedRecommendation::PersonalizedRecommendation(QObject* parent)
    : QObject(parent)
{
}

bool PersonalizedRecommendation::initialize()
{
    if (m_initialized) return true;

    LOG_INFO("Initializing Personalized Recommendation System");

    // 设置默认偏好
    m_preference.style = InvestmentStyle::Balanced;
    m_preference.riskTolerance = 50.0;
    m_preference.tradingFrequency = 2;

    m_initialized = true;
    LOG_INFO("Personalized Recommendation System initialized");
    return true;
}

void PersonalizedRecommendation::setUserPreference(const UserPreference& preference)
{
    m_preference = preference;
    LOG_INFO(QString("User preference updated: style=%1, riskTolerance=%2")
        .arg(styleToString(preference.style))
        .arg(preference.riskTolerance));

    // 触发重新计算推荐
    updateRecommendationScores();
}

void PersonalizedRecommendation::analyzePreferenceFromHistory()
{
    // 从历史交易数据分析用户偏好
    // 1. 分析持仓周期
    // 2. 分析风险承受能力
    // 3. 分析行业偏好
    // 4. 分析交易频率
    
    // 从交易历史数据库加载分析数据
    // 注意：当前使用模拟数据演示，实际应用中应从数据库加载
    
    // 模拟分析结果
    double avgHoldingDays = 30.0; // 平均持仓天数
    double maxDrawdown = 15.0;    // 最大回撤
    double winRate = 0.55;        // 胜率
    int tradeCount = 50;          // 交易次数
    
    // 根据持仓周期判断风格
    if (avgHoldingDays > 60) {
        m_preference.style = InvestmentStyle::Conservative;
        m_preference.tradingFrequency = 1; // 低频
    } else if (avgHoldingDays > 20) {
        m_preference.style = InvestmentStyle::Balanced;
        m_preference.tradingFrequency = 2; // 中频
    } else {
        m_preference.style = InvestmentStyle::Aggressive;
        m_preference.tradingFrequency = 3; // 高频
    }
    
    // 根据最大回撤调整风险承受能力
    if (maxDrawdown > 20) {
        m_preference.riskTolerance = 70.0; // 高风险承受
    } else if (maxDrawdown > 10) {
        m_preference.riskTolerance = 50.0; // 中等
    } else {
        m_preference.riskTolerance = 30.0; // 低风险承受
    }
    
    // 根据胜率调整
    if (winRate > 0.6) {
        m_preference.riskTolerance = qMin(100.0, m_preference.riskTolerance + 10);
    }
    
    InvestmentStyle style = determineStyleFromHistory();
    m_preference.style = style;

    LOG_INFO(QString("Preference analyzed from history: style=%1, riskTolerance=%2, tradingFreq=%3")
        .arg(styleToString(style))
        .arg(m_preference.riskTolerance)
        .arg(m_preference.tradingFrequency));
}

QVector<StockRecommendation> PersonalizedRecommendation::getRecommendations(int count)
{
    if (m_currentRecommendations.isEmpty()) {
        updateRecommendationScores();
    }

    // 按分数排序
    QVector<StockRecommendation> sorted = m_currentRecommendations;
    std::sort(sorted.begin(), sorted.end(),
              [](const StockRecommendation& a, const StockRecommendation& b) {
                  return a.score > b.score;
              });

    // 返回前N个
    if (sorted.size() > count) {
        sorted = sorted.mid(0, count);
    }

    return sorted;
}

QVector<PortfolioSuggestion> PersonalizedRecommendation::getPortfolioSuggestions()
{
    if (m_portfolioSuggestions.isEmpty()) {
        // 生成组合建议
        PortfolioSuggestion conservative = optimizePortfolio({"sh600000", "sh600519", "sz000001"});
        conservative.name = QStringLiteral("稳健型组合");
        conservative.description = QStringLiteral("适合风险承受能力较低的投资者");
        conservative.strategy = QStringLiteral("价值投资，长期持有");

        PortfolioSuggestion balanced = optimizePortfolio({"sh600036", "sh601318", "sz000002"});
        balanced.name = QStringLiteral("平衡型组合");
        balanced.description = QStringLiteral("适合追求稳健增长的投资者");
        balanced.strategy = QStringLiteral("均衡配置，动态调整");

        PortfolioSuggestion aggressive = optimizePortfolio({"sz300750", "sz002594", "sh688981"});
        aggressive.name = QStringLiteral("进取型组合");
        aggressive.description = QStringLiteral("适合风险承受能力强的投资者");
        aggressive.strategy = QStringLiteral("成长投资，把握趋势");

        m_portfolioSuggestions = {conservative, balanced, aggressive};
    }

    return m_portfolioSuggestions;
}

QString PersonalizedRecommendation::getSmartReminder()
{
    QString reminder;

    // 根据用户偏好生成智能提醒
    if (m_preference.style == InvestmentStyle::Conservative) {
        reminder = QStringLiteral("建议关注低估值蓝筹股，控制仓位在70%以内。");
    } else if (m_preference.style == InvestmentStyle::Aggressive) {
        reminder = QStringLiteral("可适当关注成长股，但注意设置止损位控制风险。");
    } else {
        reminder = QStringLiteral("建议均衡配置，关注价值与成长兼备的标的。");
    }

    // 添加市场环境提醒
    reminder += QStringLiteral("\n\n当前市场波动较大，建议：\n");
    reminder += QStringLiteral("• 分批建仓，避免追高\n");
    reminder += QStringLiteral("• 设置止损位，控制单笔损失\n");
    reminder += QStringLiteral("• 关注基本面，避免题材炒作");

    return reminder;
}

void PersonalizedRecommendation::updateRecommendationScores()
{
    m_currentRecommendations.clear();

    // 示例推荐股票池
    QStringList candidateSymbols = {
        "sh600000", "sh600519", "sh600036", "sh601318",
        "sz000001", "sz000002", "sz300750", "sz002594"
    };

    for (const QString& symbol : candidateSymbols) {
        double score = calculateRecommendationScore(symbol);

        StockRecommendation rec;
        rec.symbol = symbol;
        rec.name = getStockName(symbol); // 从数据源获取股票名称
        rec.score = score;
        rec.riskLevel = score > 70 ? RiskLevel::High :
                       score > 50 ? RiskLevel::Medium : RiskLevel::Low;
        rec.reasons = generateReasons(symbol, score);
        rec.suggestion = generateSuggestion(symbol, score, rec.riskLevel);
        rec.timestamp = QDateTime::currentDateTime();

        m_currentRecommendations.append(rec);
    }

    emit recommendationsUpdated(m_currentRecommendations);
    LOG_INFO(QString("Updated %1 recommendations").arg(m_currentRecommendations.size()));
}

double PersonalizedRecommendation::calculateRecommendationScore(const QString& symbol)
{
    double score = 50.0; // 基础分数

    // 根据用户偏好调整分数
    // 1. 风险承受能力
    if (m_preference.style == InvestmentStyle::Conservative) {
        // 保守型偏好低风险股票
        score += QRandomGenerator::global()->bounded(-10, 10);
    } else if (m_preference.style == InvestmentStyle::Aggressive) {
        // 进取型偏好高成长股票
        score += QRandomGenerator::global()->bounded(-5, 20);
    } else {
        // 平衡型
        score += QRandomGenerator::global()->bounded(-5, 15);
    }

    // 2. 行业偏好
    // 根据股票行业和用户偏好调整分数
    QString industry = getStockIndustry(symbol);
    if (m_preference.style == InvestmentStyle::Conservative) {
        // 保守型偏好稳健行业（银行、公用事业等）
        if (industry.contains(QStringLiteral("银行")) || industry.contains(QStringLiteral("公用"))) {
            score += 10;
        }
    } else if (m_preference.style == InvestmentStyle::Aggressive) {
        // 进取型偏好成长行业（科技、新能源等）
        if (industry.contains(QStringLiteral("科技")) || industry.contains(QStringLiteral("新能源"))) {
            score += 15;
        }
    }

    // 3. 市值偏好
    // 根据市值范围调整分数
    double marketCap = getStockMarketCap(symbol);
    if (m_preference.style == InvestmentStyle::Conservative) {
        // 保守型偏好大盘股
        if (marketCap > 1000) { // 1000亿以上
            score += 8;
        }
    } else if (m_preference.style == InvestmentStyle::Aggressive) {
        // 进取型偏好中小盘股
        if (marketCap < 500) { // 500亿以下
            score += 12;
        }
    }

    // 限制在0-100范围
    return qBound(0.0, score, 100.0);
}

QVector<RecommendationReason> PersonalizedRecommendation::generateReasons(const QString& symbol, double score)
{
    QVector<RecommendationReason> reasons;

    if (score > 70) {
        RecommendationReason r1;
        r1.factor = QStringLiteral("成长性");
        r1.weight = 0.3;
        r1.description = QStringLiteral("业绩增长强劲，成长性突出");
        reasons.append(r1);

        RecommendationReason r2;
        r2.factor = QStringLiteral("技术面");
        r2.weight = 0.25;
        r2.description = QStringLiteral("技术指标向好，趋势向上");
        reasons.append(r2);
    } else if (score > 50) {
        RecommendationReason r1;
        r1.factor = QStringLiteral("基本面");
        r1.weight = 0.3;
        r1.description = QStringLiteral("基本面稳健，估值合理");
        reasons.append(r1);

        RecommendationReason r2;
        r2.factor = QStringLiteral("分红");
        r2.weight = 0.2;
        r2.description = QStringLiteral("股息率较高，适合长期持有");
        reasons.append(r2);
    } else {
        RecommendationReason r1;
        r1.factor = QStringLiteral("风险提示");
        r1.weight = 0.4;
        r1.description = QStringLiteral("当前风险较高，建议谨慎");
        reasons.append(r1);
    }

    return reasons;
}

QString PersonalizedRecommendation::generateSuggestion(const QString& symbol, double score, RiskLevel risk)
{
    Q_UNUSED(symbol);

    if (score > 80) {
        return QStringLiteral("强烈推荐，建议重点关注。");
    } else if (score > 70) {
        return QStringLiteral("推荐买入，注意控制仓位。");
    } else if (score > 60) {
        return QStringLiteral("可以关注，等待更好买点。");
    } else if (score > 50) {
        return QStringLiteral("谨慎观望，注意风险。");
    } else {
        return QStringLiteral("暂不推荐，风险较高。");
    }
}

PortfolioSuggestion PersonalizedRecommendation::optimizePortfolio(const QVector<QString>& symbols)
{
    PortfolioSuggestion portfolio;

    if (symbols.isEmpty()) return portfolio;

    // 简单的等权重配置
    double weight = 100.0 / symbols.size();
    for (const QString& symbol : symbols) {
        portfolio.allocations[symbol] = weight;
    }

    // 计算预期收益和风险
    portfolio.expectedReturn = calculateExpectedReturn(portfolio.allocations);
    portfolio.riskScore = calculateRiskScore(portfolio.allocations);

    // 添加优势和风险
    portfolio.advantages.append(QStringLiteral("分散投资降低风险"));
    portfolio.advantages.append(QStringLiteral("均衡配置稳健增长"));
    portfolio.risks.append(QStringLiteral("市场系统性风险"));
    portfolio.risks.append(QStringLiteral("行业轮动风险"));

    return portfolio;
}

double PersonalizedRecommendation::calculateExpectedReturn(const QMap<QString, double>& allocations)
{
    // 简化的预期收益计算
    // 实际应该基于历史数据和预测模型
    double baseReturn = 8.0; // 基础预期收益8%

    if (m_preference.style == InvestmentStyle::Aggressive) {
        baseReturn += 5.0;
    } else if (m_preference.style == InvestmentStyle::Conservative) {
        baseReturn -= 3.0;
    }

    return baseReturn;
}

double PersonalizedRecommendation::calculateRiskScore(const QMap<QString, double>& allocations)
{
    // 简化的风险分数计算
    // 实际应该基于波动率、相关性等
    double baseRisk = 30.0;

    if (m_preference.style == InvestmentStyle::Aggressive) {
        baseRisk += 20.0;
    } else if (m_preference.style == InvestmentStyle::Conservative) {
        baseRisk -= 10.0;
    }

    return baseRisk;
}

InvestmentStyle PersonalizedRecommendation::determineStyleFromHistory()
{
    // 从历史交易数据分析投资风格
    // 1. 分析持仓周期
    // 2. 分析风险偏好
    // 3. 分析收益波动
    
    // 当前返回用户设置的偏好，实际应用中应从交易历史数据库分析
    return m_preference.style;
}

QString PersonalizedRecommendation::getStockIndustry(const QString& symbol)
{
    // 从数据库或API查询股票行业
    // 当前使用模拟数据
    static QMap<QString, QString> industryMap = {
        {"sh600000", QStringLiteral("银行")},
        {"sh600519", QStringLiteral("白酒")},
        {"sz000001", QStringLiteral("银行")},
        {"sh600036", QStringLiteral("银行")},
        {"sh601318", QStringLiteral("保险")},
        {"sz000002", QStringLiteral("房地产")},
        {"sz300750", QStringLiteral("新能源")},
        {"sz002594", QStringLiteral("汽车")},
        {"sh688981", QStringLiteral("科技")},
    };
    
    return industryMap.value(symbol, QStringLiteral("其他"));
}

double PersonalizedRecommendation::getStockMarketCap(const QString& symbol)
{
    // 从数据库或API查询市值信息
    // 当前使用模拟数据（单位：亿元）
    static QMap<QString, double> marketCapMap = {
        {"sh600000", 3500.0},
        {"sh600519", 22000.0},
        {"sz000001", 2800.0},
        {"sh600036", 12000.0},
        {"sh601318", 9000.0},
        {"sz000002", 1800.0},
        {"sz300750", 9500.0},
        {"sz002594", 7500.0},
        {"sh688981", 450.0},
    };
    
    return marketCapMap.value(symbol, 500.0);
}

QString PersonalizedRecommendation::getStockName(const QString& symbol)
{
    // 从数据库或API查询股票名称
    // 当前使用模拟数据
    static QMap<QString, QString> nameMap = {
        {"sh600000", QStringLiteral("浦发银行")},
        {"sh600519", QStringLiteral("贵州茅台")},
        {"sz000001", QStringLiteral("平安银行")},
        {"sh600036", QStringLiteral("招商银行")},
        {"sh601318", QStringLiteral("中国平安")},
        {"sz000002", QStringLiteral("万科A")},
        {"sz300750", QStringLiteral("宁德时代")},
        {"sz002594", QStringLiteral("比亚迪")},
        {"sh688981", QStringLiteral("中芯国际")},
    };
    
    return nameMap.value(symbol, symbol);
}

QString PersonalizedRecommendation::styleToString(InvestmentStyle style) const
{
    switch (style) {
    case InvestmentStyle::Conservative: return QStringLiteral("保守型");
    case InvestmentStyle::Balanced: return QStringLiteral("平衡型");
    case InvestmentStyle::Aggressive: return QStringLiteral("进取型");
    default: return QStringLiteral("未知");
    }
}
