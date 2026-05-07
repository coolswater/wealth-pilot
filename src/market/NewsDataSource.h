/**
 * @file NewsDataSource.h
 * @brief 新闻数据源 - 财经新闻获取与管理
 *
 * @details 提供新闻数据功能：
 * - 多源新闻获取（华尔街见闻、财联社等）
 * - 新闻分类管理
 * - 自选股新闻推送
 * - 社交媒体热度监控
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef NEWSDATASOURCE_H
#define NEWSDATASOURCE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QMap>
#include <QVector>
#include "core/analysis/NewsSentimentAnalyzer.h"

/**
 * @brief 新闻数据源
 */
class NewsDataSource : public QObject
{
    Q_OBJECT

public:
    static NewsDataSource* instance();

    /**
     * @brief 初始化数据源
     */
    bool initialize();

    /**
     * @brief 请求新闻列表
     */
    void requestNews(const QString& symbol = QString(), int count = 50);

    /**
     * @brief 请求公告列表
     */
    void requestAnnouncements(const QString& symbol, int count = 20);

    /**
     * @brief 请求财报列表
     */
    void requestFinancialReports(const QString& symbol, int count = 10);

    /**
     * @brief 请求研报列表
     */
    void requestResearchReports(const QString& symbol, int count = 10);

    /**
     * @brief 请求社交媒体热度
     */
    void requestSocialHeat(const QString& symbol);

    /**
     * @brief 订阅自选股新闻
     */
    void subscribeSymbols(const QVector<QString>& symbols);

    /**
     * @brief 取消订阅
     */
    void unsubscribeSymbol(const QString& symbol);

    /**
     * @brief 获取缓存的新闻
     */
    QVector<NewsItem> getCachedNews(const QString& symbol = QString()) const;

    /**
     * @brief 获取社交媒体热度
     */
    QVector<SocialHeatData> getSocialHeat(const QString& symbol) const;

    /**
     * @brief 设置更新间隔（秒）
     */
    void setUpdateInterval(int seconds);

signals:
    /**
     * @brief 新闻更新信号
     */
    void newsUpdated(const QString& symbol, const QVector<NewsItem>& news);

    /**
     * @brief 公告更新信号
     */
    void announcementsUpdated(const QString& symbol, const QVector<NewsItem>& announcements);

    /**
     * @brief 财报更新信号
     */
    void financialReportsUpdated(const QString& symbol, const QVector<NewsItem>& reports);

    /**
     * @brief 研报更新信号
     */
    void researchReportsUpdated(const QString& symbol, const QVector<NewsItem>& reports);

    /**
     * @brief 社交热度更新信号
     */
    void socialHeatUpdated(const QString& symbol, const SocialHeatData& heat);

    /**
     * @brief 重要新闻推送信号
     */
    void importantNewsPush(const NewsItem& news);

private slots:
    void onNetworkReply(QNetworkReply* reply);
    void onPeriodicUpdate();

private:
    explicit NewsDataSource(QObject* parent = nullptr);
    ~NewsDataSource() override;

    // 数据请求
    void fetchNewsFromAPI(const QString& symbol, const QString& category);
    void fetchSocialHeatFromAPI(const QString& symbol);

    // 数据解析
    QVector<NewsItem> parseNewsResponse(const QByteArray& data);
    SocialHeatData parseSocialHeatResponse(const QByteArray& data);

    // 新闻推送
    void checkAndPushImportantNews(const NewsItem& news);

    // 数据成员
    QNetworkAccessManager* m_networkManager = nullptr;
    QTimer* m_updateTimer = nullptr;

    QMap<QString, QVector<NewsItem>> m_newsCache;      // symbol -> news
    QMap<QString, SocialHeatData> m_socialHeatCache;    // symbol -> heat

    QVector<QString> m_subscribedSymbols;
    int m_updateInterval = 300; // 5分钟

    bool m_initialized = false;
};

#endif // NEWSDATASOURCE_H