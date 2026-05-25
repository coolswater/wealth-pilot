#ifndef QUOTEDATAMANAGER_H
#define QUOTEDATAMANAGER_H
#include <QNetworkReply>
#include <QTimer>
#include <QMap>

#include "data/models/StockQuoteItem.h"

/**
 * @brief 行情数据管理器
 * 支持实时行情获取(股票+期货)
 * 支持历史数据缓存
 */
class QuoteDataManager : public QObject
{
    Q_OBJECT

public:
    explicit QuoteDataManager(QObject *parent = nullptr);

    // 数据获取
    void requestRealTimeQuotes(const QStringList &codes);
    void requestMarketList(const QString &market);
    void requestHistoryData(const QString &code, const QDateTime &start,
                           const QDateTime &end);

    // 模拟数据(用于演示)
    void generateMockData(int count = 500);
    void generateMockFuturesData();

    // 数据查询
    QVector<StockQuoteItem> getAllItems() const;
    QVector<StockQuoteItem> getItemsByMarket(const QString &market) const;
    StockQuoteItem getItem(const QString &code) const;

    // 市场分类
    static QStringList getMarketCategories();
    static QStringList getFuturesCategories();

    signals:
        void dataUpdated(const QVector<StockQuoteItem> &items);
    void itemUpdated(const StockQuoteItem &item);
    void error(const QString &message);

public slots:
    void startAutoUpdate(int intervalMs = 8000);  // 默认8秒更新
    void stopAutoUpdate();
    void refresh();

private slots:
    void parseHistoryData(const QByteArray& data, const QString& code);
    void onNetworkReply(QNetworkReply *reply);
    void onUpdateTimer();

private:
    void parseSinaQuote(const QString &response);  // 解析新浪行情
    void parseEastMoneyQuote(const QByteArray &data); // 解析东方财富

    QNetworkAccessManager *m_networkManager;
    QTimer *m_updateTimer;

    QMap<QString, StockQuoteItem> m_itemMap;
    QStringList m_currentCodes;
    int m_updateInterval;

    // 支持的数据源
    enum DataSource {
        Sina,           // 新浪财经(免费)
        EastMoney,      // 东方财富
        Tushare,        // Tushare Pro(需API Key)
        Mock            // 模拟数据
    };
    DataSource m_source;
};

#endif