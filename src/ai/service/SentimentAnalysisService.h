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

// 使用统一的类型定义
#include "../../core/types/NewsTypes.h"

namespace WealthPilot
{

/**
 * @brief 分析结果（服务特有）
 */
struct AnalysisResult
{
    QString taskId;
    QString stockCode;
    SentimentResult sentiment;
    QList<SentimentPoint> timeline;
    QDateTime createdAt;
    QString errorMessage;
    bool success = false;
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
                          std::function<void(const QList<SentimentResult>&)> callback);

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
    void getSectorSentiment(const QString& sectorName,
                            std::function<void(const SentimentResult&)> callback);

    /**
     * @brief 获取股票情绪指标
     */
    void getStockSentimentIndex(const QString& stockCode,
                                std::function<void(const SentimentIndex&)> callback);

    // ========== 社交媒体情绪 ==========

    /**
     * @brief 分析社交媒体情绪
     */
    void analyzeSocialMedia(const QString& platform,
                            const QString& stockCode,
                            std::function<void(const SentimentResult&)> callback);

    /**
     * @brief 获取社交媒体热点
     */
    void getSocialHotspots(std::function<void(const QList<QString>&)> callback);

    // ========== 情绪趋势 ==========

    /**
     * @brief 获取情绪趋势
     */
    void getSentimentTrend(const QString& stockCode,
                           const QDateTime& from,
                           const QDateTime& to,
                           std::function<void(const QList<SentimentPoint>&)> callback);

    /**
     * @brief 获取情绪变化预警
     */
    void getSentimentAlerts(std::function<void(const QList<QString>&)> callback);

    // ========== 配置 ==========

    /**
     * @brief 设置分析参数
     */
    void setAnalysisParams(const QJsonObject& params);

    /**
     * @brief 获取分析参数
     */
    QJsonObject getAnalysisParams() const;

signals:
    /**
     * @brief 情绪分析完成
     */
    void analysisCompleted(const QString& taskId, const SentimentResult& result);

    /**
     * @brief 市场情绪更新
     */
    void marketSentimentUpdated(const SentimentResult& sentiment);

    /**
     * @brief 情绪预警
     */
    void sentimentAlert(const QString& stockCode, const QString& alertMessage);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
    void analyzeTextWithAI(const QString& text, std::function<void(const SentimentResult&)> callback);
    void updateSentimentIndex(const QString& stockCode, const SentimentResult& result);
    SentimentType scoreToType(double score);
};

} // namespace WealthPilot

#endif // SENTIMENTANALYSISSERVICE_H