/**
 * @file StockQuoteModel.h
 * @brief 股票行情模型
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef STOCKQUOTEMODEL_H
#define STOCKQUOTEMODEL_H

#include "QuoteModelBase.h"
#include "StockQuoteItem.h"
#include "market/StockDataSource.h"
#include <QVector>

namespace WealthPilot
{
    /**
 * @brief 股票行情模型
 */
    class StockQuoteModel : public QuoteModelBase
    {
        Q_OBJECT

    public:
        explicit StockQuoteModel(QObject* parent = nullptr);
        ~StockQuoteModel() override = default;

        // 添加行情数据
        void addQuote(const StockQuoteItem& item);

        // 批量设置数据
        void setData(const QVector<StockQuote>& quotes);

        // 更新行情数据
        void updateQuote(const QString& code, const StockQuoteItem& item);

        // 获取行情数据
        StockQuoteItem getQuote(int row) const;
        StockQuoteItem getQuoteByCode(const QString& code) const;

        // 清空数据
        void clear();

        // 重写基类方法
        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        // 列定义
        enum Column
        {
            ColCode = 0, ///< 代码
            ColName, ///< 名称
            ColPrice, ///< 现价
            ColChange, ///< 涨跌额
            ColChangePercent, ///< 涨跌幅
            ColVolume, ///< 成交量
            ColTurnover, ///< 成交额
            ColMarketCap, ///< 市值
            ColCount ///< 列数
        };

    private:
        QVector<StockQuoteItem> m_data;
        QHash<QString, int> m_codeIndex; ///< 代码到行的索引
    };
} // namespace WealthPilot

#endif // STOCKQUOTEMODEL_H