/**
 * @file ErrorCode.h
 * @brief 统一错误码定义
 * @author WealthPilot Team
 * @version 2.0.0
 * 
 * @details 定义所有模块的错误码，按模块分段：
 * - 1xxx: 通用错误
 * - 2xxx: 网络错误
 * - 3xxx: 数据库错误
 * - 4xxx: CTP错误
 * - 5xxx: AI错误
 * - 6xxx: 配置错误
 */

#ifndef WEALTHPILOT_CORE_ERRORCODE_H
#define WEALTHPILOT_CORE_ERRORCODE_H

#include <QString>
#include <QVariant>

namespace WealthPilot {

/**
 * @brief 统一错误码枚举
 */
enum class ErrorCode : int {
    // ========== 通用错误 1xxx ==========
    Success = 0,
    Unknown = 1000,
    InvalidArgument = 1001,
    NullPointer = 1002,
    OperationFailed = 1003,
    Timeout = 1004,
    Cancelled = 1005,
    NotInitialized = 1006,
    AlreadyInitialized = 1007,
    
    // ========== 网络错误 2xxx ==========
    NetworkError = 2000,
    NetworkTimeout = 2001,
    NetworkDisconnected = 2002,
    NetworkInvalidResponse = 2003,
    NetworkServerError = 2004,
    NetworkNotFound = 2005,
    NetworkUnauthorized = 2006,
    NetworkRateLimited = 2007,
    
    // ========== 数据库错误 3xxx ==========
    DatabaseError = 3000,
    DatabaseOpenFailed = 3001,
    DatabaseQueryFailed = 3002,
    DatabaseTransactionFailed = 3003,
    DatabaseConnectionFailed = 3004,
    DatabaseSchemaError = 3005,
    DatabaseConstraintViolation = 3006,
    
    // ========== CTP错误 4xxx ==========
    CtpError = 4000,
    CtpConnectFailed = 4001,
    CtpLoginFailed = 4002,
    CtpSubscribeFailed = 4003,
    CtpOrderFailed = 4004,
    CtpQueryFailed = 4005,
    CtpDisconnectFailed = 4006,
    CtpInvalidInstrument = 4007,
    CtpNotConnected = 4008,
    
    // ========== AI错误 5xxx ==========
    AiError = 5000,
    AiRequestFailed = 5001,
    AiParseError = 5002,
    AiInvalidResponse = 5003,
    AiRateLimited = 5004,
    AiModelNotAvailable = 5005,
    AiContextTooLong = 5006,
    
    // ========== 配置错误 6xxx ==========
    ConfigError = 6000,
    ConfigFileNotFound = 6001,
    ConfigParseError = 6002,
    ConfigInvalidValue = 6003,
    ConfigMissingKey = 6004,
    
    // ========== 缓存错误 7xxx ==========
    CacheError = 7000,
    CacheMiss = 7001,
    CacheExpired = 7002,
    CacheFull = 7003,
    
    // ========== 插件错误 8xxx ==========
    PluginError = 8000,
    PluginLoadFailed = 8001,
    PluginInitializeFailed = 8002,
    PluginNotFound = 8003,
    PluginVersionMismatch = 8004,
    PluginDependencyMissing = 8005
};

/**
 * @brief 错误信息结构
 */
struct Error {
    ErrorCode code = ErrorCode::Success;
    QString message;
    QString detail;
    QVariant context;
    
    Error() = default;
    
    Error(ErrorCode c, const QString& msg, const QString& det = {}, const QVariant& ctx = {})
        : code(c), message(msg), detail(det), context(ctx) {}
    
    /**
     * @brief 是否成功（无错误）
     */
    bool isOk() const { return code == ErrorCode::Success; }
    
    /**
     * @brief 是否有错误
     */
    bool isError() const { return code != ErrorCode::Success; }
    
    /**
     * @brief 获取错误码数值
     */
    int codeValue() const { return static_cast<int>(code); }
    
    /**
     * @brief 获取错误码名称
     */
    QString codeName() const;
    
    /**
     * @brief 创建成功结果
     */
    static Error ok() { return Error(); }
    
    /**
     * @brief 创建错误
     */
    static Error create(ErrorCode code, const QString& message, 
                       const QString& detail = {}, const QVariant& context = {}) {
        return Error(code, message, detail, context);
    }
};

/**
 * @brief 获取错误码的字符串名称
 */
inline QString errorCodeName(ErrorCode code) {
    switch (code) {
        case ErrorCode::Success: return "Success";
        case ErrorCode::Unknown: return "Unknown";
        case ErrorCode::InvalidArgument: return "InvalidArgument";
        case ErrorCode::NullPointer: return "NullPointer";
        case ErrorCode::OperationFailed: return "OperationFailed";
        case ErrorCode::Timeout: return "Timeout";
        case ErrorCode::Cancelled: return "Cancelled";
        case ErrorCode::NotInitialized: return "NotInitialized";
        case ErrorCode::AlreadyInitialized: return "AlreadyInitialized";
        
        case ErrorCode::NetworkError: return "NetworkError";
        case ErrorCode::NetworkTimeout: return "NetworkTimeout";
        case ErrorCode::NetworkDisconnected: return "NetworkDisconnected";
        case ErrorCode::NetworkInvalidResponse: return "NetworkInvalidResponse";
        case ErrorCode::NetworkServerError: return "NetworkServerError";
        case ErrorCode::NetworkNotFound: return "NetworkNotFound";
        case ErrorCode::NetworkUnauthorized: return "NetworkUnauthorized";
        case ErrorCode::NetworkRateLimited: return "NetworkRateLimited";
        
        case ErrorCode::DatabaseError: return "DatabaseError";
        case ErrorCode::DatabaseOpenFailed: return "DatabaseOpenFailed";
        case ErrorCode::DatabaseQueryFailed: return "DatabaseQueryFailed";
        case ErrorCode::DatabaseTransactionFailed: return "DatabaseTransactionFailed";
        case ErrorCode::DatabaseConnectionFailed: return "DatabaseConnectionFailed";
        case ErrorCode::DatabaseSchemaError: return "DatabaseSchemaError";
        case ErrorCode::DatabaseConstraintViolation: return "DatabaseConstraintViolation";
        
        case ErrorCode::CtpError: return "CtpError";
        case ErrorCode::CtpConnectFailed: return "CtpConnectFailed";
        case ErrorCode::CtpLoginFailed: return "CtpLoginFailed";
        case ErrorCode::CtpSubscribeFailed: return "CtpSubscribeFailed";
        case ErrorCode::CtpOrderFailed: return "CtpOrderFailed";
        case ErrorCode::CtpQueryFailed: return "CtpQueryFailed";
        case ErrorCode::CtpDisconnectFailed: return "CtpDisconnectFailed";
        case ErrorCode::CtpInvalidInstrument: return "CtpInvalidInstrument";
        case ErrorCode::CtpNotConnected: return "CtpNotConnected";
        
        case ErrorCode::AiError: return "AiError";
        case ErrorCode::AiRequestFailed: return "AiRequestFailed";
        case ErrorCode::AiParseError: return "AiParseError";
        case ErrorCode::AiInvalidResponse: return "AiInvalidResponse";
        case ErrorCode::AiRateLimited: return "AiRateLimited";
        case ErrorCode::AiModelNotAvailable: return "AiModelNotAvailable";
        case ErrorCode::AiContextTooLong: return "AiContextTooLong";
        
        case ErrorCode::ConfigError: return "ConfigError";
        case ErrorCode::ConfigFileNotFound: return "ConfigFileNotFound";
        case ErrorCode::ConfigParseError: return "ConfigParseError";
        case ErrorCode::ConfigInvalidValue: return "ConfigInvalidValue";
        case ErrorCode::ConfigMissingKey: return "ConfigMissingKey";
        
        case ErrorCode::CacheError: return "CacheError";
        case ErrorCode::CacheMiss: return "CacheMiss";
        case ErrorCode::CacheExpired: return "CacheExpired";
        case ErrorCode::CacheFull: return "CacheFull";
        
        case ErrorCode::PluginError: return "PluginError";
        case ErrorCode::PluginLoadFailed: return "PluginLoadFailed";
        case ErrorCode::PluginInitializeFailed: return "PluginInitializeFailed";
        case ErrorCode::PluginNotFound: return "PluginNotFound";
        case ErrorCode::PluginVersionMismatch: return "PluginVersionMismatch";
        case ErrorCode::PluginDependencyMissing: return "PluginDependencyMissing";
        
        default: return "UnknownErrorCode";
    }
}

inline QString Error::codeName() const {
    return errorCodeName(code);
}

} // namespace WealthPilot

#endif // WEALTHPILOT_CORE_ERRORCODE_H
