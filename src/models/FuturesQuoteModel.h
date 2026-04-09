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
        SerialNo = 0,       // 序号
        ContractCode,       // 代码
        ContractName,       // 名称
        LatestPrice,        // 最新
        Change,             // 涨跌
        ChangePercent,      // 涨幅%
        CurrentHand,        // 现手
        BidPrice,           // 买价
        AskPrice,           // 卖价
        BidVolume,          // 买量
        AskVolume,          // 卖量
        Volume,             // 成交量
        OpenInterest,       // 持仓量
        OiChange,           // 日增仓
        OpenPrice,          // 开盘
        HighPrice,          // 最高
        LowPrice,           // 最低
        Settlement,         // 今结算
        SpeedChange,        // 速涨
        PreSettlement,      // 昨结
        PreClose,           // 昨收
        ColumnCount
    };

};

#endif // FUTURESQUOTEMODEL_H
