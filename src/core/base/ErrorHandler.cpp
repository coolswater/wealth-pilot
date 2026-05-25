/**
 * @file ErrorHandler.cpp
 * @brief 统一错误处理系统实现
 */

#include "ErrorHandler.h"
#include "utils/Logger.h"
#include <QDateTime>
#include <QMessageBox>
#include <QApplication>

namespace WealthPilot {

// ============================================================================
// DetailedErrorInfo 实现
// ============================================================================

QString DetailedErrorInfo::userFriendlyMessage() const
{
    if (!suggestion.isEmpty()) {
        return QString("%1\n建议：%2").arg(message, suggestion);
    }
    return message;
}

// ============================================================================
// ErrorHandler 实现
// ============================================================================

ErrorHandler& ErrorHandler::instance()
{
    static ErrorHandler instance;
    return instance;
}

ErrorHandler::ErrorHandler()
{
    // 注册默认错误码
    registerError(ErrorCodes::NetworkTimeout,
                  QStringLiteral("网络连接超时"),
                  QStringLiteral("请检查网络连接后重试"));

    registerError(ErrorCodes::NetworkConnectionFailed,
                  QStringLiteral("网络连接失败"),
                  QStringLiteral("请检查网络设置"));

    registerError(ErrorCodes::NetworkInvalidResponse,
                  QStringLiteral("服务器响应无效"),
                  QStringLiteral("请稍后重试"));

    registerError(ErrorCodes::DatabaseConnectionFailed,
                  QStringLiteral("数据库连接失败"),
                  QStringLiteral("请检查数据库配置"));

    registerError(ErrorCodes::DatabaseQueryFailed,
                  QStringLiteral("数据库查询失败"),
                  QStringLiteral("请联系技术支持"));

    registerError(ErrorCodes::DatabaseWriteFailed,
                  QStringLiteral("数据库写入失败"),
                  QStringLiteral("请检查数据格式"));

    registerError(ErrorCodes::OrderRejected,
                  QStringLiteral("订单被拒绝"),
                  QStringLiteral("请检查订单参数"));

    registerError(ErrorCodes::InsufficientFunds,
                  QStringLiteral("资金不足"),
                  QStringLiteral("请检查账户余额"));

    registerError(ErrorCodes::InvalidOrder,
                  QStringLiteral("无效订单"),
                  QStringLiteral("请检查订单信息"));

    registerError(ErrorCodes::RiskLimitExceeded,
                  QStringLiteral("超出风控限制"),
                  QStringLiteral("请调整持仓或联系风控部门"));

    registerError(ErrorCodes::DataNotFound,
                  QStringLiteral("数据不存在"),
                  QStringLiteral("请刷新数据或检查查询条件"));

    registerError(ErrorCodes::InvalidInput,
                  QStringLiteral("输入无效"),
                  QStringLiteral("请检查输入内容"));
}

bool ErrorHandler::initialize()
{
    LOG_INFO("[ErrorHandler] Initialized");
    return true;
}

void ErrorHandler::handle(const DetailedErrorInfo& error)
{
    if (!error.isValid()) {
        return;
    }

    // 记录日志
    logError(error);

    // 记录历史
    {
        QMutexLocker locker(&m_historyMutex);
        m_errorHistory.append(error);
        if (m_errorHistory.size() > 100) {
            m_errorHistory.removeFirst();
        }
        m_lastError = error;
    }

    // 执行处理器
    executeHandler(error);

    // 发送信号
    emit errorOccurred(error);

    // 严重错误特殊处理
    if (error.level == ErrorLevel::Critical) {
        emit criticalError(error);
    } else if (error.level == ErrorLevel::Fatal) {
        emit fatalError(error);
    }
}

void ErrorHandler::handle(ErrorLevel level, const QString& code,
                          const QString& message, const QString& context)
{
    DetailedErrorInfo error;
    error.level = level;
    error.code = code;
    error.message = message;
    error.context = context;
    error.timestamp = QDateTime::currentMSecsSinceEpoch();

    // 查找默认消息和建议
    if (message.isEmpty()) {
        error.message = getDefaultMessage(code);
    }

    handle(error);
}

void ErrorHandler::showToUser(const DetailedErrorInfo& error, bool showSuggestion)
{
    QString title;
    QMessageBox::Icon icon;

    switch (error.level) {
        case ErrorLevel::Info:
            title = QStringLiteral("信息");
            icon = QMessageBox::Information;
            break;
        case ErrorLevel::Warning:
            title = QStringLiteral("警告");
            icon = QMessageBox::Warning;
            break;
        case ErrorLevel::Error:
            title = QStringLiteral("错误");
            icon = QMessageBox::Critical;
            break;
        case ErrorLevel::Critical:
            title = QStringLiteral("严重错误");
            icon = QMessageBox::Critical;
            break;
        case ErrorLevel::Fatal:
            title = QStringLiteral("致命错误");
            icon = QMessageBox::Critical;
            break;
    }

    QString text = error.message;
    if (showSuggestion && !error.suggestion.isEmpty()) {
        text += QString("\n\n建议：%1").arg(error.suggestion);
    }

    QMessageBox msgBox(icon, title, text, QMessageBox::Ok,
                       qApp->activeWindow());
    msgBox.exec();
}

void ErrorHandler::registerError(const QString& code,
                                  const QString& defaultMessage,
                                  const QString& defaultSuggestion)
{
    m_defaultMessages[code] = defaultMessage;
    m_defaultSuggestions[code] = defaultSuggestion;
}

QString ErrorHandler::getDefaultMessage(const QString& code) const
{
    return m_defaultMessages.value(code, QStringLiteral("未知错误"));
}

DetailedErrorInfo ErrorHandler::lastError() const
{
    QMutexLocker locker(&m_historyMutex);
    return m_lastError;
}

void ErrorHandler::clearHistory()
{
    QMutexLocker locker(&m_historyMutex);
    m_errorHistory.clear();
    m_lastError = DetailedErrorInfo();
}

QVector<DetailedErrorInfo> ErrorHandler::errorHistory() const
{
    QMutexLocker locker(&m_historyMutex);
    return m_errorHistory;
}

void ErrorHandler::setErrorHandler(ErrorLevel level,
                                    std::function<void(const DetailedErrorInfo&)> callback)
{
    m_handlers[level] = callback;
}

void ErrorHandler::logError(const DetailedErrorInfo& error)
{
    QString logMsg = QString("[%1] %2: %3")
        .arg(error.code, error.context, error.message);

    switch (error.level) {
        case ErrorLevel::Info:
            LOG_INFO(logMsg);
            break;
        case ErrorLevel::Warning:
            LOG_WARNING(logMsg);
            break;
        case ErrorLevel::Error:
            LOG_ERROR(logMsg);
            break;
        case ErrorLevel::Critical:
            LOG_ERROR(logMsg);  // 使用 LOG_ERROR 代替 LOG_CRITICAL
            break;
        case ErrorLevel::Fatal:
            LOG_ERROR(logMsg);  // 使用 LOG_ERROR 代替 LOG_FATAL
            break;
    }
}

void ErrorHandler::notifyUser(const DetailedErrorInfo& error)
{
    if (error.needsUserIntervention()) {
        showToUser(error);
    }
}

void ErrorHandler::executeHandler(const DetailedErrorInfo& error)
{
    auto it = m_handlers.find(error.level);
    if (it != m_handlers.end()) {
        it.value()(error);
    }
}

} // namespace WealthPilot