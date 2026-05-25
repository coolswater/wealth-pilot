/**
 * @file Result.h
 * @brief Result<T> 错误处理类型 - 函数式错误处理
 *
 * @details 参考 Rust 的 Result<T, E> 和 FinceptTerminal 的实现
 * 提供一种类型安全、可组合的错误处理方式，避免使用异常或错误码。
 *
 * @example
 * // 创建成功结果
 * Result<int> success = Result<int>::ok(42);
 *
 * // 创建错误结果
 * Result<int> error = Result<int>::error("Something went wrong");
 *
 * // 检查结果
 * if (success.isOk()) {
 *     int value = success.value();
 * }
 *
 * // 链式调用
 * auto result = fetchData()
 *     .andThen([](const Data& d) { return processData(d); })
 *     .map([](const ProcessedData& p) { return p.toString(); });
 */

#ifndef WEALTHPILOT_CORE_RESULT_H
#define WEALTHPILOT_CORE_RESULT_H

#include <QString>
#include <QVariant>
#include <optional>
#include <type_traits>
#include <utility>
#include <functional>

namespace WealthPilot {

/**
 * @brief 错误信息结构
 */
class Error {
public:
    Error() = default;
    explicit Error(const QString& message) : m_message(message) {}
    Error(const QString& code, const QString& message)
        : m_code(code), m_message(message) {}

    /**
     * @brief 获取错误消息
     */
    QString message() const { return m_message; }

    /**
     * @brief 获取错误码
     */
    QString code() const { return m_code; }

    /**
     * @brief 是否有错误码
     */
    bool hasCode() const { return !m_code.isEmpty(); }

    /**
     * @brief 转换为字符串
     */
    QString toString() const {
        if (hasCode()) {
            return QString("[%1] %2").arg(m_code, m_message);
        }
        return m_message;
    }

    /**
     * @brief 转换为 QVariant
     */
    QVariant toVariant() const {
        QVariantMap map;
        if (hasCode()) {
            map["code"] = m_code;
        }
        map["message"] = m_message;
        return map;
    }

private:
    QString m_code;
    QString m_message;
};

/**
 * @brief Result<T> - 函数式错误处理类型
 * @tparam T 成功时的值类型
 */
template <typename T>
class Result {
public:
    /**
     * @brief 创建成功结果
     */
    static Result ok(const T& value) {
        return Result(value);
    }

    static Result ok(T&& value) {
        return Result(std::move(value));
    }

    /**
     * @brief 创建错误结果
     */
    static Result error(const QString& message) {
        return Result(Error(message));
    }

    static Result error(const QString& code, const QString& message) {
        return Result(Error(code, message));
    }

    static Result error(const Error& err) {
        return Result(err);
    }

    // 默认构造（错误状态）
    Result() : m_error("Unknown error") {}

    // 拷贝和移动
    Result(const Result&) = default;
    Result(Result&&) = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) = default;

    /**
     * @brief 检查是否成功
     */
    bool isOk() const { return m_value.has_value(); }

    /**
     * @brief 检查是否错误
     */
    bool isError() const { return !m_value.has_value(); }

    /**
     * @brief 获取值（成功时）
     * @warning 如果是错误状态，会抛出异常
     */
    const T& value() const& {
        if (isError()) {
            throw std::runtime_error(m_error.toString().toStdString());
        }
        return m_value.value();
    }

    T& value() & {
        if (isError()) {
            throw std::runtime_error(m_error.toString().toStdString());
        }
        return m_value.value();
    }

    T&& value() && {
        if (isError()) {
            throw std::runtime_error(m_error.toString().toStdString());
        }
        return std::move(m_value.value());
    }

    /**
     * @brief 获取值或默认值
     */
    T valueOr(T defaultValue) const {
        return isOk() ? m_value.value() : std::move(defaultValue);
    }

    /**
     * @brief 获取错误信息
     */
    const Error& error() const { return m_error; }

    /**
     * @brief 获取错误消息
     */
    QString errorMessage() const { return m_error.message(); }

    /**
     * @brief 获取错误码
     */
    QString errorCode() const { return m_error.code(); }

    /**
     * @brief 转换为 bool
     */
    explicit operator bool() const { return isOk(); }

    /**
     * @brief 解引用操作符
     */
    const T& operator*() const& { return value(); }
    T& operator*() & { return value(); }
    T&& operator*() && { return std::move(value()); }

    const T* operator->() const { return &value(); }
    T* operator->() { return &value(); }

    /**
     * @brief map - 转换成功值
     */
    template <typename F>
    auto map(F&& f) const -> Result<std::invoke_result_t<F, const T&>> {
        using U = std::invoke_result_t<F, const T&>;
        if (isOk()) {
            return Result<U>::ok(std::invoke(std::forward<F>(f), value()));
        }
        return Result<U>::error(m_error);
    }

    /**
     * @brief andThen - 链式调用
     */
    template <typename F>
    auto andThen(F&& f) const -> std::invoke_result_t<F, const T&> {
        using ResultType = std::invoke_result_t<F, const T&>;
        if (isOk()) {
            return std::invoke(std::forward<F>(f), value());
        }
        return ResultType::error(m_error);
    }

    /**
     * @brief orElse - 错误处理
     */
    template <typename F>
    Result orElse(F&& f) const {
        if (isError()) {
            return std::invoke(std::forward<F>(f), m_error);
        }
        return *this;
    }

    /**
     * @brief match - 模式匹配
     */
    template <typename OnOk, typename OnError>
    auto match(OnOk&& onOk, OnError&& onError) const
        -> std::common_type_t<std::invoke_result_t<OnOk, const T&>,
                              std::invoke_result_t<OnError, const Error&>> {
        if (isOk()) {
            return std::invoke(std::forward<OnOk>(onOk), value());
        }
        return std::invoke(std::forward<OnError>(onError), m_error);
    }

private:
    explicit Result(const T& value) : m_value(value) {}
    explicit Result(T&& value) : m_value(std::move(value)) {}
    explicit Result(const Error& error) : m_error(error) {}

    std::optional<T> m_value;
    Error m_error;
};

/**
 * @brief Result<void> 特化 - 无返回值的操作
 */
template <>
class Result<void> {
public:
    static Result ok() { return Result(true); }
    static Result error(const QString& message) { return Result(Error(message)); }
    static Result error(const QString& code, const QString& message) {
        return Result(Error(code, message));
    }
    static Result error(const Error& err) { return Result(err); }

    Result() : m_error("Unknown error") {}

    bool isOk() const { return m_success; }
    bool isError() const { return !m_success; }

    void value() const {}

    const Error& error() const { return m_error; }
    QString errorMessage() const { return m_error.message(); }
    QString errorCode() const { return m_error.code(); }

    explicit operator bool() const { return m_success; }

    template <typename F>
    auto andThen(F&& f) const -> std::invoke_result_t<F> {
        using ResultType = std::invoke_result_t<F>;
        if (isOk()) {
            return std::invoke(std::forward<F>(f));
        }
        return ResultType::error(m_error);
    }

private:
    explicit Result(bool success) : m_success(success) {}
    explicit Result(const Error& error) : m_success(false), m_error(error) {}

    bool m_success = false;
    Error m_error;
};

// 便捷宏定义
#define TRY(expr)                                                  \
    ({                                                             \
        auto _result = (expr);                                     \
        if (_result.isError()) {                                   \
            return std::decay_t<decltype(_result)>::error(         \
                _result.error());                                  \
        }                                                          \
        std::move(_result.value());                                \
    })

#define TRY_VOID(expr)                                             \
    do {                                                           \
        auto _result = (expr);                                     \
        if (_result.isError()) {                                   \
            return std::decay_t<decltype(_result)>::error(         \
                _result.error());                                  \
        }                                                          \
    } while (0)

} // namespace WealthPilot

#endif // WEALTHPILOT_CORE_RESULT_H
