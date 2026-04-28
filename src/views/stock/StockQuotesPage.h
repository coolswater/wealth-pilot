/**
 * @file StockQuotesPage.h
 * @brief 股票行情页面 - 对接真实行情数据
 */

#pragma once

#include <QTableView>
#include <QSortFilterProxyModel>
#include <QAbstractTableModel>
#include <memory>
#include <core/base/BasePage.h>
#include "market/StockDataSource.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
QT_END_NAMESPACE

/**
 * @brief 股票行情表格模型
 */
class StockQuoteModel : public QAbstractTableModel {
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
        ColCount
    };

    explicit StockQuoteModel(QObject* parent = nullptr);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    void setData(const QVector<StockQuote>& quotes);
    void updateQuote(const StockQuote& quote);
    void clear();

private:
    QVector<StockQuote> m_data;
    QHash<QString, int> m_symbolIndex;  ///< 快速查找索引
    
    static QString formatVolume(qint64 volume);
    static QString formatMoney(double value);
};

/**
 * @brief 股票行情页面
 */
class StockQuotesPage : public BasePage {
    Q_OBJECT

public:
    explicit StockQuotesPage(QWidget* parent = nullptr);
    ~StockQuotesPage() override;

    QString pageId() const override;
    void initializePage() override;
    void onPageActivated(const QVariantMap& params = {}) override;
    void onPageDeactivated() override;

signals:
    void navigateToKLinePage(const QString& symbol, const QString& name);

private slots:
    void onQuotesReceived(const QVector<StockQuote>& quotes);
    void onSearchChanged(const QString& text);
    void onFilterChanged(int index);
    void onRefreshData();
    void onRowDoubleClicked(const QModelIndex& index);

private:
    void setupUI();
    void setupConnections();
    void requestStockData();
    void updateStatus(const QString& text);

    class Impl;
    std::unique_ptr<Impl> d;
};
