#ifndef PAGEFACTORYREGISTRY_H
#define PAGEFACTORYREGISTRY_H

#include "../../ui/components/BasePage.h"
#include <QMutex>
#include <QReadWriteLock>
#include <functional>
#include <memory>
#include <unordered_map>

// 使用 WealthPilot 命名空间中的 BasePage
using WealthPilot::BasePage;

/// 页面工厂函数类型
using PageFactory = std::function<std::shared_ptr<BasePage>(QWidget*)>;

/**
 * @brief 工厂元数据
 */
struct FactoryMeta {
    PageFactory creator;        ///< 创建函数
    QString description;        ///< 描述
    bool cacheable = true;      ///< 是否可缓存
    int priority = 0;           ///< 优先级
};

/**
 * @brief 页面工厂注册表 - 线程安全的单例模式
 * @details 使用读写锁（QReadWriteLock）优化读多写少场景，比互斥锁（QMutex）性能更好
 */
class PageFactoryRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static PageFactoryRegistry* instance();

    /**
     * @brief 模板注册接口 - 编译时类型检查
     * @tparam T 必须继承自 BasePage
     * @param pageId 页面唯一标识
     * @param description 页面描述
     * @param cacheable 是否可缓存
     * @param priority 优先级
     */
    template<typename T>
    void registerPage(const QString &pageId, const QString &description = QString(),
                      bool cacheable = true, int priority = 0) {
        static_assert(std::is_base_of_v<BasePage, T>, "T must inherit from BasePage");

        QWriteLocker locker(&m_lock);  // 写锁保护

        FactoryMeta meta;
        meta.creator = [](QWidget *parent) -> std::shared_ptr<BasePage> {
            // 自动删除，确保Qt对象在主线程销毁
            return std::shared_ptr<T>(new T(parent), [](T *p) {
                if (p) p->deleteLater();
            });
        };
        meta.description = description.isEmpty() ? pageId : description;
        meta.cacheable = cacheable;
        meta.priority = priority;

        m_registry[pageId] = std::move(meta);
    }

    /**
     * @brief 创建页面实例
     */
    std::shared_ptr<BasePage> createPage(const QString &pageId, QWidget *parent = nullptr);
    
    /**
     * @brief 检查页面是否已注册
     */
    bool hasPage(const QString &pageId) const;
    
    /**
     * @brief 获取所有已注册页面ID
     */
    QStringList registeredPages() const;
    
    /**
     * @brief 注销页面
     */
    void unregisterPage(const QString &pageId);
    
    /**
     * @brief 获取工厂元数据
     */
    FactoryMeta getMeta(const QString &pageId) const;

private:
    PageFactoryRegistry() = default;
    ~PageFactoryRegistry() = default;
    PageFactoryRegistry(const PageFactoryRegistry&) = delete;
    PageFactoryRegistry& operator=(const PageFactoryRegistry&) = delete;

    mutable QReadWriteLock m_lock;  ///< 读写锁，比QMutex支持并发读
    std::unordered_map<QString, FactoryMeta> m_registry;  ///< 注册表
};

#endif // PAGEFACTORYREGISTRY_H