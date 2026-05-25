/**
 * @file ErrorHandler.cpp
 * @brief 统一错误处理器实现
 * @author WealthPilot Team
 */

#include "ErrorHandler.h"
#include "Logger.h"
#include <QMessageBox>
#include <QApplication>
#include <algorithm>

namespace WealthPilot {

struct ErrorHandler::Impl
{
    QVector<DetailedErrorInfo> errorHistory;
    int maxHistorySize = 100;
    bool showUserNotifications = true;
};

ErrorHandler& ErrorHandler::instance()
{
    static ErrorHandler instance;
    return instance;
}

ErrorHandler::ErrorHandler()
    : QObject(nullptr)
    , d(std::make_unique<Impl>())
{
}

ErrorHandler::~ErrorHandler()
{
}

void ErrorHandler::handleError(const ErrorCode& code, const QString& context)
{
    DetailedErrorInfo error;
    error.errorCode = code;
    error.errorCode.context = context;
    error.errorCode.timestamp = QDateTime::currentDateTime();
    
    // 根据级别记录日志
    switch (code.level) {
    case ErrorLevel::Info:
        LOG_INFO(QString("[%1] %2 - %3")
            .arg(errorCategoryToString(code.category))
            .arg(code.code)
            .arg(code.message));
        break;
    case ErrorLevel::Warning:
        LOG_WARNING(QString("[%1] %2 - %3")
            .arg(errorCategoryToString(code.category))
            .arg(code.code)
            .arg(code.message));
        break;
    case ErrorLevel::Error:
        LOG_ERROR(QString("[%1] %2 - %3")
            .arg(errorCategoryToString(code.category))
            .arg(code.code)
            .arg(code.message));
        break;
    case ErrorLevel::Critical:
        LOG_ERROR(QString("[CRITICAL] [%1] %2 - %3")
            .arg(errorCategoryToString(code.category))
            .arg(code.code)
            .arg(code.message));
        break;
    }
    
    // 添加到历史
    d->errorHistory.append(error);
    if (d->errorHistory.size() > d->maxHistorySize) {
        d->errorHistory.removeFirst();
    }
    
    // 发射信号
    emit errorOccurred(error);
    
    // 显示用户提示
    if (d->showUserNotifications && code.level >= ErrorLevel::Error) {
        QString userMsg = error.userMessage.isEmpty() ? code.message : error.userMessage;
        if (code.level == ErrorLevel::Critical) {
            QMessageBox::critical(nullptr, QStringLiteral("严重错误"), userMsg);
        } else {
            QMessageBox::warning(nullptr, QStringLiteral("错误"), userMsg);
        }
    }
}

void ErrorHandler::handleException(const std::exception& e, const QString& context)
{
    ErrorCode code;
    code.code = -1;
    code.message = QString::fromStdString(e.what());
    code.level = ErrorLevel::Error;
    code.category = ErrorCategory::System;
    code.context = context;
    
    handleError(code, context);
}

void ErrorHandler::logInfo(const QString& message, const QString& context)
{
    ErrorCode code;
    code.code = 0;
    code.message = message;
    code.level = ErrorLevel::Info;
    code.category = ErrorCategory::System;
    code.context = context;
    
    handleError(code, context);
}

void ErrorHandler::logWarning(const QString& message, const QString& context)
{
    ErrorCode code;
    code.code = 0;
    code.message = message;
    code.level = ErrorLevel::Warning;
    code.category = ErrorCategory::System;
    code.context = context;
    
    emit warningOccurred(message);
    LOG_WARNING(QString("[%1] %2").arg(context, message));
}

void ErrorHandler::logError(const QString& message, const QString& context)
{
    ErrorCode code;
    code.code = -1;
    code.message = message;
    code.level = ErrorLevel::Error;
    code.category = ErrorCategory::System;
    code.context = context;
    
    handleError(code, context);
}

std::optional<DetailedErrorInfo> ErrorHandler::lastError() const
{
    if (d->errorHistory.isEmpty()) {
        return std::nullopt;
    }
    return d->errorHistory.last();
}

QVector<DetailedErrorInfo> ErrorHandler::errorHistory(int count) const
{
    if (count <= 0 || count >= d->errorHistory.size()) {
        return d->errorHistory;
    }
    return d->errorHistory.mid(d->errorHistory.size() - count);
}

void ErrorHandler::clearHistory()
{
    d->errorHistory.clear();
}

void ErrorHandler::setShowUserNotifications(bool show)
{
    d->showUserNotifications = show;
}

void ErrorHandler::setMaxHistorySize(int size)
{
    d->maxHistorySize = size;
    while (d->errorHistory.size() > d->maxHistorySize) {
        d->errorHistory.removeFirst();
    }
}

ErrorCode ErrorCode::fromInt(int code, const QString& message)
{
    ErrorCode ec;
    ec.code = code;
    ec.message = message;
    ec.timestamp = QDateTime::currentDateTime();
    return ec;
}

} // namespace WealthPilot