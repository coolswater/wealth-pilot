#include "PageFactoryRegistry.h"
#include <QDebug>

PageFactoryRegistry* PageFactoryRegistry::instance() {
    // C++11线程安全静态初始化
    static PageFactoryRegistry instance;
    return &instance;
}

std::shared_ptr<BasePage> PageFactoryRegistry::createPage(const QString &pageId, QWidget *parent) {
    FactoryMeta meta;

    // 读锁保护查找操作 - 支持并发创建不同页面
    {
        QReadLocker locker(&m_lock);
        auto it = m_registry.find(pageId);
        if (it == m_registry.end()) {
            qWarning() << "PageFactoryRegistry: Page" << pageId << "not registered!";
            return nullptr;
        }
        meta = it->second;  // 拷贝元数据（FactoryMeta含function，拷贝开销小）
    } // 读锁在此释放

    // 锁外执行耗时的对象创建（关键性能优化点）
    try {
        auto page = meta.creator(parent);
        if (page && parent) {
            // 确保Qt对象树关系，防止内存泄漏
            page->setParent(parent);
        }
        return page;
    } catch (const std::exception &e) {
        qCritical() << "PageFactoryRegistry: Exception creating page" << pageId << ":" << e.what();
        return nullptr;
    }
}

bool PageFactoryRegistry::hasPage(const QString &pageId) const {
    QReadLocker locker(&m_lock);
    return m_registry.find(pageId) != m_registry.end();
}

QStringList PageFactoryRegistry::registeredPages() const {
    QReadLocker locker(&m_lock);
    QStringList list;
    list.reserve(static_cast<int>(m_registry.size()));  // 预分配避免重新分配
    for (const auto &pair : m_registry) {
        list.append(pair.first);
    }
    return list;
}

void PageFactoryRegistry::unregisterPage(const QString &pageId) {
    QWriteLocker locker(&m_lock);
    m_registry.erase(pageId);
}

FactoryMeta PageFactoryRegistry::getMeta(const QString &pageId) const {
    QReadLocker locker(&m_lock);
    auto it = m_registry.find(pageId);
    if (it != m_registry.end()) {
        return it->second;
    }
    return {};
}
