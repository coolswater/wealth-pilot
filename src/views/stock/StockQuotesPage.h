#ifndef STOCKQUOTESPAGE_H
#define STOCKQUOTESPAGE_H

/**
 * @file StockQuotesPage.h
 * @brief 股票行情页面
 */

#pragma once

#include "ui/components/BasePage.h"
#include <QTableView>
#include <QAbstractTableModel>
#include <memory>

class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;

namespace WealthPilot {

/**
 * @brief 股票行情数据
 */
struct StockQuoteData {
    QString symbol;
    QString name;
    double price = 0.0;
    double change = 0.0;
    double changePercent = 0.0;
    qint64 volume = 0;
};

/**
 * @brief 股票行情表格模型
 */
class StockQuoteModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column {
        ColCode = 0,
        ColName,
        ColPrice,
        ColChange,
        ColChangePercent,
        ColVolume,
        ColCount
    };

    explicit StockQuoteModel(QObject* parent = nullptr);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    void setData(const QVector<StockQuoteData>& quotes);
    void clear();

private:
    QVector<StockQuoteData> m_data;
};

/**
 * @brief 股票行情页面
 */
class StockQuotesPage : public BasePage {
    Q_OBJECT

public:
    explicit StockQuotesPage(QWidget* parent = nullptr);
    ~StockQuotesPage() override;

    QString pageId() const override { return "stock-quotes"; }
    QString pageName() const override { return QStringLiteral("股票行情"); }
    void initializePage() override;

signals:
    void navigateToKLinePage(const QString& symbol, const QString& name);

private slots:
    void onSearchChanged(const QString& text);
    void onRefreshData();
    void onRowDoubleClicked(const QModelIndex& index);

private:
    void setupUI();
    void setupConnections();

    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_filterCombo = nullptr;
    QTableView* m_tableView = nullptr;
    StockQuoteModel* m_model = nullptr;
    QLabel* m_statusLabel = nullptr;
};



} // namespace WealthPilot

#endif
