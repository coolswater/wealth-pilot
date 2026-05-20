/**
 * @file ErrorHandler.h
 * @brief 统一错误处理助手类
 *
 * @details 提供错误处理、日志记录、用户提示等功能
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include "ErrorCode.h"
#include <QObject>
#include <QString>
#include <functional>

namespace WealthPilot {

/**
 * @brief 错误处理助手类
 *
 * 提供统一的错误处理机制：
 * - 错误日志记录
 * - 用户友好提示
 * - 错误恢复建议
 */
class ErrorHandler : public QObject {
    Q_OBJECT

public:
    static ErrorHandler* instance();

    /**
     * @brief 处理错误
     * @param error 错误对象
     * @param showUser 是否显示给用户
     */
    void handleError(const ErrorInfo& error, bool showUser = true);

    /**
     * @brief 处理错误（简化版）
     * @param code 错误码
     * @param message 错误消息
     * @param showUser 是否显示给用户
     */
    void handleError(ErrorCode code, const QString& message, bool showUser = true);

    /**
     * @brief 获取用户友好的错误提示
     * @param error 错误对象
     * @return 用户友好的提示文本
     */
    QString getUserMessage(const ErrorInfo& error) const;

    /**
     * @brief 获取错误恢复建议
     * @param error 错误对象
     * @return 恢复建议
     */
    QString getRecoverySuggestion(const ErrorInfo& error) const;

    /**
     * @brief 设置错误回调
     * @param callback 错误处理回调函数
     */
    void setErrorCallback(std::function<void(const ErrorInfo&)> callback);

signals:
    /**
     * @brief 错误发生信号
     * @param error 错误对象
     */
    void errorOccurred(const ErrorInfo& error);

    /**
     * @brief 需要显示给用户的错误提示
     * @param title 标题
     * @param message 消息
     * @param suggestion 建议
     */
    void showUserError(const QString& title, const QString& message, const QString& suggestion);

private:
    explicit ErrorHandler(QObject* parent = nullptr);
    ~ErrorHandler() override = default;

    std::function<void(const ErrorInfo&)> m_errorCallback;
};

// ========== 便捷宏定义 ==========

/**
 * @brief 检查错误并返回
 */
#define WP_CHECK_ERROR(error) \
    if ((error).isError()) { \
        WealthPilot::ErrorHandler::instance()->handleError(error); \
        return error; \
    }

/**
 * @brief 检查错误并返回默认值
 */
#define WP_CHECK_ERROR_RET(error, defaultValue) \
    if ((error).isError()) { \
        WealthPilot::ErrorHandler::instance()->handleError(error); \
        return defaultValue; \
    }

/**
 * @brief 检查错误并继续
 */
#define WP_CHECK_ERROR_CONTINUE(error) \
    if ((error).isError()) { \
        WealthPilot::ErrorHandler::instance()->handleError(error); \
        continue; \
    }

/**
 * @brief 记录错误日志
 */
#define WP_LOG_ERROR(error) \
    if ((error).isError()) { \
        WealthPilot::ErrorHandler::instance()->handleError(error, false); \
    }

} // namespace WealthPilot

#endif // ERRORHANDLER_H