/**
 * @file MainWindow.cpp
 * @brief Main Window - Refactored version with new architecture
 */

#include "MainWindow.h"
#include "../../app/ApplicationInitializer.h"
#include "../../core/di/ServiceLocator.h"
#include "../../core/config/EnvironmentConfig.h"
#include "../../ui/components/ThemeEngine.h"
#include "../../ui/components/LayoutConstants.h"
#include "../../ui/components/ChartStyles.h"
#include "../../plugins/PluginLoader.h"
#include "../../utils/Logger.h"
#include "../widgets/SidebarWidget.h"
#include "../widgets/TitleBarWidget.h"
#include "../widgets/StatusBarWidget.h"
#include "../widgets/DividerWidget.h"
#include "../widgets/AIAssistantPanelWidget.h"
#include "../dashboard/DashboardPage.h"
#include "../stock/StockQuotesPage.h"
#include "../futures/FuturesQuotesPage.h"
#include "../futures/FuturesKLinePage.h"
#include "../../core/base/BasePage.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QSettings>
#include <QCloseEvent>
#include <QElapsedTimer>
#include <QApplication>

#include "plugins/IAIPlugin.h"
#include "plugins/ICTPPlugin.h"
#include "views/aboutus/AboutUSPage.h"
#include "views/news/NewsPage.h"
#include "views/portfolio/PortfolioPage.h"
#include "views/settings/SettingsPage.h"
#include "views/signalCenter/SignalCenterPage.h"
#include "views/watchList/WatchListPage.h"
#include "views/account/AccountPage.h"
#include "views/trading/TradeHistoryPage.h"
#include "views/trading/ConditionOrderPage.h"
#include "views/settings/RiskSettingsPage.h"

// PIMPL 实现
struct MainWindow::Impl
{
    // Layout components
    QWidget* centralWidget{};
    QVBoxLayout* rootLayout{};
    QHBoxLayout* mainLayout{};

    // 标题栏
    TitleBarWidget* titleBar{};

    // 侧边栏
    SidebarWidget* sidebar{};
    QStackedWidget* contentStack{};

    // AI分析面板
    AIAssistantPanelWidget* aiPanel{};

    // 状态栏
    StatusBarWidget* statusBar{};

    // 页面缓存
    QMap<QString, QWidget*> pageCache;

    // 当前页面ID
    QString currentPageId;
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , d(std::make_unique<Impl>())
      , m_initialized(false)
      , m_splashLabel(nullptr)
{
    setWindowTitle("WealthPilot - 财富领航AI助手");

    // 使用统一的布局常量
    setMinimumSize(Layout::Window::MinWidth, Layout::Window::MinHeight);
    resize(Layout::Window::DefaultWidth, Layout::Window::DefaultHeight);

    // 设置窗口无边框属性（关键标志）
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
        Qt::WindowMinimizeButtonHint |
        Qt::WindowMaximizeButtonHint |
        Qt::WindowCloseButtonHint);

    QElapsedTimer timer;
    timer.start();

    // 展示动画
    showSplashScreen();

    // 初始化应用
    if (!initializeApplication())
    {
        LOG_ERROR("Application initialization failed");
        return;
    }
    // 构建UI
    setupUI();

    // 创建页面
    createPages();

    // 连接信号
    connectSignals();

    // 加载设置
    loadSettings();

    // 应用主题
    applyTheme();

    m_initialized = true;

    // 隐藏动画
    hideSplashScreen();

    LOG_INFO(QString("MainWindow created in %1ms").arg(timer.elapsed()));
}

/**
 * 析构方法
 */
MainWindow::~MainWindow()
{
    if (m_initialized)
    {
        saveSettings();

        // 清理页面缓存
        for (auto page : d->pageCache)
        {
            if (page)
            {
                page->deleteLater();
            }
        }
        d->pageCache.clear();
    }

    LOG_DEBUG("MainWindow destroyed");
}

/**
 * 更新最大化按钮
 * @param isMaximized
 */
void MainWindow::updateMaximizeButton(const bool isMaximized)
{
    Q_UNUSED(isMaximized);
    // 更新最大化按钮图标
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    adjustLayout();
}

/**
 *关闭窗口事件
 */
void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();

    // 关闭应用
    ApplicationInitializer::instance().shutdown();

    event->accept();

    LOG_INFO("Application closed");
}

/**
 *点击侧边栏触发事件
 */
void MainWindow::onSidebarItemClicked(const QString& id)
{
    LOG_DEBUG(QString("Sidebar item clicked: %1").arg(id));

    // 设置侧边栏选中状态
    d->sidebar->setCurrentItem(id);

    // Get page
    QWidget* page = getPage(id);
    if (page)
    {
        d->contentStack->setCurrentWidget(page);
        d->currentPageId = id;

        // Call onPageActivated if it's a BasePage
        auto* basePage = qobject_cast<BasePage*>(page);
        if (basePage)
        {
            basePage->onPageActivated();
        }

        LOG_DEBUG(QString("Switched to page: %1").arg(id));
    }
}

/**
 * 触发应用主题事件
 * @param themeName
 */
void MainWindow::onThemeChanged(const QString& themeName)
{
    LOG_INFO(QString("Theme changed to: %1").arg(themeName));
    applyTheme();
}

/**
 * 初始化进程
 * @param current
 * @param total
 * @param currentModule
 */
void MainWindow::onInitializationProgress(const int current, const int total, const QString& currentModule) const
{
    LOG_DEBUG(QString("Initialization progress: %1/%2 - %3")
        .arg(current).arg(total).arg(currentModule));

    // 更新启动画面进度
    if (m_splashLabel)
    {
        m_splashLabel->setText(QString("正在加载: %1 (%2/%3)")
                               .arg(currentModule).arg(current).arg(total));
    }
}

/**
 * 初始化完成
 * @param success
 */
void MainWindow::onInitializationComplete(const bool success)
{
    if (success)
    {
        LOG_INFO("Application initialization complete");
    }
    else
    {
        LOG_ERROR("Application initialization failed");
    }
}

void MainWindow::onNavigateToKLinePage(const QString& instrumentId, const QVariantMap& params)
{
    LOG_INFO(QString("Navigating to KLine page for: %1").arg(instrumentId));

    // Get or create KLine page
    QWidget* klinePage = getPage("FuturesKLine");
    if (klinePage)
    {
        // Set instrument
        auto* kline = qobject_cast<FuturesKLinePage*>(klinePage);
        if (kline)
        {
            // Prepare params
            QVariantMap pageParams = params;
            pageParams["instrumentId"] = instrumentId;

            // Activate page with params
            kline->onPageActivated(pageParams);
        }
        // Switch to KLine page
        d->contentStack->setCurrentWidget(klinePage);
        d->currentPageId = "FuturesKLine";
    }
}

void MainWindow::setupUI()
{
    // 创建中心控件
    d->centralWidget = new QWidget(this);
    setCentralWidget(d->centralWidget);

    // 布局：垂直布局
    d->rootLayout = new QVBoxLayout(d->centralWidget);
    d->rootLayout->setContentsMargins(Layout::Margin::None());
    d->rootLayout->setSpacing(Layout::Spacing::None);

    // 标题栏
    d->titleBar = new TitleBarWidget(this);
    d->titleBar->setFixedHeight(Layout::Height::TitleBar);
    d->rootLayout->addWidget(d->titleBar);

    // 分割线
    DividerWidget *titleDivider = DividerWidget::createHorizontal(d->titleBar, ChartStyles::Colors::BgElevated, 1, 0);
    d->rootLayout->addWidget(titleDivider);

    // 主要内容区域
    auto* mainContent = new QWidget();
    d->mainLayout = new QHBoxLayout(mainContent);
    d->mainLayout->setContentsMargins(Layout::Margin::None());
    d->mainLayout->setSpacing(Layout::Spacing::None);
    d->rootLayout->addWidget(mainContent, 1);

    // 侧边栏
    d->sidebar = new SidebarWidget(this);
    d->sidebar->setObjectName("sidebar");
    d->sidebar->setFixedWidth(Layout::Width::Sidebar);

    d->sidebar->addItem("dashboard", "全局");
    d->sidebar->addItem("stock", "股票");
    d->sidebar->addItem("futures", "期货");
    d->sidebar->addItem("portfolio", "持仓");
    d->sidebar->addItem("account", "账户");
    d->sidebar->addItem("tradeHistory", "成交");
    d->sidebar->addItem("conditionOrder", "条件单");
    d->sidebar->addItem("watchlist", "自选");
    d->sidebar->addItem("signal", "信号");
    d->sidebar->addItem("news", "资讯");
    d->sidebar->addItem("riskSettings", "风控");
    d->sidebar->addItem("settings", "设置");
    d->sidebar->addItem("about", "关于");

    d->mainLayout->addWidget(d->sidebar);

    // 分割线
    DividerWidget *sidebarDivider = DividerWidget::createVertical(d->sidebar, ChartStyles::Colors::BgElevated, 1, 0);
    d->mainLayout->addWidget(sidebarDivider);

    // 内容重叠层
    d->contentStack = new QStackedWidget(mainContent);
    d->contentStack->setObjectName("contentStack");
    d->contentStack->setContentsMargins(Layout::Margin::MD());
    d->mainLayout->addWidget(d->contentStack, 1);

    // 分割线
    DividerWidget *aiDivider = DividerWidget::createVertical(d->contentStack, ChartStyles::Colors::BgElevated, 1, 0);
    d->mainLayout->addWidget(aiDivider);

    // AI分析面板
    d->aiPanel = new AIAssistantPanelWidget(mainContent);
    d->aiPanel->setFixedWidth(Layout::Width::AIPanel);
    d->aiPanel->setObjectName("aiPanel");
    d->mainLayout->addWidget(d->aiPanel);

    // 分割线
    DividerWidget *statusBarDivider = DividerWidget::createHorizontal(d->contentStack, ChartStyles::Colors::BgElevated, 1, 0);
    d->rootLayout->addWidget(statusBarDivider);

    // 状态栏
    d->statusBar = new StatusBarWidget(d->centralWidget);
    d->statusBar->setObjectName("statusBar");
    d->statusBar->setFixedHeight(Layout::Height::StatusBar);
    d->rootLayout->addWidget(d->statusBar);


}

/**
 * 创建页面
 */
void MainWindow::createPages() const
{
    // Create placeholder pages (lazy loading)
    QStringList pageIds = {
        "dashboard",
        "stock",
        "futures",
        "portfolio",
        "account",
        "tradeHistory",
        "conditionOrder",
        "watchlist",
        "signal",
        "news",
        "riskSettings",
        "settings",
        "about"
    };

    for (const QString& id : pageIds)
    {
        // Create placeholder
        auto* placeholder = new QWidget();
        placeholder->setObjectName(id + "_placeholder");
        d->pageCache[id] = nullptr; // Will be created on demand
        d->contentStack->addWidget(placeholder);
    }

    LOG_DEBUG(QString("Created %1 page placeholders").arg(pageIds.size()));
}

void MainWindow::connectSignals()
{
    // 链接应用初始化信号
    connect(&ApplicationInitializer::instance(), &ApplicationInitializer::initializationComplete,
            this, &MainWindow::onInitializationComplete);

    // 链接侧边栏点击信号
    connect(d->sidebar, &SidebarWidget::itemClicked,
            this, &MainWindow::onSidebarItemClicked);

    // 链接主题管理信号
    connect(&ThemeEngine::instance(), &ThemeEngine::themeChanged,
            this, &MainWindow::onThemeChanged);
}

void MainWindow::adjustLayout()
{
    // Responsive layout adjustment

    if (const int width = this->width(); width < 1000)
    {
        // 小屏幕：隐藏AI面板，折叠侧边栏
        d->aiPanel->hide();
        d->sidebar->setCollapsed(true);
    }
    else if (width < 1300)
    {
        // 中等屏幕：显示AI面板，缩小侧边栏
        d->aiPanel->show();
        d->sidebar->setCollapsed(false);
        d->sidebar->setExpandedWidth(150);
    }
    else
    {
        // 大屏幕：完整布局
        d->aiPanel->show();
        d->sidebar->setCollapsed(false);
        d->sidebar->setExpandedWidth(80);
    }
}

QWidget* MainWindow::getPage(const QString& pageId)
{
    // 验证缓存
    if (d->pageCache.contains(pageId) && d->pageCache[pageId])
    {
        return d->pageCache[pageId];
    }

    // 创建新页面
    QWidget* page = nullptr;
    if (pageId == "dashboard")
    {
        page = new DashboardPage(this);
    }
    else if (pageId == "stock")
    {
        page = new StockQuotesPage(this);
    }
    else if (pageId == "futures")
    {
        auto* quotesPage = new FuturesQuotesPage(this);
        page = quotesPage;
        // 链接K线页面信号
        connect(quotesPage, &FuturesQuotesPage::navigateToKLinePage,
                this, &MainWindow::onNavigateToKLinePage);
    }
    else if (pageId == "portfolio")
    {
        page = new PortfolioPage(this);
    }
    else if (pageId == "watchlist")
    {
        page = new WatchListPage(this);
    }
    else if (pageId == "signal")
    {
        page = new SignalCenterPage(this);
    }
    else if (pageId == "news")
    {
        page = new NewsPage(this);
    }
    else if (pageId == "settings")
    {
        page = new SettingsPage(this);
    }
    else if (pageId == "about")
    {
        page = new AboutUSPage(this);
    }
    else if (pageId == "FuturesKLine")
    {
        page = new FuturesKLinePage(this);
    }
    else if (pageId == "account")
    {
        page = new AccountPage(this);
    }
    else if (pageId == "tradeHistory")
    {
        page = new TradeHistoryPage(this);
    }
    else if (pageId == "conditionOrder")
    {
        page = new ConditionOrderPage(this);
    }
    else if (pageId == "riskSettings")
    {
        page = new RiskSettingsPage(this);
    }

    if (page)
    {
        d->pageCache[pageId] = page;
        d->contentStack->addWidget(page);

        // 如果是 BasePage，调用 initializePage
        auto* basePage = qobject_cast<BasePage*>(page);
        if (basePage && !basePage->isInitialized())
        {
            basePage->initializePage();
            basePage->setInitialized(true);
        }

        LOG_DEBUG(QString("Page loaded: %1").arg(pageId));
    }

    return page;
}

bool MainWindow::initializeApplication()
{
    LOG_INFO("Starting application initialization...");

    // Initialize application
    if (!ApplicationInitializer::instance().initialize())
    {
        return false;
    }

    // Register services to ServiceLocator
    // CTP plugin - may not be available if not loaded
    auto ctpPlugin = PluginLoader::instance().getPlugin<ICTPPlugin>("CTPPlugin");
    if (ctpPlugin)
    {
        ServiceLocator::instance().registerInstance<ICTPPlugin>(ctpPlugin);
        LOG_DEBUG("CTPPlugin registered to ServiceLocator");
    }
    else
    {
        LOG_WARNING("CTPPlugin not available - CTP features will be limited");
    }

    // AI plugin - may not be available if not loaded
    auto aiPlugin = PluginLoader::instance().getPlugin<IAIPlugin>("AIPlugin");
    if (aiPlugin)
    {
        ServiceLocator::instance().registerInstance<IAIPlugin>(aiPlugin);
        LOG_DEBUG("AIPlugin registered to ServiceLocator");
    }
    else
    {
        LOG_WARNING("AIPlugin not available - AI features will be limited");
    }

    LOG_INFO("Application initialized successfully");
    return true;
}

void MainWindow::loadSettings()
{
    QSettings settings;

    // Restore window geometry
    QByteArray geometry = settings.value("window/geometry").toByteArray();
    if (!geometry.isEmpty())
    {
        restoreGeometry(geometry);
    }

    // Restore window state
    QByteArray state = settings.value("window/state").toByteArray();
    if (!state.isEmpty())
    {
        restoreState(state);
    }
    
    // 默认启动页面为 dashboard
    // 注意：这里强制使用 dashboard 作为启动页面，而不是从设置中读取
    // 这样可以确保每次启动都显示 dashboard
    QString lastPage = "dashboard";
    
    // 设置侧边栏选中状态
    d->sidebar->setCurrentItem(lastPage);
    
    // 切换到目标页面
    onSidebarItemClicked(lastPage);

    LOG_DEBUG(QString("Startup with default page: %1").arg(lastPage));
}

void MainWindow::saveSettings() const
{
    QSettings settings;

    // Save window geometry
    settings.setValue("window/geometry", saveGeometry());

    // Save window geometry

    // 保存最后访问的页面
    settings.setValue("window/lastPage", d->currentPageId);

    settings.sync();

    LOG_DEBUG("Settings saved");
}

void MainWindow::applyTheme()
{
    // Save current page ID
    // setStyleSheet(styleSheet);

    LOG_DEBUG("Theme applied");
}

void MainWindow::showSplashScreen()
{
    // 创建启动画面
    m_splashLabel = new QLabel(this);
    m_splashLabel->setAlignment(Qt::AlignCenter);
    m_splashLabel->setStyleSheet(R"(
        QLabel {
            background-color: #1A1A2E;
            color: #E2E8F0;
            font-size: 16px;
            font-weight: bold;
            padding: 20px;
        }
    )");
    // 应用主题
    m_splashLabel->setGeometry(0, 0, width(), height());
    m_splashLabel->raise();
    m_splashLabel->show();
}

void MainWindow::hideSplashScreen()
{
    if (m_splashLabel)
    {
        m_splashLabel->hide();
        m_splashLabel->deleteLater();
        m_splashLabel = nullptr;
    }
}



