/**
 * @file QuoteModelBase.h
 * @brief 统一的行情模型基类
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef QUOTEMODELBASE_H
#define QUOTEMODELBASE_H

#include <QAbstractTableModel>
#include <QHash>
#include <QVector>
#include <QTimer>
#include <QElapsedTimer>
#include <functional>

namespace WealthPilot {

/**
 * @brief 行情模型基类 - 提供高性能数据管理
 *
 * @details 功能：
 * - 哈希索引快速查找
 * - 增量更新减少重绘
 * - 批量更新优化
 * - 排序支持
 */
class QuoteModelBase : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit QuoteModelBase(QObject* parent = nullptr);
    ~QuoteModelBase() override = default;

    // QAbstractTableModel 接口
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    
    /**
     * @brief 批量设置数据
     * @param data 新数据
     * @param useDiff 是否使用差异比较
     */
    template<typename T>
    void setData(const QVector<T>& data, bool useDiff = true);
    
    /**
     * @brief 增量更新数据
     * @param newData 新数据
     * @param getKey 获取键的函数
     * @param hasChanged 判断是否变化的函数
     */
    template<typename T>
    void updateData(const QVector<T>& newData,
                    std::function<QString(const T&)> getKey,
                    std::function<bool(const T&, const T&)> hasChanged);
    
    /**
     * @brief 更新单个项目
     * @param key 项目键
     * @param updateFunc 更新函数
     */
    void updateItem(const QString& key, std::function<void(int row)> updateFunc);
    
    /**
     * @brief 删除项目
     * @param key 项目键
     */
    void removeItem(const QString& key);
    
    /**
     * @brief 清空数据
     */
    void clear();
    
    /**
     * @brief 获取数据数量
     */
    int dataSize() const { return m_data.size(); }
    
    /**
     * @brief 根据键查找行号
     * @param key 项目键
     * @return 行号，不存在返回 -1
     */
    int findRowByKey(const QString& key) const;
    
    /**
     * @brief 排序
     * @param column 列
     * @param order 排序顺序
     */
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

protected:
    /**
     * @brief 重建索引
     */
    void rebuildIndex();
    
    /**
     * @brief 发射批量数据变化信号
     * @param startRow 开始行
     * @param endRow 结束行
     */
    void emitBatchDataChanged(int startRow, int endRow);

    // 数据存储
    QVector<QVariant> m_data;
    QHash<QString, int> m_keyToRow;
    
    // 性能统计
    QElapsedTimer m_updateTimer;
    qint64 m_lastUpdateTimeMs = 0;
};

// ============================================================================
// 模板实现
// ============================================================================

template<typename T>
void QuoteModelBase::setData(const QVector<T>& data, bool useDiff)
{
    if (useDiff && !m_data.isEmpty()) {
        // 转换并增量更新
        QVector<QVariant> variantData;
        for (const T& item : data) {
            variantData.append(QVariant::fromValue(item));
        }
        
        // 比较差异
        QVector<int> changedRows;
        for (int i = 0; i < variantData.size() && i < m_data.size(); ++i) {
            if (m_data[i] != variantData[i]) {
                m_data[i] = variantData[i];
                changedRows.append(i);
            }
        }
        
        // 发射变化信号
        if (!changedRows.isEmpty()) {
            emitBatchDataChanged(changedRows.first(), changedRows.last());
        }
    } else {
        // 全量替换
        beginResetModel();
        m_data.clear();
        for (const T& item : data) {
            m_data.append(QVariant::fromValue(item));
        }
        rebuildIndex();
        endResetModel();
    }
}

template<typename T>
void QuoteModelBase::updateData(const QVector<T>& newData,
                                  std::function<QString(const T&)> getKey,
                                  std::function<bool(const T&, const T&)> hasChanged)
{
    QElapsedTimer timer;
    timer.start();
    
    QVector<int> changedRows;
    QVector<int> newRows;
    
    for (int i = 0; i < newData.size(); ++i) {
        const T& item = newData[i];
        QString key = getKey(item);
        
        if (m_keyToRow.contains(key)) {
            int row = m_keyToRow[key];
            const T& oldItem = m_data[row].value<T>();
            if (hasChanged(oldItem, item)) {
                m_data[row] = QVariant::fromValue(item);
                changedRows.append(row);
            }
        } else {
            newRows.append(i);
        }
    }
    
    // 批量发射变化信号
    if (!changedRows.isEmpty()) {
        std::sort(changedRows.begin(), changedRows.end());
        emitBatchDataChanged(changedRows.first(), changedRows.last());
    }
    
    // 添加新行
    if (!newRows.isEmpty()) {
        int firstNewRow = m_data.size();
        beginInsertRows(QModelIndex(), firstNewRow, firstNewRow + newRows.size() - 1);
        for (int i : newRows) {
            m_data.append(QVariant::fromValue(newData[i]));
        }
        rebuildIndex();
        endInsertRows();
    }
    
    m_lastUpdateTimeMs = timer.elapsed();
}

} // namespace WealthPilot

#endif // QUOTEMODELBASE_H