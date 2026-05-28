/**
 * @file LazyPageLoader.cpp
 * @brief 懒加载页面管理器实现
 */

#include "LazyPageLoader.h"
#include "shared/utils/Logger.h"
#include <QDateTime>
#include <QApplication>

namespace WealthPilot {

LazyPageLoader::LazyPageLoader(QStackedWidget* container, QObject* parent)
    : QObject(parent)
    , m_container(container)
    , m_idleCheckTimer(new QTimer(this))
    , m_preloadTimer(new QTimer(this))
{
    // 每分钟检查一次空闲页面
    m_idleCheckTimer->setInterval(60000);
    connect(m_idleCheckTimer, &QTimer::timeout, this, &LazyPageLoader::checkIdlePages);
    m_idleCheckTimer->start();
    
    // 预加载定时器（延迟执行，避免阻塞UI）
    m_preloadTimer->setInterval(100);
    connect(m_preloadTimer, &QTimer::timeout, this, &LazyPageLoader::processPreloadQueue);
    
    LOG_DEBUG("[LazyPageLoader] Initialized");
}

LazyPageLoader::~LazyPageLoader()
{
    m_idleCheckTimer->stop();
    m_preloadTimer->stop();
    
    // 清理所有页面
    for (auto it = m_states.begin(); it != m_states.end(); ++it) {
        if (it->widget) {
            it->widget->deleteLater();
        }
    }
    
    LOG_DEBUG("[LazyPageLoader] Destroyed");
}

// ========== 页面注册 ==========

void LazyPageLoader::registerPage(const PageConfig& config)
{
    if (config.id.isEmpty() || !config.factory) {
        LOG_WARNING(QString("[LazyPageLoader] Invalid page config: %1").arg(config.id));
        return;
    }
    
    m_configs[config.id] = config;
    m_states[config.id] = PageState();
    
    LOG_DEBUG(QString("[LazyPageLoader] Registered page: %1 (preload=%2, priority=%3)")
              .arg(config.id)
              .arg(config.preload)
              .arg(config.priority));
}

void LazyPageLoader::registerPages(const QVector<PageConfig>& configs)
{
    for (const auto& config : configs) {
        registerPage(config);
    }
}

void LazyPageLoader::unregisterPage(const QString& pageId)
{
    auto stateIt = m_states.find(pageId);
    if (stateIt != m_states.end()) {
        if (stateIt->widget) {
            stateIt->widget->deleteLater();
        }
        m_states.erase(stateIt);
    }
    
    m_configs.remove(pageId);
    LOG_DEBUG(QString("[LazyPageLoader] Unregistered page: %1").arg(pageId));
}

// ========== 页面访问 ==========

bool LazyPageLoader::switchToPage(const QString& pageId)
{
    auto configIt = m_configs.find(pageId);
    if (configIt == m_configs.end()) {
        LOG_WARNING(QString("[LazyPageLoader] Page not found: %1").arg(pageId));
        return false;
    }
    
    auto& state = m_states[pageId];
    
    // 懒加载：首次访问时创建
    if (!state.created) {
        state.widget = createPage(pageId);
        if (!state.widget) {
            LOG_ERROR(QString("[LazyPageLoader] Failed to create page: %1").arg(pageId));
            return false;
        }
        state.created = true;
        emit pageCreated(pageId);
    }
    
    // 更新访问记录
    state.lastAccessTime = QDateTime::currentMSecsSinceEpoch();
    state.accessCount++;
    
    // 切换到页面
    m_container->setCurrentWidget(state.widget);
    m_currentPageId = pageId;
    
    LOG_DEBUG(QString("[LazyPageLoader] Switched to page: %1 (access=%2)")
              .arg(pageId)
              .arg(state.accessCount));
    
    emit pageChanged(pageId);
    return true;
}

QString LazyPageLoader::currentPageId() const
{
    return m_currentPageId;
}

QWidget* LazyPageLoader::currentPage() const
{
    auto it = m_states.find(m_currentPageId);
    return it != m_states.end() ? it->widget : nullptr;
}

QWidget* LazyPageLoader::getPage(const QString& pageId) const
{
    auto it = m_states.find(pageId);
    return it != m_states.end() ? it->widget : nullptr;
}

bool LazyPageLoader::isPageCreated(const QString& pageId) const
{
    auto it = m_states.find(pageId);
    return it != m_states.end() && it->created;
}

// ========== 预加载管理 ==========

void LazyPageLoader::preloadPage(const QString& pageId)
{
    if (!m_configs.contains(pageId)) {
        return;
    }
    
    // 已创建则跳过
    if (m_states[pageId].created) {
        return;
    }
    
    // 添加到预加载队列
    if (!m_preloadQueue.contains(pageId)) {
        m_preloadQueue.append(pageId);
        LOG_DEBUG(QString("[LazyPageLoader] Queued for preload: %1").arg(pageId));
    }
}

void LazyPageLoader::preloadPages(const QStringList& pageIds)
{
    for (const QString& pageId : pageIds) {
        preloadPage(pageId);
    }
}

void LazyPageLoader::startPreloading()
{
    if (m_preloadingInProgress) {
        return;
    }
    
    // 按优先级排序
    QVector<QPair<int, QString>> prioritized;
    for (auto it = m_configs.begin(); it != m_configs.end(); ++it) {
        if (it->preload && !m_states[it.key()].created) {
            prioritized.append({it->priority, it.key()});
        }
    }
    
    std::sort(prioritized.begin(), prioritized.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    m_preloadQueue.clear();
    for (const auto& item : prioritized) {
        m_preloadQueue.append(item.second);
    }
    
    if (!m_preloadQueue.isEmpty()) {
        m_preloadingInProgress = true;
        m_preloadTimer->start();
        LOG_INFO(QString("[LazyPageLoader] Started preloading %1 pages").arg(m_preloadQueue.size()));
    }
}

void LazyPageLoader::cancelPreloading()
{
    m_preloadTimer->stop();
    m_preloadQueue.clear();
    m_preloadingInProgress = false;
}

void LazyPageLoader::processPreloadQueue()
{
    if (m_preloadQueue.isEmpty()) {
        m_preloadTimer->stop();
        m_preloadingInProgress = false;
        LOG_INFO("[LazyPageLoader] Preloading complete");
        return;
    }
    
    QString pageId = m_preloadQueue.takeFirst();
    auto& state = m_states[pageId];
    
    if (!state.created) {
        // 使用 processEvents 避免阻塞UI
        QApplication::processEvents();
        
        state.widget = createPage(pageId);
        if (state.widget) {
            state.created = true;
            state.preloaded = true;
            emit pagePreloaded(pageId);
            LOG_DEBUG(QString("[LazyPageLoader] Preloaded: %1").arg(pageId));
        }
    }
}

// ========== 内存管理 ==========

void LazyPageLoader::unloadPage(const QString& pageId)
{
    // 不能卸载当前页面
    if (pageId == m_currentPageId) {
        LOG_DEBUG(QString("[LazyPageLoader] Cannot unload current page: %1").arg(pageId));
        return;
    }
    
    auto& state = m_states[pageId];
    if (state.widget) {
        m_container->removeWidget(state.widget);
        state.widget->deleteLater();
        state.widget = nullptr;
        state.created = false;
        state.preloaded = false;
        
        LOG_DEBUG(QString("[LazyPageLoader] Unloaded page: %1").arg(pageId));
        emit pageUnloaded(pageId);
    }
}

void LazyPageLoader::unloadIdlePages()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    QStringList toUnload;
    
    for (auto it = m_states.begin(); it != m_states.end(); ++it) {
        // 跳过当前页面
        if (it.key() == m_currentPageId) {
            continue;
        }
        
        // 跳过配置为永不卸载的页面
        auto configIt = m_configs.find(it.key());
        if (configIt != m_configs.end() && configIt->unloadTimeoutMs == 0) {
            continue;
        }
        
        // 检查空闲时间
        if (it->created && it->lastAccessTime > 0) {
            int timeout = configIt != m_configs.end() ? 
                          configIt->unloadTimeoutMs : m_idleTimeoutMs;
            
            if (now - it->lastAccessTime > timeout) {
                toUnload.append(it.key());
            }
        }
    }
    
    for (const QString& pageId : toUnload) {
        unloadPage(pageId);
    }
    
    if (!toUnload.isEmpty()) {
        LOG_INFO(QString("[LazyPageLoader] Unloaded %1 idle pages").arg(toUnload.size()));
    }
}

void LazyPageLoader::setIdleTimeout(int milliseconds)
{
    m_idleTimeoutMs = milliseconds;
}

// ========== 统计信息 ==========

int LazyPageLoader::createdPageCount() const
{
    int count = 0;
    for (auto it = m_states.begin(); it != m_states.end(); ++it) {
        if (it->created) {
            count++;
        }
    }
    return count;
}

int LazyPageLoader::registeredPageCount() const
{
    return m_configs.size();
}

QMap<QString, QVariant> LazyPageLoader::getStatistics() const
{
    QMap<QString, QVariant> stats;
    stats["registeredPages"] = m_configs.size();
    stats["createdPages"] = createdPageCount();
    stats["preloadedPages"] = m_preloadQueue.size();
    stats["currentPage"] = m_currentPageId;
    
    return stats;
}

void LazyPageLoader::checkIdlePages()
{
    unloadIdlePages();
}

// ========== 私有方法 ==========

QWidget* LazyPageLoader::createPage(const QString& pageId)
{
    auto configIt = m_configs.find(pageId);
    if (configIt == m_configs.end() || !configIt->factory) {
        return nullptr;
    }
    
    QWidget* page = configIt->factory();
    if (page) {
        page->setObjectName(pageId);
        m_container->addWidget(page);
    }
    
    return page;
}

} // namespace WealthPilot
