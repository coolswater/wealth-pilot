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
#include "core/Tokens.h"
#include "utils/Logger.h"
#include "views/dashboard/DashboardPage.h"
#include "views/signalCenter/SignalCenterPage.h"
#include "views/watchList/WatchListPage.h"

#include <QHBoxLayout>
#include <QLabel>

#include <core/ConfigManager.h>
#include <core/PageFactoryRegistry.h>
#include <core/PageNavigatorManager.h>
#include <core/ThemeManager.h>
#include <views/aboutus/AboutUSPage.h>
#include <views/futures/FuturesQuotesPage.h>
#include <views/news/NewsPage.h>
#include <views/portfolio/PortfolioPage.h>
#include <views/settings/SettingsPage.h>
#include <views/stock/StockQuotesPage.h>
#include <views/warning/WarningPage.h>
#include <views/widgets/DividerWidget.h>
#include <views/widgets/SidebarWidget.h>
#include <views/widgets/StatusBarWidget.h>
#include <views/widgets/TitleBarWidget.h>

#include "views/widgets/AIAssistantPanelWidget.h"

struct MainWindow::Impl
{
    QHBoxLayout* contentLayout = nullptr; // 内容布局
    TitleBarWidget* m_titleBarWidget{}; // 自定义标题栏实例
    StatusBarWidget* m_statusBarWidget{}; // 自定义状态栏实例
    AIAssistantPanelWidget* aiPanel{};    // AI面板

    SidebarWidget* sidebar = nullptr;
    QStackedWidget* contentStack = nullptr;
    QWidget* centralWidget = nullptr;
    bool aiPanelVisible = true;
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , d(std::make_unique<Impl>())
{
    setWindowTitle("WealthPilot - 财富领航AI助手");
    setMinimumSize(1280, 800);
    resize(1600, 900);

    // 设置窗口无边框属性（关键标志）
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
        Qt::WindowMinimizeButtonHint |
        Qt::WindowMaximizeButtonHint |
        Qt::WindowCloseButtonHint);

    // 严格按照依赖顺序初始化
    setupUI(); // 1. 构建基础UI框架
    createPages(); // 2. 注册页面到工厂（必须先于导航）
    connectSignals(); // 3. 连接信号槽
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

    // 主布局： 上中下布局
    auto* mainLayout = new QVBoxLayout(d->centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 标题栏
    d->m_titleBarWidget = new TitleBarWidget(this);
    mainLayout->addWidget(d->m_titleBarWidget);

    // 风格线
    DividerWidget* titleDivider =
        DividerWidget::createHorizontal(d->m_titleBarWidget, Tokens::Colors::BgElevated, 1, 0);
    mainLayout->addWidget(titleDivider);

    // 内容布局
    d->contentLayout = new QHBoxLayout();
    mainLayout->addLayout(d->contentLayout);
    d->contentLayout->setContentsMargins(0, 0, 0, 0);
    d->contentLayout->setSpacing(0);

    // 左侧导航栏
    d->sidebar = new SidebarWidget(this);
    d->sidebar->setFixedWidth(80);
    d->contentLayout->addWidget(d->sidebar);
    DividerWidget* contentDivider = DividerWidget::createVertical(d->sidebar, Tokens::Colors::BgElevated, 1, 0);
    d->contentLayout->addWidget(contentDivider); // 使用便捷方法

    // 页面堆栈容器
    d->contentStack = new QStackedWidget(this);
    d->contentLayout->addWidget(d->contentStack);

    // 分割线
    DividerWidget* aiDivider = DividerWidget::createVertical(d->sidebar, Tokens::Colors::BgElevated, 1, 0);
    d->contentLayout->addWidget(aiDivider); // 使用便捷方法

    // AI 助理面板
    d->aiPanel = new AIAssistantPanelWidget(this);
    d->aiPanel->setFixedWidth(Tokens::Size::AIPanelWidth);
    d->contentLayout->addWidget(d->aiPanel);

    // 底部状态栏布局
    d->m_statusBarWidget = new StatusBarWidget(this);
    DividerWidget* statusBarDivider = DividerWidget::createHorizontal(d->m_statusBarWidget, Tokens::Colors::BgElevated,
                                                                      1, 0);
    mainLayout->addWidget(statusBarDivider);
    mainLayout->addWidget(d->m_statusBarWidget);

    // 初始化导航器（绑定到UI容器）
    PageNavigatorManager::instance()->initialize(d->contentStack);
}

/**
 * @brief 注册所有页面
 */
void MainWindow::createPages() const
{
    auto* registry = PageFactoryRegistry::instance();
    auto* navigator = PageNavigatorManager::instance();

    // 全局
    registry->registerPage<DashboardPage>(QStringLiteral("DashboardPage"), tr("全局"), true);
    d->sidebar->addItem("DashboardPage", "全局");
    d->sidebar->setCurrentItem("DashboardPage");

    // 自选
    registry->registerPage<WatchListPage>(QStringLiteral("WatchListPage"));
    d->sidebar->addItem("WatchListPage", "自选");

    // 股票
    registry->registerPage<StockQuotesPage>(QStringLiteral("StockQuotesPage"));
    d->sidebar->addItem("StockQuotesPage", "股票");

    // 期货
    registry->registerPage<FuturesQuotesPage>(QStringLiteral("FuturesQuotesPage"));
    d->sidebar->addItem("FuturesQuotesPage", "期货");

    // TODO: 外汇
    // TODO: 基金
    // TODO: 数字货币

    // 订阅
    registry->registerPage<SignalCenterPage>(QStringLiteral("SignalCenterPage"));
    d->sidebar->addItem("SignalCenterPage", "订阅");


    // 资讯
    registry->registerPage<NewsPage>(QStringLiteral("NewsPage"));
    d->sidebar->addItem("NewsPage", "资讯");

    // 持仓
    registry->registerPage<PortfolioPage>(QStringLiteral("PortfolioPage"));
    d->sidebar->addItem("PortfolioPage", "持仓");

    // 预警
    registry->registerPage<WarningPage>(QStringLiteral("WarningPage"));
    d->sidebar->addItem("WarningPage", "预警");


    // // 通知
    // registry->registerPage<PortfolioPage>(QStringLiteral("PortfolioPage"));
    // d->sidebar->addItem("PortfolioPage","通知");

    // 设置
    registry->registerPage<SettingsPage>(QStringLiteral("SettingsPage"));
    d->sidebar->addItem("SettingsPage", "设置");

    // 关于
    registry->registerPage<AboutUSPage>(QStringLiteral("AboutUSPage"));
    d->sidebar->addItem("AboutUSPage", "关于");

    // 所有页面准备就绪后，执行默认导航
    // 必须在register之后调用，否则报"not registered in factory"
    QTimer::singleShot(0, this, [navigator]()
    {
        navigator->navigateTo(QStringLiteral("DashboardPage"));
    });
}

/**
 * @brief 连接信号槽
 */
void MainWindow::connectSignals()
{
    // 侧边栏导航 - 点击时切换页面
    connect(d->sidebar, &SidebarWidget::itemClicked,
            this, [](const QString& pageId)
            {
                PageNavigatorManager::instance()->navigateTo(pageId);
            });

    // 导航状态监听（可选：用于调试或状态栏显示）
    auto* navigator = PageNavigatorManager::instance();
    connect(navigator, &PageNavigatorManager::pageChanged,
            this, [](const QString& pageId, const QVariantMap&)
            {
                LOG_INFO(QStringLiteral("Navigated to: %1").arg(pageId));
            });
};

/**
 * @brief 调整布局
 * @details 根据当前窗口大小重新计算各区域尺寸
 */
void MainWindow::adjustLayout() const
{
    int width = this->width();

    if (width < Tokens::Breakpoint::LG && d->aiPanelVisible)
    {
        // hideAIPanel();
    }
    else if (width >= Tokens::Breakpoint::XL && !d->aiPanelVisible)
    {
        // showAIPanel();
    }
};


// 窗口大小变化事件
void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    adjustLayout();
}

// 窗口关闭事件
void MainWindow::closeEvent(QCloseEvent* event)
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
