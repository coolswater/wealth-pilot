/**
 * @file VirtualListModel.cpp
 * @brief 虚拟化列表模型实现
 */

#include "VirtualListModel.h"
#include "shared/utils/Logger.h"
#include <QDateTime>
#include <QtConcurrent>

namespace WealthPilot {

// ========== VirtualListModel ==========

VirtualListModel::VirtualListModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_loadTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
{
    // 延迟加载定时器（避免滚动时频繁加载）
    m_loadTimer->setSingleShot(true);
    m_loadTimer->setInterval(50);
    connect(m_loadTimer, &QTimer::timeout, this, &VirtualListModel::loadVisibleData);
    
    // 缓存清理定时器
    m_cleanupTimer->setInterval(10000);  // 每10秒清理一次
    connect(m_cleanupTimer, &QTimer::timeout, this, &VirtualListModel::cleanupCache);
    m_cleanupTimer->start();
    
    // 默认角色
    m_roleNames[Qt::DisplayRole] = "display";
    m_roleNames[Qt::UserRole + 1] = "data";
    
    LOG_DEBUG("[VirtualListModel] Initialized");
}

VirtualListModel::~VirtualListModel()
{
    LOG_DEBUG(QString("[VirtualListModel] Destroyed, cache size: %1").arg(m_cache.size()));
}

// ========== QAbstractListModel 接口 ==========

int VirtualListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    
    // 返回可见区域大小（加上缓存）
    return std::min(m_visibleCount + m_cacheBeforeVisible + m_cacheAfterVisible, 
                    static_cast<int>(m_totalCount - m_firstVisibleIndex));
}

QVariant VirtualListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0) {
        return QVariant();
    }
    
    // 计算实际数据索引
    qint64 actualIndex = m_firstVisibleIndex - m_cacheBeforeVisible + index.row();
    
    if (actualIndex < 0 || actualIndex >= m_totalCount) {
        return QVariant();
    }
    
    // 从缓存获取
    QVariant cached = getFromCache(actualIndex);
    if (cached.isValid()) {
        if (role == Qt::DisplayRole) {
            return cached;
        }
        return cached;
    }
    
    // 触发异步加载
    const_cast<VirtualListModel*>(this)->m_loadTimer->start();
    
    return QVariant();
}

QHash<int, QByteArray> VirtualListModel::roleNames() const
{
    return m_roleNames;
}

// ========== 数据加载 ==========

void VirtualListModel::setDataLoader(std::function<QVector<QVariant>(qint64, int)> loader, qint64 totalCount)
{
    m_dataLoader = std::move(loader);
    m_totalCount = totalCount;
    m_cache.clear();
    
    emit totalCountChanged();
    
    LOG_DEBUG(QString("[VirtualListModel] Set data loader, total count: %1").arg(totalCount));
}

void VirtualListModel::setDataLoader(IVirtualDataLoader* loader)
{
    m_interfaceLoader = loader;
    if (loader) {
        m_totalCount = loader->totalCount();
        emit totalCountChanged();
    }
}

void VirtualListModel::setRoleNames(const QHash<int, QByteArray>& roles)
{
    m_roleNames = roles;
}

// ========== 可见区域管理 ==========

void VirtualListModel::setVisibleRange(qint64 first, int count)
{
    bool changed = false;
    
    if (m_firstVisibleIndex != first) {
        m_firstVisibleIndex = first;
        changed = true;
    }
    
    if (m_visibleCount != count) {
        m_visibleCount = count;
        changed = true;
    }
    
    if (changed) {
        // 触发异步加载
        m_loadTimer->start();
        
        // 发出信号通知视图更新
        emit beginResetModel();
        emit endResetModel();
    }
}

void VirtualListModel::setFirstVisibleIndex(qint64 index)
{
    if (m_firstVisibleIndex != index) {
        m_firstVisibleIndex = qBound(0LL, index, m_totalCount - m_visibleCount);
        m_loadTimer->start();
        
        emit beginResetModel();
        emit endResetModel();
    }
}

void VirtualListModel::setVisibleCount(int count)
{
    if (m_visibleCount != count && count > 0) {
        m_visibleCount = count;
        m_loadTimer->start();
        
        emit beginResetModel();
        emit endResetModel();
    }
}

qint64 VirtualListModel::firstVisibleIndex() const
{
    return m_firstVisibleIndex;
}

int VirtualListModel::visibleCount() const
{
    return m_visibleCount;
}

// ========== 缓存管理 ==========

void VirtualListModel::setCacheSize(int beforeVisible, int afterVisible)
{
    m_cacheBeforeVisible = beforeVisible;
    m_cacheAfterVisible = afterVisible;
}

void VirtualListModel::clearCache()
{
    m_cache.clear();
}

void VirtualListModel::setCacheTtl(int milliseconds)
{
    m_cacheTtlMs = milliseconds;
}

// ========== 数据管理 ==========

qint64 VirtualListModel::totalCount() const
{
    return m_totalCount;
}

void VirtualListModel::setTotalCount(qint64 count)
{
    if (m_totalCount != count) {
        m_totalCount = count;
        m_cache.clear();
        emit totalCountChanged();
    }
}

void VirtualListModel::refresh()
{
    m_cache.clear();
    m_loadTimer->start();
    
    emit beginResetModel();
    emit endResetModel();
}

void VirtualListModel::refreshRange(qint64 first, int count)
{
    // 清除指定范围的缓存
    for (qint64 i = first; i < first + count && i < m_totalCount; ++i) {
        m_cache.remove(i);
    }
    
    // 触发加载
    m_loadTimer->start();
}

// ========== 私有方法 ==========

void VirtualListModel::loadVisibleData()
{
    if (!m_dataLoader && !m_interfaceLoader) {
        return;
    }
    
    auto [loadStart, loadCount] = calculateLoadRange();
    
    if (loadCount <= 0) {
        return;
    }
    
    // 检查哪些数据需要加载
    QVector<qint64> indicesToLoad;
    for (qint64 i = loadStart; i < loadStart + loadCount && i < m_totalCount; ++i) {
        if (!m_cache.contains(i)) {
            indicesToLoad.append(i);
        }
    }
    
    if (indicesToLoad.isEmpty()) {
        return;
    }
    
    LOG_DEBUG(QString("[VirtualListModel] Loading data: start=%1, count=%2")
              .arg(loadStart)
              .arg(loadCount));
    
    // 加载数据
    QVector<QVariant> data;
    if (m_interfaceLoader) {
        data = m_interfaceLoader->loadRange(loadStart, loadCount);
    } else if (m_dataLoader) {
        data = m_dataLoader(loadStart, loadCount);
    }
    
    // 存入缓存
    for (int i = 0; i < data.size() && loadStart + i < m_totalCount; ++i) {
        putToCache(loadStart + i, data[i]);
    }
    
    emit dataLoaded(loadStart, data.size());
    
    // 通知视图更新
    emit dataChanged(index(0), index(rowCount() - 1));
}

void VirtualListModel::cleanupCache()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    // 清理过期缓存
    auto it = m_cache.begin();
    while (it != m_cache.end()) {
        if (now - it.value().second > m_cacheTtlMs) {
            // 不在可见区域的才清理
            qint64 index = it.key();
            if (index < m_firstVisibleIndex - m_cacheBeforeVisible ||
                index > m_firstVisibleIndex + m_visibleCount + m_cacheAfterVisible) {
                it = m_cache.erase(it);
                continue;
            }
        }
        ++it;
    }
}

QPair<qint64, int> VirtualListModel::calculateLoadRange() const
{
    qint64 start = qMax(0LL, m_firstVisibleIndex - m_cacheBeforeVisible);
    qint64 end = qMin(m_totalCount - 1, m_firstVisibleIndex + m_visibleCount + m_cacheAfterVisible);
    
    return {start, static_cast<int>(end - start + 1)};
}

QVariant VirtualListModel::getFromCache(qint64 index) const
{
    auto it = m_cache.find(index);
    if (it != m_cache.end()) {
        return it.value().first;
    }
    return QVariant();
}

void VirtualListModel::putToCache(qint64 index, const QVariant& data)
{
    m_cache[index] = {data, QDateTime::currentMSecsSinceEpoch()};
}

// ========== VirtualOrderBookModel ==========

VirtualOrderBookModel::VirtualOrderBookModel(QObject* parent)
    : VirtualListModel(parent)
{
    // 设置角色
    QHash<int, QByteArray> roles;
    roles[PriceRole] = "price";
    roles[VolumeRole] = "volume";
    roles[DirectionRole] = "direction";
    roles[LevelRole] = "level";
    roles[TotalVolumeRole] = "totalVolume";
    setRoleNames(roles);
}

void VirtualOrderBookModel::setOrderBookData(const QVector<QVariant>& bids, const QVector<QVariant>& asks)
{
    Q_UNUSED(bids)
    Q_UNUSED(asks)
    // TODO: 实现盘口数据设置
}

void VirtualOrderBookModel::updateLevel(int level, double price, qint64 volume, bool isBid)
{
    Q_UNUSED(level)
    Q_UNUSED(price)
    Q_UNUSED(volume)
    Q_UNUSED(isBid)
    // TODO: 实现单档更新
}

// ========== VirtualKLineModel ==========

VirtualKLineModel::VirtualKLineModel(QObject* parent)
    : VirtualListModel(parent)
{
    // 设置角色
    QHash<int, QByteArray> roles;
    roles[TimestampRole] = "timestamp";
    roles[OpenRole] = "open";
    roles[HighRole] = "high";
    roles[LowRole] = "low";
    roles[CloseRole] = "close";
    roles[VolumeRole] = "volume";
    roles[AmountRole] = "amount";
    setRoleNames(roles);
}

QPair<qint64, qint64> VirtualKLineModel::getVisibleTimeRange() const
{
    // TODO: 实现时间范围计算
    return {0, 0};
}

void VirtualKLineModel::loadTimeRange(qint64 startTime, qint64 endTime)
{
    Q_UNUSED(startTime)
    Q_UNUSED(endTime)
    // TODO: 实现按时间范围加载
}

} // namespace WealthPilot
