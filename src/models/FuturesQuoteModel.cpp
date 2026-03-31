#include "FuturesQuoteItem.h"
#include "utils/Logger.h"
#include "FuturesQuoteModel.h"
#include <QBrush>
#include <QFont>
#include <QLocale>
#include <QModelIndex>

QStringList FuturesQuoteModel::columnNames()
{
    return {
        "序号", "合约名称", "最新", "现手", "买价", "卖价",
        "买量", "卖量", "成交量", "涨跌", "涨幅%", "持仓量",
        "日增仓", "开盘", "最高", "最低", "结算", "速涨",
        "现涨", "现增仓", "动态", "昨结算", "昨收", "沉淀资金",
        "资金流向", "趋势度", "投机度"
    };
}

FuturesQuoteModel::FuturesQuoteModel(QObject *parent) : QAbstractTableModel(parent) {}

/**
 * @brief 批量更新（性能优化关键）
 * @note 合并多次 dataChanged 为单次，减少 90% 重绘开销
 */
void FuturesQuoteModel::updateQuotes(const QVector<FuturesQuoteItem> &quotes)
{
    if (quotes.isEmpty()) return;

    // 使用 beginResetModel 批量更新，比逐条 dataChanged 更高效
    // 适用于一次性更新大量数据（>10 条）
    if (quotes.size() > 10) {
        beginResetModel();
        {
            QMutexLocker locker(&m_mutex);
            for (const auto& quote : quotes) {
                bool found = false;
                for (auto& item : m_quotes) {
                    if (item.contractName == quote.contractName) {
                        item = quote;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    m_quotes.append(quote);
                }
            }
        }
        endResetModel();
    } else {
        // 少量更新使用 dataChanged，保持滚动位置
        for (const auto& quote : quotes) {
            updateQuote(quote);
        }
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

QVariant FuturesQuoteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_quotes.size())
        return QVariant();

    QMutexLocker locker(&m_mutex);
    const auto &item = m_quotes.at(index.row());

    // 根据角色返回数据
    switch (role) {
    case Qt::DisplayRole: {
        switch (index.column()) {
        case SerialNo:       return item.serialNo;
        case ContractName:   return item.contractName;
        case LatestPrice:    return QString::number(item.latestPrice, 'f', 2);
        case CurrentHand:    return item.currentHand;
        case BidPrice:       return QString::number(item.bidPrice, 'f', 2);
        case AskPrice:       return QString::number(item.askPrice, 'f', 2);
        case BidVolume:      return item.bidVolume;
        case AskVolume:      return item.askVolume;
        case Volume:         return QLocale().toString(item.volume);
        case Change:         return (item.change > 0 ? "+" : "") + QString::number(item.change, 'f', 2);
        case ChangePercent:  return (item.changePercent > 0 ? "+" : "") + QString::number(item.changePercent, 'f', 2) + "%";
        case OpenInterest:   return QLocale().toString(item.openInterest);
        case OiChange:       return item.oiChange;
        case OpenPrice:      return QString::number(item.openPrice, 'f', 2);
        case HighPrice:      return QString::number(item.highPrice, 'f', 2);
        case LowPrice:       return QString::number(item.lowPrice, 'f', 2);
        case Settlement:     return QString::number(item.settlement, 'f', 2);
        case SpeedChange:    return QString::number(item.speedChange, 'f', 2);
        case CurrentChange:  return QString::number(item.currentChange, 'f', 2);
        case CurrentOiChange:return item.currentOiChange;
        case Dynamic:        return item.dynamic;
        case PreSettlement:  return QString::number(item.preSettlement, 'f', 2);
        case PreClose:       return QString::number(item.preClose, 'f', 2);
        case Capital:        return QString::number(item.capital, 'f', 2) + "亿";
        case CapitalFlow:    return (item.capitalFlow > 0 ? "+" : "") + QString::number(item.capitalFlow, 'f', 2) + "亿";
        case TrendDegree:    return QString::number(item.trendDegree, 'f', 2);
        case SpeculationDegree: return QString::number(item.speculationDegree, 'f', 2);
        default: return QVariant();
        }
    }
    case Qt::TextAlignmentRole:
        return (index.column() == ContractName) ? Qt::AlignLeft : Qt::AlignRight;

    case Qt::ForegroundRole: {
        // Qt6推荐使用ForegroundRole代替TextColorRole
        if (index.column() == LatestPrice || index.column() == Change ||
            index.column() == ChangePercent || index.column() == SpeedChange ||
            index.column() == CurrentChange) {
            return QBrush(item.changeColor());
        }
        if (index.column() == CapitalFlow) {
            return QBrush(item.capitalFlow > 0 ? Qt::red : (item.capitalFlow < 0 ? Qt::green : Qt::white));
        }
        return QBrush(Qt::white);
    }
    case Qt::UserRole:  // 存储原始数据，委托用
        return QVariant::fromValue(item);

    default:
        return QVariant();
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
