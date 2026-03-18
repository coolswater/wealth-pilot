/**
 * @brief 页面导航控制器（单例）
 * 管理页面切换、缓存、历史栈，提供MVVM级别的解耦
 */

#include "PageFactoryRegistry.h"
#include "PageNavigatorManager.h"
#include <QDebug>
#include <QCoreApplication>

PageNavigatorManager::PageNavigatorManager(QObject *parent)
    : QObject(parent)
{
    // 设置定时清理，防止weak_ptr堆积造成内存碎片
    m_cleanupTimer = new QTimer(this);
    connect(m_cleanupTimer, &QTimer::timeout, [this]() {
        for (auto it = m_weakCache.begin(); it != m_weakCache.end();) {
            if (it.value().expired()) {
                it = m_weakCache.erase(it);
                emit cacheStatusChanged(it.key(), false);
            } else {
                ++it;
            }
        }
    });
    m_cleanupTimer->start(30000); // 30秒清理一次
}

PageNavigatorManager* PageNavigatorManager::instance(QObject *parent) {
    static PageNavigatorManager *instance = new PageNavigatorManager(parent);
    return instance;
}

void PageNavigatorManager::initialize(QStackedWidget *container) {
    if (!container) {
        qCritical() << "PageNavigatorManager: Container cannot be null!";
        return;
    }
    m_container = container;
    // 确保容器本身有合理的堆栈管理策略
    m_container->setAttribute(Qt::WA_DeleteOnClose, false);
}

void PageNavigatorManager::registerPage(const QString &pageId, CachePolicy policy) {
    m_cachePolicies[pageId] = policy;

    // 强缓存策略立即创建（热启动优化）
    if (policy == CachePolicy::StrongCache) {
        preloadPage(pageId);
    }
}

void PageNavigatorManager::navigateTo(const QString &pageId, const QVariantMap &params, bool replaceCurrent) {
    if (!m_container) {
        qWarning() << "PageNavigatorManager: Not initialized!";
        return;
    }

    if (!PageFactoryRegistry::instance()->hasPage(pageId)) {
        qWarning() << "PageNavigatorManager: Page" << pageId << "not registered in factory!";
        return;
    }

    // 处理历史栈
    if (!replaceCurrent && m_currentPage) {
        HistoryEntry entry;
        entry.pageId = m_currentPage->pageId();
        // 保存当前页面状态到历史（如果需要恢复状态可扩展此处）
        m_historyStack.append(entry);
        if (m_historyStack.size() > m_maxHistorySize) {
            m_historyStack.removeFirst(); // LRU淘汰
        }
        emit historyStackChanged(m_historyStack.size());
    }

    switchToPage(pageId, params);
}

bool PageNavigatorManager::navigateBack() {
    if (m_historyStack.isEmpty()) {
        return false;
    }

    HistoryEntry entry = m_historyStack.takeLast();
    emit historyStackChanged(m_historyStack.size());

    switchToPage(entry.pageId, entry.params);
    return true;
}

void PageNavigatorManager::preloadPage(const QString &pageId) {
    if (m_weakCache.contains(pageId) || m_strongCache.contains(pageId)) {
        return; // 已缓存
    }

    auto page = PageFactoryRegistry::instance()->createPage(pageId, m_container);
    if (page) {
        putCachedPage(pageId, page, m_cachePolicies.value(pageId, CachePolicy::WeakCache));
        qDebug() << "PageNavigatorManager: Preloaded page" << pageId;
    }
}

void PageNavigatorManager::clearCache(const QString &pageId) {
    if (pageId.isEmpty()) {
        m_weakCache.clear();
        m_strongCache.clear();
    } else {
        m_weakCache.remove(pageId);
        m_strongCache.remove(pageId);
    }
}

std::shared_ptr<BasePage> PageNavigatorManager::getCachedPage(const QString &pageId) {
    // 优先强缓存
    if (m_strongCache.contains(pageId)) {
        return m_strongCache[pageId];
    }

    // 检查弱缓存
    if (m_weakCache.contains(pageId)) {
        auto shared = m_weakCache[pageId].lock();
        if (shared) {
            return shared;
        }
    }

    return nullptr;
}

void PageNavigatorManager::putCachedPage(const QString &pageId, std::shared_ptr<BasePage> page, CachePolicy policy) {
    if (policy == CachePolicy::StrongCache) {
        m_strongCache[pageId] = page;
    } else if (policy == CachePolicy::WeakCache) {
        m_weakCache[pageId] = page;
    }
    // NoCache策略不存储

    // 添加到UI容器但隐藏（预热）
    QWidget *widget = dynamic_cast<QWidget*>(page.get());
    if (widget && m_container->indexOf(widget) < 0) {
        m_container->addWidget(widget);
        widget->hide();
    }
}

void PageNavigatorManager::switchToPage(const QString &pageId, const QVariantMap &params) {
    // 触发当前页面离开事件
    if (m_currentPage) {
        m_currentPage->onPageDeactivated();
        // 断开旧页面的导航信号（防止内存泄漏和重复连接）
        disconnect(m_currentPage.get(), &BasePage::requestNavigation,
                   this, nullptr);
    }


    // 获取或创建页面
    auto page = getCachedPage(pageId);
    if (!page) {
        page = PageFactoryRegistry::instance()->createPage(pageId, m_container);
        if (!page) {
            qCritical() << "PageNavigatorManager: Failed to create page" << pageId;
            return;
        }
        putCachedPage(pageId, page, m_cachePolicies.value(pageId, CachePolicy::WeakCache));
    }

    // 添加到容器并显示
    QWidget *widget = dynamic_cast<QWidget*>(page.get());
    if (!widget) {
        qCritical() << "PageNavigatorManager: Page is not a QWidget!";
        return;
    }

    // 确保在堆栈中
    int index = m_container->indexOf(widget);
    if (index < 0) {
        index = m_container->addWidget(widget);
    }

    // 激活页面
    m_currentPage = page;
    m_container->setCurrentIndex(index);

    // 触发进入事件（带参数传递）
    page->onPageActivated(params);

    emit pageChanged(pageId, params);

    // 连接页面导航请求信号（实现页面间解耦通信）
    connect(
        page.get(),
        &BasePage::requestNavigation,
        this,
        [this](const QString &targetPageId, const QMap<QString, QVariant> &targetParams) {
            // 页面请求导航时，默认不替换当前历史记录（false）
            // 如果页面需要替换当前记录，可以通过 params 传递特殊标记
            bool replace = targetParams.value("__replace_current", false).toBool();
            this->navigateTo(targetPageId, targetParams, replace);
        }, Qt::UniqueConnection);
}
