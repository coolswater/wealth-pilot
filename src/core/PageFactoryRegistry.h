#ifndef PAGEFACTORYREGISTRY_H
#define PAGEFACTORYREGISTRY_H

#include "BasePage.h"
#include <QMutex>
#include <QReadWriteLock>
#include <functional>
#include <memory>
#include <unordered_map>

using PageFactory = std::function<std::shared_ptr<BasePage>(QWidget*)>;

struct FactoryMeta {
    PageFactory creator;
    QString description;
    bool cacheable = true;
    int priority = 0;
};

/**
 * @brief 页面工厂注册表 - 线程安全的单例工厂
 * @details 采用读写锁（QReadWriteLock）优化读多写少场景，性能优于QMutex
 */
class PageFactoryRegistry {
public:
    static PageFactoryRegistry* instance();

    /**
     * @brief 模板注册接口 - 编译时类型检查
     * @tparam T 必须继承自BasePage
     */
    template<typename T>
    void registerPage(const QString &pageId, const QString &description = QString(),
                      bool cacheable = true, int priority = 0) {
        static_assert(std::is_base_of_v<BasePage, T>, "T must inherit from BasePage");

        QWriteLocker locker(&m_lock);  // 写锁保护

        FactoryMeta meta;
        meta.creator = [](QWidget *parent) -> std::shared_ptr<BasePage> {
            // 自定义删除器确保Qt对象在主线程销毁
            return std::shared_ptr<T>(new T(parent), [](T *p) {
                if (p) p->deleteLater();
            });
        };
        meta.description = description.isEmpty() ? pageId : description;
        meta.cacheable = cacheable;
        meta.priority = priority;

        m_registry[pageId] = std::move(meta);
    }

    std::shared_ptr<BasePage> createPage(const QString &pageId, QWidget *parent = nullptr);
    bool hasPage(const QString &pageId) const;
    QStringList registeredPages() const;
    void unregisterPage(const QString &pageId);
    FactoryMeta getMeta(const QString &pageId) const;

private:
    PageFactoryRegistry() = default;
    ~PageFactoryRegistry() = default;
    PageFactoryRegistry(const PageFactoryRegistry&) = delete;
    PageFactoryRegistry& operator=(const PageFactoryRegistry&) = delete;

    mutable QReadWriteLock m_lock;  // 读写锁替代QMutex，支持并发读
    std::unordered_map<QString, FactoryMeta> m_registry;
};

#endif // PAGEFACTORYREGISTRY_H
