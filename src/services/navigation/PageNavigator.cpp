/**
 * @file PageNavigator.cpp
 * @brief Page Navigator Implementation
 */

#include "PageNavigator.h"
#include "presentation/components/BasePage.h"
#include "shared/utils/Logger.h"
#include <QDateTime>

// 使用 WealthPilot 命名空间中的 BasePage
using WealthPilot::BasePage;

// ========== PageNavigator::Impl Implementation ==========

struct PageNavigator::Impl {
    // Navigation history stack
    QStack<NavigationHistory> historyStack;
    
    // Current page info
    QString currentPageId;
    QVariantMap currentParams;
    
    // Page registry
    QMap<QString, std::function<BasePage*()>> pageRegistry;
    
    // Max history size
    static constexpr int MAX_HISTORY_SIZE = 50;
};

// ========== PageNavigator Implementation ==========

PageNavigator::PageNavigator()
    : d(std::make_unique<Impl>())
{
    LOG_DEBUG("PageNavigator created");
}

PageNavigator::~PageNavigator() = default;

PageNavigator& PageNavigator::instance()
{
    static PageNavigator instance;
    return instance;
}

void PageNavigator::navigateTo(const QString& pageId, const QVariantMap& params, bool addToHistory)
{
    // Add to history
    if (addToHistory && !d->currentPageId.isEmpty()) {
        NavigationHistory history;
        history.pageId = d->currentPageId;
        history.params = d->currentParams;
        history.timestamp = QDateTime::currentDateTime();
        
        d->historyStack.push(history);
        
        // Limit history size
        while (d->historyStack.size() > Impl::MAX_HISTORY_SIZE) {
            d->historyStack.pop_front();
        }
    }
    
    // Emit navigating signal
    emit navigating(pageId, params);
    
    // Update current page
    d->currentPageId = pageId;
    d->currentParams = params;
    
    LOG_DEBUG(QString("Navigate to: %1").arg(pageId));
    
    emit navigated(pageId, params);
}

bool PageNavigator::goBack()
{
    if (d->historyStack.isEmpty()) {
        LOG_DEBUG("No history to go back");
        return false;
    }
    
    NavigationHistory history = d->historyStack.pop();
    
    QString fromPageId = d->currentPageId;
    d->currentPageId = history.pageId;
    d->currentParams = history.params;
    
    LOG_DEBUG(QString("Go back from %1 to: %2").arg(fromPageId, history.pageId));
    
    emit goingBack(fromPageId, history.pageId);
    emit navigated(history.pageId, history.params);
    return true;
}

bool PageNavigator::goBackTo(const QString& pageId)
{
    int targetIndex = -1;
    
    for (int i = d->historyStack.size() - 1; i >= 0; --i) {
        if (d->historyStack[i].pageId == pageId) {
            targetIndex = i;
            break;
        }
    }
    
    if (targetIndex < 0) {
        LOG_DEBUG(QString("Page not found in history: %1").arg(pageId));
        return false;
    }
    
    QString fromPageId = d->currentPageId;
    
    // Pop to target page
    while (d->historyStack.size() > targetIndex + 1) {
        d->historyStack.pop();
    }
    
    NavigationHistory history = d->historyStack.pop();
    
    d->currentPageId = history.pageId;
    d->currentParams = history.params;
    
    LOG_DEBUG(QString("Go back from %1 to: %2").arg(fromPageId, history.pageId));
    
    emit goingBack(fromPageId, history.pageId);
    emit navigated(history.pageId, history.params);
    return true;
}

void PageNavigator::clearHistory()
{
    d->historyStack.clear();
    LOG_DEBUG("Navigation history cleared");
}

QString PageNavigator::currentPageId() const
{
    return d->currentPageId;
}

QVariantMap PageNavigator::currentParams() const
{
    return d->currentParams;
}

int PageNavigator::historyCount() const
{
    return d->historyStack.size();
}

bool PageNavigator::canGoBack() const
{
    return !d->historyStack.isEmpty();
}

void PageNavigator::registerPage(const QString& pageId, std::function<BasePage*()> creator)
{
    d->pageRegistry[pageId] = creator;
    LOG_DEBUG(QString("Page registered: %1").arg(pageId));
}

void PageNavigator::unregisterPage(const QString& pageId)
{
    d->pageRegistry.remove(pageId);
    LOG_DEBUG(QString("Page unregistered: %1").arg(pageId));
}

BasePage* PageNavigator::createPage(const QString& pageId)
{
    auto it = d->pageRegistry.find(pageId);
    if (it != d->pageRegistry.end()) {
        return it.value()();
    }
    
    LOG_WARNING(QString("Page not registered: %1").arg(pageId));
    return nullptr;
}
