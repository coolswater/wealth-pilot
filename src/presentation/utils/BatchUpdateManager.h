/**
 * @file BatchUpdateManager.h
 * @brief 批量更新管理器 - 合并UI更新请求，优化性能
 *
 * @details 功能：
 * - 合并短时间内的多次更新请求
 * - 使用 QTimer::singleShot 延迟执行
 * - 支持 setUpdatesEnabled 批量更新
 * - 支持主题切换等场景
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef BATCHUPDATEMANAGER_H
#define BATCHUPDATEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QHash>
#include <functional>

namespace WealthPilot {

/**
 * @brief 批量更新项
 */
struct BatchUpdateItem {
    QString id;                         ///< 更新项ID
    std::function<void()> update;       ///< 更新函数
    int priority = 0;                   ///< 优先级（越高越先执行）
    qint64 timestamp = 0;               ///< 请求时间戳
};

/**
 * @brief 批量更新管理器
 * 
 * 用于合并短时间内的大量更新请求，避免频繁刷新UI
 * 
 * 使用示例：
 * @code
 * // 请求更新（会被合并）
 * BatchUpdateManager::instance()->requestUpdate("widget1", []() {
 *     widget1->update();
 * });
 * 
 * // 立即执行所有待处理的更新
 * BatchUpdateManager::instance()->flush();
 * @endcode
 */
class BatchUpdateManager : public QObject {
    Q_OBJECT

public:
    static BatchUpdateManager* instance();
    
    /**
     * @brief 请求更新
     * @param id 更新项ID（相同ID的更新会被合并）
     * @param update 更新函数
     * @param priority 优先级（越高越先执行）
     */
    void requestUpdate(const QString& id, std::function<void()> update, int priority = 0);
    
    /**
     * @brief 取消更新请求
     */
    void cancelUpdate(const QString& id);
    
    /**
     * @brief 立即执行所有待处理的更新
     */
    void flush();
    
    /**
     * @brief 清空所有待处理的更新
     */
    void clear();
    
    /**
     * @brief 设置合并窗口时间（毫秒）
     */
    void setMergeWindow(int milliseconds);
    
    /**
     * @brief 设置是否启用 setUpdatesEnabled 优化
     */
    void setUpdatesEnabledOptimization(bool enabled);
    
    /**
     * @brief 获取待处理更新数量
     */
    int pendingCount() const;
    
    /**
     * @brief 开始批量更新块
     * 
     * 用于手动控制批量更新的开始和结束
     */
    void beginBatchUpdate();
    
    /**
     * @brief 结束批量更新块
     */
    void endBatchUpdate();

signals:
    /**
     * @brief 批量更新开始信号
     */
    void batchUpdateStarted();
    
    /**
     * @brief 批量更新完成信号
     */
    void batchUpdateCompleted(int updateCount);
    
    /**
     * @brief 更新被合并信号
     */
    void updateMerged(const QString& id);

private slots:
    /**
     * @brief 执行批量更新
     */
    void processBatch();

private:
    BatchUpdateManager();
    ~BatchUpdateManager() override;
    
    /**
     * @brief 执行更新
     */
    void executeUpdates();

private:
    QHash<QString, BatchUpdateItem> m_pendingUpdates;    ///< 待处理更新
    QTimer* m_batchTimer;                                 ///< 批量更新定时器
    QSet<QObject*> m_updateTargets;                      ///< 更新目标集合
    
    int m_mergeWindowMs = 50;                            ///< 合并窗口时间
    bool m_updatesEnabledOptimization = true;            ///< 是否启用 setUpdatesEnabled
    bool m_inBatchUpdate = false;                        ///< 是否在批量更新块中
    int m_batchDepth = 0;                                ///< 批量更新嵌套深度
    
    // 统计
    qint64 m_totalUpdates = 0;
    qint64 m_totalMerged = 0;
};

/**
 * @brief 批量更新作用域守卫
 * 
 * RAII方式管理批量更新块
 * 
 * 使用示例：
 * @code
 * {
 *     BatchUpdateScope scope;
 *     widget1->update();
 *     widget2->update();
 *     widget3->update();
 * } // 自动执行批量更新
 * @endcode
 */
class BatchUpdateScope {
public:
    BatchUpdateScope() {
        BatchUpdateManager::instance()->beginBatchUpdate();
    }
    
    ~BatchUpdateScope() {
        BatchUpdateManager::instance()->endBatchUpdate();
    }
};

/**
 * @brief 主题切换批量更新助手
 */
class ThemeBatchUpdater {
public:
    /**
     * @brief 注册主题更新回调
     */
    static void registerThemeUpdate(const QString& id, std::function<void()> update, int priority = 0);
    
    /**
     * @brief 执行主题切换批量更新
     */
    static void applyThemeBatch();
    
    /**
     * @brief 获取预计更新时间（毫秒）
     */
    static int estimateUpdateTime();

private:
    static QHash<QString, std::pair<std::function<void()>, int>> s_themeUpdates;
};

} // namespace WealthPilot

#endif // BATCHUPDATEMANAGER_H
