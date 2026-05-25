/**
 * @file FuturesPageIntegration.cpp
 * @brief 期货页面集成实现 - 将行情列表页和K线详情页集成
 *
 * @details 实现功能�? * - 页面跳转管理
 * - 参数传�? * - 实时行情订阅
 * - CTP连接共享
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "FuturesPageIntegration.h"
#include "FuturesQuotesPage.h"
#include "FuturesKLinePage.h"
#include "core/navigation/PageNavigator.h"
#include "core/di/ServiceLocator.h"
#include "plugins/ICTPPlugin.h"
#include "shared/utils/Logger.h"
#include <QStackedWidget>

// 使用 WealthPilot 命名空间中的类
using WealthPilot::BasePage;
using WealthPilot::FuturesQuotesPage;

// ========== FuturesPageIntegration::Impl 实现 ==========

struct FuturesPageIntegration::Impl {
    QStackedWidget* stackedWidget = nullptr;
    WealthPilot::FuturesQuotesPage* quotesPage = nullptr;
    FuturesKLinePage* klinePage = nullptr;
    PageNavigator* navigator = nullptr;
};

// ========== FuturesPageIntegration 实现 ==========

/**
 * @brief 获取单例实例
 */
FuturesPageIntegration& FuturesPageIntegration::instance()
{
    static FuturesPageIntegration instance;
    return instance;
}

/**
 * @brief 构造函�? */
FuturesPageIntegration::FuturesPageIntegration()
    : d(std::make_unique<Impl>())
{
    LOG_DEBUG("FuturesPageIntegration created");
}

/**
 * @brief 析构函数
 */
FuturesPageIntegration::~FuturesPageIntegration()
{
    LOG_DEBUG("FuturesPageIntegration destroyed");
}

/**
 * @brief 初始化集�? * @param stackedWidget 页面容器
 */
void FuturesPageIntegration::initialize(QStackedWidget* stackedWidget)
{
    LOG_INFO("Initializing FuturesPageIntegration...");
    
    d->stackedWidget = stackedWidget;
    d->navigator = &PageNavigator::instance();
    
    // 创建页面
    d->quotesPage = new WealthPilot::FuturesQuotesPage(d->stackedWidget);
    d->klinePage = new FuturesKLinePage(d->stackedWidget);
    
    // 添加到容�?    d->stackedWidget->addWidget(d->quotesPage);
    d->stackedWidget->addWidget(d->klinePage);
    
    // 注册页面到导航器
    d->navigator->registerPage("FuturesQuotesPage", [this]() -> BasePage* {
        return d->quotesPage;
    });
    
    d->navigator->registerPage("FuturesKLinePage", [this]() -> BasePage* {
        return d->klinePage;
    });
    
    // 设置连接
    setupConnections();
    
    // 默认显示行情列表�?    showQuotesPage();
    
    LOG_INFO("FuturesPageIntegration initialized");
}

/**
 * @brief 设置信号连接
 */
void FuturesPageIntegration::setupConnections()
{
    // 连接行情列表页的导航信号
    connect(d->quotesPage, &FuturesQuotesPage::navigateToKLinePage,
            this, &FuturesPageIntegration::onNavigateToKLinePage);
    
    // 连接导航器的导航完成信号
    connect(d->navigator, &PageNavigator::navigated,
            this, &FuturesPageIntegration::onNavigated);
    
    LOG_DEBUG("FuturesPageIntegration connections setup");
}

/**
 * @brief 显示行情列表�? */
void FuturesPageIntegration::showQuotesPage()
{
    if (!d->stackedWidget || !d->quotesPage) {
        LOG_WARNING("Cannot show quotes page: not initialized");
        return;
    }
    
    d->stackedWidget->setCurrentWidget(d->quotesPage);
    d->quotesPage->onPageActivated();
    
    emit pageChanged("FuturesQuotesPage");
    
    LOG_INFO("Showing FuturesQuotesPage");
}

/**
 * @brief 显示K线详情页
 * @param instrumentId 合约代码
 */
void FuturesPageIntegration::showKLinePage(const QString& instrumentId)
{
    if (!d->stackedWidget || !d->klinePage) {
        LOG_WARNING("Cannot show kline page: not initialized");
        return;
    }
    
    // 构建参数
    QVariantMap params;
    params[NavParam::INSTRUMENT_ID] = instrumentId;
    params[NavParam::SOURCE_PAGE] = "FuturesQuotesPage";
    
    // 设置合约
    d->klinePage->setInstrument(instrumentId);
    
    // 切换页面
    d->stackedWidget->setCurrentWidget(d->klinePage);
    d->klinePage->onPageActivated(params);
    
    emit pageChanged("FuturesKLinePage");
    emit instrumentSelected(instrumentId);
    
    LOG_INFO(QString("Showing FuturesKLinePage for: %1").arg(instrumentId));
}

/**
 * @brief 返回行情列表�? */
void FuturesPageIntegration::goBack()
{
    if (d->navigator->canGoBack()) {
        d->navigator->goBack();
        showQuotesPage();
    }
}

/**
 * @brief 获取行情列表�? */
FuturesQuotesPage* FuturesPageIntegration::quotesPage() const
{
    return d->quotesPage;
}

/**
 * @brief 获取K线详情页
 */
FuturesKLinePage* FuturesPageIntegration::klinePage() const
{
    return d->klinePage;
}

/**
 * @brief 处理导航到K线页面的请求
 */
void FuturesPageIntegration::onNavigateToKLinePage(const QString& instrumentId, 
                                                   const QVariantMap& params)
{
    LOG_INFO(QString("Navigating to KLine page for: %1").arg(instrumentId));
    
    // 使用导航器进行跳�?    d->navigator->navigateTo("FuturesKLinePage", params);
    
    // 显示K线页�?    showKLinePage(instrumentId);
}

/**
 * @brief 处理导航完成事件
 */
void FuturesPageIntegration::onNavigated(const QString& pageId, 
                                         const QVariantMap& params)
{
    LOG_DEBUG(QString("Navigation completed to: %1").arg(pageId));
    
    // 根据页面ID切换显示
    if (pageId == "FuturesQuotesPage") {
        d->stackedWidget->setCurrentWidget(d->quotesPage);
    } else if (pageId == "FuturesKLinePage") {
        d->stackedWidget->setCurrentWidget(d->klinePage);
    }
}
