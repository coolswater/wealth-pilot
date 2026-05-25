/**
 * @file ErrorHandler.h
 * @brief 统一错误处理系统
 *
 * @details 功能：
 * - 统一错误码定义
 * - 错误分级处理
 * - 用户友好错误提示
 * - 错误日志记录
 * - 错误恢复机制
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QMutex>
#include <QVector>
#include <functional>

namespace WealthPilot {

/**
 * @brief 错误级别
 */
enum class ErrorLevel {
    Info,       ///< 信息级别，不需要特别处理
    Warning,    ///< 警告级别，可能影响功能
    Error,      ///< 错误级别，功能受影响但可继续
    Critical,   ///< 严重错误，需要立即处理
    Fatal       ///< 致命错误，程序无法继续
};

/**
 * @brief 错误码分类
 */
enum class ErrorCategory {
    Network,        ///< 网络错误
    Database,       ///< 数据库错误
    Trading,        ///< 交易错误
    Data,           ///< 数据错误
    Configuration,  ///< 配置错误
    Permission,     ///< 权限错误
    System,         ///< 系统错误
    UserInput,      ///< 用户输入错误
    Plugin,         ///< 插件错误
    AI,             ///< AI服务错误
    Unknown         ///< 未知错误
};

/**
 * @brief 详细错误信息结构（用于错误处理器）
 */
struct DetailedErrorInfo {
    ErrorCategory category = ErrorCategory::Unknown;    ///< 错误分类
    ErrorLevel level = ErrorLevel::Error;               ///< 错误级别
    QString code;                                        ///< 错误码
    QString message;                                     ///< 错误消息
    QString detail;                                      ///< 详细信息
    QString context;                                     ///< 错误上下文
    QString suggestion;                                  ///< 建议解决方案
    qint64 timestamp = 0;                                ///< 发生时间戳

    /**
     * @brief 是否有效
     */
    bool isValid() const { return !code.isEmpty(); }

    /**
     * @brief 是否需要用户干预
     */
    bool needsUserIntervention() const {
        return level >= ErrorLevel::Error;
    }

    /**
     * @brief 是否可恢复
     */
    bool isRecoverable() const {
        return level < ErrorLevel::Fatal;
    }

    /**
     * @brief 获取用户友好的错误描述
     */
    QString userFriendlyMessage() const;
};

/**
 * @brief 错误处理器 - 统一错误管理
 *
 * @details 功能：
 * - 错误注册和查询
 * - 错误处理策略
 * - 错误日志记录
 * - 用户通知
 */
class ErrorHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例
     */
    static ErrorHandler& instance();

    /**
     * @brief 初始化错误处理器
     */
    bool initialize();

    /**
     * @brief 处理错误
     * @param error 错误信息
     */
    void handle(const DetailedErrorInfo& error);

    /**
     * @brief 处理错误（简化版）
     * @param level 错误级别
     * @param code 错误码
     * @param message 错误消息
     * @param context 错误上下文（可选）
     */
    void handle(ErrorLevel level, const QString& code,
                const QString& message, const QString& context = {});

    /**
     * @brief 显示错误给用户
     * @param error 错误信息
     * @param showSuggestion 是否显示建议解决方案
     */
    void showToUser(const DetailedErrorInfo& error, bool showSuggestion = true);

    /**
     * @brief 注册错误码
     * @param code 错误码
     * @param defaultMessage 默认消息
     * @param defaultSuggestion 默认建议
     */
    void registerError(const QString& code,
                       const QString& defaultMessage,
                       const QString& defaultSuggestion = {});

    /**
     * @brief 获取错误码的默认消息
     */
    QString getDefaultMessage(const QString& code) const;

    /**
     * @brief 获取最近的错误
     */
    DetailedErrorInfo lastError() const;

    /**
     * @brief 清除错误历史
     */
    void clearHistory();

    /**
     * @brief 获取错误历史
     */
    QVector<DetailedErrorInfo> errorHistory() const;

    /**
     * @brief 设置错误处理回调
     * @param level 错误级别
     * @param callback 处理回调
     */
    void setErrorHandler(ErrorLevel level,
                         std::function<void(const DetailedErrorInfo&)> callback);

signals:
    /**
     * @brief 错误发生信号
     */
    void errorOccurred(const DetailedErrorInfo& error);

    /**
     * @brief 严重错误信号（需要立即处理）
     */
    void criticalError(const DetailedErrorInfo& error);

    /**
     * @brief 致命错误信号（程序需要退出）
     */
    void fatalError(const DetailedErrorInfo& error);

private:
    ErrorHandler();
    ~ErrorHandler() = default;
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    void logError(const DetailedErrorInfo& error);
    void notifyUser(const DetailedErrorInfo& error);
    void executeHandler(const DetailedErrorInfo& error);

    // 错误码注册表
    QHash<QString, QString> m_defaultMessages;
    QHash<QString, QString> m_defaultSuggestions;

    // 错误历史
    QVector<DetailedErrorInfo> m_errorHistory;
    mutable QMutex m_historyMutex;

    // 错误处理器
    QHash<ErrorLevel, std::function<void(const DetailedErrorInfo&)>> m_handlers;

    // 最近错误
    DetailedErrorInfo m_lastError;
};

// ============================================================================
// 错误码定义（常用错误）
// ============================================================================

namespace ErrorCodes {
    // 网络错误
    inline const QString NetworkTimeout = "NET_TIMEOUT";
    inline const QString NetworkConnectionFailed = "NET_CONN_FAILED";
    inline const QString NetworkInvalidResponse = "NET_INVALID_RESP";

    // 数据库错误
    inline const QString DatabaseConnectionFailed = "DB_CONN_FAILED";
    inline const QString DatabaseQueryFailed = "DB_QUERY_FAILED";
    inline const QString DatabaseWriteFailed = "DB_WRITE_FAILED";

    // 交易错误
    inline const QString OrderRejected = "TRD_ORDER_REJECTED";
    inline const QString InsufficientFunds = "TRD_INSUFFICIENT_FUNDS";
    inline const QString InvalidOrder = "TRD_INVALID_ORDER";
    inline const QString RiskLimitExceeded = "TRD_RISK_LIMIT";

    // 数据错误
    inline const QString DataNotFound = "DATA_NOT_FOUND";
    inline const QString DataInvalid = "DATA_INVALID";
    inline const QString DataExpired = "DATA_EXPIRED";

    // 配置错误
    inline const QString ConfigNotFound = "CFG_NOT_FOUND";
    inline const QString ConfigInvalid = "CFG_INVALID";

    // 权限错误
    inline const QString PermissionDenied = "PERM_DENIED";
    inline const QString AuthenticationFailed = "AUTH_FAILED";

    // 系统错误
    inline const QString SystemOutOfMemory = "SYS_OOM";
    inline const QString SystemCrash = "SYS_CRASH";

    // 用户输入错误
    inline const QString InvalidInput = "INPUT_INVALID";
    inline const QString EmptyInput = "INPUT_EMPTY";
}

} // namespace WealthPilot

#endif // ERRORHANDLER_H
