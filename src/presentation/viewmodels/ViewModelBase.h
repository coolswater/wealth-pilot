/**
 * @file ViewModelBase.h
 * @brief ViewModel 基类 - MVVM 架构核心组件
 * 
 * @details 提供：
 * - 属性绑定机制
 * - 命令执行框架
 * - 服务注入
 * - 状态管理
 * - 错误处理
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef VIEWMODELBASE_H
#define VIEWMODELBASE_H

#include <QObject>
#include <QVariant>
#include <QHash>
#include <QString>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>
#include "core/di/ServiceLocator.h"
#include <functional>
#include <memory>

// 前向声明
class Command;
class ServiceLocator;

namespace WealthPilot
{
    /**
 * @brief 命令状态
 */
    enum class CommandState
    {
        Ready, ///< 就绪，可执行
        Executing, ///< 执行中
        Completed, ///< 已完成
        Failed, ///< 失败
        Cancelled ///< 已取消
    };

    /**
 * @brief 命令类 - 封装可执行操作
 *
 * @details 用于 QML 绑定，支持：
 * - 异步执行
 * - 执行状态跟踪
 * - 可执行条件判断
 * - 取消操作
 *
 * @example
 * @code
 * // QML 中使用
 * Button {
 *     text: "买入"
 *     enabled: viewModel.buyCommand.canExecute
 *     onClicked: viewModel.buyCommand.execute()
 * }
 * @endcode
 */
    class Command : public QObject
    {
        Q_OBJECT

        /// 是否可执行
        Q_PROPERTY(bool canExecute READ canExecute NOTIFY canExecuteChanged)
        /// 是否正在执行
        Q_PROPERTY(bool isExecuting READ isExecuting NOTIFY isExecutingChanged)
        /// 执行进度 (0-100)
        Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
        /// 执行状态
        Q_PROPERTY(CommandState state READ state NOTIFY stateChanged)
        /// 错误信息
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorOccurred)

    public:
        explicit Command(QObject* parent = nullptr);
        ~Command() override;

        // ========== 属性访问 ==========

        bool canExecute() const { return m_canExecute; }
        bool isExecuting() const { return m_isExecuting; }
        int progress() const { return m_progress; }
        CommandState state() const { return m_state; }
        QString errorMessage() const { return m_errorMessage; }

        // ========== 执行操作 ==========

        /**
     * @brief 执行命令
     */
    Q_INVOKABLE void execute();

        /**
     * @brief 取消命令
     */
    Q_INVOKABLE void cancel();

        /**
     * @brief 重置命令状态
     */
    Q_INVOKABLE void reset();

        // ========== 配置 ==========

        /**
     * @brief 设置执行函数
     */
        void setExecuteFunc(std::function<QVariant()> func);

        /**
     * @brief 设置异步执行函数
     */
        void setAsyncExecuteFunc(std::function<QFuture<QVariant>()> func);

        /**
     * @brief 设置可执行条件函数
     */
        void setCanExecuteFunc(std::function<bool()> func);

        /**
     * @brief 设置完成回调
     */
        void setOnCompleted(std::function<void(const QVariant &)> callback);

        /**
     * @brief 设置错误回调
     */
        void setOnError(std::function<void(const QString &)> callback);

        /**
     * @brief 更新可执行状态
     */
        void updateCanExecute();

        signals :

        void canExecuteChanged();
        void isExecutingChanged();
        void progressChanged();
        void stateChanged();
        void errorOccurred(const QString& message);
        void completed(const QVariant& result);
        void cancelled();

    private:
        void setState(CommandState state);
        void setProgress(int progress);
        void setError(const QString& message);

        std::function<QVariant()> m_executeFunc;
        std::function<QFuture<QVariant>()> m_asyncExecuteFunc;
        std::function<bool()> m_canExecuteFunc;
        std::function<void(const QVariant &)> m_onCompleted;
        std::function<void(const QString &)> m_onError;

        bool m_canExecute = true;
        bool m_isExecuting = false;
        int m_progress = 0;
        CommandState m_state = CommandState::Ready;
        QString m_errorMessage;

        QFutureWatcher<QVariant>* m_watcher = nullptr;
    };

    /**
 * @brief ViewModel 基类
 *
 * @details 所有 ViewModel 的基类，提供：
 * - 公共属性（加载状态、错误信息、状态消息）
 * - 命令管理
 * - 服务注入
 * - 属性绑定
 * - 状态管理
 *
 * @example
 * @code
 * class TradingViewModel : public ViewModelBase {
 *     Q_OBJECT
 *     Q_PROPERTY(double price READ price NOTIFY priceChanged)
 *     Q_PROPERTY(Command* buyCommand READ buyCommand CONSTANT)
 *
 * public:
 *     double price() const { return m_price; }
 *     Command* buyCommand() { return m_buyCommand; }
 *
 * signals:
 *     void priceChanged();
 *
 * private:
 *     double m_price = 0.0;
 *     Command* m_buyCommand;
 * };
 * @endcode
 */
    class ViewModelBase : public QObject
    {
        Q_OBJECT

        /// 是否正在加载
        Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
        /// 错误信息
        Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
        /// 状态消息
        Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
        /// 是否有错误
        Q_PROPERTY(bool hasError READ hasError NOTIFY errorMessageChanged)

    public:
        explicit ViewModelBase(QObject* parent = nullptr);
        ~ViewModelBase() override;

        // ========== 属性访问 ==========

        bool isLoading() const { return m_isLoading; }
        QString errorMessage() const { return m_errorMessage; }
        QString statusMessage() const { return m_statusMessage; }
        bool hasError() const { return !m_errorMessage.isEmpty(); }

        // ========== 命令执行 ==========

        /**
     * @brief 执行命令（带加载状态）
     * @param func 执行函数
     * @param description 命令描述
     * @return 命令结果
     */
        template <typename Func>
        QFuture<QVariant> executeCommand(Func func, const QString& description = QString());

        /**
     * @brief 执行同步命令
     */
        template <typename Func>
        QVariant executeSync(Func func, const QString& description = QString());

        // ========== 服务注入 ==========

        /**
     * @brief 获取服务
     */
        template <typename T>
        T* getService() const;

        /**
     * @brief 注册服务依赖
     */
        template <typename T>
        void requireService();

        // ========== 状态管理 ==========

        /**
     * @brief 设置状态
     */
    Q_INVOKABLE void setState(const QString& key, const QVariant& value);

        /**
     * @brief 获取状态
     */
    Q_INVOKABLE QVariant getState(const QString& key) const;

        /**
     * @brief 清除状态
     */
    Q_INVOKABLE void clearState(const QString& key);

        /**
     * @brief 清除所有状态
     */
    Q_INVOKABLE void clearAllStates();

        // ========== 错误处理 ==========

        /**
     * @brief 设置错误信息
     */
        void setError(const QString& message);

        /**
     * @brief 清除错误
     */
        void clearError();

        /**
     * @brief 设置状态消息
     */
        void setStatus(const QString& message);

        /**
     * @brief 清除状态消息
     */
        void clearStatus();

        // ========== 命令管理 ==========

        /**
     * @brief 创建命令
     */
        Command* createCommand(std::function<QVariant()> func,
                               std::function<bool()> canExecute = nullptr);

        /**
     * @brief 创建异步命令
     */
        Command* createAsyncCommand(std::function<QFuture<QVariant>()> func,
                                    std::function<bool()> canExecute = nullptr);

        // ========== 生命周期 ==========

        /**
     * @brief 初始化（子类实现）
     */
        virtual void initialize()
        {
        }

        /**
     * @brief 清理（子类实现）
     */
        virtual void cleanup()
        {
        }

        signals :

        void isLoadingChanged();
        void errorMessageChanged();
        void statusMessageChanged();
        void stateChanged(const QString& key, const QVariant& value);
        void commandStarted(const QString& description);
        void commandCompleted(const QString& description, bool success);
        void errorOccurred(const QString& message);

    protected:
        /**
     * @brief 设置加载状态
     */
        void setLoading(bool loading);

        /**
     * @brief 绑定信号到属性
     */
        template <typename Sender, typename Signal, typename Receiver, typename Slot>
        void bind(Sender* sender, Signal signal, Receiver* receiver, Slot slot);

        /**
     * @brief 绑定服务信号到属性更新
     */
        template <typename T>
        void bindServiceProperty(const char* signal, const char* propertyName,
                                 std::function<void()> updateFunc);

    private:
        bool m_isLoading = false;
        QString m_errorMessage;
        QString m_statusMessage;
        QHash<QString, QVariant> m_states;
        QList<Command*> m_commands;
    };

    // ============================================================================
    // 模板实现
    // ============================================================================

    template <typename Func>
    QFuture<QVariant> ViewModelBase::executeCommand(Func func, const QString& description)
    {
        setLoading(true);
        clearError();

        if (!description.isEmpty())
        {
            setStatus(description);
            emit commandStarted(description);
        }

        return QtConcurrent::run([this, func, description]() -> QVariant
        {
            try
            {
                QVariant result = func();

                QMetaObject::invokeMethod(this, [this, description, result]()
                {
                    setLoading(false);
                    if (!description.isEmpty())
                    {
                        emit commandCompleted(description, true);
                    }
                }, Qt::QueuedConnection);

                return result;
            }
            catch (const std::exception& e)
            {
                QMetaObject::invokeMethod(this, [this, e, description]()
                {
                    setError(QString::fromStdString(e.what()));
                    setLoading(false);
                    if (!description.isEmpty())
                    {
                        emit commandCompleted(description, false);
                    }
                }, Qt::QueuedConnection);

                return QVariant();
            }
        });
    }

    template <typename Func>
    QVariant ViewModelBase::executeSync(Func func, const QString& description)
    {
        setLoading(true);
        clearError();

        if (!description.isEmpty())
        {
            setStatus(description);
        }

        try
        {
            QVariant result = func();
            setLoading(false);
            return result;
        }
        catch (const std::exception& e)
        {
            setError(QString::fromStdString(e.what()));
            setLoading(false);
            return QVariant();
        }
    }

    template <typename T>
    T* ViewModelBase::getService() const
    {
        // 使用 ServiceLocator 获取服务
        return ServiceLocator::instance().template resolve<T>();
    }

    template <typename T>
    void ViewModelBase::requireService()
    {
        if (!ServiceLocator::instance().template isRegistered<T>())
        {
            qWarning() << "Required service not registered:" << typeid(T).name();
        }
    }

    template <typename Sender, typename Signal, typename Receiver, typename Slot>
    void ViewModelBase::bind(Sender* sender, Signal signal, Receiver* receiver, Slot slot)
    {
        connect(sender, signal, receiver, slot);
    }

    template <typename T>
    void ViewModelBase::bindServiceProperty(const char* signal, const char* propertyName,
                                            std::function<void()> updateFunc)
    {
        T* service = getService<T>();
        if (service)
        {
            connect(service, signal, this, [this, propertyName, updateFunc]()
            {
                updateFunc();
                // 自动发射属性变化信号
                QMetaObject::invokeMethod(this, (QString("%1Changed").arg(propertyName)).toUtf8());
            });
        }
    }
} // namespace WealthPilot

// 注册类型到 QML
Q_DECLARE_METATYPE(WealthPilot::CommandState)

#endif // VIEWMODELBASE_H
