/**
 * @file UseCaseBase.cpp
 * @brief 用例基类实现
 */

#include "UseCaseBase.h"
#include <QFutureWatcher>

namespace WealthPilot {
namespace Application {

UseCaseBase::UseCaseBase(QObject* parent)
    : QObject(parent)
{
}

UseCaseBase::~UseCaseBase() = default;

QFuture<bool> UseCaseBase::execute()
{
    if (m_status == UseCaseStatus::Running) {
        return QtConcurrent::run([] { return false; });
    }

    m_status = UseCaseStatus::Running;
    m_errorMessage.clear();
    m_progress = 0;
    emit statusChanged(m_status);

    return QtConcurrent::run([this] {
        bool success = doExecute();
        m_status = success ? UseCaseStatus::Success : UseCaseStatus::Failed;
        emit statusChanged(m_status);
        emit completed(success);
        return success;
    });
}

void UseCaseBase::cancel()
{
    if (m_status != UseCaseStatus::Running) return;

    doCancel();
    m_status = UseCaseStatus::Cancelled;
    emit statusChanged(m_status);
    emit completed(false);
}

void UseCaseBase::setError(const QString& message)
{
    m_errorMessage = message;
}

void UseCaseBase::setProgress(int percent)
{
    m_progress = qBound(0, percent, 100);
    emit progressUpdated(m_progress);
}

} // namespace Application
} // namespace WealthPilot
