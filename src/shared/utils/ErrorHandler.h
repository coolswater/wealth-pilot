/**
 * @file ErrorHandler.h
 * @brief 统一错误处理器
 * @author WealthPilot Team
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QVector>
#include <memory>
#include <optional>

namespace WealthPilot {

/**
 * @brief 错误级别
 */
enum class ErrorLevel
{
    Info,       ///< 信息
    Warning,    ///< 警告
    Error,      ///< 错误
    Critical    ///< 严重错误
};

/**
 * @brief 错误类别
 */
enum class ErrorCategory
{
    Network,    ///< 网络错误
    Database,   ///< 数据库错误
    Trading,    ///< 交易错误
    Analysis,   ///< 分析错误
    System,     ///< 系统错误
    UserInput,  ///< 用户输入错误
    Plugin,     ///< 插件错误
    Unknown     ///< 未知错误
};

/**
 * @brief 错误代码结构
 */
struct ErrorCode
{
    int code = 0;               ///< 错误码
    QString message;            ///< 错误消息
    ErrorLevel level = ErrorLevel::Error;
    ErrorCategory category = ErrorCategory::Unknown;
    QString context;            ///< 上下文信息
    QDateTime timestamp;        ///< 时间戳
    
    static ErrorCode fromInt(int code, const QString& message = {});
    
    bool isError() const { return level == ErrorLevel::Error || level == ErrorLevel::Critical; }
    bool isWarning() const { return level == ErrorLevel::Warning; }
};

/**
 * @brief 详细错误信息结构
 */
struct DetailedErrorInfo
{
    ErrorCode errorCode;
    QString source;             ///< 错误来源
    QString stackTrace;         ///< 堆栈跟踪
    QVariantMap additionalData; ///< 附加数据
    QString userMessage;        ///< 用户友好消息
    QString suggestion;         ///< 建议操作
};

/**
 * @brief 统一错误处理器
 */
class ErrorHandler : public QObject
{
    Q_OBJECT

public:
    static ErrorHandler& instance();

    /**
     * @brief 处理错误
     */
    void handleError(const ErrorCode& code, const QString& context = {});
    
    /**
     * @brief 处理异常
     */
    void handleException(const std::exception& e, const QString& context = {});
    
    /**
     * @brief 记录信息
     */
    void logInfo(const QString& message, const QString& context = {});
    
    /**
     * @brief 记录警告
     */
    void logWarning(const QString& message, const QString& context = {});
    
    /**
     * @brief 记录错误
     */
    void logError(const QString& message, const QString& context = {});
    
    /**
     * @brief 获取最近的错误
     */
    std::optional<DetailedErrorInfo> lastError() const;
    
    /**
     * @brief 获取错误历史
     */
    QVector<DetailedErrorInfo> errorHistory(int count = 100) const;
    
    /**
     * @brief 清除错误历史
     */
    void clearHistory();
    
    /**
     * @brief 设置是否显示用户提示
     */
    void setShowUserNotifications(bool show);
    
    /**
     * @brief 设置最大历史记录数
     */
    void setMaxHistorySize(int size);

signals:
    /**
     * @brief 错误发生信号
     */
    void errorOccurred(const DetailedErrorInfo& error);
    
    /**
     * @brief 警告发生信号
     */
    void warningOccurred(const QString& message);

private:
    ErrorHandler();
    ~ErrorHandler();
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

/**
 * @brief 获取错误级别显示名称
 */
inline QString errorLevelToString(ErrorLevel level)
{
    switch (level) {
        case ErrorLevel::Info:     return QStringLiteral("信息");
        case ErrorLevel::Warning:  return QStringLiteral("警告");
        case ErrorLevel::Error:    return QStringLiteral("错误");
        case ErrorLevel::Critical: return QStringLiteral("严重");
        default: return QStringLiteral("未知");
    }
}

/**
 * @brief 获取错误类别显示名称
 */
inline QString errorCategoryToString(ErrorCategory category)
{
    switch (category) {
        case ErrorCategory::Network:   return QStringLiteral("网络");
        case ErrorCategory::Database:  return QStringLiteral("数据库");
        case ErrorCategory::Trading:   return QStringLiteral("交易");
        case ErrorCategory::Analysis:  return QStringLiteral("分析");
        case ErrorCategory::System:    return QStringLiteral("系统");
        case ErrorCategory::UserInput: return QStringLiteral("用户输入");
        case ErrorCategory::Plugin:    return QStringLiteral("插件");
        default: return QStringLiteral("未知");
    }
}

} // namespace WealthPilot

#endif // ERROR_HANDLER_H