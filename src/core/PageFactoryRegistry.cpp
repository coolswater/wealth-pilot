/**
 * @file PageFactoryRegistry.cpp
 * @brief
 */

#include "PageFactoryRegistry.h"
#include <QDebug>

PageFactoryRegistry* PageFactoryRegistry::instance() {
    // C++11静态局部变量线程安全初始化（双重检查锁定优化版）
    static PageFactoryRegistry *instance = new PageFactoryRegistry();
    return instance;
}

void PageFactoryRegistry::registerFactory(const QString &pageId, const FactoryMeta &meta) {
    QMutexLocker locker(&m_mutex);
    m_registry[pageId] = meta;
}

std::shared_ptr<BasePage> PageFactoryRegistry::createPage(const QString &pageId, QWidget *parent) {
    FactoryMeta meta;

    // 临界区：仅复制配置，不持有锁创建对象（降低锁粒度，提升并发性能）
    {
        QMutexLocker locker(&m_mutex);
        auto it = m_registry.find(pageId);
        if (it == m_registry.end()) {
            qWarning() << "PageFactoryRegistry: Page" << pageId << "not registered!";
            return nullptr;
        }
        meta = it->second;  // 复制元数据
    } // 锁在此释放

    // 在锁外执行耗时的对象创建（高性能关键）
    try {
        auto page = meta.creator(parent);
        if (page && parent) {
            page->setParent(parent);  // 确保Qt对象树关系
        }
        return page;
    } catch (const std::exception &e) {
        qCritical() << "PageFactoryRegistry: Failed to create page" << pageId << ":" << e.what();
        return nullptr;
    }
}

bool PageFactoryRegistry::hasPage(const QString &pageId) const {
    QMutexLocker locker(&m_mutex);
    return m_registry.find(pageId) != m_registry.end();
}

QStringList PageFactoryRegistry::registeredPages() const {
    QMutexLocker locker(&m_mutex);
    QStringList list;
    for (const auto &pair : m_registry) {
        list.append(pair.first);
    }
    return list;
}

void PageFactoryRegistry::unregisterPage(const QString &pageId) {
    QMutexLocker locker(&m_mutex);
    m_registry.erase(pageId);
}
