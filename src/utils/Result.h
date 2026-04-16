/**
 * @file Result.h
 * @brief 结果类型 - 统一的错误处理机制
 * @author WealthPilot Team
 * @version 2.0.0
 * 
 * @details 参考 Rust 的 Result<T, E> 设计
 * 用于函数返回值，明确区分成功和失败
 * 
 * @example
 * @code
 * Result<Quote> result = DataService::fetchQuote("SH600000");
 * if (result.isOk()) {
 *     Quote quote = result.unwrap();
 * } else {
 *     qWarning() << result.error().message;
 * }
 * @endcode
 */

#ifndef WEALTHPILOT_UTILS_RESULT_H
#define WEALTHPILOT_UTILS_RESULT_H

#include "core/base/ErrorCode.h"
#include <QString>
#include <QVariant>
#include <optional>
#include <type_traits>

namespace WealthPilot {

/**
 * @brief 结果类型模板
 * @tparam T 成功时的值类型
 * 
 * @details 使用方式：
 * - Result<T>::ok(value) 创建成功结果
 * - Result<T>::err(code, message) 创建错误结果
 * - isOk() / isError() 检查状态
 * - unwrap() / unwrapOr() 获取值
 */
template<typename T>
class Result
{
public:
    // ========== 构造方法 ==========

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
     * @brief 创建错误结果
     */
    static Result<T> err(ErrorCode code, const QString& message = {}, 
                         const QString& detail = {}, const QVariant& context = {}) {
        Result<T> r;
        r.m_error = Error(code, message, detail, context);
        r.m_ok = false;
        return r;
    }

    /**
     * @brief 从 Error 创建错误结果
     */
    static Result<T> fromError(const Error& error) {
        Result<T> r;
        r.m_error = error;
        r.m_ok = false;
        return r;
    }

    // ========== 状态检查 ==========

    /**
     * @brief 是否成功
     */
    bool isOk() const { return m_ok; }

    /**
     * @brief 是否失败
     */
    bool isError() const { return !m_ok; }

    /**
     * @brief bool 转换（成功为 true）
     */
    explicit operator bool() const { return m_ok; }

    // ========== 值访问 ==========

    /**
     * @brief 获取值（成功时）
     * @warning 如果是错误结果，行为未定义
     */
    const T& unwrap() const& {
        return m_value.value();
    }

    /**
     * @brief 获取值（成功时，移动语义）
     */
    T&& unwrap() && {
        return std::move(m_value.value());
    }

    /**
     * @brief 获取值或默认值
     */
    T unwrapOr(const T& defaultValue) const {
        return m_ok ? m_value.value() : defaultValue;
    }

    /**
     * @brief 获取值（通过函数提供默认值）
     */
    template<typename F>
    T unwrapOrElse(F&& f) const {
        return m_ok ? m_value.value() : f();
    }

    /**
     * @brief 获取值指针（失败为空）
     */
    const T* operator->() const {
        return m_ok ? &m_value.value() : nullptr;
    }

    /**
     * @brief 获取值引用（失败抛异常）
     * @throws 如果是错误结果
     */
    const T& expect(const QString& message) const {
        if (!m_ok) {
            throw std::runtime_error(message.toStdString());
        }
        return m_value.value();
    }

    // ========== 错误访问 ==========

    /**
     * @brief 获取错误信息
     */
    const Error& error() const& {
        return m_error;
    }

    /**
     * @brief 获取错误码
     */
    ErrorCode errorCode() const {
        return m_error.code;
    }

    /**
     * @brief 获取错误消息
     */
    QString errorMessage() const {
        return m_error.message;
    }

    // ========== 转换 ==========

    /**
     * @brief 映射成功值
     */
    template<typename U>
    Result<U> map(std::function<U(const T&)> f) const {
        if (m_ok) {
            return Result<U>::ok(f(m_value.value()));
        }
        return Result<U>::fromError(m_error);
    }

    /**
     * @brief 映射错误
     */
    Result<T> mapError(std::function<Error(const Error&)> f) const {
        if (!m_ok) {
            return Result<T>::fromError(f(m_error));
        }
        return *this;
    }

private:
    Result() = default;

    std::optional<T> m_value;   ///< 成功时的值
    Error m_error;              ///< 失败时的错误
    bool m_ok = false;          ///< 是否成功
};

// ========== 特化：void 类型 ==========

/**
 * @brief void 类型的 Result，只表示成功/失败
 */
template<>
class Result<void>
{
public:
    static Result<void> ok() {
        Result<void> r;
        r.m_ok = true;
        return r;
    }

    static Result<void> err(ErrorCode code, const QString& message = {},
                            const QString& detail = {}, const QVariant& context = {}) {
        Result<void> r;
        r.m_error = Error(code, message, detail, context);
        r.m_ok = false;
        return r;
    }

    static Result<void> fromError(const Error& error) {
        Result<void> r;
        r.m_error = error;
        r.m_ok = false;
        return r;
    }

    bool isOk() const { return m_ok; }
    bool isError() const { return !m_ok; }
    explicit operator bool() const { return m_ok; }

    const Error& error() const& { return m_error; }
    ErrorCode errorCode() const { return m_error.code; }
    QString errorMessage() const { return m_error.message; }

private:
    Error m_error;
    bool m_ok = false;
};

// ========== 宏 ==========

/**
 * @brief 尝试执行表达式，失败则提前返回错误
 */
#define TRY(expr) \
    ({ \
        auto _result = (expr); \
        if (_result.isError()) { \
            return _result; \
        } \
        _result.unwrap(); \
    })

/**
 * @brief 尝试执行表达式，失败则返回默认值
 */
#define TRY_OR(expr, defaultVal) \
    ({ \
        auto _result = (expr); \
        if (_result.isError()) { \
            return defaultVal; \
        } \
        _result.unwrap(); \
    })

} // namespace WealthPilot

// 便捷导入，避免全局命名空间污染
using WealthPilot::Result;
using WealthPilot::Error;
using WealthPilot::ErrorCode;

#endif // WEALTHPILOT_UTILS_RESULT_H