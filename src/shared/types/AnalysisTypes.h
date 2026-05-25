/**
 * @file AnalysisTypes.h
 * @brief 分析相关类型定义
 * @details 统一管理技术分析、情绪分析等类型
 * @author WealthPilot Team
 * @date 2026-01-01
 */

#pragma once

#include <QString>
#include <QDateTime>
#include <QVariant>
#include <QVector>
#include <vector>
#include <optional>

namespace WealthPilot {

/**
 * @brief 情绪类型枚举
 */
enum class SentimentType
{
    VeryNegative = -2,  ///< 非常负面
    Negative = -1,      ///< 负面
    Neutral = 0,        ///< 中性
    Positive = 1,       ///< 正面
    VeryPositive = 2    ///< 非常正面
};

/**
 * @brief 新闻分类枚举
 */
enum class NewsCategory
{
    General,        ///< 综合新闻
    Announcement,   ///< 公告
    Financial,      ///< 财报
    Research,       ///< 研报
    Industry,       ///< 行业
    Policy          ///< 政策
};

/**
 * @brief 分析结果类型
 */
enum class AnalysisResult
{
    StrongBuy,   ///< 强烈买入
    Buy,         ///< 买入
    Hold,        ///< 持有
    Sell,        ///< 卖出
    StrongSell   ///< 强烈卖出
};

/**
 * @brief 情绪分析结果结构
 */
struct SentimentResult
{
    SentimentType sentiment = SentimentType::Neutral;
    double score = 0.0;           ///< 情绪分数 (-1.0 ~ 1.0)
    double confidence = 0.0;      ///< 置信度 (0.0 ~ 1.0)
    double impactScore = 0.0;     ///< 影响分数
    QString sentimentLabel;       ///< 情绪标签
    QString riskHint;             ///< 风险提示
    QVector<QString> keywords;    ///< 关键词
    QString summary;              ///< 摘要
    
    static SentimentResult fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief 新闻项结构
 */
struct NewsItem
{
    QString id;
    QString title;
    QString content;
    QString source;
    QString url;
    QString category;           ///< 分类名称
    QString symbol;             ///< 相关证券代码
    QDateTime publishTime;
    QDateTime fetchTime;
    SentimentResult sentiment;  ///< 情绪分析结果
    double sentimentScore = 0.0;
    QString sentimentLabel;
    double impactScore = 0.0;
    bool isImportant = false;
    bool isRead = false;
    
    bool isValid() const { return !id.isEmpty() && !title.isEmpty(); }
};

/**
 * @brief 社交热度数据结构
 */
struct SocialHeatData
{
    QString symbol;
    int mentionCount = 0;       ///< 提及次数
    double changePercent = 0.0; ///< 变化百分比
    double sentimentScore = 0.0; ///< 情绪分数
    QDateTime updateTime;
    
    bool isValid() const { return !symbol.isEmpty(); }
};

/**
 * @brief 技术指标结果结构
 */
struct IndicatorResult
{
    QString name;               ///< 指标名称
    double value = 0.0;         ///< 指标值
    QString signal;             ///< 信号 (买入/卖出/持有)
    QString description;        ///< 描述
    QDateTime time;
    
    bool isValid() const { return !name.isEmpty(); }
};

/**
 * @brief 情绪数据点结构
 */
struct SentimentPoint
{
    QDateTime time;
    double value = 0.0;
    QString source;
};

/**
 * @brief 情绪指数结构
 */
struct SentimentIndex
{
    QString symbol;
    double currentValue = 0.0;
    double previousValue = 0.0;
    double change = 0.0;
    double changePercent = 0.0;
    QVector<SentimentPoint> history;
    QDateTime updateTime;
};

// 类型别名，保持向后兼容
using NewsSentimentResult = SentimentResult;
using NewsSentimentType = SentimentType;

/**
 * @brief 获取情绪类型显示名称
 */
inline QString sentimentTypeToString(SentimentType type)
{
    switch (type) {
        case SentimentType::VeryNegative: return QStringLiteral("非常负面");
        case SentimentType::Negative:     return QStringLiteral("负面");
        case SentimentType::Neutral:      return QStringLiteral("中性");
        case SentimentType::Positive:     return QStringLiteral("正面");
        case SentimentType::VeryPositive: return QStringLiteral("非常正面");
        default: return QStringLiteral("未知");
    }
}

/**
 * @brief 获取分析结果显示名称
 */
inline QString analysisResultToString(AnalysisResult result)
{
    switch (result) {
        case AnalysisResult::StrongBuy:  return QStringLiteral("强烈买入");
        case AnalysisResult::Buy:        return QStringLiteral("买入");
        case AnalysisResult::Hold:       return QStringLiteral("持有");
        case AnalysisResult::Sell:       return QStringLiteral("卖出");
        case AnalysisResult::StrongSell: return QStringLiteral("强烈卖出");
        default: return QStringLiteral("未知");
    }
}

} // namespace WealthPilot