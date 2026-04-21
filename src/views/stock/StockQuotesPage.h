#ifndef STOCKQUOTES_H
#define STOCKQUOTES_H

#include <memory>
#include <QVector>
#include <core/base/BasePage.h>
#include "market/StockDataSource.h"
#include "models/StockQuoteItem.h"

class QTableWidget;
class QComboBox;
class QLineEdit;
class QLabel;
class QTimer;

/**
 * @brief 股票行情页面
 * @details 展示实时股票行情数据，支持搜索、筛选、自选
 */
class StockQuotesPage : public BasePage
{
    Q_OBJECT

public:
    explicit StockQuotesPage(QWidget *parent = nullptr);
    ~StockQuotesPage();

    QString pageId() const override;
    void initializePage() override;

private slots:
    void onQuotesReceived(const QVector<StockQuote>& quotes);
    void onSearchTextChanged(const QString& text);
    void onFilterChanged(int index);
    void onRefreshData();
    void onViewButtonClicked();
    void onAddToFavoriteClicked();

private:
    void setupUI();
    void setupConnections();
    void updateTable(const QVector<StockQuote>& quotes);
    void updateTableRow(int row, const StockQuote& quote);
    void applyFilter();
    QString formatVolume(qint64 volume) const;
    QString formatMoney(double value) const;

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // STOCKQUOTES_H
