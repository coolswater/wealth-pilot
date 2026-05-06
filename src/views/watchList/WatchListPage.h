#ifndef WATCHLISTPAGE_H
#define WATCHLISTPAGE_H

/**
 * @file WatchListPage.h
 * @brief 自选股页面 - 个人自选股管理
 */



#include <QTableView>
#include <QSortFilterProxyModel>
#include <QAbstractTableModel>
#include <memory>
#include "ui/components/BasePage.h"
#include "market/StockDataSource.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
QT_END_NAMESPACE

namespace WealthPilot {

/**
 * @brief 自选股表格模型
 */
class WatchListModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColCode = 0,      ///< 代码
        ColName,          ///< 名称
        ColPrice,         ///< 最新价
        ColChange,        ///< 涨跌幅
        ColChangeAmount,  ///< 涨跌额
        ColVolume,        ///< 成交量
        ColAmount,        ///< 成交额
        ColHigh,          ///< 最高价
        ColLow,           ///< 最低价
        ColCount
    };

    explicit WatchListModel(QObject* parent = nullptr);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    void setData(const QVector<StockQuote>& quotes);
    void addSymbol(const QString& symbol);
    void removeSymbol(int row);
    void clear();
    QStringList symbols() const;

private:
    QVector<StockQuote> m_data;
    QSet<QString> m_symbolSet;
    
    static QString formatVolume(qint64 volume);
    static QString formatMoney(double value);
};

/**
 * @brief 自选股页面
 */
class WatchListPage : public BasePage {
    Q_OBJECT

public:
    explicit WatchListPage(QWidget* parent = nullptr);
    ~WatchListPage() override;

    QString pageId() const override;
    void initializePage() override;
    void onPageActivated(const QVariantMap& params = {}) override;
    void onPageDeactivated() override;

public slots:
    void addStock(const QString& symbol, const QString& name = QString());

signals:
    void navigateToKLinePage(const QString& symbol, const QVariantMap& params);

private slots:
    void onQuotesReceived(const QVector<StockQuote>& quotes);
    void onSearchChanged(const QString& text);
    void onRefreshData();
    void onAddStock();
    void onRemoveStock();
    void onRowDoubleClicked(const QModelIndex& index);

private:
    void setupUI();
    void setupConnections();
    void loadWatchList();
    void saveWatchList();
    void requestStockData();

    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif
