// FuturesQuoteModel.h
#ifndef FUTURESQUOTEMODEL_H
#define FUTURESQUOTEMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QMutex>
#include "FuturesQuoteItem.h"

class FuturesQuoteModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit FuturesQuoteModel(QObject *parent = nullptr);

    // 必须实现的基础接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // 数据操作接口（关键修改：确保线程安全）
    void setQuotes(const QVector<FuturesQuoteItem> &quotes);  // 全量替换
    void updateQuote(const FuturesQuoteItem &quote);           // 单条更新
    void updateQuotes(const QVector<FuturesQuoteItem> &quotes); // 批量更新（新增）
    void clear();
    const FuturesQuoteItem* itemAt(int row) const;

    static QStringList columnNames();

private:
    QVector<FuturesQuoteItem> m_quotes;
    mutable QMutex m_mutex;  // 保护 m_quotes

    // 列定义
    enum Column {
        SerialNo = 0, ContractName, LatestPrice, CurrentHand,
        BidPrice, AskPrice, BidVolume, AskVolume, Volume,
        Change, ChangePercent, OpenInterest, OiChange,
        OpenPrice, HighPrice, LowPrice, Settlement,
        SpeedChange, CurrentChange, CurrentOiChange,
        Dynamic, PreSettlement, PreClose, Capital,
        CapitalFlow, TrendDegree, SpeculationDegree,
        ColumnCount
    };
};

#endif // FUTURESQUOTEMODEL_H
