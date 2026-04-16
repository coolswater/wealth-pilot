/**
 * @file MainWindow.cpp
 * @brief Main Window - Refactored version with new architecture
 */

#include "MainWindow.h"
#include "../../app/ApplicationInitializer.h"
#include "../../core/di/ServiceLocator.h"
#include "../../core/config/EnvironmentConfig.h"
#include "../../ui/components/ThemeEngine.h"
#include "../../plugins/PluginLoader.h"
#include "../../utils/Logger.h"
#include "../widgets/SidebarWidget.h"
#include "../widgets/TitleBarWidget.h"
#include "../widgets/StatusBarWidget.h"
#include "../widgets/AIAssistantPanelWidget.h"
#include "../dashboard/DashboardPage.h"
#include "../stock/StockQuotesPage.h"
#include "../futures/FuturesQuotesPage.h"

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

// PIMPL implementation
struct MainWindow::Impl {
    // Layout components
    QWidget* centralWidget;
    QVBoxLayout* rootLayout;
    QHBoxLayout* mainLayout;
    
    // Title bar
    TitleBarWidget* titleBar;
    
    // Sidebar
    SidebarWidget* sidebar;
    QStackedWidget* contentStack;
    
    // AI assistant panel
    AIAssistantPanelWidget* aiPanel;
    
    // Status bar
    StatusBarWidget* statusBar;
    
    // Page cache (for performance optimization)
    QMap<QString, QWidget*> pageCache;
    
    // Current page ID
    QString currentPageId;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d(std::make_unique<Impl>())
    , m_initialized(false)
    , m_splashLabel(nullptr)
{
    QElapsedTimer timer;
    timer.start();
    
    // Show splash screen
    showSplashScreen();
    
    // Initialize application
    if (!initializeApplication()) {
        LOG_ERROR("Application initialization failed");
        return;
    }
    // Build UI
    setupUI();
    
    // Create pages
    createPages();
    
    // Load settings
    loadSettings();
    
    // Apply theme
    applyTheme();
    
    m_initialized = true;
    
    // Hide splash screen
    hideSplashScreen();
    
    LOG_INFO(QString("MainWindow created in %1ms").arg(timer.elapsed()));
}

MainWindow::~MainWindow()
{
    if (m_initialized) {
        saveSettings();
        
        // 清理页面缓存
        for (auto page : d->pageCache) {
            if (page) {
                page->deleteLater();
            }
        }
        d->pageCache.clear();
    }
    
    LOG_DEBUG("MainWindow destroyed");
}

void MainWindow::updateMaximizeButton(bool isMaximized)
{
    Q_UNUSED(isMaximized);
    // 更新最大化按钮图标
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    adjustLayout();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    
    // 关闭应用
    ApplicationInitializer::instance().shutdown();
    
    event->accept();
    
    LOG_INFO("Application closed");
}

void MainWindow::onSidebarItemClicked(const QString& id)
{
    LOG_DEBUG(QString("Sidebar item clicked: %1").arg(id));
    
    // Get page
    QWidget* page = getPage(id);
    if (page) {
        d->currentPageId = id;
    }
}

void MainWindow::onThemeChanged(const QString& themeName)
{
    LOG_INFO(QString("Theme changed to: %1").arg(themeName));
    applyTheme();
}

void MainWindow::onInitializationProgress(int current, int total, const QString& currentModule)
{
    LOG_DEBUG(QString("Initialization progress: %1/%2 - %3")
        .arg(current).arg(total).arg(currentModule));
    
    // 更新启动画面进度
    if (m_splashLabel) {
        m_splashLabel->setText(QString("正在加载: %1 (%2/%3)")
            .arg(currentModule).arg(current).arg(total));
    }
}

void MainWindow::onInitializationComplete(bool success)
{
    if (success) {
        LOG_INFO("Application initialization complete");
    } else {
        LOG_ERROR("Application initialization failed");
    }
}

void MainWindow::setupUI()
{
    // Create central widget
    d->centralWidget = new QWidget(this);
    setCentralWidget(d->centralWidget);
    
    // Layout: vertical
    d->rootLayout = new QVBoxLayout(d->centralWidget);
    d->rootLayout->setContentsMargins(0, 0, 0, 0);
    d->rootLayout->setSpacing(0);
    
    // Title bar
    d->titleBar = new TitleBarWidget(this);
    d->titleBar->setFixedHeight(40);
    d->rootLayout->addWidget(d->titleBar);
    
    // Main content area
    QWidget* mainContent = new QWidget();
    d->mainLayout = new QHBoxLayout(mainContent);
    d->mainLayout->setContentsMargins(0, 0, 0, 0);
    d->mainLayout->setSpacing(0);
    d->rootLayout->addWidget(mainContent, 1);
    
    // Sidebar
    d->sidebar = new SidebarWidget(this);
    d->sidebar->setObjectName("sidebar");
    d->sidebar->setFixedWidth(200);
    
    d->sidebar->addItem("dashboard", "Dashboard");
    d->sidebar->addItem("stock", "Stock");
    d->sidebar->addItem("futures", "Futures");
    d->sidebar->addItem("portfolio", "Portfolio");
    d->sidebar->addItem("watchlist", "Watchlist");
    d->sidebar->addItem("signal", "Signal");
    d->sidebar->addItem("news", "News");
    d->sidebar->addItem("settings", "Settings");
    d->sidebar->addItem("about", "About");
    
    d->mainLayout->addWidget(d->sidebar);
    
    // Content stack
    d->contentStack = new QStackedWidget(mainContent);
    d->contentStack->setObjectName("contentStack");
    d->mainLayout->addWidget(d->contentStack, 1);
    
    // AI assistant panel
    d->aiPanel = new AIAssistantPanelWidget(mainContent);
    d->aiPanel->setFixedWidth(300);
    d->aiPanel->setObjectName("aiPanel");
    d->mainLayout->addWidget(d->aiPanel);
    
    // Status bar
    d->statusBar = new StatusBarWidget(d->centralWidget);
    d->statusBar->setObjectName("statusBar");
    d->statusBar->setFixedHeight(30);
    d->rootLayout->addWidget(d->statusBar);
    
    // Frameless window
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // Frameless window
}

void MainWindow::createPages()
{
    // Create placeholder pages (lazy loading)
    QStringList pageIds = {
        "stock",
        "futures",
        "portfolio",
        "watchlist",
        "signal",
        "news",
        "settings",
        "about"
    };
    
    for (const QString& id : pageIds) {
        // Create placeholder
        QWidget* placeholder = new QWidget();
        placeholder->setObjectName(id + "_placeholder");
        d->pageCache[id] = nullptr;  // Will be created on demand
        d->contentStack->addWidget(placeholder);
    }
    
    LOG_DEBUG(QString("Created %1 page placeholders").arg(pageIds.size()));
}

void MainWindow::connectSignals()
{
    // Connect ApplicationInitializer signals
    connect(&ApplicationInitializer::instance(), &ApplicationInitializer::initializationComplete,
            this, &MainWindow::onInitializationComplete);
    
    
    // Connect ThemeEngine signals
    connect(&ThemeEngine::instance(), &ThemeEngine::themeChanged,
            this, &MainWindow::onThemeChanged);
}

void MainWindow::adjustLayout()
{
    // Responsive layout adjustment
    int width = this->width();
    
    if (width < 1000) {
        // 小屏幕：隐藏AI面板，折叠侧边栏
        d->aiPanel->hide();
        d->sidebar->setCollapsed(true);
    } else if (width < 1300) {
        // 中等屏幕：显示AI面板，缩小侧边栏
        d->aiPanel->show();
        d->sidebar->setCollapsed(false);
        d->sidebar->setExpandedWidth(150);
    } else {
        // 大屏幕：完整布局
        d->aiPanel->show();
        d->sidebar->setCollapsed(false);
        d->sidebar->setExpandedWidth(200);
    }
}

QWidget* MainWindow::getPage(const QString& pageId)
{
    // Check cache
    if (d->pageCache.contains(pageId) && d->pageCache[pageId]) {
        return d->pageCache[pageId];
    }
    
    // Create new page
    QWidget* page = nullptr;
    if (pageId == "dashboard") {
        page = new DashboardPage(this);
    } else if (pageId == "stock") {
        page = new StockQuotesPage(this);
    } else if (pageId == "futures") {
        page = new FuturesQuotesPage(this);
    }
    // ... other pages
    
    if (page) {
        d->pageCache[pageId] = page;
        d->contentStack->addWidget(page);
        
        LOG_DEBUG(QString("Page loaded: %1").arg(pageId));
    }
    
    return page;
}

bool MainWindow::initializeApplication()
{
    LOG_INFO("Starting application initialization...");
    
    // Initialize application
    if (!ApplicationInitializer::instance().initialize()) {
        return false;
    }
    
    // Register services to ServiceLocator
    // CTP plugin
    auto ctpPlugin = PluginLoader::instance().getPlugin<ICTPPlugin>("CTPPlugin");
    if (ctpPlugin) {
        ServiceLocator::instance().registerInstance<ICTPPlugin>(ctpPlugin);
    }
    
    // AI plugin
    auto aiPlugin = PluginLoader::instance().getPlugin<IAIPlugin>("AIPlugin");
    if (aiPlugin) {
        ServiceLocator::instance().registerInstance<IAIPlugin>(aiPlugin);
    }
    
    LOG_INFO("Application initialized successfully");
    return true;
}

void MainWindow::loadSettings()
{
    QSettings settings;
    
    // Restore window geometry
    QByteArray geometry = settings.value("window/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    
    // Restore window state
    QByteArray state = settings.value("window/state").toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }
    // Restore last active page
    QString lastPage = settings.value("window/lastPage", "dashboard").toString();
    onSidebarItemClicked(lastPage);
    
    LOG_DEBUG("Settings loaded");
}

void MainWindow::saveSettings()
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
    // Apply theme
    m_splashLabel->setGeometry(0, 0, width(), height());
    m_splashLabel->raise();
    m_splashLabel->show();
}

void MainWindow::hideSplashScreen()
{
    if (m_splashLabel) {
        m_splashLabel->hide();
        m_splashLabel->deleteLater();
        m_splashLabel = nullptr;
    }
}



