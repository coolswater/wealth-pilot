#include "FuturesQuoteItem.h"
#include "utils/Logger.h"
#include "FuturesQuoteModel.h"
#include <QBrush>
#include <QFont>
#include <QLocale>
#include <QModelIndex>
#include <cmath>
#include <algorithm>  // for std::sort

QStringList FuturesQuoteModel::columnNames()
{
    return {
        "序号", "代码", "名称", "最新", "涨跌", "涨幅%","现手",
        "买价", "卖价", "买量", "卖量", "成交量","持仓量","日增仓",
        "开盘", "最高", "最低", "今结算", "速涨","昨结", "昨收"
    };
}

FuturesQuoteModel::FuturesQuoteModel(QObject *parent) : QAbstractTableModel(parent) {}

/**
 * @brief 批量更新（性能优化关键）
 * @note 使用 dataChanged 批量更新，避免全表重置
 */
void FuturesQuoteModel::updateQuotes(const QVector<FuturesQuoteItem> &quotes)
{
    if (quotes.isEmpty()) return;

    // 收集需要更新的行
    QVector<int> updatedRows;
    QVector<int> newRows;

    {
        QMutexLocker locker(&m_mutex);
        
        for (const auto& quote : quotes) {
            bool found = false;
            for (int i = 0; i < m_quotes.size(); ++i) {
                if (m_quotes[i].contractName == quote.contractName) {
                    m_quotes[i] = quote;
                    updatedRows.append(i);
                    found = true;
                    break;
                }
            }
            if (!found) {
                newRows.append(m_quotes.size());
                m_quotes.append(quote);
            }
        }
    }

    // 批量更新现有行（不重置整个模型）
    if (!updatedRows.isEmpty()) {
        // 合并连续的行，减少 dataChanged 调用次数
        std::sort(updatedRows.begin(), updatedRows.end());
        int startRow = updatedRows.first();
        int endRow = startRow;
        
        for (int i = 1; i < updatedRows.size(); ++i) {
            if (updatedRows[i] == endRow + 1) {
                endRow = updatedRows[i];
            } else {
                // 发射连续区间的更新信号
                emit dataChanged(index(startRow, 0), index(endRow, ColumnCount - 1));
                startRow = endRow = updatedRows[i];
            }
        }
        // 发射最后一个区间
        emit dataChanged(index(startRow, 0), index(endRow, ColumnCount - 1));
    }

    // 插入新行
    for (int row : newRows) {
        beginInsertRows(QModelIndex(), row, row);
        endInsertRows();
    }
}

int FuturesQuoteModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    QMutexLocker locker(&m_mutex);
    return m_quotes.size();
}

int FuturesQuoteModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return ColumnCount;
}

QVariant FuturesQuoteModel::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid() || index.row() >= m_quotes.size())
        return {};

    QMutexLocker locker(&m_mutex);
    const auto &item = m_quotes.at(index.row());

    switch (role) {
        case Qt::DisplayRole: {
            switch (index.column()) {
                case SerialNo:       return index.row() + 1;
                case ContractCode:   return item.contractName;  // 代码列显示合约名
                case ContractName:   return item.contractName;  // 名称列（如果有单独的名称字段）
                case LatestPrice:
                    if (item.lastPrice > 1e308) return "--";
                    return QString::number(item.lastPrice, 'f', 2);
                case Change:
                    if (qAbs(item.change) < 0.0001) return "0.00";
                    return QString((item.change > 0 ? "+" : "")) + QString::number(item.change, 'f', 2);
                case ChangePercent:
                    if (qAbs(item.changePercent) < 0.0001) return "0.00%";
                    return QString((item.changePercent > 0 ? "+" : "")) + QString::number(item.changePercent, 'f', 2) + "%";
                case CurrentHand:    return item.currentHand;
                case BidPrice:
                    if (item.bidPrice > 1e308) return "--";
                    return QString::number(item.bidPrice, 'f', 2);
                case AskPrice:
                    if (item.askPrice > 1e308) return "--";
                    return QString::number(item.askPrice, 'f', 2);
                case BidVolume:      return item.bidVolume;
                case AskVolume:      return item.askVolume;
                case Volume:         return QLocale().toString(item.volume);
                case OpenInterest:   return QLocale().toString(item.openInterest);
                case OiChange:       return item.oiChange;
                case OpenPrice:
                    if (item.openPrice > 1e308) return "--";
                    return QString::number(item.openPrice, 'f', 2);
                case HighPrice:
                    if (item.highPrice > 1e308) return "--";
                    return QString::number(item.highPrice, 'f', 2);
                case LowPrice:
                    if (item.lowPrice > 1e308) return "--";
                    return QString::number(item.lowPrice, 'f', 2);
                case Settlement:
                    if (item.settlement > 1e308) return "--";
                    return QString::number(item.settlement, 'f', 2);
                case SpeedChange:
                    if (qAbs(item.speedChange) < 0.0001) return "0.00";
                    return QString::number(item.speedChange, 'f', 2);
                case PreSettlement:
                    if (item.preSettlementPrice > 1e308) return "--";
                    return QString::number(item.preSettlementPrice, 'f', 2);
                case PreClose:
                    if (item.preClose > 1e308) return "--";
                    return QString::number(item.preClose, 'f', 2);
                default: return {};
            }
        }
    case Qt::TextAlignmentRole:
        return (index.column() == ContractName) ? Qt::AlignLeft : Qt::AlignRight;

    case Qt::UserRole:  // 存储原始数据，委托用
        return QVariant::fromValue(item);

    default:
        return {};
    }
}

QVariant FuturesQuoteModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section >= 0 && section < columnNames().size())
            return columnNames().at(section);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

Qt::ItemFlags FuturesQuoteModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

/**
 * @brief 全量设置数据（线程安全）
 * @note 关键修复：在类内部调用 beginResetModel/endResetModel（合法）
 */
void FuturesQuoteModel::setQuotes(const QVector<FuturesQuoteItem> &quotes)
{
    // 在类内部调用 protected 方法是完全合法的
    beginResetModel();  // 通知视图开始重置（冻结 UI 更新）

    {
        QMutexLocker locker(&m_mutex);
        m_quotes = quotes;
    }  // 锁在这里释放

    endResetModel();    // 通知视图结束重置（触发一次性重绘）

    LOG_INFO(QString("Model reset with %1 quotes").arg(quotes.size()));
}

/**
 * @brief 单条更新（用于实时行情）
 * @note 使用 dataChanged 信号而非 reset，避免全表重绘
 */
void FuturesQuoteModel::updateQuote(const FuturesQuoteItem &quote)
{
    int row = -1;

    {
        QMutexLocker locker(&m_mutex);
        // 查找现有合约
        for (int i = 0; i < m_quotes.size(); ++i) {
            if (m_quotes[i].contractName == quote.contractName) {
                m_quotes[i] = quote;
                row = i;
                break;
            }
        }
        // 如果未找到，追加（实际业务中可能需要限制大小）
        if (row == -1) {
            beginInsertRows(QModelIndex(), m_quotes.size(), m_quotes.size());
            m_quotes.append(quote);
            endInsertRows();
            return;
        }
    }

    // 发射数据变更信号（指定行），视图只重绘该行
    if (row >= 0) {
        emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
    }
}


void FuturesQuoteModel::clear()
{
    beginResetModel();
    m_quotes.clear();
    endResetModel();
}

const FuturesQuoteItem* FuturesQuoteModel::itemAt(int row) const
{
    QMutexLocker locker(&m_mutex);
    if (row < 0 || row >= m_quotes.size()) return nullptr;
    return &m_quotes[row];
}
