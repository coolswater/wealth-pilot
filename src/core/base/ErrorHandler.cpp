/**
 * @file ErrorHandler.cpp
 * @brief 统一错误处理系统实现
 */

#include "ErrorHandler.h"
#include "utils/Logger.h"
#include <QMessageBox>
#include <QDateTime>
#include <QMutexLocker>
#include <QApplication>

namespace WealthPilot {

// ============================================================================
// Error 实现
// ============================================================================

QString Error::userFriendlyMessage() const
{
    QString friendly;

    // 根据错误分类生成友好消息
    switch (category) {
        case ErrorCategory::Network:
            friendly = QStringLiteral("网络连接出现问题，请检查网络设置");
            break;
        case ErrorCategory::Database:
            friendly = QStringLiteral("数据存储出现问题，请稍后重试");
            break;
        case ErrorCategory::Trading:
            friendly = QStringLiteral("交易操作失败，请检查订单信息");
            break;
        case ErrorCategory::Data:
            friendly = QStringLiteral("数据加载出现问题");
            break;
        case ErrorCategory::Configuration:
            friendly = QStringLiteral("配置加载失败，请检查配置文件");
            break;
        case ErrorCategory::Permission:
            friendly = QStringLiteral("权限不足，无法执行此操作");
            break;
        case ErrorCategory::System:
            friendly = QStringLiteral("系统出现问题，请重启程序");
            break;
        case ErrorCategory::UserInput:
            friendly = QStringLiteral("输入信息有误，请检查后重试");
            break;
        case ErrorCategory::Plugin:
            friendly = QStringLiteral("插件加载失败");
            break;
        case ErrorCategory::AI:
            friendly = QStringLiteral("AI服务暂时不可用");
            break;
        default:
            friendly = QStringLiteral("发生未知错误");
            break;
    }

    // 添加具体消息
    if (!message.isEmpty()) {
        friendly += QStringLiteral("：") + message;
    }

    return friendly;
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
                  QStringLiteral("网络请求超时"),
                  QStringLiteral("请检查网络连接后重试"));

    registerError(ErrorCodes::NetworkConnectionFailed,
                  QStringLiteral("无法连接到服务器"),
                  QStringLiteral("请检查网络设置或联系技术支持"));

    registerError(ErrorCodes::DatabaseConnectionFailed,
                  QStringLiteral("数据库连接失败"),
                  QStringLiteral("请检查数据库配置"));

    registerError(ErrorCodes::OrderRejected,
                  QStringLiteral("订单被拒绝"),
                  QStringLiteral("请检查订单参数或联系客服"));

    registerError(ErrorCodes::InsufficientFunds,
                  QStringLiteral("资金不足"),
                  QStringLiteral("请检查账户余额"));

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

ErrorHandler::~ErrorHandler()
{
}

bool ErrorHandler::initialize()
{
    LOG_INFO("[ErrorHandler] Initialized");
    return true;
}

void ErrorHandler::handle(const Error& error)
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
    if (error.level >= ErrorLevel::Critical) {
        emit criticalError(error);
    }

    if (error.level == ErrorLevel::Fatal) {
        emit fatalError(error);
    }
}

void ErrorHandler::handle(ErrorLevel level, const QString& code,
                          const QString& message, const QString& context)
{
    Error error;
    error.level = level;
    error.code = code;
    error.message = message;
    error.context = context;
    error.timestamp = QDateTime::currentMSecsSinceEpoch();

    // 尝试获取默认消息和建议
    if (message.isEmpty()) {
        error.message = getDefaultMessage(code);
    }

    handle(error);
}

void ErrorHandler::showToUser(const Error& error, bool showSuggestion)
{
    if (!error.isValid()) {
        return;
    }

    // 根据错误级别选择对话框类型
    QMessageBox::Icon icon;
    QString title;

    switch (error.level) {
        case ErrorLevel::Info:
            icon = QMessageBox::Information;
            title = QStringLiteral("提示");
            break;
        case ErrorLevel::Warning:
            icon = QMessageBox::Warning;
            title = QStringLiteral("警告");
            break;
        case ErrorLevel::Error:
            icon = QMessageBox::Critical;
            title = QStringLiteral("错误");
            break;
        case ErrorLevel::Critical:
            icon = QMessageBox::Critical;
            title = QStringLiteral("严重错误");
            break;
        case ErrorLevel::Fatal:
            icon = QMessageBox::Critical;
            title = QStringLiteral("致命错误");
            break;
        default:
            icon = QMessageBox::Warning;
            title = QStringLiteral("提示");
            break;
    }

    // 构造消息内容
    QString text = error.userFriendlyMessage();

    if (showSuggestion && !error.suggestion.isEmpty()) {
        text += QStringLiteral("\n\n建议：") + error.suggestion;
    }

    // 显示对话框
    QMessageBox box(icon, title, text, QMessageBox::Ok);
    box.exec();
}

void ErrorHandler::registerError(const QString& code,
                                  const QString& defaultMessage,
                                  const QString& defaultSuggestion)
{
    m_defaultMessages[code] = defaultMessage;
    if (!defaultSuggestion.isEmpty()) {
        m_defaultSuggestions[code] = defaultSuggestion;
    }
}

QString ErrorHandler::getDefaultMessage(const QString& code) const
{
    return m_defaultMessages.value(code, QStringLiteral("未知错误"));
}

Error ErrorHandler::lastError() const
{
    QMutexLocker locker(&m_historyMutex);
    return m_lastError;
}

void ErrorHandler::clearHistory()
{
    QMutexLocker locker(&m_historyMutex);
    m_errorHistory.clear();
    m_lastError = Error{};
}

QVector<Error> ErrorHandler::errorHistory() const
{
    QMutexLocker locker(&m_historyMutex);
    return m_errorHistory;
}

void ErrorHandler::setErrorHandler(ErrorLevel level,
                                    std::function<void(const Error&)> callback)
{
    m_handlers[level] = callback;
}

void ErrorHandler::logError(const Error& error)
{
    QString logMsg = QString("[%1] %2: %3")
        .arg(error.code)
        .arg(static_cast<int>(error.level))
        .arg(error.message);

    if (!error.context.isEmpty()) {
        logMsg += QStringLiteral(" | Context: ") + error.context;
    }

    if (!error.detail.isEmpty()) {
        logMsg += QStringLiteral(" | Detail: ") + error.detail;
    }

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
            LOG_CRITICAL(logMsg);
            break;
        case ErrorLevel::Fatal:
            LOG_FATAL(logMsg);
            break;
    }
}

void ErrorHandler::executeHandler(const Error& error)
{
    auto it = m_handlers.find(error.level);
    if (it != m_handlers.end()) {
        it.value()(error);
    }
}

} // namespace WealthPilot