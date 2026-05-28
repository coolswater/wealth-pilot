/**
 * @file ToastWidget.h
 * @brief Toast 提示控件 - 轻量级消息提示
 *
 * @details 功能：
 * - 自动消失的消息提示
 * - 支持多种类型（信息、警告、错误、成功）
 * - 动画效果（淡入淡出）
 * - 可堆叠显示
 * - 支持自定义位置
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef TOASTWIDGET_H
#define TOASTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QQueue>
#include <memory>
#include "services/feedback/UserFeedbackManager.h"

namespace WealthPilot {
namespace UI {

// 使用 WealthPilot::FeedbackType
using WealthPilot::FeedbackType;

/**
 * @brief Toast 显示位置
 */
enum class ToastPosition {
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    Center
};

/**
 * @brief 单个 Toast 消息控件
 */
class ToastItem : public QWidget {
    Q_OBJECT

public:
    explicit ToastItem(const QString& title, const QString& message,
                       FeedbackType type, int duration,
                       QWidget* parent = nullptr);
    ~ToastItem() override;

    void show();
    void hide();

signals:
    void finished();

private:
    void setupUI();
    void applyStyle();
    void startAnimation();

    QString m_title;
    QString m_message;
    FeedbackType m_type;
    int m_duration;

    QLabel* m_iconLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_messageLabel = nullptr;
    QWidget* m_container = nullptr;

    QTimer* m_autoHideTimer = nullptr;
    QGraphicsOpacityEffect* m_opacityEffect = nullptr;
    QPropertyAnimation* m_fadeInAnimation = nullptr;
    QPropertyAnimation* m_fadeOutAnimation = nullptr;
};

/**
 * @brief Toast 管理器 - 管理多个 Toast 的显示
 */
class ToastManager : public QWidget {
    Q_OBJECT

public:
    static ToastManager* instance();

    /**
     * @brief 显示 Toast
     */
    void showToast(const QString& title, const QString& message,
                   FeedbackType type, int duration = 3000);

    /**
     * @brief 设置 Toast 位置
     */
    void setPosition(ToastPosition position);

    /**
     * @brief 设置最大显示数量
     */
    void setMaxVisible(int max) { m_maxVisible = max; }

    /**
     * @brief 设置间距
     */
    void setSpacing(int spacing) { m_spacing = spacing; }

    /**
     * @brief 清除所有 Toast
     */
    void clearAll();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    explicit ToastManager(QWidget* parent = nullptr);
    ~ToastManager() override;

    void updateLayout();
    void processQueue();
    void removeToast(ToastItem* toast);

    ToastPosition m_position = ToastPosition::TopRight;
    int m_maxVisible = 5;
    int m_spacing = 8;

    QList<ToastItem*> m_activeToasts;
    QQueue<std::tuple<QString, QString, FeedbackType, int>> m_queue;
};

/**
 * @brief 便捷函数 - 显示信息 Toast
 */
inline void showInfoToast(const QString& title, const QString& message, int duration = 3000) {
    ToastManager::instance()->showToast(title, message, FeedbackType::Info, duration);
}

/**
 * @brief 便捷函数 - 显示警告 Toast
 */
inline void showWarningToast(const QString& title, const QString& message, int duration = 5000) {
    ToastManager::instance()->showToast(title, message, FeedbackType::Warning, duration);
}

/**
 * @brief 便捷函数 - 显示错误 Toast
 */
inline void showErrorToast(const QString& title, const QString& message, int duration = 8000) {
    ToastManager::instance()->showToast(title, message, FeedbackType::Error, duration);
}

/**
 * @brief 便捷函数 - 显示成功 Toast
 */
inline void showSuccessToast(const QString& title, const QString& message, int duration = 3000) {
    ToastManager::instance()->showToast(title, message, FeedbackType::Success, duration);
}

} // namespace UI
} // namespace WealthPilot

#endif // TOASTWIDGET_H