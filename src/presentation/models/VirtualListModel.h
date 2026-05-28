/**
 * @file VirtualListModel.h
 * @brief 虚拟化列表模型 - 只渲染可见区域数据
 *
 * @details 功能：
 * - 只加载和渲染可见区域的数据
 * - 支持百万级数据量
 * - 滚动时动态加载数据
 * - 缓存滚动位置
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef VIRTUALLISTMODEL_H
#define VIRTUALLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QTimer>
#include <functional>

namespace WealthPilot {

/**
 * @brief 数据加载器接口
 */
class IVirtualDataLoader {
public:
    virtual ~IVirtualDataLoader() = default;
    
    /**
     * @brief 获取总数据量
     */
    virtual qint64 totalCount() const = 0;
    
    /**
     * @brief 加载指定范围的数据
     * @param offset 起始位置
     * @param count 数量
     * @return 数据项列表
     */
    virtual QVector<QVariant> loadRange(qint64 offset, int count) = 0;
    
    /**
     * @brief 刷新数据
     */
    virtual void refresh() = 0;
};

/**
 * @brief 虚拟化列表模型
 * 
 * 支持大数据量显示，只加载可见区域数据
 * 
 * 使用示例：
 * @code
 * // 创建模型
 * auto* model = new VirtualListModel(this);
 * model->setDataLoader([](qint64 offset, int count) {
 *     // 从数据库或网络加载数据
 *     return fetchFromDatabase(offset, count);
 * }, 1000000);  // 100万条数据
 * 
 * // 设置到 ListView
 * listView->setModel(model);
 * @endcode
 */
class VirtualListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(qint64 totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(int visibleCount READ visibleCount WRITE setVisibleCount)
    Q_PROPERTY(qint64 firstVisibleIndex READ firstVisibleIndex WRITE setFirstVisibleIndex)

public:
    explicit VirtualListModel(QObject* parent = nullptr);
    ~VirtualListModel() override;
    
    // ========== QAbstractListModel 接口 ==========
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    // ========== 数据加载 ==========
    
    /**
     * @brief 设置数据加载器（函数式）
     * @param loader 加载函数
     * @param totalCount 总数据量
     */
    void setDataLoader(std::function<QVector<QVariant>(qint64, int)> loader, qint64 totalCount);
    
    /**
     * @brief 设置数据加载器（接口式）
     */
    void setDataLoader(IVirtualDataLoader* loader);
    
    /**
     * @brief 设置角色名称
     */
    void setRoleNames(const QHash<int, QByteArray>& roles);
    
    // ========== 可见区域管理 ==========
    
    /**
     * @brief 设置可见区域
     * @param first 第一个可见项索引
     * @param count 可见项数量
     */
    void setVisibleRange(qint64 first, int count);
    
    /**
     * @brief 设置第一个可见项索引
     */
    void setFirstVisibleIndex(qint64 index);
    
    /**
     * @brief 设置可见项数量
     */
    void setVisibleCount(int count);
    
    /**
     * @brief 获取第一个可见项索引
     */
    qint64 firstVisibleIndex() const;
    
    /**
     * @brief 获取可见项数量
     */
    int visibleCount() const;
    
    // ========== 缓存管理 ==========
    
    /**
     * @brief 设置缓存大小（额外加载的数据项数）
     */
    void setCacheSize(int beforeVisible, int afterVisible);
    
    /**
     * @brief 清空缓存
     */
    void clearCache();
    
    /**
     * @brief 设置缓存TTL（毫秒）
     */
    void setCacheTtl(int milliseconds);
    
    // ========== 数据管理 ==========
    
    /**
     * @brief 获取总数据量
     */
    qint64 totalCount() const;
    
    /**
     * @brief 设置总数据量
     */
    void setTotalCount(qint64 count);
    
    /**
     * @brief 刷新数据
     */
    Q_INVOKABLE void refresh();
    
    /**
     * @brief 刷新指定范围
     */
    void refreshRange(qint64 first, int count);

signals:
    /**
     * @brief 总数据量变化信号
     */
    void totalCountChanged();
    
    /**
     * @brief 数据加载完成信号
     */
    void dataLoaded(qint64 first, int count);
    
    /**
     * @brief 加载错误信号
     */
    void loadError(const QString& error);

private slots:
    /**
     * @brief 异步加载数据
     */
    void loadVisibleData();
    
    /**
     * @brief 清理过期缓存
     */
    void cleanupCache();

private:
    /**
     * @brief 计算需要加载的范围
     */
    QPair<qint64, int> calculateLoadRange() const;
    
    /**
     * @brief 从缓存获取数据
     */
    QVariant getFromCache(qint64 index) const;
    
    /**
     * @brief 存入缓存
     */
    void putToCache(qint64 index, const QVariant& data);

private:
    // 数据加载器
    std::function<QVector<QVariant>(qint64, int)> m_dataLoader;
    IVirtualDataLoader* m_interfaceLoader = nullptr;
    
    // 总数据量
    qint64 m_totalCount = 0;
    
    // 可见区域
    qint64 m_firstVisibleIndex = 0;
    int m_visibleCount = 50;
    
    // 缓存
    mutable QHash<qint64, QPair<QVariant, qint64>> m_cache;  // index -> (data, timestamp)
    int m_cacheBeforeVisible = 20;
    int m_cacheAfterVisible = 20;
    int m_cacheTtlMs = 30000;  // 30秒
    
    // 角色名称
    QHash<int, QByteArray> m_roleNames;
    
    // 异步加载
    QTimer* m_loadTimer = nullptr;
    QTimer* m_cleanupTimer = nullptr;
    bool m_loadPending = false;
};

/**
 * @brief 千档盘口虚拟化模型
 * 
 * 专门用于股票/期货千档盘口的虚拟化模型
 */
class VirtualOrderBookModel : public VirtualListModel {
    Q_OBJECT
    
public:
    enum Roles {
        PriceRole = Qt::UserRole + 1,
        VolumeRole,
        DirectionRole,  // 买/卖
        LevelRole,      // 档位
        TotalVolumeRole // 累计量
    };
    
    explicit VirtualOrderBookModel(QObject* parent = nullptr);
    
    /**
     * @brief 设置盘口数据
     */
    void setOrderBookData(const QVector<QVariant>& bids, const QVector<QVariant>& asks);
    
    /**
     * @brief 更新单个档位
     */
    void updateLevel(int level, double price, qint64 volume, bool isBid);
};

/**
 * @brief K线数据虚拟化模型
 * 
 * 专门用于K线图的虚拟化模型
 */
class VirtualKLineModel : public VirtualListModel {
    Q_OBJECT
    
public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        OpenRole,
        HighRole,
        LowRole,
        CloseRole,
        VolumeRole,
        AmountRole
    };
    
    explicit VirtualKLineModel(QObject* parent = nullptr);
    
    /**
     * @brief 获取可见时间范围
     */
    QPair<qint64, qint64> getVisibleTimeRange() const;
    
    /**
     * @brief 按时间范围加载
     */
    void loadTimeRange(qint64 startTime, qint64 endTime);
};

} // namespace WealthPilot

#endif // VIRTUALLISTMODEL_H
