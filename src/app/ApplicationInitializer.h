/**
 * @file ApplicationInitializer.h
 * @brief 应用初始化管理器 - 统一管理应用启动和初始化流程
 *
 * @details 功能：
 * - 统一初始化所有核心模块
 * - 管理模块依赖关系
 * - 性能优化：并行初始化
 * - 错误处理和回滚
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef APPLICATIONINITIALIZER_H
#define APPLICATIONINITIALIZER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <functional>

/**
 * @brief 初始化阶段
 */
enum class InitPhase {
    Core,           // 核心模块
    Services,       // 服务层
    Plugins,        // 插件系统
    UI,            // UI层
    Complete       // 完成
};

/**
 * @brief 初始化结果
 */
struct InitResult {
    bool success;
    QString moduleName;
    QString error;
    qint64 duration;  // 毫秒
};

/**
 * @brief 应用初始化管理器
 */
class ApplicationInitializer : public QObject
{
    Q_OBJECT

public:
    static ApplicationInitializer& instance();

    /**
     * @brief 初始化应用
     */
    bool initialize();

    /**
     * @brief 关闭应用
     */
    void shutdown();

    /**
     * @brief 获取初始化结果
     */
    QMap<QString, InitResult> results() const;

    /**
     * @brief 获取当前阶段
     */
    InitPhase currentPhase() const;

    /**
     * @brief 注册初始化模块
     */
    void registerModule(const QString& name,
                       InitPhase phase,
                       std::function<bool()> initFunc,
                       std::function<void()> shutdownFunc = nullptr);

signals:
    /**
     * @brief 阶段开始信号
     */
    void phaseStarted(InitPhase phase);

    /**
     * @brief 模块初始化完成信号
     */
    void moduleInitialized(const QString& moduleName, bool success, qint64 duration);

    /**
     * @brief 初始化完成信号
     */
    void initializationComplete(bool success);

    /**
     * @brief 进度更新信号
     */
    void progressUpdated(int current, int total, const QString& currentModule);

private:
    ApplicationInitializer();
    ~ApplicationInitializer();
    ApplicationInitializer(const ApplicationInitializer&) = delete;
    ApplicationInitializer& operator=(const ApplicationInitializer&) = delete;

    // 初始化各阶段
    bool initializeCore();
    bool initializeServices();
    bool initializePlugins();
    bool initializeUI();

    // 模块信息
    struct ModuleInfo {
        QString name;
        InitPhase phase;
        std::function<bool()> initFunc;
        std::function<void()> shutdownFunc;
        InitResult result;
    };

    QMap<InitPhase, QList<ModuleInfo>> m_modules;
    QMap<QString, InitResult> m_results;
    InitPhase m_currentPhase;
    bool m_initialized;
};

#endif // APPLICATIONINITIALIZER_H
