/**
 * @file NewsTypes.h
 * @brief 新闻相关类型定义
 *
 * @details 定义新闻数据结构，供数据层、服务层、UI层使用
 */

#ifndef NEWSTYPES_H
#define NEWSTYPES_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QUrl>
#include <QVector>
#include <QJsonObject>

namespace WealthPilot {

/**
 * @brief 情绪类型
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
 * @brief 情绪分析结果
 */
struct SentimentResult
{
    double positive = 0;              ///< 正面情绪占比 (0-1)
    double negative = 0;              ///< 负面情绪占比 (0-1)
    double neutral = 0;               ///< 中性情绪占比 (0-1)
    double score = 0;                 ///< 情绪得分 (-1 到 1)
    SentimentType sentiment = SentimentType::Neutral; ///< 情绪类型
    double confidence = 0;            ///< 置信度 (0-1)
    QStringList keywords;             ///< 关键词
    QString summary;                  ///< 摘要
    QDateTime analyzedAt;             ///< 分析时间
    
    // 兼容字段
    double impactScore = 0.0;         ///< 影响分数
    QString riskHint;                 ///< 风险提示
    QString sentimentLabel;           ///< 情绪标签
    
    static SentimentResult fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

// 别名，兼容旧代码
using NewsSentimentResult = SentimentResult;

/**
 * @brief 情绪数据点
 */
struct SentimentPoint
{
    QDateTime timestamp;  ///< 时间戳
    double score = 0;     ///< 情绪得分
    double positive = 0;  ///< 正面占比
    double negative = 0;  ///< 负面占比
    int sampleCount = 0;  ///< 样本数量

    static SentimentPoint fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief 情绪指标
 */
struct SentimentIndex
{
    QString stockCode;            ///< 股票代码
    double currentScore = 0;      ///< 当前得分
    double avgScore7d = 0;        ///< 7日平均
    double avgScore30d = 0;       ///< 30日平均
    double trend = 0;             ///< 趋势 (-1 到 1)
    QString trendDirection;       ///< 趋势方向
    QDateTime updatedAt;          ///< 更新时间
    QList<SentimentPoint> history; ///< 历史数据

    static SentimentIndex fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief 新闻项
 */
struct NewsItem
{
    QString id;                      ///< 新闻ID
    QString title;                   ///< 标题
    QString summary;                 ///< 摘要
    QString content;                 ///< 正文内容
    QString source;                  ///< 来源
    QString author;                  ///< 作者
    QUrl url;                        ///< 原文链接
    QDateTime publishedAt;           ///< 发布时间
    QDateTime fetchTime;             ///< 抓取时间
    QString category;                ///< 分类
    QStringList relatedStocks;       ///< 关联股票代码
    
    // 情绪分析结果
    SentimentResult sentiment;       ///< 情绪分析结果
    double sentimentScore = 0.0;     ///< 情绪分数 (-1 到 1)
    QString sentimentLabel;          ///< 情绪标签
    
    // 热度指标
    int readCount = 0;               ///< 阅读数
    int commentCount = 0;            ///< 评论数
    int likeCount = 0;               ///< 点赞数
    double heatScore = 0.0;          ///< 热度分数
    bool isImportant = false;        ///< 是否重要新闻
    
    // 兼容旧字段名
    QDateTime publishTime;           ///< 发布时间（兼容）
    QVector<QString> relatedSymbols; ///< 关联股票代码（兼容）
    double impactScore = 0.0;        ///< 影响分数（兼容）

    static NewsItem fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

/**
 * @brief 社交热度数据
 */
struct SocialHeatData
{
    QString symbol;              ///< 股票代码
    QString platform;            ///< 平台名称
    int mentionCount = 0;        ///< 提及次数
    int previousCount = 0;       ///< 上期提及次数
    double changePercent = 0.0;  ///< 变化百分比
    double sentimentScore = 0.0; ///< 情感分数
    QDateTime updateTime;        ///< 更新时间
};

} // namespace WealthPilot

#endif // NEWSTYPES_H
