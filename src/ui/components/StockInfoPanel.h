/**
 * @file StockInfoPanel.h
 * @brief 股票信息面板 - 显示实时行情数据
 */

#ifndef STOCKINFOPANEL_H
#define STOCKINFOPANEL_H

#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QScrollArea>
#include <QVector>
#include "market/StockDataSource.h"

// 使用 WealthPilot 命名空间中的类型
using WealthPilot::StockQuote;
using WealthPilot::TickData;

/**
 * @brief 股票信息面板
 */
class StockInfoPanel : public QWidget
{
    Q_OBJECT

public:
    explicit StockInfoPanel(QWidget* parent = nullptr);
    ~StockInfoPanel() override;

    void setStock(const QString& stockCode, const QString& stockName = QString());
    void updateQuote(const StockQuote& quote);
    void updateTickData(const QVector<TickData>& ticks);
    void clearData();

signals:
    void stockChanged(const QString& stockCode);

private slots:
    void onQuoteReceived(const QString& symbol, const StockQuote& quote);

private:
    void setupUI();
    void updatePriceLabel(QLabel* label, double price, double prevPrice = 0.0);
    QString formatVolume(qint64 volume) const;
    QString formatAmount(double amount) const;

    // 三层缓存机制
    void loadQuoteWithFallback();
    bool loadQuoteFromCache();
    bool loadQuoteFromDatabase();
    void loadQuoteFromNetwork();
    void saveQuoteToCache();
    void saveQuoteToDatabase();

    QString quoteCacheKey() const;

    // UI组件
    QLabel* m_stockNameLabel = nullptr;
    QLabel* m_priceLabel = nullptr;
    QLabel* m_changeLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_orderRatioLabel = nullptr;
    QLabel* m_bidLabels[5];
    QLabel* m_askLabels[5];
    QTableWidget* m_detailTable = nullptr;
    QTableWidget* m_tickTable = nullptr;

    // 数据源
    StockDataSource* m_dataSource = nullptr;

    // 数据
    QString m_stockCode;
    StockQuote m_currentQuote;
    QVector<TickData> m_tickData;
};

#endif // STOCKINFOPANEL_H
