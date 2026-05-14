/**
 * @file ApplicationInitializer.h
 * @brief 应用初始化管理器 - 统一管理应用启动和初始化流程
 *
 * @details 功能：
 * - 统一初始化所有核心模块
 * - 管理模块依赖关系
 * - 性能优化：并行初始化
 * - 错误处理和回滚
 * - DataHub 数据中心集成
 *
 * @author WealthPilot Team
 * @version 2.1.0
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
    Core,           ///< 核心模块（Logger, Config, Cache, DataHub）
    Services,       ///< 服务层（数据源、交易服务等）
    Plugins,        ///< 插件系统（CTP, AI等）
    UI,             ///< UI层（主题、页面等）
    Complete        ///< 初始化完成
};

/**
 * @brief 初始化结果
 */
struct InitResult {
    bool success;           ///< 是否成功
    QString moduleName;     ///< 模块名称
    QString error;          ///< 错误信息
    qint64 duration;        ///< 耗时（毫秒）
};

/**
 * @brief 应用初始化管理器
 * 
 * 单例模式，负责应用启动时的模块初始化和关闭时的资源释放。
 * 初始化顺序：Core -> Services -> Plugins -> UI
 * 关闭顺序：UI -> Plugins -> Services -> Core（反向）
 */
class ApplicationInitializer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static ApplicationInitializer& instance();

    /**
     * @brief 初始化应用
     * @return true 初始化成功
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
     * @param name 模块名称
     * @param phase 初始化阶段
     * @param initFunc 初始化函数
     * @param shutdownFunc 关闭函数（可选）
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
    bool initializeCore();      ///< 核心模块初始化
    bool initializeServices();  ///< 服务层初始化
    bool initializePlugins();   ///< 插件系统初始化
    bool initializeUI();        ///< UI层初始化

    // 模块信息
    struct ModuleInfo {
        QString name;                       ///< 模块名称
        InitPhase phase;                    ///< 初始化阶段
        std::function<bool()> initFunc;     ///< 初始化函数
        std::function<void()> shutdownFunc; ///< 关闭函数
        InitResult result;                  ///< 初始化结果
    };

    QMap<InitPhase, QList<ModuleInfo>> m_modules;  ///< 按阶段分组的模块
    QMap<QString, InitResult> m_results;          ///< 初始化结果
    InitPhase m_currentPhase;                      ///< 当前阶段
    bool m_initialized;                            ///< 是否已初始化
};

#endif // APPLICATIONINITIALIZER_H
