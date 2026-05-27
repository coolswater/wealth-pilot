/**
 * @file ToastManager.h
 * @brief Toast 通知管理器 - 占位实现
 */

#ifndef TOASTMANAGER_H
#define TOASTMANAGER_H

#include "shared/base/Singleton.h"
#include "core/services/feedback/UserFeedbackManager.h"
#include <QObject>
#include <QString>

namespace WealthPilot {
namespace UI {

/**
 * @brief Toast 管理器 - 占位实现
 */
class ToastManager : public QObject, public Singleton<ToastManager>
{
    Q_OBJECT

public:
    /**
     * @brief 显示 Toast 通知
     */
    void showToast(const QString& title, 
                   const QString& message,
                   FeedbackType type = FeedbackType::Info,
                   int duration = 3000) {
        Q_UNUSED(title)
        Q_UNUSED(message)
        Q_UNUSED(type)
        Q_UNUSED(duration)
        // 占位实现
    }

private:
    friend class Singleton<ToastManager>;
    ToastManager() = default;
};

} // namespace UI
} // namespace WealthPilot

#endif // TOASTMANAGER_H
