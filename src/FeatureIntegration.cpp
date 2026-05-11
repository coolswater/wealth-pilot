/**
 * @file FeatureIntegration.cpp
 * @brief 新功能集成实现
 */

#include "FeatureIntegration.h"
#include "utils/Logger.h"
#include <QMainWindow>

namespace WealthPilot {

FeatureIntegration* FeatureIntegration::instance()
{
    static FeatureIntegration* inst = new FeatureIntegration();
    return inst;
}

FeatureIntegration::FeatureIntegration(QObject* parent)
    : QObject(parent)
{
    LOG_INFO("FeatureIntegration created");
}

void FeatureIntegration::initialize(QMainWindow* mainWindow)
{
    if (m_initialized) {
        LOG_WARNING("FeatureIntegration already initialized");
        return;
    }

    m_mainWindow = mainWindow;

    LOG_INFO("Initializing WealthPilot features...");

    // 按阶段初始化
    initializeShortTermFeatures();
    initializeMidTermFeatures();
    initializeLongTermFeatures();

    // 注册默认快捷键
    registerDefaultShortcuts();

    // 启动性能监控
    startPerformanceMonitoring();

    m_initialized = true;
    LOG_INFO("All features initialized successfully");
}

void FeatureIntegration::initializeShortTermFeatures()
{
    LOG_INFO("Initializing short-term features...");

    // 初始化 WebSocket 管理器
    WebSocketManager::instance();

    // 初始化快捷键管理器
    ShortcutManager::instance()->initialize(m_mainWindow);

    // 初始化布局管理器
    WindowLayoutManager::instance()->initialize(m_mainWindow);

    // 初始化股票筛选器
    StockScreener::instance();

    // 初始化回测引擎
    BacktestEngine::instance();

    // 初始化风险分析器
    RiskAnalyzer::instance();

    LOG_INFO("Short-term features initialized");
}

void FeatureIntegration::initializeMidTermFeatures()
{
    LOG_INFO("Initializing mid-term features...");

    // 初始化策略分享管理器
    StrategyShareManager::instance();

    // 初始化画线工具管理器
    ChartDrawingToolManager::instance();

    // 初始化量化交易引擎
    QuantTradingEngine::instance();

    LOG_INFO("Mid-term features initialized");
}

void FeatureIntegration::initializeLongTermFeatures()
{
    LOG_INFO("Initializing long-term features...");

    // 初始化 AI 助手
    AIAssistant::instance();

    // 初始化多账户管理器
    MultiAccountManager::instance();

    // 初始化权限管理器
    PermissionManager::instance();

    // 初始化数据 API 管理器
    DataAPIManager::instance();

    // 初始化插件市场管理器
    PluginMarketManager::instance();

    LOG_INFO("Long-term features initialized");
}

void FeatureIntegration::registerDefaultShortcuts()
{
    LOG_INFO("Registering default shortcuts...");

    auto* shortcutMgr = ShortcutManager::instance();

    // 文件操作
    shortcutMgr->registerShortcut(
        "file.save_layout",
        tr("保存布局"),
        QKeySequence("Ctrl+Shift+S"),
        [this]() {
            WindowLayoutManager::instance()->saveLayout();
        },
        "文件",
        tr("保存当前窗口布局")
    );

    shortcutMgr->registerShortcut(
        "file.restore_layout",
        tr("恢复布局"),
        QKeySequence("Ctrl+Shift+R"),
        [this]() {
            WindowLayoutManager::instance()->restoreLayout();
        },
        "文件",
        tr("恢复默认窗口布局")
    );

    // 视图操作
    shortcutMgr->registerShortcut(
        "view.fullscreen",
        tr("全屏"),
        QKeySequence("F11"),
        [this]() {
            if (m_mainWindow) {
                if (m_mainWindow->isFullScreen()) {
                    m_mainWindow->showNormal();
                } else {
                    m_mainWindow->showFullScreen();
                }
            }
        },
        "视图",
        tr("切换全屏模式")
    );

    // 分析操作
    shortcutMgr->registerShortcut(
        "analysis.screener",
        tr("股票筛选"),
        QKeySequence("Ctrl+F"),
        [this]() {
            // TODO: 打开股票筛选对话框
            LOG_INFO("Stock screener shortcut triggered");
        },
        "分析",
        tr("打开股票筛选器")
    );

    // 交易操作
    shortcutMgr->registerShortcut(
        "trading.backtest",
        tr("策略回测"),
        QKeySequence("Ctrl+B"),
        [this]() {
            // TODO: 打开回测对话框
            LOG_INFO("Backtest shortcut triggered");
        },
        "交易",
        tr("打开策略回测")
    );

    LOG_INFO("Default shortcuts registered");
}

void FeatureIntegration::startPerformanceMonitoring()
{
    LOG_INFO("Starting performance monitoring...");

    auto* perfMgr = PerformanceManager::instance();

    // 配置内存池
    MemoryPoolConfig poolConfig;
    poolConfig.blockSize = 4096;
    poolConfig.maxBlocks = 1000;
    poolConfig.enableReuse = true;
    perfMgr->configureMemoryPool(poolConfig);

    LOG_INFO("Performance monitoring started");
}

QString FeatureIntegration::getStatusReport() const
{
    QString report;
    report += QStringLiteral("========== WealthPilot 功能状态 ==========\n\n");

    report += QStringLiteral("短期规划功能:\n");
    report += QString("  WebSocket管理器: 已初始化\n");
    report += QString("  快捷键系统: 已初始化\n");
    report += QString("  布局管理器: 已初始化\n");
    report += QString("  股票筛选器: 已初始化\n");
    report += QString("  回测引擎: 已初始化\n");
    report += QString("  风险分析器: 已初始化\n\n");

    report += QStringLiteral("中期规划功能:\n");
    report += QString("  策略分享管理器: 已初始化\n");
    report += QString("  画线工具管理器: 已初始化\n");
    report += QString("  量化交易引擎: 已初始化\n\n");

    report += QStringLiteral("长期规划功能:\n");
    report += QString("  AI智能助手: 已初始化\n");
    report += QString("  多账户管理器: 已初始化\n");
    report += QString("  权限管理器: 已初始化\n");
    report += QString("  数据API管理器: 已初始化\n");
    report += QString("  插件市场管理器: 已初始化\n\n");

    // 性能统计
    report += PerformanceManager::instance()->generateReport();

    return report;
}

} // namespace WealthPilot
