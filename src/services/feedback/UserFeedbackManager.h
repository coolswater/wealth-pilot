/**
 * @file UserFeedbackManager.h
 * @brief 用户反馈管理器 - 统一管理用户反馈系统
 *
 * @details 功能：
 * - 反馈收集（对话框、通知、进度条）
 * - 反馈类型管理（信息、警告、错误、成功）
 * - 反馈历史记录
 * - 反馈导出和统计
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef USERFEEDBACKMANAGER_H
#define USERFEEDBACKMANAGER_H

#include <QObject>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTimer>
#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <memory>

namespace WealthPilot {

/**
 * @brief 反馈类型
 */
enum class FeedbackType {
    Info,       ///< 信息
    Warning,    ///< 警告
    Error,      ///< 错误
    Success     ///< 成功
};

/**
 * @brief 反馈级别
 */
enum class FeedbackLevel {
    Toast,      ///< 轻提示（自动消失）
    Dialog,     ///< 对话框
    Notification ///< 系统通知
};

/**
 * @brief 反馈记录
 */
struct FeedbackRecord {
    QString id;                     ///< 唯一ID
    QString title;                  ///< 标题
    QString message;                ///< 消息内容
    FeedbackType type;              ///< 反馈类型
    FeedbackLevel level;            ///< 反馈级别
    QDateTime timestamp;            ///< 时间戳
    QString context;                ///< 上下文信息
    bool acknowledged = false;     ///< 是否已确认
    int duration = 0;               ///< 显示时长（毫秒）
};

/**
 * @brief 进度反馈配置
 */
struct ProgressConfig {
    QString title;                  ///< 进度标题
    QString cancelButtonText;        ///< 取消按钮文本
    int minimum = 0;                ///< 最小值
    int maximum = 100;              ///< 最大值
    bool cancellable = true;        ///< 是否可取消
    int autoCloseDelay = 1000;      ///< 完成后自动关闭延迟（毫秒）
};

/**
 * @brief 用户反馈管理器
 *
 * 提供统一的用户反馈接口：
 * - 信息提示
 * - 警告对话框
 * - 错误处理
 * - 进度显示
 * - 反馈历史
 */
class UserFeedbackManager : public QObject {
    Q_OBJECT

public:
    static UserFeedbackManager* instance();

    // ========== 基础反馈 ==========

    /**
     * @brief 显示信息
     */
    void showInfo(const QString& title, const QString& message,
                  FeedbackLevel level = FeedbackLevel::Toast,
                  int duration = 3000);

    /**
     * @brief 显示警告
     */
    void showWarning(const QString& title, const QString& message,
                     FeedbackLevel level = FeedbackLevel::Dialog);

    /**
     * @brief 显示错误
     */
    void showError(const QString& title, const QString& message,
                   FeedbackLevel level = FeedbackLevel::Dialog);

    /**
     * @brief 显示成功
     */
    void showSuccess(const QString& title, const QString& message,
                     FeedbackLevel level = FeedbackLevel::Toast,
                     int duration = 3000);

    // ========== 对话框 ==========

    /**
     * @brief 显示确认对话框
     * @return 用户是否点击了确认
     */
    bool showConfirm(const QString& title, const QString& message,
                     const QString& confirmText = tr("确定"),
                     const QString& cancelText = tr("取消"));

    /**
     * @brief 显示输入对话框
     * @return 用户输入的文本，取消返回空
     */
    QString showInput(const QString& title, const QString& label,
                      const QString& defaultValue = QString());

    // ========== 进度反馈 ==========

    /**
     * @brief 开始进度显示
     */
    void beginProgress(const QString& operationId, const ProgressConfig& config);

    /**
     * @brief 更新进度
     * @param operationId 操作ID
     * @param value 进度值（0-100）
     * @param message 进度消息（可选）
     */
    void updateProgress(const QString& operationId, int value, const QString& message = QString());

    /**
     * @brief 结束进度显示
     */
    void endProgress(const QString& operationId, bool success = true, const QString& message = QString());

    /**
     * @brief 检查进度是否被取消
     */
    bool isProgressCancelled(const QString& operationId) const;

    // ========== 反馈历史 ==========

    /**
     * @brief 获取反馈历史
     */
    QList<FeedbackRecord> getHistory(int limit = 100) const;

    /**
     * @brief 清除历史记录
     */
    void clearHistory();

    /**
     * @brief 导出反馈历史
     */
    QString exportHistory(const QString& format = "text") const;

    /**
     * @brief 获取反馈统计
     */
    struct FeedbackStats {
        int totalCount = 0;
        int infoCount = 0;
        int warningCount = 0;
        int errorCount = 0;
        int successCount = 0;
        int acknowledgedCount = 0;
    };
    FeedbackStats getStats() const;

    // ========== 配置 ==========

    /**
     * @brief 设置默认显示时长
     */
    void setDefaultDuration(int durationMs) { m_defaultDuration = durationMs; }

    /**
     * @brief 设置是否启用声音提示
     */
    void setSoundEnabled(bool enabled) { m_soundEnabled = enabled; }

    /**
     * @brief 设置最大历史记录数
     */
    void setMaxHistorySize(int size) { m_maxHistorySize = size; }

signals:
    /**
     * @brief 反馈显示信号
     */
    void feedbackShown(const FeedbackRecord& record);

    /**
     * @brief 反馈确认信号
     */
    void feedbackAcknowledged(const QString& feedbackId);

    /**
     * @brief 进度更新信号
     */
    void progressUpdated(const QString& operationId, int value, const QString& message);

    /**
     * @brief 进度取消信号
     */
    void progressCancelled(const QString& operationId);

private:
    explicit UserFeedbackManager(QObject* parent = nullptr);
    ~UserFeedbackManager() override;

    // 创建反馈记录
    FeedbackRecord createRecord(const QString& title, const QString& message,
                                FeedbackType type, FeedbackLevel level);

    // 记录反馈
    void recordFeedback(const FeedbackRecord& record);

    // 显示 Toast 提示
    void showToast(const FeedbackRecord& record, int duration);

    // 显示对话框
    void showDialog(const FeedbackRecord& record);

    // 显示系统通知
    void showNotification(const FeedbackRecord& record);

    // 清理过期历史
    void cleanupHistory();

    // 反馈历史
    QList<FeedbackRecord> m_history;
    mutable QMutex m_historyMutex;
    int m_maxHistorySize = 1000;

    // 进度对话框
    QHash<QString, QProgressDialog*> m_progressDialogs;
    mutable QMutex m_progressMutex;

    // 配置
    int m_defaultDuration = 3000;
    bool m_soundEnabled = false;
};

// ========== 便捷函数 ==========

/**
 * @brief 显示信息提示
 */
inline void showInfo(const QString& title, const QString& message,
                     FeedbackLevel level = FeedbackLevel::Toast,
                     int duration = 3000) {
    UserFeedbackManager::instance()->showInfo(title, message, level, duration);
}

/**
 * @brief 显示警告
 */
inline void showWarning(const QString& title, const QString& message,
                        FeedbackLevel level = FeedbackLevel::Dialog) {
    UserFeedbackManager::instance()->showWarning(title, message, level);
}

/**
 * @brief 显示错误
 */
inline void showError(const QString& title, const QString& message,
                      FeedbackLevel level = FeedbackLevel::Dialog) {
    UserFeedbackManager::instance()->showError(title, message, level);
}

/**
 * @brief 显示成功
 */
inline void showSuccess(const QString& title, const QString& message,
                        FeedbackLevel level = FeedbackLevel::Toast,
                        int duration = 3000) {
    UserFeedbackManager::instance()->showSuccess(title, message, level, duration);
}

/**
 * @brief 显示确认对话框
 */
inline bool showConfirm(const QString& title, const QString& message,
                        const QString& confirmText = QObject::tr("确定"),
                        const QString& cancelText = QObject::tr("取消")) {
    return UserFeedbackManager::instance()->showConfirm(title, message, confirmText, cancelText);
}

// ========== 便捷宏 ==========

/**
 * @brief 信息提示宏
 */
#define FEEDBACK_INFO(title, msg) \
    WealthPilot::UserFeedbackManager::instance()->showInfo(title, msg)

/**
 * @brief 警告提示宏
 */
#define FEEDBACK_WARNING(title, msg) \
    WealthPilot::UserFeedbackManager::instance()->showWarning(title, msg)

/**
 * @brief 错误提示宏
 */
#define FEEDBACK_ERROR(title, msg) \
    WealthPilot::UserFeedbackManager::instance()->showError(title, msg)

/**
 * @brief 成功提示宏
 */
#define FEEDBACK_SUCCESS(title, msg) \
    WealthPilot::UserFeedbackManager::instance()->showSuccess(title, msg)

/**
 * @brief 进度开始宏
 */
#define PROGRESS_BEGIN(id, config) \
    WealthPilot::UserFeedbackManager::instance()->beginProgress(id, config)

/**
 * @brief 进度更新宏
 */
#define PROGRESS_UPDATE(id, value, msg) \
    WealthPilot::UserFeedbackManager::instance()->updateProgress(id, value, msg)

/**
 * @brief 进度结束宏
 */
#define PROGRESS_END(id, success, msg) \
    WealthPilot::UserFeedbackManager::instance()->endProgress(id, success, msg)

} // namespace WealthPilot

#endif // USERFEEDBACKMANAGER_H