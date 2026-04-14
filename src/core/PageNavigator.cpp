/**
 * @file PageNavigator.cpp
 * @brief 页面导航管理器实现 - 统一管理页面跳转和参数传递
 *
 * @details 实现功能：
 * - 页面跳转管理
 * - 参数传递
 * - 历史记录栈
 * - 返回导航
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "PageNavigator.h"
#include "BasePage.h"
#include "../utils/Logger.h"
#include <QDateTime>

// ========== PageNavigator::Impl 实现 ==========

struct PageNavigator::Impl {
    // 导航历史栈
    QStack<NavigationHistory> historyStack;
    
    // 当前页面信息
    QString currentPage;
    QVariantMap currentParams;
    
    // 页面注册表
    QMap<QString, std::function<BasePage*()>> pageRegistry;
    
    // 最大历史记录数
    static constexpr int MAX_HISTORY_SIZE = 50;
};

// ========== PageNavigator 实现 ==========

/**
 * @brief 获取单例实例
 */
PageNavigator& PageNavigator::instance()
{
    static PageNavigator instance;
    return instance;
}

/**
 * @brief 构造函数
 */
PageNavigator::PageNavigator()
    : d(std::make_unique<Impl>())
{
    LOG_DEBUG("PageNavigator created");
}

/**
 * @brief 析构函数
 */
PageNavigator::~PageNavigator()
{
    LOG_DEBUG("PageNavigator destroyed");
}

/**
 * @brief 导航到指定页面
 * @param pageId 目标页面ID
 * @param params 导航参数
 * @param addToHistory 是否添加到历史记录
 */
void PageNavigator::navigateTo(const QString& pageId, 
                               const QVariantMap& params,
                               bool addToHistory)
{
    LOG_INFO(QString("Navigating to: %1, params: %2")
        .arg(pageId).arg(params.size()));
    
    // 发送导航前信号
    emit navigating(pageId, params);
    
    // 添加到历史记录
    if (addToHistory && !d->currentPage.isEmpty()) {
        NavigationHistory history;
        history.pageId = d->currentPage;
        history.params = d->currentParams;
        history.timestamp = QDateTime::currentDateTime();
        
        d->historyStack.push(history);
        
        // 限制历史记录大小
        while (d->historyStack.size() > Impl::MAX_HISTORY_SIZE) {
            d->historyStack.pop_front();
        }
    }
    
    // 更新当前页面信息
    d->currentPage = pageId;
    d->currentParams = params;
    
    // 发送导航完成信号
    emit navigated(pageId, params);
    
    LOG_DEBUG(QString("Navigation completed: %1").arg(pageId));
}

/**
 * @brief 返回上一页
 * @return 是否成功返回
 */
bool PageNavigator::goBack()
{
    if (d->historyStack.isEmpty()) {
        LOG_WARNING("Cannot go back: history stack is empty");
        return false;
    }
    
    // 获取上一页信息
    NavigationHistory history = d->historyStack.pop();
    
    QString fromPage = d->currentPage;
    
    // 发送返回信号
    emit goingBack(fromPage, history.pageId);
    
    // 更新当前页面
    d->currentPage = history.pageId;
    d->currentParams = history.params;
    
    // 发送导航完成信号
    emit navigated(history.pageId, history.params);
    
    LOG_INFO(QString("Going back from %1 to %2")
        .arg(fromPage).arg(history.pageId));
    
    return true;
}

/**
 * @brief 返回到指定页面
 * @param pageId 目标页面ID
 * @return 是否成功返回
 */
bool PageNavigator::goBackTo(const QString& pageId)
{
    // 在历史栈中查找目标页面
    int targetIndex = -1;
    for (int i = d->historyStack.size() - 1; i >= 0; --i) {
        if (d->historyStack[i].pageId == pageId) {
            targetIndex = i;
            break;
        }
    }
    
    if (targetIndex < 0) {
        LOG_WARNING(QString("Cannot go back to: %1, not found in history")
            .arg(pageId));
        return false;
    }
    
    // 弹出到目标页面
    while (d->historyStack.size() > targetIndex + 1) {
        d->historyStack.pop();
    }
    
    // 执行返回
    return goBack();
}

/**
 * @brief 清空历史记录
 */
void PageNavigator::clearHistory()
{
    d->historyStack.clear();
    LOG_DEBUG("Navigation history cleared");
}

/**
 * @brief 获取当前页面ID
 */
QString PageNavigator::currentPageId() const
{
    return d->currentPage;
}

/**
 * @brief 获取当前页面参数
 */
QVariantMap PageNavigator::currentParams() const
{
    return d->currentParams;
}

/**
 * @brief 获取历史记录数量
 */
int PageNavigator::historyCount() const
{
    return d->historyStack.size();
}

/**
 * @brief 是否可以返回
 */
bool PageNavigator::canGoBack() const
{
    return !d->historyStack.isEmpty();
}

/**
 * @brief 注册页面创建函数
 * @param pageId 页面ID
 * @param creator 创建函数
 */
void PageNavigator::registerPage(const QString& pageId,
                                std::function<BasePage*()> creator)
{
    d->pageRegistry[pageId] = creator;
    LOG_DEBUG(QString("Page registered: %1").arg(pageId));
}

/**
 * @brief 注销页面
 * @param pageId 页面ID
 */
void PageNavigator::unregisterPage(const QString& pageId)
{
    d->pageRegistry.remove(pageId);
    LOG_DEBUG(QString("Page unregistered: %1").arg(pageId));
}

/**
 * @brief 创建页面实例
 * @param pageId 页面ID
 * @return 页面实例（如果已注册）
 */
BasePage* PageNavigator::createPage(const QString& pageId)
{
    if (!d->pageRegistry.contains(pageId)) {
        LOG_WARNING(QString("Page not registered: %1").arg(pageId));
        return nullptr;
    }
    
    BasePage* page = d->pageRegistry[pageId]();
    LOG_DEBUG(QString("Page created: %1").arg(pageId));
    
    return page;
}
