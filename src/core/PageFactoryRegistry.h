#ifndef PAGEFACTORYREGISTRY_H
#define PAGEFACTORYREGISTRY_H

#include "BasePage.h"
#include <QMutex>
#include <QMutexLocker>
#include <functional>
#include <memory>
#include <unordered_map>

/**
 * @brief 页面工厂函数类型
 * 使用std::function支持Lambda、函数指针、std::bind等多种方式注册
 */
using PageFactory = std::function<std::shared_ptr<BasePage>(QWidget*)>;

/**
 * @brief 工厂元数据，包含创建器与配置信息
 */
struct FactoryMeta {
    PageFactory creator;           // 工厂函数
    QString description;           // 页面描述
    bool cacheable = true;         // 是否允许缓存（设为false每次重新创建）
    int priority = 0;              // 加载优先级（预留，用于预加载策略）
};

/**
 * @brief 单例工厂注册表（线程安全）
 * 采用双重检查锁定实现高性能单例
 */
class PageFactoryRegistry {
public:
    /**
     * @brief 获取单例实例（线程安全）
     */
    static PageFactoryRegistry* instance();

    /**
     * @brief 注册页面工厂
     * @tparam T 页面类型，必须继承自BasePage
     * @param pageId 页面唯一标识
     * @param description 页面描述
     * @param cacheable 是否缓存实例
     */
    template<typename T>
    void registerPage(const QString &pageId, const QString &description = QString(),
                      bool cacheable = true) {
        static_assert(std::is_base_of<BasePage, T>::value, "T must inherit from BasePage");

        QMutexLocker locker(&m_mutex);

        FactoryMeta meta;
        meta.creator = [](QWidget *parent) -> std::shared_ptr<BasePage> {
            // 使用智能指针自动管理QWidget生命周期
            // 注意：QWidget必须由QObject树管理，此处使用自定义删除器
            return std::shared_ptr<T>(new T(parent), [](T *p) {
                if (p) p->deleteLater();  // 确保在主线程销毁
            });
        };
        meta.description = description.isEmpty() ? pageId : description;
        meta.cacheable = cacheable;

        m_registry[pageId] = std::move(meta);
    }

    /**
     * @brief 注册自定义工厂函数（高级用法，支持依赖注入）
     */
    void registerFactory(const QString &pageId, const FactoryMeta &meta);

    /**
     * @brief 创建页面实例
     * @param pageId 页面ID
     * @param parent 父控件（Qt对象树使用）
     * @return 页面智能指针，失败返回nullptr
     */
    std::shared_ptr<BasePage> createPage(const QString &pageId, QWidget *parent = nullptr);

    /**
     * @brief 判断页面是否已注册
     */
    bool hasPage(const QString &pageId) const;

    /**
     * @brief 获取所有已注册页面ID
     */
    QStringList registeredPages() const;

    /**
     * @brief 注销页面（热插拔支持）
     */
    void unregisterPage(const QString &pageId);

private:
    PageFactoryRegistry() = default;
    ~PageFactoryRegistry() = default;

    // 禁止拷贝
    PageFactoryRegistry(const PageFactoryRegistry&) = delete;
    PageFactoryRegistry& operator=(const PageFactoryRegistry&) = delete;

    mutable QMutex m_mutex;
    std::unordered_map<QString, FactoryMeta> m_registry;
};

#endif // PAGEFACTORYREGISTRY_H
