/**
 * @file ViewModelBase.cpp
 * @brief ViewModel 基类实现
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ViewModelBase.h"
#include "core/di/ServiceLocator.h"
#include "utils/Logger.h"
#include <QMetaMethod>

namespace WealthPilot
{
    // ============================================================================
    // Command 实现
    // ============================================================================

    Command::Command(QObject* parent)
        : QObject(parent)
          , m_watcher(new QFutureWatcher<QVariant>(this))
    {
        connect(m_watcher, &QFutureWatcher<QVariant>::finished, this, [this]()
        {
            if (m_state == CommandState::Executing)
            {
                QVariant result = m_watcher->result();
                setState(CommandState::Completed);
                m_isExecuting = false;
                emit isExecutingChanged();
                emit completed(result);

                if (m_onCompleted)
                {
                    m_onCompleted(result);
                }
            }
        });

        connect(m_watcher, &QFutureWatcher<QVariant>::progressValueChanged, this, [this](int progress)
        {
            setProgress(progress);
        });
    }

    Command::~Command()
    {
        if (m_watcher)
        {
            m_watcher->cancel();
            m_watcher->waitForFinished();
        }
    }

    void Command::execute()
    {
        if (!m_canExecute || m_isExecuting)
        {
            LOG_WARNING("Command cannot be executed: canExecute=" + QString::number(m_canExecute) +
                ", isExecuting=" + QString::number(m_isExecuting));
            return;
        }

        m_isExecuting = true;
        m_errorMessage.clear();
        setState(CommandState::Executing);
        emit isExecutingChanged();

        try
        {
            if (m_asyncExecuteFunc)
            {
                // 异步执行
                QFuture<QVariant> future = m_asyncExecuteFunc();
                m_watcher->setFuture(future);
            }
            else if (m_executeFunc)
            {
                // 同步执行
                QVariant result = m_executeFunc();
                setState(CommandState::Completed);
                m_isExecuting = false;
                emit isExecutingChanged();
                emit completed(result);

                if (m_onCompleted)
                {
                    m_onCompleted(result);
                }
            }
            else
            {
                // 无执行函数
                setError("No execute function set");
                setState(CommandState::Failed);
                m_isExecuting = false;
                emit isExecutingChanged();
            }
        }
        catch (const std::exception& e)
        {
            setError(QString::fromStdString(e.what()));
            setState(CommandState::Failed);
            m_isExecuting = false;
            emit isExecutingChanged();

            if (m_onError)
            {
                m_onError(m_errorMessage);
            }
        }
    }

    void Command::cancel()
    {
        if (!m_isExecuting)
        {
            return;
        }

        if (m_watcher&& m_watcher
        ->
        isRunning()
        )
        {
            m_watcher->cancel();
            m_watcher->waitForFinished();
        }

        setState(CommandState::Cancelled);
        m_isExecuting = false;
        emit isExecutingChanged();
        emit cancelled();

        LOG_INFO("Command cancelled");
    }

    void Command::reset()
    {
        m_errorMessage.clear();
        m_progress = 0;
        setState(CommandState::Ready);
        updateCanExecute();

        LOG_DEBUG("Command reset");
    }

    void Command::setExecuteFunc(std::function<QVariant()> func)
    {
        m_executeFunc = std::move(func);
    }

    void Command::setAsyncExecuteFunc(std::function<QFuture<QVariant>()> func)
    {
        m_asyncExecuteFunc = std::move(func);
    }

    void Command::setCanExecuteFunc(std::function<bool()> func)
    {
        m_canExecuteFunc = std::move(func);
        updateCanExecute();
    }

    void Command::setOnCompleted(std::function < void(const QVariant &) > callback)
    {
        m_onCompleted = std::move(callback);
    }

    void Command::setOnError(std::function < void(const QString &) > callback)
    {
        m_onError = std::move(callback);
    }

    void Command::updateCanExecute()
    {
        bool newCanExecute = true;

        if (m_canExecuteFunc)
        {
            newCanExecute = m_canExecuteFunc();
        }

        // 执行中不可再次执行
        if (m_isExecuting)
        {
            newCanExecute = false;
        }

        if (newCanExecute != m_canExecute)
        {
            m_canExecute = newCanExecute;
            emit canExecuteChanged();
        }
    }

    void Command::setState(CommandState state)
    {
        if (m_state != state)
        {
            m_state = state;
            emit stateChanged();
        }
    }

    void Command::setProgress(int progress)
    {
        if (m_progress != progress)
        {
            m_progress = progress;
            emit progressChanged();
        }
    }

    void Command::setError(const QString& message)
    {
        if (m_errorMessage != message)
        {
            m_errorMessage = message;
            emit errorOccurred(message);
        }
    }

    // ============================================================================
    // ViewModelBase 实现
    // ============================================================================

    ViewModelBase::ViewModelBase(QObject* parent)
        : QObject(parent)
    {
        LOG_DEBUG("ViewModelBase created");
    }

    ViewModelBase::~ViewModelBase()
    {
        // 清理所有命令
        for (Command * cmd
        :
        m_commands
        )
        {
            if (cmd)
            {
                cmd->cancel();
            }
        }

        cleanup();

        LOG_DEBUG("ViewModelBase destroyed");
    }

    void ViewModelBase::setState(const QString& key, const QVariant& value)
    {
        m_states[key] = value;
        emit stateChanged(key, value);
    }

    QVariant ViewModelBase::getState(const QString& key) const
    {
        return m_states.value(key);
    }

    void ViewModelBase::clearState(const QString& key)
    {
        m_states.remove(key);
    }

    void ViewModelBase::clearAllStates()
    {
        m_states.clear();
    }

    void ViewModelBase::setError(const QString& message)
    {
        if (m_errorMessage != message)
        {
            m_errorMessage = message;
            emit errorMessageChanged();
            emit errorOccurred(message);

            LOG_ERROR("ViewModel error: " + message);
        }
    }

    void ViewModelBase::clearError()
    {
        if (!m_errorMessage.isEmpty())
        {
            m_errorMessage.clear();
            emit errorMessageChanged();
        }
    }

    void ViewModelBase::setStatus(const QString& message)
    {
        if (m_statusMessage != message)
        {
            m_statusMessage = message;
            emit statusMessageChanged();
        }
    }

    void ViewModelBase::clearStatus()
    {
        if (!m_statusMessage.isEmpty())
        {
            m_statusMessage.clear();
            emit statusMessageChanged();
        }
    }

    Command* ViewModelBase::createCommand(std::function<QVariant()> func,
                                          std::function<bool()> canExecute)
    {
        Command * cmd = new Command(this);
        cmd->setExecuteFunc(std::move(func));

        if (canExecute)
        {
            cmd->setCanExecuteFunc(std::move(canExecute));
        }

        m_commands.append(cmd);
        return cmd;
    }

    Command* ViewModelBase::createAsyncCommand(std::function<QFuture<QVariant>()> func,
                                               std::function<bool()> canExecute)
    {
        Command * cmd = new Command(this);
        cmd->setAsyncExecuteFunc(std::move(func));

        if (canExecute)
        {
            cmd->setCanExecuteFunc(std::move(canExecute));
        }

        m_commands.append(cmd);
        return cmd;
    }

    void ViewModelBase::setLoading(bool loading)
    {
        if (m_isLoading != loading)
        {
            m_isLoading = loading;
            emit isLoadingChanged();
        }
    }
} // namespace WealthPilot

// ============================================================================
// ServiceLocator 辅助函数
// ============================================================================

ServiceLocator& GetServiceLocator()
{
    return ServiceLocator::instance();
}
