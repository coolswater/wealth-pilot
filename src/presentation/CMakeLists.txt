# ==================== Presentation 模块 CMakeLists.txt ====================
# 表现层模块：UI组件、视图、控制器、视图模型

add_library(Presentation STATIC)

# ==================== Components UI组件 ====================
target_sources(Presentation PRIVATE
        components/UIComponents.h
        components/ThemeColors.h
        components/PageStyles.h
        components/ChartStyles.h
        components/LayoutConstants.h
        components/BaseChartWidget.h
        components/BaseChartWidget.cpp
        components/ComponentFactory.h
        components/ComponentFactory.cpp
        components/ChartConfig.h
        components/ChartConfig.cpp
        components/KLineChart.h
        components/EquityCurveWidget.cpp
        components/DrawingTool.cpp
        components/TimeShareChart.cpp
        components/KLineChart.cpp
        components/ChartToolBar.h
        components/ChartToolBar.cpp
        components/MarketDepthWidget.h
        components/MarketDepthWidget.cpp
        components/TickTableView.h
        components/TickTableView.cpp
        components/ChartStatusBar.h
        components/ChartStatusBar.cpp
        components/BaseWidget.h
        components/BaseWidget.cpp
        components/BasePage.h
        components/BasePage.cpp
        components/DataHubPageBase.h
        components/DataHubPageBase.cpp
        components/PageTemplate.h
        components/PageTemplate.cpp
        components/SidebarWidget.h
        components/SidebarWidget.cpp
        components/SvgIconEngine.h
        components/SvgIconEngine.cpp
        components/SvgIconWidget.h
        components/SvgIconWidget.cpp
        components/TitleBarWidget.h
        components/TitleBarWidget.cpp
        components/ThemeToggleButton.h
        components/ThemeToggleButton.cpp
        components/CardWidget.h
        components/CardWidget.cpp
        components/StatusBarWidget.h
        components/StatusBarWidget.cpp
        components/ToastWidget.h
        components/ToastWidget.cpp
        components/AssetCardWidget.h
        components/AssetCardWidget.cpp
        components/AssetPieChart.h
        components/AssetPieChart.cpp
        components/AIAssistantPanelWidget.cpp
        components/AIAssistantPanelWidget.h
        components/NetworkIndicator.cpp
        components/NetworkIndicator.h
        components/TreeMapWidget.cpp
        components/TreeMapWidget.h
        components/AddSymbolDialog.cpp
        components/AddSymbolDialog.h
        components/DividerWidget.h
        components/DividerWidget.cpp
        components/OrderDialog.h
        components/OrderDialog.cpp
        components/SignalMarker.h
        components/SignalMarker.cpp
        components/SignalDetailPanel.h
        components/SignalDetailPanel.cpp
        components/KLineChartWithSignals.h
        components/KLineChartWithSignals.cpp
        components/StockInfoPanel.h
        components/StockInfoPanel.cpp
        components/RiskIndicatorWidget.h
        components/RiskIndicatorWidget.cpp
        components/RecommendationListWidget.h
        components/RecommendationListWidget.cpp
        components/PortfolioOptimizationDialog.h
        components/PortfolioOptimizationDialog.cpp
        components/NewsPanelWidget.h
        components/NewsPanelWidget.cpp
        components/AlertSettingDialog.h
        components/AlertSettingDialog.cpp
        components/LoadingOverlay.h
        components/LoadingOverlay.cpp
        components/QmlKLineWidget.h
        components/QmlKLineWidget.cpp
        components/BacktestChartWidget.h
        components/BacktestChartWidget.cpp
        components/BacktestReportWidget.h
        components/BacktestReportWidget.cpp
)

# ==================== Views 视图 ====================
target_sources(Presentation PRIVATE
        views/mainWindow/MainWindow.cpp
        views/mainWindow/MainWindow.h
        views/dashboard/DashboardPage.h
        views/dashboard/DashboardPage.cpp
        views/watchList/WatchListPage.h
        views/watchList/WatchListPage.cpp
        views/signalCenter/SignalCenterPage.h
        views/signalCenter/SignalCenterPage.cpp
        views/news/NewsPage.h
        views/news/NewsPage.cpp
        views/portfolio/PortfolioPage.h
        views/portfolio/PortfolioPage.cpp
        views/futures/FuturesQuotesPage.h
        views/futures/FuturesQuotesPage.cpp
        views/futures/FuturesKLinePage.h
        views/futures/FuturesKLinePage.cpp
        views/futures/FuturesPageIntegration.h
        views/futures/FuturesPageIntegration.cpp
        views/futures/FuturesPagesRegistration.cpp
        views/quotes/QuotesPage.h
        views/quotes/QuotesPage.cpp
        views/stock/StockQuotesPage.h
        views/stock/StockQuotesPage.cpp
        views/stock/StockKLinePage.h
        views/stock/StockKLinePage.cpp
        views/feedback/FeedbackDialog.h
        views/feedback/FeedbackDialog.cpp
        views/settings/SettingsPage.h
        views/settings/SettingsPage.cpp
        views/settings/CTPBrokerDialog.h
        views/settings/CTPBrokerDialog.cpp
        views/settings/RiskSettingsPage.h
        views/settings/RiskSettingsPage.cpp
        views/warning/WarningPage.h
        views/warning/WarningPage.cpp
        views/aboutus/AboutUSPage.h
        views/aboutus/AboutUSPage.cpp
        views/account/AccountPage.h
        views/account/AccountPage.cpp
        views/trading/TradeHistoryPage.h
        views/trading/TradeHistoryPage.cpp
        views/trading/ConditionOrderPage.h
        views/trading/ConditionOrderPage.cpp
        views/trading/TradingIntegration.h
        views/trading/TradingIntegration.cpp
        views/trading/TradingPanel.h
        views/trading/TradingPanel.cpp
        views/trading/TradingQmlPage.h
        views/trading/TradingQmlPage.cpp
        views/TradingPagesRegistration.cpp
        views/fund/FundPage.h
        views/fund/FundPage.cpp
        views/forex/ForexPage.h
        views/forex/ForexPage.cpp
        views/crypto/CryptoPage.h
        views/crypto/CryptoPage.cpp
        views/backtest/BacktestPage.h
        views/backtest/BacktestPage.cpp
        views/alert/AlertCenterPage.h
        views/alert/AlertCenterPage.cpp
        views/demo/QmlChartDemoPage.h
        views/demo/QmlChartDemoPage.cpp
)

# ==================== ViewModels MVVM ====================
target_sources(Presentation PRIVATE
        viewmodels/ViewModelBase.h
        viewmodels/ViewModelBase.cpp
        viewmodels/TradingViewModel.h
        viewmodels/TradingViewModel.cpp
        viewmodels/OrderViewModel.h
        viewmodels/OrderViewModel.cpp
        viewmodels/BacktestViewModel.h
        viewmodels/BacktestViewModel.cpp
        viewmodels/ViewModelRegistration.h
)

# ==================== Controllers MVP ====================
target_sources(Presentation PRIVATE
        controllers/ControllerBase.h
        controllers/ControllerBase.cpp
)

# ==================== Styles 样式 ====================
target_sources(Presentation PRIVATE
        styles/ThemeManager.h
        styles/ThemeManager.cpp
        styles/ButtonStyles.h
        styles/ButtonStyles.cpp
)

# ==================== Animation 动画 ====================
target_sources(Presentation PRIVATE
        animation/AnimationManager.h
        animation/AnimationManager.cpp
)

# ==================== Chart 图表工具 ====================
target_sources(Presentation PRIVATE
        chart/DrawingToolManager.h
        chart/DrawingToolManager.cpp
)

# ==================== Shortcuts 快捷键 ====================
target_sources(Presentation PRIVATE
        shortcuts/ShortcutManager.h
        shortcuts/ShortcutManager.cpp
)

# ==================== Layout 布局 ====================
target_sources(Presentation PRIVATE
        layout/LayoutManager.h
        layout/LayoutManager.cpp
)

# ==================== Delegates 代理 ====================
target_sources(Presentation PRIVATE
        delegates/ColorDelegates.h
        delegates/DelegateHelper.h
)

# ==================== Utils 工具 ====================
target_sources(Presentation PRIVATE
        utils/StyleGenerator.h
        utils/StyleGenerator.cpp
        utils/PageStyleHelper.h
        utils/PageStyleHelper.cpp
)

# ==================== QML ====================
target_sources(Presentation PRIVATE
        qml/QmlDataBridge.h
        qml/QmlDataBridge.cpp
)

# ==================== 包含目录 ====================
target_include_directories(Presentation PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

# ==================== 链接库 ====================
target_link_libraries(Presentation PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        Qt6::Charts
        Qt6::Svg
        Qt6::Quick
        Qt6::QuickWidgets
        Qt6::Qml
        Qt6::Network
        Qt6::Sql
        WealthPilot::Shared
        WealthPilot::Core
        WealthPilot::Data
)

# ==================== 编译选项 ====================
if (MSVC)
    target_compile_options(Presentation PRIVATE /W4)
else ()
    target_compile_options(Presentation PRIVATE -Wall -Wextra -Wpedantic)
endif ()

# 添加别名，允许其他模块使用 WealthPilot::Presentation
add_library(WealthPilot::Presentation ALIAS Presentation)
