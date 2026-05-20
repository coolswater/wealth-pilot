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
    Success = 0,                    ///< 成功
    Unknown = 1000,                 ///< 未知错误
    InvalidArgument = 1001,         ///< 无效参数
    NullPointer = 1002,             ///< 空指针
    OperationFailed = 1003,         ///< 操作失败
    Timeout = 1004,                 ///< 超时
    Cancelled = 1005,               ///< 已取消
    NotInitialized = 1006,          ///< 未初始化
    AlreadyInitialized = 1007,      ///< 已初始化
    
    // ========== 网络错误 2xxx ==========
    NetworkError = 2000,            ///< 网络错误
    NetworkTimeout = 2001,          ///< 网络超时
    NetworkDisconnected = 2002,     ///< 网络断开
    NetworkInvalidResponse = 2003,  ///< 无效响应
    NetworkServerError = 2004,      ///< 服务器错误
    NetworkNotFound = 2005,         ///< 资源未找到
    NetworkUnauthorized = 2006,     ///< 未授权
    NetworkRateLimited = 2007,      ///< 请求限流
    
    // ========== 数据库错误 3xxx ==========
    DatabaseError = 3000,           ///< 数据库错误
    DatabaseOpenFailed = 3001,      ///< 打开失败
    DatabaseQueryFailed = 3002,     ///< 查询失败
    DatabaseTransactionFailed = 3003, ///< 事务失败
    DatabaseConnectionFailed = 3004,  ///< 连接失败
    DatabaseSchemaError = 3005,       ///< 模式错误
    DatabaseConstraintViolation = 3006, ///< 约束冲突
    
    // ========== CTP错误 4xxx ==========
    CtpError = 4000,                ///< CTP错误
    CtpConnectFailed = 4001,        ///< 连接失败
    CtpLoginFailed = 4002,          ///< 登录失败
    CtpSubscribeFailed = 4003,      ///< 订阅失败
    CtpOrderFailed = 4004,          ///< 下单失败
    CtpQueryFailed = 4005,          ///< 查询失败
    CtpDisconnectFailed = 4006,     ///< 断开失败
    CtpInvalidInstrument = 4007,    ///< 无效合约
    CtpNotConnected = 4008,         ///< 未连接
    
    // ========== AI错误 5xxx ==========
    AiError = 5000,                 ///< AI错误
    AiRequestFailed = 5001,         ///< 请求失败
    AiParseError = 5002,            ///< 解析错误
    AiInvalidResponse = 5003,       ///< 无效响应
    AiRateLimited = 5004,           ///< 请求限流
    AiModelNotAvailable = 5005,     ///< 模型不可用
    AiContextTooLong = 5006,        ///< 上下文过长
    
    // ========== 配置错误 6xxx ==========
    ConfigError = 6000,             ///< 配置错误
    ConfigFileNotFound = 6001,      ///< 文件未找到
    ConfigParseError = 6002,        ///< 解析错误
    ConfigInvalidValue = 6003,      ///< 无效值
    ConfigMissingKey = 6004,        ///< 缺少键
    
    // ========== 缓存错误 7xxx ==========
    CacheError = 7000,              ///< 缓存错误
    CacheMiss = 7001,               ///< 缓存未命中
    CacheExpired = 7002,            ///< 缓存过期
    CacheFull = 7003,               ///< 缓存已满
    
    // ========== 插件错误 8xxx ==========
    PluginError = 8000,             ///< 插件错误
    PluginLoadFailed = 8001,        ///< 加载失败
    PluginInitializeFailed = 8002,  ///< 初始化失败
    PluginNotFound = 8003,          ///< 插件未找到
    PluginVersionMismatch = 8004,   ///< 版本不匹配
    PluginDependencyMissing = 8005  ///< 依赖缺失
};

/**
 * @brief 错误信息结构（带错误码）
 */
struct ErrorInfo {
    ErrorCode code = ErrorCode::Success;  ///< 错误码
    QString message;                       ///< 错误消息
    QString detail;                        ///< 详细信息
    QVariant context;                      ///< 上下文数据
    
    ErrorInfo() = default;
    
    ErrorInfo(ErrorCode c, const QString& msg, const QString& det = {}, const QVariant& ctx = {})
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
    static ErrorInfo ok() { return ErrorInfo(); }
    
    /**
     * @brief 创建错误
     */
    static ErrorInfo create(ErrorCode code, const QString& message, 
                       const QString& detail = {}, const QVariant& context = {}) {
        return ErrorInfo(code, message, detail, context);
    }
};

/**
 * @brief 获取错误码字符串名称
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

inline QString ErrorInfo::codeName() const {
    return errorCodeName(code);
}

} // namespace WealthPilot

#endif // WEALTHPILOT_CORE_ERRORCODE_H