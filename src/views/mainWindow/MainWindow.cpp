/**
 * @file MainWindow.cpp
 * @brief 主窗口实现
 *
 * @details 三栏布局：
 * - 左侧：导航侧边栏（可折叠）
 * - 中间：内容区域（页面堆栈）
 * - 右侧：AI助理面板（可隐藏）
 */

#include "MainWindow.h"
#include "../../core/Tokens.h"
#include "utils/Logger.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QCloseEvent>

#include <core/ConfigManager.h>
#include <core/IconProvider.h>
#include <core/PageFactoryRegistry.h>
#include <core/PageNavigatorManager.h>
#include <core/ThemeManager.h>
#include <views/dashboard/Dashboard.h>
#include <views/widgets/SidebarWidget.h>

using namespace Tokens;

struct MainWindow::Impl {
    SidebarWidget* sidebar = nullptr;
    QStackedWidget* contentStack = nullptr;
    QWidget* centralWidget = nullptr;
    QLabel* statusLabel = nullptr;
    bool aiPanelVisible = true;

};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , d(std::make_unique<Impl>())
{
    setWindowTitle("WealthPilot - 智能投资管理");
    setMinimumSize(Size::WindowMinWidth, Size::WindowMinHeight);
    resize(Size::WindowDefaultWidth, Size::WindowDefaultHeight);

    // 严格按照依赖顺序初始化
    setupUI();          // 1. 构建基础UI框架
    createPages();      // 2. 注册页面到工厂（必须先于导航）
    connectSignals();   // 3. 连接信号槽

}

MainWindow::~MainWindow() = default;

/**
 * 构建UI界面
 * @brief MainWindow::setupUI
 */
void MainWindow::setupUI()
{
    d->centralWidget = new QWidget(this);
    setCentralWidget(d->centralWidget);

    // 主布局：侧边栏 + 内容区（水平排列）
    QHBoxLayout* mainLayout = new QHBoxLayout(d->centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. 左侧导航栏
    d->sidebar = new SidebarWidget(this);
    d->sidebar->setFixedWidth(Size::SidebarExpanded);
    mainLayout->addWidget(d->sidebar);

    // 2. 中间内容区
    QWidget* contentArea = new QWidget(this);
    mainLayout->addWidget(contentArea, 1);  // 拉伸因子1占据剩余空间

    QVBoxLayout* contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // 页面堆栈容器
    d->contentStack = new QStackedWidget(contentArea);
    contentLayout->addWidget(d->contentStack);

    // 初始化导航器（绑定到UI容器）
    PageNavigatorManager::instance()->initialize(d->contentStack);


    // // AI 助理面板
    // d->aiPanel = new AIAssistantPanel(this);
    // d->aiPanel->setFixedWidth(Size::AIPanelWidth);
    // mainLayout->addWidget(d->aiPanel);

}

/**
 * @brief 注册所有页面
 */
void MainWindow::createPages()
{

    auto *registry = PageFactoryRegistry::instance();
    auto *navigator = PageNavigatorManager::instance();


    // 注册页面（模板方法自动推导类型）
    registry->registerPage<Dashboard>(QStringLiteral("dashboard"), tr("全局概览"), true); // 可缓存


    // 配置缓存策略（与注册分离，支持运行时动态调整）
    navigator->registerCachePolicy(QStringLiteral("dashboard"), CachePolicy::StrongCache);

    // 预加载首页（提升感知性能）
    navigator->preloadPage(QStringLiteral("dashboard"));

    // 预加载首页（提升感知性能）
    navigator->preloadPage(QStringLiteral("dashboard"));

    // 添加侧边栏导航项（与页面ID绑定）
    d->sidebar->addItem("dashboard","全局",QIcon(":/icons/dashboard.svg"));

    // 【关键修复】所有页面准备就绪后，执行默认导航
    // 必须在register之后调用，否则报"not registered in factory"
    QTimer::singleShot(0, this, [navigator]() {
        navigator->navigateTo(QStringLiteral("dashboard"));
    });



    // 添加侧栏

    // registerPage("dashboard", [](QWidget* parent) -> QWidget* {
    //     return new Dasha(parent);
    // }, "看板", ":/icons/dashboard.svg");
    // registerPage("watchlist", [](QWidget* parent) -> QWidget* {
    //     return new QWidget(parent);
    // }, "自选", ":/icons/watchlist.svg");
    // registerPage("stock_quotes", [](QWidget* parent) -> QWidget* {
    //     return new QWidget(parent);
    // }, "股票", ":/icons/candlestick-chart.svg");
    // registerPage("futures_quotes", [](QWidget* parent) -> QWidget* {
    //     return new QWidget(parent);
    // }, "期货", ":/icons/line-chart.svg");
    // registerPage("signals", [](QWidget* parent) -> QWidget* {
    //     return new QWidget(parent);
    // }, "订阅", ":/icons/subscribe.svg");
    // registerPage("news", [](QWidget* parent) -> QWidget* {
    //     return new QWidget(parent);
    // }, "资讯", ":/icons/news.svg");
    // registerPage("portfolio", [](QWidget* parent) -> QWidget* {
    //     return new QWidget(parent);
    // }, "持仓", ":/icons/portfolio.svg");
    // registerPage("warning", [](QWidget* parent) -> QWidget* {
    //     return new QWidget(parent);
    // }, "预警", ":/icons/alert.svg");
    // registerPage("settings", [](QWidget* parent) -> QWidget* {
    //     return new QWidget(parent);
    // }, "设置", ":/icons/settings.svg");

    LOG_INFO(QStringLiteral("MainWindow: Pages registered successfully"));
}

/**
 * @brief 连接信号槽
 */
void MainWindow::connectSignals()
{
    // 侧边栏导航 - 点击时切换页面
    connect(d->sidebar, &SidebarWidget::itemClicked,
            this, [](const QString &pageId) {
                PageNavigatorManager::instance()->navigateTo(pageId);
            });

    // 主题变更响应
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MainWindow::onThemeChanged);

    // 导航状态监听（可选：用于调试或状态栏显示）
    auto *navigator = PageNavigatorManager::instance();
    connect(navigator, &PageNavigatorManager::pageChanged,
            this, [](const QString &pageId, const QVariantMap &) {
                LOG_INFO(QStringLiteral("Navigated to: %1").arg(pageId));
            });

    LOG_INFO(QStringLiteral("MainWindow: Signals connected"));
};

/**
 * @brief 调整布局
 * @details 根据当前窗口大小重新计算各区域尺寸
 */
void MainWindow::adjustLayout()
{
    int width = this->width();

    if (width < Breakpoint::LG && d->aiPanelVisible) {
        // hideAIPanel();
    } else if (width >= Breakpoint::XL && !d->aiPanelVisible) {
        // showAIPanel();
    }
};


// 窗口大小变化事件
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    // adjustLayout();
}

// 窗口关闭事件
void MainWindow::closeEvent(QCloseEvent *event)
{
    LOG_INFO("Application closing...");

    // 保存窗口状态
    ConfigManager::instance()->set("window/geometry", saveGeometry().toBase64());
    ConfigManager::instance()->set("window/state", saveState().toBase64());

    // 断开 CTP
    // CTPService::instance()->disconnect();

    event->accept();
}

// 侧栏点击事件
void MainWindow::onSidebarItemClicked(const QString& id)
{
    PageNavigatorManager::instance()->navigateTo(id);
}

// 触发切换主题事件
void MainWindow::onThemeChanged()
{
    ThemeManager::instance()->applyCurrentTheme();
}
