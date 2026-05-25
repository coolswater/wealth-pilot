/**
 * @file ControllerBase.cpp
 * @brief Controller 基类实现
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "ControllerBase.h"
#include "core/di/ServiceLocator.h"
#include "shared/utils/Logger.h"

namespace WealthPilot
{
    ControllerBase::ControllerBase(QObject* parent)
        : QObject(parent)
    {
        LOG_DEBUG("ControllerBase created");
    }

    ControllerBase::~ControllerBase()
    {
        cleanup();
        LOG_DEBUG("ControllerBase destroyed");
    }

    void ControllerBase::initialize()
    {
        LOG_DEBUG("ControllerBase initialized");
    }

    void ControllerBase::cleanup()
    {
        clearAllStates();
        clearError();
        LOG_DEBUG("ControllerBase cleaned up");
    }

    void ControllerBase::reset()
    {
        clearAllStates();
        clearError();
        m_isOperating = false;
        m_currentOperation.clear();
        LOG_DEBUG("ControllerBase reset");
    }

    void ControllerBase::setState(const QString& key, const QVariant& value)
    {
        m_states[key] = value;
        emit stateChanged(key, value);
    }

    QVariant ControllerBase::getState(const QString& key) const
    {
        return m_states.value(key);
    }

    bool ControllerBase::hasState(const QString& key) const
    {
        return m_states.contains(key);
    }

    void ControllerBase::clearState(const QString& key)
    {
        m_states.remove(key);
    }

    void ControllerBase::clearAllStates()
    {
        m_states.clear();
    }

    void ControllerBase::setError(const QString& error)
    {
        m_lastError = error;
        emit errorOccurred(error);
        LOG_ERROR("Controller error: " + error);
    }

    void ControllerBase::clearError()
    {
        m_lastError.clear();
    }

    void ControllerBase::beginOperation(const QString& operation)
    {
        m_currentOperation = operation;
        m_isOperating = true;
        emit operationStarted(operation);
        LOG_DEBUG("Operation started: " + operation);
    }

    void ControllerBase::endOperation(const QString& operation, bool success)
    {
        m_isOperating = false;
        m_currentOperation.clear();
        emit operationCompleted(operation, success);
        LOG_DEBUG("Operation completed: " + operation + " (success=" + QString::number(success));
    }

    void ControllerBase::updateProgress(int progress, const QString& message)
    {
        emit progressChanged(progress, message);
    }
} // namespace WealthPilot