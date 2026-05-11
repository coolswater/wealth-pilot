/**
 * @file FeatureIntegration.h
 * @brief 新功能集成入口
 *
 * @details 提供所有新开发功能的统一入口：
 * - 短期规划功能
 * - 中期规划功能
 * - 长期规划功能
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef FEATUREINTEGRATION_H
#define FEATUREINTEGRATION_H

// ========== 短期规划功能 ==========

// 网络模块 - WebSocket 断线重连
#include "core/network/WebSocketManager.h"

// UI管理 - 快捷键系统
#include "core/ui/ShortcutManager.h"

// 布局管理 - 使用原有模块
#include "core/layout/LayoutManager.h"

// 分析模块 - 股票筛选器
#include "core/analysis/StockScreener.h"

// 回测模块 - 策略回测引擎
#include "core/backtest/BacktestEngine.h"

// 分析模块 - 风险分析器
#include "core/analysis/RiskAnalyzer.h"

// 分析模块 - 示例策略
#include "core/analysis/ExampleStrategies.h"

// ========== 中期规划功能 ==========

// 社交交易 - 策略分享管理器
#include "core/social/StrategyShareManager.h"

// 图表工具 - 画线工具管理器
#include "core/chart/DrawingToolManager.h"

// 量化交易 - 量化交易引擎
#include "core/quant/QuantTradingEngine.h"

// ========== 长期规划功能 ==========

// AI智能助手
#include "core/ai/AIAssistant.h"

// 多账户管理
#include "core/account/MultiAccountManager.h"

// 权限管理
#include "core/security/PermissionManager.h"

// 数据API
#include "core/api/DataAPIManager.h"

// 插件市场
#include "core/plugin/PluginMarketManager.h"

// ========== 性能管理 ==========

#include "core/performance/PerformanceManager.h"

#include <QObject>
#include <QMainWindow>

namespace WealthPilot {

/**
 * @brief 功能集成管理器
 *
 * 提供统一的初始化和管理接口
 */
class FeatureIntegration : public QObject {
    Q_OBJECT

public:
    static FeatureIntegration* instance();

    /**
     * @brief 初始化所有新功能
     * @param mainWindow 主窗口
     */
    void initialize(QMainWindow* mainWindow);

    /**
     * @brief 初始化短期规划功能
     */
    void initializeShortTermFeatures();

    /**
     * @brief 初始化中期规划功能
     */
    void initializeMidTermFeatures();

    /**
     * @brief 初始化长期规划功能
     */
    void initializeLongTermFeatures();

    /**
     * @brief 注册默认快捷键
     */
    void registerDefaultShortcuts();

    /**
     * @brief 启动性能监控
     */
    void startPerformanceMonitoring();

    /**
     * @brief 获取功能状态报告
     */
    QString getStatusReport() const;

private:
    explicit FeatureIntegration(QObject* parent = nullptr);
    ~FeatureIntegration() override = default;

    QMainWindow* m_mainWindow = nullptr;
    bool m_initialized = false;
};

} // namespace WealthPilot

#endif // FEATUREINTEGRATION_H
