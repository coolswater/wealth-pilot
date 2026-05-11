/**
 * @file ApplicationBootstrap.h
 * @brief 应用启动引导 - 统一的初始化流程
 *
 * @details 提供分阶段的初始化流程：
 * - Phase 1: 基础服务（日志、配置、缓存）
 * - Phase 2: 数据层（数据库、数据源）
 * - Phase 3: 业务层（分析、交易、风控）
 * - Phase 4: UI层（窗口、组件）
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef APPLICATIONBOOTSTRAP_H
#define APPLICATIONBOOTSTRAP_H

#include <QObject>
#include <QHash>
#include <functional>

class QMainWindow;

namespace WealthPilot {

/**
 * @brief 初始化阶段
 */
enum class InitPhase {
    None,           ///< 未初始化
    CoreServices,   ///< 基础服务
    DataLayer,      ///< 数据层
    BusinessLayer,  ///< 业务层
    UILayer,        ///< UI层
    Complete        ///< 完成
};

/**
 * @brief 初始化结果
 */
struct InitResult {
    bool success = false;
    QString phase;
    QString error;
    qint64 elapsedMs = 0;
};

/**
 * @brief 应用启动引导
 *
 * 提供统一的启动流程管理：
 * - 分阶段初始化
 * - 依赖管理
 * - 错误处理
 * - 进度回调
 */
class ApplicationBootstrap : public QObject {
    Q_OBJECT

public:
    static ApplicationBootstrap* instance();

    /**
     * @brief 执行完整初始化
     * @param mainWindow 主窗口
     * @return 初始化结果
     */
    InitResult initialize(QMainWindow* mainWindow);

    /**
     * @brief 执行指定阶段初始化
     */
    InitResult initializePhase(InitPhase phase);

    /**
     * @brief 获取当前阶段
     */
    InitPhase currentPhase() const { return m_currentPhase; }

    /**
     * @brief 注册阶段回调
     */
    void registerPhaseCallback(InitPhase phase, std::function<InitResult()> callback);

    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取启动报告
     */
    QString getStartupReport() const;

signals:
    /**
     * @brief 阶段开始信号
     */
    void phaseStarted(InitPhase phase);

    /**
     * @brief 阶段完成信号
     */
    void phaseCompleted(InitPhase phase, const InitResult& result);

    /**
     * @brief 进度更新信号
     */
    void progressChanged(int current, int total, const QString& description);

private:
    explicit ApplicationBootstrap(QObject* parent = nullptr);
    ~ApplicationBootstrap() override = default;

    InitResult initializeCoreServices();
    InitResult initializeDataLayer();
    InitResult initializeBusinessLayer();
    InitResult initializeUILayer();

    QMainWindow* m_mainWindow = nullptr;
    InitPhase m_currentPhase = InitPhase::None;
    bool m_initialized = false;

    QHash<InitPhase, std::function<InitResult()>> m_phaseCallbacks;
    QVector<InitResult> m_phaseResults;
};

} // namespace WealthPilot

#endif // APPLICATIONBOOTSTRAP_H
