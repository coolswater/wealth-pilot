/**
 * @file Result.h
 * @brief 结果类型 - 统一的错误处理机制
 *
 * @details 灵感来自 Rust 的 Result<T, E> 类型
 * 用于函数返回值，明确区分成功和失败
 *
 * @example
 * @code
 * Result<Quote> result = DataService::fetchQuote("SH600000");
 * if (result.isOk()) {
 *     Quote quote = result.unwrap();
 * } else {
 *     qWarning() << result.error() << result.message();
 * }
 * @endcode
 */

#ifndef RESULT_H
#define RESULT_H

#include <QString>
#include <QVariant>
#include <optional>
#include <type_traits>

/**
 * @brief 错误信息结构
 */
struct Error {
    QString code;       ///< 错误码 (如 "NET_001", "AUTH_002")
    QString message;    ///< 错误描述
    QVariant detail;    ///< 附加信息（可选）

    Error() = default;
    Error(const QString& c, const QString& msg, const QVariant& d = {})
        : code(c), message(msg), detail(d) {}

    bool isNull() const { return code.isEmpty(); }
};

/**
 * @brief 结果类型模板
 * @tparam T 成功时的值类型
 */
template<typename T>
class Result
{
public:
    // ========== 构造方式 ==========

    /**
     * @brief 创建成功结果
     */
    static Result<T> ok(const T& value) {
        Result<T> r;
        r.m_value = value;
        r.m_ok = true;
        return r;
    }

    /**
     * @brief 创建成功结果（移动语义）
     */
    static Result<T> ok(T&& value) {
        Result<T> r;
        r.m_value = std::move(value);
        r.m_ok = true;
        return r;
    }

    /**
     * @brief 创建失败结果
     */
    static Result<T> err(const QString& code, const QString& message) {
        Result<T> r;
        r.m_error = Error{code, message};
        r.m_ok = false;
        return r;
    }

    /**
     * @brief 创建失败结果（带附加信息）
     */
    static Result<T> err(const QString& code, const QString& message, const QVariant& detail) {
        Result<T> r;
        r.m_error = Error{code, message, detail};
        r.m_ok = false;
        return r;
    }

    /**
     * @brief 从 Error 创建失败结果
     */
    static Result<T> fromError(const Error& error) {
        Result<T> r;
        r.m_error = error;
        r.m_ok = false;
        return r;
    }

    // ========== 查询方法 ==========

    bool isOk() const { return m_ok; }
    bool isErr() const { return !m_ok; }
    explicit operator bool() const { return m_ok; }

    // ========== 值获取 ==========

    /**
     * @brief 获取值（需确保 isOk()）
     * @warning 如果是错误结果，行为未定义
     */
    const T& unwrap() const {
        Q_ASSERT(m_ok && "Called unwrap on an error result");
        return *m_value;
    }

    T& unwrap() {
        Q_ASSERT(m_ok && "Called unwrap on an error result");
        return *m_value;
    }

    /**
     * @brief 获取值，失败返回默认值
     */
    T unwrapOr(const T& defaultValue) const {
        return m_ok ? *m_value : defaultValue;
    }

    /**
     * @brief 安全获取值指针
     */
    const T* operator->() const { return m_ok ? &(*m_value) : nullptr; }
    T* operator->() { return m_ok ? &(*m_value) : nullptr; }

    // ========== 错误获取 ==========

    const Error& error() const {
        Q_ASSERT(!m_ok && "Called error on an ok result");
        return m_error;
    }

    const QString& errorCode() const { return m_error.code; }
    const QString& errorMessage() const { return m_error.message; }

    // ========== 链式操作 ==========

    /**
     * @brief 映射成功值
     * @tparam U 新类型
     * @param f 映射函数 T -> U
     */
    template<typename U, typename F>
    Result<U> map(F&& f) const {
        if (m_ok) {
            return Result<U>::ok(f(*m_value));
        }
        return Result<U>::fromError(m_error);
    }

    /**
     * @brief 成功时执行操作
     */
    template<typename F>
    Result<T>& inspect(F&& f) {
        if (m_ok) {
            f(*m_value);
        }
        return *this;
    }

    /**
     * @brief 失败时执行操作
     */
    template<typename F>
    Result<T>& inspectErr(F&& f) {
        if (!m_ok) {
            f(m_error);
        }
        return *this;
    }

private:
    Result() = default;

    std::optional<T> m_value;
    Error m_error;
    bool m_ok = false;
};

// ========== 特化：void 类型 ==========

template<>
class Result<void>
{
public:
    static Result<void> ok() {
        Result<void> r;
        r.m_ok = true;
        return r;
    }

    static Result<void> err(const QString& code, const QString& message) {
        Result<void> r;
        r.m_error = Error{code, message};
        r.m_ok = false;
        return r;
    }

    bool isOk() const { return m_ok; }
    bool isErr() const { return !m_ok; }
    explicit operator bool() const { return m_ok; }

    const Error& error() const { return m_error; }
    const QString& errorCode() const { return m_error.code; }
    const QString& errorMessage() const { return m_error.message; }

private:
    Error m_error;
    bool m_ok = false;
};

// ========== 错误码定义 ==========

namespace ErrorCode {
    // 网络错误
    constexpr auto NET_TIMEOUT      = "NET_001";
    constexpr auto NET_CONNECTION   = "NET_002";
    constexpr auto NET_PARSE        = "NET_003";
    constexpr auto NET_SSL          = "NET_004";

    // 数据错误
    constexpr auto DATA_NOT_FOUND   = "DATA_001";
    constexpr auto DATA_INVALID     = "DATA_002";
    constexpr auto DATA_PARSE       = "DATA_003";

    // 业务错误
    constexpr auto BIZ_AUTH         = "BIZ_001";
    constexpr auto BIZ_PERMISSION   = "BIZ_002";
    constexpr auto BIZ_LIMIT        = "BIZ_003";

    // CTP 错误
    constexpr auto CTP_CONNECT      = "CTP_001";
    constexpr auto CTP_LOGIN        = "CTP_002";
    constexpr auto CTP_ORDER        = "CTP_003";
    constexpr auto CTP_MARKET       = "CTP_004";

    // AI 错误
    constexpr auto AI_SERVICE       = "AI_001";
    constexpr auto AI_PARSE         = "AI_002";
    constexpr auto AI_LIMIT         = "AI_003";
}

#endif // RESULT_H
