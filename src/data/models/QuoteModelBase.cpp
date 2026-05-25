/**
 * @file QuoteModelBase.cpp
 * @brief 行情模型基类实现
 */

#include "QuoteModelBase.h"

namespace WealthPilot {

QuoteModelBase::QuoteModelBase(QObject* parent)
    : QAbstractTableModel(parent)
{
    m_updateTimer.start();
}

int QuoteModelBase::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

void QuoteModelBase::updateItem(const QString& key, std::function<void(int row)> updateFunc)
{
    if (m_keyToRow.contains(key)) {
        int row = m_keyToRow[key];
        updateFunc(row);
        emit dataChanged(index(row, 0), index(row, columnCount() - 1));
    }
}

void QuoteModelBase::removeItem(const QString& key)
{
    if (m_keyToRow.contains(key)) {
        int row = m_keyToRow[key];
        beginRemoveRows(QModelIndex(), row, row);
        m_data.remove(row);
        rebuildIndex();
        endRemoveRows();
    }
}

void QuoteModelBase::clear()
{
    beginResetModel();
    m_data.clear();
    m_keyToRow.clear();
    endResetModel();
}

int QuoteModelBase::findRowByKey(const QString& key) const
{
    return m_keyToRow.value(key, -1);
}

void QuoteModelBase::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();
    
    // 排序逻辑由子类实现
    Q_UNUSED(column);
    Q_UNUSED(order);
    
    rebuildIndex();
    emit layoutChanged();
}

void QuoteModelBase::rebuildIndex()
{
    m_keyToRow.clear();
    m_keyToRow.reserve(m_data.size());
    // 键由子类提供
}

void QuoteModelBase::emitBatchDataChanged(int startRow, int endRow)
{
    if (startRow <= endRow && startRow >= 0 && endRow < m_data.size()) {
        emit dataChanged(index(startRow, 0), index(endRow, columnCount() - 1));
    }
}

} // namespace WealthPilot
