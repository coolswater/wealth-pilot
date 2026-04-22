/**
 * @file NewsDataSource.h
 * @brief 新闻数据源 - 对接华尔街见闻API
 */

#ifndef NEWSDATASOURCE_H
#define NEWSDATASOURCE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QVector>
#include <QDateTime>

/**
 * @brief 新闻数据结构
 */
struct NewsItem {
    QString id;             ///< 新闻ID
    QString title;          ///< 标题
    QString content;        ///< 内容摘要
    QString source;         ///< 来源
    QString author;         ///< 作者
    QString url;            ///< 原文链接
    QString imageUrl;       ///< 图片链接
    QStringList categories; ///< 分类
    qint64 timestamp = 0;   ///< 发布时间戳
    QDateTime publishTime;  ///< 发布时间
};

/**
 * @brief 新闻数据源
 */
class NewsDataSource : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 新闻频道
     */
    enum class Channel {
        Global,     ///< 全球
        AShares,    ///< A股
        USStocks,   ///< 美股
        Forex,      ///< 外汇
        Commodities ///< 商品
    };

    explicit NewsDataSource(QObject* parent = nullptr);
    ~NewsDataSource() override;

    /**
     * @brief 请求新闻列表
     * @param channel 频道
     * @param limit 数量限制
     */
    void requestNews(Channel channel = Channel::Global, int limit = 20);

    /**
     * @brief 获取缓存的新闻
     */
    QVector<NewsItem> cachedNews() const;

signals:
    /**
     * @brief 新闻数据接收完成
     */
    void newsReceived(const QVector<NewsItem>& news);

    /**
     * @brief 发生错误
     */
    void errorOccurred(const QString& error);

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    QString channelToString(Channel channel) const;
    void parseWallStNews(const QByteArray& data);

    QNetworkAccessManager* m_networkManager;
    QVector<NewsItem> m_cachedNews;
};

#endif // NEWSDATASOURCE_H
