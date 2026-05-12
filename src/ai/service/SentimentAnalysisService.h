/**
 * @file SentimentAnalysisService.h
 * @brief 情绪分析服务
 *
 * @details 功能：
 * - 新闻情绪分析
 * - 社交媒体情绪分析
 * - 市场情绪指标计算
 * - 情绪趋势追踪
 * - 情绪可视化数据
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SENTIMENTANALYSISSERVICE_H
#define SENTIMENTANALYSISSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <functional>

namespace WealthPilot
{
    /**
 * @brief 情绪类型
 */
    enum class SentimentType
    {
        VeryNegative = -2, ///< 非常负面
        Negative = -1, ///< 负面
        Neutral = 0, ///< 中性
        Positive = 1, ///< 正面
        VeryPositive = 2 ///< 非常正面
    };

    /**
 * @brief 情绪分析结果
 */
    struct SentimentResult
    {
        double positive = 0; ///< 正面情绪占比 (0-1)
        double negative = 0; ///< 负面情绪占比 (0-1)
        double neutral = 0; ///< 中性情绪占比 (0-1)
        double score = 0; ///< 情绪得分 (-1 到 1)
        SentimentType type = SentimentType::Neutral; ///< 情绪类型
        double confidence = 0; ///< 置信度 (0-1)
        QStringList keywords; ///< 关键词
        QString summary; ///< 摘要
        QDateTime analyzedAt; ///< 分析时间

        static SentimentResult fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 情绪数据点
 */
    struct SentimentPoint
    {
        QDateTime timestamp; ///< 时间戳
        double score = 0; ///< 情绪得分
        double positive = 0; ///< 正面占比
        double negative = 0; ///< 负面占比
        int sampleCount = 0; ///< 样本数量

        static SentimentPoint fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 情绪指标
 */
    struct SentimentIndex
    {
        QString stockCode; ///< 股票代码
        double currentScore = 0; ///< 当前得分
        double avgScore7d = 0; ///< 7日平均
        double avgScore30d = 0; ///< 30日平均
        double trend = 0; ///< 趋势 (-1 到 1)
        QString trendDirection; ///< 趋势方向
        QDateTime updatedAt; ///< 更新时间
        QList<SentimentPoint> history; ///< 历史数据

        static SentimentIndex fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 新闻项
 */
    struct NewsItem
    {
        QString id;
        QString title;
        QString content;
        QString source;
        QDateTime publishedAt;
        QStringList relatedStocks;
        SentimentResult sentiment;

        static NewsItem fromJson(const QJsonObject& json);
        QJsonObject toJson() const;
    };

    /**
 * @brief 情绪分析服务
 */
    class SentimentAnalysisService : public QObject
    {
        Q_OBJECT

    public:
        explicit SentimentAnalysisService(QObject* parent = nullptr);
        ~SentimentAnalysisService() override;

        // ========== 新闻情绪分析 ==========

        /**
     * @brief 分析单条新闻情绪
     */
        void analyzeNews(const NewsItem& news,
                         std::function<void(const SentimentResult&)> callback);

        /**
     * @brief 批量分析新闻情绪
     */
        void analyzeNewsBatch(const QList<NewsItem>& newsList,
                              std::function<void(const QList<SentimentResult> &)> callback);

        /**
     * @brief 获取股票相关新闻情绪
     */
        void getStockNewsSentiment(const QString& stockCode,
                                   std::function<void(const SentimentResult&)> callback);

        // ========== 市场情绪 ==========

        /**
     * @brief 获取市场整体情绪
     */
        void getMarketSentiment(std::function<void(const SentimentResult&)> callback);

        /**
     * @brief 获取板块情绪
     */
        void getSectorSentiment(const QString& sector,
                                std::function<void(const SentimentResult&)> callback);

        // ========== 情绪指标 ==========

        /**
     * @brief 获取股票情绪指标
     */
        SentimentIndex getSentimentIndex(const QString& stockCode) const;

        /**
     * @brief 获取情绪趋势
     */
        QList<SentimentPoint> getSentimentTrend(const QString& stockCode, int days = 30) const;

        /**
     * @brief 计算情绪指标
     */
        void calculateSentimentIndex(const QString& stockCode,
                                     std::function<void(const SentimentIndex&)> callback);

        // ========== 情绪监控 ==========

        /**
     * @brief 添加情绪监控
     */
        void addSentimentWatch(const QString& stockCode);

        /**
     * @brief 移除情绪监控
     */
        void removeSentimentWatch(const QString& stockCode);

        // ========== 工具方法 ==========

        static QString getSentimentTypeName(SentimentType type);
        static SentimentType scoreToType(double score);

        signals :

        void sentimentAnalyzed(const QString& stockCode, const SentimentResult& result);
        void sentimentIndexChanged(const QString& stockCode, const SentimentIndex& index);
        void errorOccurred(const QString& error);

    private:
        SentimentResult analyzeTextLocally(const QString& text);
        void analyzeTextWithAI(const QString& text,
                               std::function<void(const SentimentResult&)> callback);
        void updateSentimentIndex(const QString& stockCode, const SentimentResult& result);

    private:
        QMap<QString, SentimentIndex> m_sentimentIndices;
        QStringList m_watchList;
        QString m_storagePath;
    };
} // namespace WealthPilot

#endif // SENTIMENTANALYSISSERVICE_H