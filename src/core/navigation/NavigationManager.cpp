/**
 * @file NavigationManager.cpp
 * @brief 导航管理器实现
 */

#include "NavigationManager.h"
#include "ui/components/BasePage.h"
#include "utils/Logger.h"

#include <QMutexLocker>

struct NavigationManager::Impl {
    QHash<QString, BasePage*> pages;
    QStack<NavigationRecord> history;
    QString currentPageId;
    mutable QMutex mutex;
    bool initialized = false;
};

NavigationManager& NavigationManager::instance()
{
    static NavigationManager instance;
    return instance;
}

NavigationManager::NavigationManager(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    LOG_DEBUG("NavigationManager created");
}

NavigationManager::~NavigationManager()
{
    LOG_DEBUG("NavigationManager destroyed");
}

bool NavigationManager::initialize()
{
    QMutexLocker locker(&d->mutex);

    if (d->initialized) {
        return true;
    }

    d->initialized = true;
    LOG_INFO("NavigationManager initialized");
    return true;
}

void NavigationManager::registerPage(const QString& pageId, BasePage* page)
{
    QMutexLocker locker(&d->mutex);

    if (!page) {
        LOG_WARNING(QString("Cannot register null page: %1").arg(pageId));
        return;
    }

    d->pages[pageId] = page;
    LOG_DEBUG(QString("Page registered: %1").arg(pageId));
}

void NavigationManager::unregisterPage(const QString& pageId)
{
    QMutexLocker locker(&d->mutex);

    if (d->pages.remove(pageId) > 0) {
        LOG_DEBUG(QString("Page unregistered: %1").arg(pageId));
    }
}

BasePage* NavigationManager::getPage(const QString& pageId) const
{
    QMutexLocker locker(&d->mutex);
    return d->pages.value(pageId, nullptr);
}

bool NavigationManager::navigateTo(const QString& pageId, const QVariantMap& params)
{
    QMutexLocker locker(&d->mutex);

    if (!d->pages.contains(pageId)) {
        LOG_WARNING(QString("Page not found: %1").arg(pageId));
        emit pageNotFound(pageId);
        return false;
    }

    BasePage* page = d->pages[pageId];
    if (!page) {
        emit navigationError(QString("Page is null: %1").arg(pageId));
        return false;
    }

    // 记录导航历史
    if (!d->currentPageId.isEmpty()) {
        NavigationRecord record;
        record.pageId = d->currentPageId;
        record.params = params;
        record.timestamp = QDateTime::currentDateTime();
        d->history.push(record);
    }

    // 更新当前页面
    d->currentPageId = pageId;

    // 调用页面的进入方法
    page->onEnter(params);

    LOG_DEBUG(QString("Navigated to: %1").arg(pageId));
    emit pageChanged(pageId, params);
    return true;
}

bool NavigationManager::goBack()
{
    QMutexLocker locker(&d->mutex);

    if (d->history.isEmpty()) {
        return false;
    }

    NavigationRecord record = d->history.pop();

    if (!d->pages.contains(record.pageId)) {
        LOG_WARNING(QString("Cannot go back, page not found: %1").arg(record.pageId));
        return false;
    }

    BasePage* page = d->pages[record.pageId];
    d->currentPageId = record.pageId;

    page->onEnter(record.params);

    LOG_DEBUG(QString("Navigated back to: %1").arg(record.pageId));
    emit pageChanged(record.pageId, record.params);
    return true;
}

bool NavigationManager::goHome()
{
    return navigateTo("home");
}

BasePage* NavigationManager::currentPage() const
{
    QMutexLocker locker(&d->mutex);

    if (d->currentPageId.isEmpty()) {
        return nullptr;
    }

    return d->pages.value(d->currentPageId, nullptr);
}

QString NavigationManager::currentPageId() const
{
    QMutexLocker locker(&d->mutex);
    return d->currentPageId;
}

bool NavigationManager::canGoBack() const
{
    QMutexLocker locker(&d->mutex);
    return !d->history.isEmpty();
}

void NavigationManager::clearHistory()
{
    QMutexLocker locker(&d->mutex);
    d->history.clear();
    LOG_DEBUG("Navigation history cleared");
}

QVector<NavigationRecord> NavigationManager::getHistory() const
{
    QMutexLocker locker(&d->mutex);
    return d->history.toVector();
}
