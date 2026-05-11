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

#include <QObject>

// 前向声明
class QMainWindow;

namespace WealthPilot
{
/**
 * @brief 功能集成管理器
 *
 * 提供统一的初始化和管理接口
 */
class FeatureIntegration : public QObject
{
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
