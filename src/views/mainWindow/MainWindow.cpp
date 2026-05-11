/**
 * @file MainWindow.cpp
 * @brief Main Window - Refactored version with new architecture
 */

#include "MainWindow.h"
#include "../../app/ApplicationInitializer.h"
#include "../../core/di/ServiceLocator.h"
#include "../../core/config/EnvironmentConfig.h"
#include "../../core/config/Tokens.h"
#include "../../core/navigation/PageNavigator.h"
#include "../../ui/ThemeManager.h"
#include "../../ui/components/StyleHelper.h"
#include "../../ui/components/ThemeEngine.h"
#include "../../ui/components/LayoutConstants.h"
#include "../../ui/components/ChartStyles.h"
#include "../../plugins/PluginLoader.h"
#include "../../utils/Logger.h"
#include "ui/components/SidebarWidget.h"
#include "ui/components/TitleBarWidget.h"
#include "ui/components/StatusBarWidget.h"
#include "ui/components/DividerWidget.h"
#include "ui/components/AIAssistantPanelWidget.h"
#include "../dashboard/DashboardPage.h"
#include "../stock/StockQuotesPage.h"
#include "../stock/StockKLinePage.h"
#include "../futures/FuturesQuotesPage.h"
#include "../futures/FuturesKLinePage.h"
#include "../portfolio/PortfolioPage.h"
#include "../watchList/WatchListPage.h"
#include "../signalCenter/SignalCenterPage.h"
#include "../news/NewsPage.h"
#include "../settings/SettingsPage.h"
#include "../aboutus/AboutUSPage.h"
#include "../account/AccountPage.h"
#include "../trading/TradeHistoryPage.h"
#include "../trading/ConditionOrderPage.h"
#include "../settings/RiskSettingsPage.h"
#include "../fund/FundPage.h"
#include "../forex/ForexPage.h"
#include "../crypto/CryptoPage.h"
#include "../backtest/BacktestPage.h"
#include "../alert/AlertCenterPage.h"
#include "../demo/QmlChartDemoPage.h"
#include "../../ui/components/BasePage.h"

// 使用 WealthPilot 命名空间中的类
using WealthPilot::BasePage;
using WealthPilot::StockQuotesPage;
using WealthPilot::WatchListPage;
using WealthPilot::SignalCenterPage;
using WealthPilot::TradeHistoryPage;
using WealthPilot::ConditionOrderPage;
using WealthPilot::CryptoPage;

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QSettings>
#include <QCloseEvent>
#include <QElapsedTimer>
#include <QApplication>
#include <QShortcut>

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
// ����ģ��
#include "views/fund/FundPage.h"
#include "views/forex/ForexPage.h"
#include "views/crypto/CryptoPage.h"
#include "views/backtest/BacktestPage.h"
#include "views/alert/AlertCenterPage.h"

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

    // 设置快捷键
    setupShortcuts();

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

void MainWindow::onNavigateToStockKLinePage(const QString& symbol, const QString& name)
{
    LOG_INFO(QString("Navigating to Stock KLine page for: %1 (%2)").arg(symbol, name));

    // Get or create Stock KLine page
    QWidget* klinePage = getPage("StockKLine");
    if (klinePage)
    {
        // Set stock
        auto* kline = qobject_cast<StockKLinePage*>(klinePage);
        if (kline)
        {
            // 确定交易所
            QString exchange = symbol.startsWith("6") ? "SH" : "SZ";
            
            // 设置股票
            kline->setStock(symbol, exchange);
        }
        // Switch to KLine page
        d->contentStack->setCurrentWidget(klinePage);
        d->currentPageId = "StockKLine";
        
        // 更新侧边栏选中状态（取消选中，因为K线页面不在侧边栏）
        d->sidebar->clearSelection();
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
    DividerWidget* titleDivider = DividerWidget::createHorizontal(d->titleBar, ChartStyles::Colors::BgElevated, 1, 0);
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

    d->sidebar->addItem("dashboard", QStringLiteral("全局"));
    d->sidebar->addItem("stock", QStringLiteral("股票"));
    d->sidebar->addItem("futures", QStringLiteral("期货"));
    d->sidebar->addItem("fund", QStringLiteral("基金"));
    d->sidebar->addItem("forex", QStringLiteral("外汇"));
    d->sidebar->addItem("crypto", QStringLiteral("数字货币"));
    d->sidebar->addItem("portfolio", QStringLiteral("持仓"));
    d->sidebar->addItem("account", QStringLiteral("账户"));
    d->sidebar->addItem("tradeHistory", QStringLiteral("成交"));
    d->sidebar->addItem("conditionOrder", QStringLiteral("条件单"));
    d->sidebar->addItem("watchlist", QStringLiteral("自选"));
    d->sidebar->addItem("backtest", QStringLiteral("回测"));
    d->sidebar->addItem("alertCenter", QStringLiteral("预警"));
    d->sidebar->addItem("signal", QStringLiteral("信号"));
    d->sidebar->addItem("news", QStringLiteral("资讯"));
    d->sidebar->addItem("riskSettings", QStringLiteral("风控"));
    d->sidebar->addItem("settings", QStringLiteral("设置"));
    d->sidebar->addItem("about", QStringLiteral("关于"));
    d->sidebar->addItem("qmlChartDemo", QStringLiteral("QML演示"));

    d->mainLayout->addWidget(d->sidebar);

    // 分割线
    DividerWidget* sidebarDivider = DividerWidget::createVertical(d->sidebar, ChartStyles::Colors::BgElevated, 1, 0);
    d->mainLayout->addWidget(sidebarDivider);

    // 内容重叠层
    d->contentStack = new QStackedWidget(mainContent);
    d->contentStack->setObjectName("contentStack");
    d->contentStack->setContentsMargins(Layout::Margin::MD());
    d->mainLayout->addWidget(d->contentStack, 1);

    // 分割线
    DividerWidget* aiDivider = DividerWidget::createVertical(d->contentStack, ChartStyles::Colors::BgElevated, 1, 0);
    d->mainLayout->addWidget(aiDivider);

    // AI分析面板
    d->aiPanel = new AIAssistantPanelWidget(mainContent);
    d->aiPanel->setFixedWidth(Layout::Width::AIPanel);
    d->aiPanel->setObjectName("aiPanel");
    d->mainLayout->addWidget(d->aiPanel);

    // 分割线
    DividerWidget* statusBarDivider = DividerWidget::createHorizontal(d->contentStack, ChartStyles::Colors::BgElevated,
                                                                      1, 0);
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
        "fund",
        "forex",
        "crypto",
        "portfolio",
        "account",
        "tradeHistory",
        "conditionOrder",
        "watchlist",
        "backtest",
        "alertCenter",
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
    
    // 链接页面导航信号
    connect(&PageNavigator::instance(), &PageNavigator::navigating,
            this, [this](const QString& pageId, const QVariantMap& params) {
        Q_UNUSED(params);
        onSidebarItemClicked(pageId);
    });
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
        auto* dashboardPage = new DashboardPage(this);
        page = dashboardPage;
        // 连接Dashboard的K线导航信号
        connect(dashboardPage, &DashboardPage::navigateToStockKLine,
                this, &MainWindow::onNavigateToStockKLinePage);
    }
    else if (pageId == "stock")
    {
        auto* quotesPage = new StockQuotesPage(this);
        page = quotesPage;
        // 链接K线页面信号
        connect(quotesPage, &StockQuotesPage::navigateToKLinePage,
                this, &MainWindow::onNavigateToStockKLinePage);
    }
    else if (pageId == "futures")
    {
        auto* quotesPage = new WealthPilot::FuturesQuotesPage(this);
        page = quotesPage;
        // 链接K线页面信号
        connect(quotesPage, &WealthPilot::FuturesQuotesPage::navigateToKLinePage,
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
    else if (pageId == "StockKLine")
    {
        page = new StockKLinePage(this);
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
    // ========== 新增模块 ==========
    else if (pageId == "fund")
    {
        page = new FundPage(this);
    }
    else if (pageId == "forex")
    {
        page = new ForexPage(this);
    }
    else if (pageId == "crypto")
    {
        page = new CryptoPage(this);
    }
    else if (pageId == "backtest")
    {
        page = new BacktestPage(this);
    }
    else if (pageId == "alertCenter")
    {
        page = new AlertCenterPage(this);
    }
    // ========== QML 演示 ==========
    else if (pageId == "qmlChartDemo")
    {
        page = new QmlChartDemoPage(this);
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

    // Ĭ������ҳ��Ϊ dashboard
    // ע�⣺����ǿ��ʹ�� dashboard ��Ϊ����ҳ�棬�����Ǵ������ж�ȡ
    // ��������ȷ��ÿ����������ʾ dashboard
    QString lastPage = "dashboard";

    // ���ò����ѡ��״̬
    d->sidebar->setCurrentItem(lastPage);

    // �л���Ŀ��ҳ��
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
    // 应用主题管理器的主题
    auto* themeManager = ThemeManager::instance();
    if (!themeManager) {
        LOG_WARNING("ThemeManager not available");
        return;
    }
    
    // 先调用 ThemeManager 应用全局主题（包括 QSS 和 Palette）
    themeManager->applyTheme();
    
    // 获取当前主题配色
    ThemeColors theme = themeManager->currentTheme();
    
    // 更新子组件样式
    if (d->titleBar) {
        d->titleBar->setStyleSheet(QString("TitleBarWidget { background-color: %1; border-bottom: 1px solid %2; }")
            .arg(theme.bgPrimary, theme.border));
        // 刷新所有子控件样式
        StyleHelper::refreshAll(d->titleBar);
    }
    if (d->sidebar) {
        d->sidebar->setStyleSheet(QString("SidebarWidget { background-color: %1; border-right: 1px solid %2; }")
            .arg(theme.bgSecondary, theme.border));
        StyleHelper::refreshAll(d->sidebar);
    }
    if (d->statusBar) {
        d->statusBar->setStyleSheet(QString("StatusBarWidget { background-color: %1; border-top: 1px solid %2; }")
            .arg(theme.bgSecondary, theme.border));
        StyleHelper::refreshAll(d->statusBar);
    }
    
    // 刷新当前页面
    QWidget* currentPage = d->contentStack ? d->contentStack->currentWidget() : nullptr;
    if (currentPage) {
        StyleHelper::refreshAll(currentPage);
    }
    
    LOG_INFO(QString("Theme applied: %1").arg(theme.name));
}

void MainWindow::showSplashScreen()
{
    // 创建启动画面
    m_splashLabel = new QLabel(this);
    m_splashLabel->setAlignment(Qt::AlignCenter);
    m_splashLabel->setStyleSheet(QString(R"(
        QLabel {
            background-color: %1;
            color: %2;
            font-size: 16px;
            font-weight: bold;
            padding: 20px;
        }
    )").arg(Tokens::Colors::BgBase, Tokens::Colors::TextPrimary));
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

void MainWindow::setupShortcuts()
{
    // Ctrl+1-9 切换页面
    QStringList pageIds = {
        QStringLiteral("dashboard"),
        QStringLiteral("stock"),
        QStringLiteral("futures"),
        QStringLiteral("fund"),
        QStringLiteral("forex"),
        QStringLiteral("crypto"),
        QStringLiteral("portfolio"),
        QStringLiteral("watchlist"),
        QStringLiteral("settings")
    };

    for (int i = 0; i < qMin(pageIds.size(), 9); ++i)
    {
        auto* shortcut = new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + i)), this);
        connect(shortcut, &QShortcut::activated, this, [this, pageIds, i]()
        {
            onSidebarItemClicked(pageIds[i]);
        });
    }

    // F5 刷新当前页面
    auto* refreshShortcut = new QShortcut(QKeySequence::Refresh, this);
    connect(refreshShortcut, &QShortcut::activated, this, [this]()
    {
        QWidget* currentPage = d->contentStack->currentWidget();
        if (currentPage)
        {
            QMetaObject::invokeMethod(currentPage, "refresh", Qt::DirectConnection);
        }
    });

    // Ctrl+Shift+A 显示/隐藏AI面板
    auto* aiToggleShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A), this);
    connect(aiToggleShortcut, &QShortcut::activated, this, [this]()
    {
        if (d->aiPanel)
        {
            d->aiPanel->setVisible(!d->aiPanel->isVisible());
        }
    });

    LOG_DEBUG("Shortcuts setup complete");
}



