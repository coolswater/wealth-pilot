/**
 * @file StockQuoteModel.cpp
 * @brief 股票行情模型实现
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "StockQuoteModel.h"
#include "market/StockDataSource.h"

namespace WealthPilot
{
    StockQuoteModel::StockQuoteModel(QObject* parent)
        : QuoteModelBase(parent)
    {
    }

    void StockQuoteModel::addQuote(const StockQuoteItem& item)
    {
        if (m_codeIndex.contains(item.code))
        {
            // 已存在，更新
            updateQuote(item.code, item);
            return;
        }

        int row = m_data.size();
        beginInsertRows(QModelIndex(), row, row);
        m_data.append(item);
        m_codeIndex[item.code] = row;
        endInsertRows();
    }

    void StockQuoteModel::setData(const QVector<StockQuote>& quotes)
    {
        beginResetModel();
        m_data.clear();
        m_codeIndex.clear();

        for (const StockQuote& quote : quotes)
        {
            StockQuoteItem item;
            item.code = quote.symbol;
            item.name = quote.name;
            item.price = quote.lastPrice;
            item.preClose = quote.preClose;
            item.change = quote.changeAmount;
            item.changePercent = quote.changePercent;
            item.volume = quote.volume;
            item.turnover = quote.turnover;
            item.marketCap = 0; // 需要额外数据
            m_data.append(item);
            m_codeIndex[item.code] = m_data.size() - 1;
        }

        endResetModel();
    }

    void StockQuoteModel::updateQuote(const QString& code, const StockQuoteItem& item)
    {
        if (!m_codeIndex.contains(code))
        {
            addQuote(item);
            return;
        }

        int row = m_codeIndex[code];
        m_data[row] = item;

        // 发射数据变化信号
        emit dataChanged(index(row, 0), index(row, ColCount - 1));
    }

    StockQuoteItem StockQuoteModel::getQuote(int row) const
    {
        if (row >= 0 && row < m_data.size())
        {
            return m_data[row];
        }
        return StockQuoteItem();
    }

    StockQuoteItem StockQuoteModel::getQuoteByCode(const QString& code) const
    {
        if (m_codeIndex.contains(code))
        {
            return m_data[m_codeIndex[code]];
        }
        return StockQuoteItem();
    }

    void StockQuoteModel::clear()
    {
        beginResetModel();
        m_data.clear();
        m_codeIndex.clear();
        endResetModel();
    }

    int StockQuoteModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) return 0;
        return m_data.size();
    }

    int StockQuoteModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) return 0;
        return ColCount;
    }

    QVariant StockQuoteModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() >= m_data.size())
        {
            return QVariant();
        }

        const StockQuoteItem& item = m_data[index.row()];

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case ColCode: return item.code;
            case ColName: return item.name;
            case ColPrice: return QString::number(item.price, 'f', 2);
            case ColChange: return QString::number(item.change, 'f', 2);
            case ColChangePercent: return QString::number(item.changePercent, 'f', 2) + "%";
            case ColVolume: return QString::number(item.volume / 10000, 'f', 0) + "万";
            case ColTurnover: return QString::number(item.turnover / 10000, 'f', 0) + "万";
            case ColMarketCap: return QString::number(item.marketCap, 'f', 2) + "亿";
            default: return QVariant();
            }
        }

        if (role == Qt::UserRole)
        {
            // 返回原始数值
            switch (index.column())
            {
            case ColPrice: return item.price;
            case ColChange: return item.change;
            case ColChangePercent: return item.changePercent;
            case ColVolume: return item.volume;
            case ColTurnover: return item.turnover;
            case ColMarketCap: return item.marketCap;
            default: return QVariant();
            }
        }

        return QVariant();
    }

    QVariant StockQuoteModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        {
            return QVariant();
        }

        switch (section)
        {
        case ColCode: return QStringLiteral("代码");
        case ColName: return QStringLiteral("名称");
        case ColPrice: return QStringLiteral("现价");
        case ColChange: return QStringLiteral("涨跌额");
        case ColChangePercent: return QStringLiteral("涨跌幅");
        case ColVolume: return QStringLiteral("成交量");
        case ColTurnover: return QStringLiteral("成交额");
        case ColMarketCap: return QStringLiteral("市值");
        default: return QVariant();
        }
    }
} // namespace WealthPilot
