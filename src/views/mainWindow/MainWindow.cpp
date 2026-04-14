/**
 * @file MainWindow.cpp
 * @brief 主窗口实现 - 重构版本，集成新架构
 */

#include "MainWindow.h"
#include "../../core/ApplicationInitializer.h"
#include "../../core/ServiceLocator.h"
#include "../../core/EnvironmentConfig.h"
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

// PIMPL实现
struct MainWindow::Impl {
    // 布局组件
    QWidget* centralWidget;
    QVBoxLayout* rootLayout;
    QHBoxLayout* mainLayout;
    
    // 标题栏
    TitleBarWidget* titleBar;
    
    // 侧边栏
    SidebarWidget* sidebar;
    QStackedWidget* contentStack;
    
    // AI助理面板
    AIAssistantPanelWidget* aiPanel;
    
    // 状态栏
    StatusBarWidget* statusBar;
    
    // 页面缓存（性能优化）
    QMap<QString, QWidget*> pageCache;
    
    // 当前页面ID
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
    
    // 显示启动画面
    showSplashScreen();
    
    // 初始化应用
    if (!initializeApplication()) {
        LOG_ERROR("Application initialization failed");
        return;
    }
    
    // 设置UI
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
    
    // 隐藏启动画面
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
    
    // 懒加载页面
    QWidget* page = getPage(id);
    if (page) {
        d->contentStack->setCurrentWidget(page);
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
    // 创建中央部件
    d->centralWidget = new QWidget(this);
    setCentralWidget(d->centralWidget);
    
    // 根布局（垂直）
    d->rootLayout = new QVBoxLayout(d->centralWidget);
    d->rootLayout->setContentsMargins(0, 0, 0, 0);
    d->rootLayout->setSpacing(0);
    
    // 标题栏
    d->titleBar = new TitleBarWidget(d->centralWidget);
    d->titleBar->setObjectName("titleBar");
    d->titleBar->setFixedHeight(40);
    d->rootLayout->addWidget(d->titleBar);
    
    // 主内容区域（水平布局）
    QWidget* mainContent = new QWidget(d->centralWidget);
    d->mainLayout = new QHBoxLayout(mainContent);
    d->mainLayout->setContentsMargins(0, 0, 0, 0);
    d->mainLayout->setSpacing(0);
    d->rootLayout->addWidget(mainContent, 1);
    
    // 侧边栏
    d->sidebar = new SidebarWidget(mainContent);
    d->sidebar->setFixedWidth(200);
    d->sidebar->setObjectName("sidebar");
    
    // 添加导航项
    d->sidebar->addItem("dashboard", "首页");
    d->sidebar->addItem("stock", "股票");
    d->sidebar->addItem("futures", "期货");
    d->sidebar->addItem("portfolio", "持仓");
    d->sidebar->addItem("watchlist", "自选");
    d->sidebar->addItem("signal", "信号");
    d->sidebar->addItem("news", "资讯");
    d->sidebar->addItem("settings", "设置");
    d->sidebar->addItem("about", "关于");
    
    d->mainLayout->addWidget(d->sidebar);
    
    // 连接侧边栏点击信号
    connect(d->sidebar, &SidebarWidget::itemClicked, this, &MainWindow::onSidebarItemClicked);
    
    // 内容区域
    d->contentStack = new QStackedWidget(mainContent);
    d->contentStack->setObjectName("contentStack");
    d->mainLayout->addWidget(d->contentStack, 1);
    
    // AI助理面板
    d->aiPanel = new AIAssistantPanelWidget(mainContent);
    d->aiPanel->setFixedWidth(300);
    d->aiPanel->setObjectName("aiPanel");
    d->mainLayout->addWidget(d->aiPanel);
    
    // 状态栏
    d->statusBar = new StatusBarWidget(d->centralWidget);
    d->statusBar->setObjectName("statusBar");
    d->statusBar->setFixedHeight(30);
    d->rootLayout->addWidget(d->statusBar);
    
    // 设置窗口属性
    setWindowTitle("WealthPilot - 智能投资助手");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // 无边框窗口
    setWindowFlags(Qt::FramelessWindowHint);
}

void MainWindow::createPages()
{
    // 创建占位页面（懒加载）
    QStringList pageIds = {
        "dashboard",
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
        // 创建占位符
        QWidget* placeholder = new QWidget();
        placeholder->setObjectName(id + "_placeholder");
        d->pageCache[id] = nullptr;  // 懒加载标记
        d->contentStack->addWidget(placeholder);
    }
    
    LOG_DEBUG(QString("Created %1 page placeholders").arg(pageIds.size()));
}

void MainWindow::connectSignals()
{
    // 连接ApplicationInitializer信号
    connect(&ApplicationInitializer::instance(), &ApplicationInitializer::progressUpdated,
            this, &MainWindow::onInitializationProgress);
    
    connect(&ApplicationInitializer::instance(), &ApplicationInitializer::initializationComplete,
            this, &MainWindow::onInitializationComplete);
    
    // 连接ThemeEngine信号
    connect(&ThemeEngine::instance(), &ThemeEngine::themeChanged,
            this, &MainWindow::onThemeChanged);
}

void MainWindow::adjustLayout()
{
    // 响应式布局调整
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
    // 检查缓存
    if (d->pageCache.contains(pageId) && d->pageCache[pageId]) {
        return d->pageCache[pageId];
    }
    
    // 懒加载页面
    QWidget* page = nullptr;
    
    // 根据页面ID创建对应页面
    if (pageId == "dashboard") {
        page = new DashboardPage(this);
    } else if (pageId == "stock") {
        page = new StockQuotesPage(this);
    } else if (pageId == "futures") {
        page = new FuturesQuotesPage(this);
    }
    // ... 其他页面
    
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
    
    // 初始化应用
    if (!ApplicationInitializer::instance().initialize()) {
        LOG_ERROR("Application initialization failed");
        return false;
    }
    
    // 注册服务到ServiceLocator
    // CTP服务
    auto ctpPlugin = PluginLoader::instance().getPlugin<ICTPPlugin>("CTPPlugin");
    if (ctpPlugin) {
        ServiceLocator::instance().registerInstance<ICTPPlugin>(ctpPlugin);
    }
    
    // AI服务
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
    
    // 加载窗口几何
    QByteArray geometry = settings.value("window/geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    
    // 加载窗口状态
    QByteArray state = settings.value("window/state").toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }
    
    // 加载最后访问的页面
    QString lastPage = settings.value("window/lastPage", "dashboard").toString();
    onSidebarItemClicked(lastPage);
    
    LOG_DEBUG("Settings loaded");
}

void MainWindow::saveSettings()
{
    QSettings settings;
    
    // 保存窗口几何
    settings.setValue("window/geometry", saveGeometry());
    
    // 保存窗口状态
    settings.setValue("window/state", saveState());
    
    // 保存最后访问的页面
    settings.setValue("window/lastPage", d->currentPageId);
    
    settings.sync();
    
    LOG_DEBUG("Settings saved");
}

void MainWindow::applyTheme()
{
    // 应用主题引擎的预编译样式表
    // 暂时注释掉
    // QString styleSheet = ThemeEngine::instance().compiledStyleSheet();
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
    m_splashLabel->setText("正在初始化...");
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
