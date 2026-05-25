/**
 * @file EventBus.h
 * @brief 事件总线 - 模块间解耦通信
 *
 * @details 提供发布/订阅模式的事件总线，实现模块间的松耦合通信。
 * 参考 FinceptTerminal 的 EventBus 实现。
 *
 * @example
 * // 发布事件
 * EventBus::instance().publish("market_update", quoteData);
 *
 * // 订阅事件
 * EventBus::instance().subscribe("market_update", this, &MyClass::onMarketUpdate);
 *
 * // 取消订阅
 * EventBus::instance().unsubscribe("market_update", this);
 */

#ifndef WEALTHPILOT_SHARED_UTILS_EVENTBUS_H
#define WEALTHPILOT_SHARED_UTILS_EVENTBUS_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QVariant>
#include <QMutex>
#include <QMutexLocker>
#include <functional>
#include <memory>
#include <vector>
#include <algorithm>
#include <map>

namespace WealthPilot {

/**
 * @brief 事件数据封装
 */
class Event {
public:
    Event() = default;
    explicit Event(const QString& type) : m_type(type) {}
    Event(const QString& type, const QVariant& data) : m_type(type), m_data(data) {}

    QString type() const { return m_type; }
    QVariant data() const { return m_data; }

    template <typename T>
    T dataAs() const { return m_data.value<T>(); }

    qint64 timestamp() const { return m_timestamp; }
    void setTimestamp(qint64 ts) { m_timestamp = ts; }

private:
    QString m_type;
    QVariant m_data;
    qint64 m_timestamp = 0;
};

/**
 * @brief 事件处理器基类
 */
class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual void handle(const Event& event) = 0;
    virtual QObject* receiver() const = 0;
};

/**
 * @brief 模板事件处理器
 */
template <typename Receiver, typename... Args>
class MemberEventHandler : public EventHandler {
public:
    using MemberFunction = void (Receiver::*)(Args...);

    MemberEventHandler(Receiver* receiver, MemberFunction function)
        : m_receiver(receiver), m_function(function) {}

    void handle(const Event& event) override {
        if (m_receiver && m_function) {
            invokeHelper(event, std::index_sequence_for<Args...>{});
        }
    }

    QObject* receiver() const override { return m_receiver; }

private:
    template <std::size_t... Is>
    void invokeHelper(const Event& event, std::index_sequence<Is...>) {
        if constexpr (sizeof...(Args) == 0) {
            (m_receiver->*m_function)();
        } else if constexpr (sizeof...(Args) == 1) {
            (m_receiver->*m_function)(event.dataAs<std::tuple_element_t<0, std::tuple<Args...>>>());
        } else {
            // 多参数情况，从 QVariantMap 中提取
            auto map = event.data().toMap();
            (m_receiver->*m_function)(map.values()[Is].value<Args>()...);
        }
    }

    Receiver* m_receiver;
    MemberFunction m_function;
};

/**
 * @brief Lambda 事件处理器
 */
class LambdaEventHandler : public EventHandler {
public:
    using HandlerFunction = std::function<void(const Event&)>;

    explicit LambdaEventHandler(HandlerFunction function) : m_function(std::move(function)) {}

    void handle(const Event& event) override {
        if (m_function) {
            m_function(event);
        }
    }

    QObject* receiver() const override { return nullptr; }

private:
    HandlerFunction m_function;
};

/**
 * @brief 事件总线 - 单例模式
 */
class EventBus : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static EventBus& instance() {
        static EventBus instance;
        return instance;
    }

    /**
     * @brief 发布事件
     */
    void publish(const QString& eventType, const QVariant& data = QVariant()) {
        Event event(eventType, data);
        event.setTimestamp(QDateTime::currentMSecsSinceEpoch());

        QMutexLocker locker(&m_mutex);

        auto it = m_handlers.find(eventType);
        if (it != m_handlers.end()) {
            const auto& handlers = it->second;  // std::map 使用 ->second
            locker.unlock();

            for (const auto& handler : handlers) {
                handler->handle(event);
            }
        }

        emit eventPublished(eventType, data);
    }

    /**
     * @brief 发布事件（Event 对象）
     */
    void publish(const Event& event) {
        QMutexLocker locker(&m_mutex);

        auto it = m_handlers.find(event.type());
        if (it != m_handlers.end()) {
            const auto& handlers = it->second;  // std::map 使用 ->second
            locker.unlock();

            for (const auto& handler : handlers) {
                handler->handle(event);
            }
        }

        emit eventPublished(event.type(), event.data());
    }

    /**
     * @brief 订阅事件（成员函数）
     */
    template <typename Receiver, typename... Args>
    void subscribe(const QString& eventType, Receiver* receiver,
                   void (Receiver::*function)(Args...)) {
        QMutexLocker locker(&m_mutex);

        auto handler = std::make_unique<MemberEventHandler<Receiver, Args...>>(receiver, function);
        // std::map 的 operator[] 会创建空 vector
        m_handlers[eventType].push_back(std::move(handler));

        // 记录接收者到事件类型的映射
        m_receiverEvents[receiver].append(eventType);
    }

    /**
     * @brief 订阅事件（Lambda）
     */
    void subscribe(const QString& eventType,
                   std::function<void(const Event&)> handler) {
        QMutexLocker locker(&m_mutex);
        m_handlers[eventType].push_back(std::make_unique<LambdaEventHandler>(std::move(handler)));
    }

    /**
     * @brief 取消订阅
     */
    void unsubscribe(const QString& eventType, QObject* receiver) {
        QMutexLocker locker(&m_mutex);

        auto it = m_handlers.find(eventType);
        if (it != m_handlers.end()) {
            auto& handlers = it->second;  // std::map 使用 ->second
            // 使用 std::remove_if 和 erase
            auto removeIt = std::remove_if(handlers.begin(), handlers.end(),
                [receiver](std::unique_ptr<EventHandler>& h) {
                    return h->receiver() == receiver;
                });
            handlers.erase(removeIt, handlers.end());
        }

        // 从接收者映射中移除
        if (m_receiverEvents.contains(receiver)) {
            m_receiverEvents[receiver].removeAll(eventType);
            if (m_receiverEvents[receiver].isEmpty()) {
                m_receiverEvents.remove(receiver);
            }
        }
    }

    /**
     * @brief 取消所有订阅
     */
    void unsubscribeAll(QObject* receiver) {
        QMutexLocker locker(&m_mutex);

        if (m_receiverEvents.contains(receiver)) {
            auto eventTypes = m_receiverEvents[receiver];
            for (const auto& eventType : eventTypes) {
                if (m_handlers.contains(eventType)) {
                    auto& handlers = m_handlers[eventType];
                    handlers.erase(
                        std::remove_if(handlers.begin(), handlers.end(),
                                       [receiver](const std::unique_ptr<EventHandler>& h) {
                                           return h->receiver() == receiver;
                                       }),
                        handlers.end());
                }
            }
            m_receiverEvents.remove(receiver);
        }
    }

    /**
     * @brief 检查是否有订阅者
     */
    bool hasSubscribers(const QString& eventType) const {
        QMutexLocker locker(&m_mutex);
        auto it = m_handlers.find(eventType);
        return it != m_handlers.end() && !it->second.empty();
    }

    /**
     * @brief 获取订阅者数量
     */
    int subscriberCount(const QString& eventType) const {
        QMutexLocker locker(&m_mutex);
        auto it = m_handlers.find(eventType);
        return it != m_handlers.end() ? static_cast<int>(it->second.size()) : 0;
    }

    /**
     * @brief 清除所有订阅
     */
    void clear() {
        QMutexLocker locker(&m_mutex);
        m_handlers.clear();
        m_receiverEvents.clear();
    }

signals:
    /**
     * @brief 事件发布信号
     */
    void eventPublished(const QString& eventType, const QVariant& data);

private:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    mutable QMutex m_mutex;
    std::map<QString, std::vector<std::unique_ptr<EventHandler>>> m_handlers;
    QMap<QObject*, QList<QString>> m_receiverEvents;
};

// 事件类型常量
namespace EventTypes {
    // 市场数据事件
    constexpr const char* MARKET_QUOTE_UPDATED = "market:quote_updated";
    constexpr const char* MARKET_BARS_RECEIVED = "market:bars_received";
    constexpr const char* MARKET_TICK_RECEIVED = "market:tick_received";

    // 交易事件
    constexpr const char* ORDER_SUBMITTED = "trading:order_submitted";
    constexpr const char* ORDER_FILLED = "trading:order_filled";
    constexpr const char* ORDER_CANCELLED = "trading:order_cancelled";
    constexpr const char* POSITION_OPENED = "trading:position_opened";
    constexpr const char* POSITION_CLOSED = "trading:position_closed";

    // 账户事件
    constexpr const char* ACCOUNT_BALANCE_CHANGED = "account:balance_changed";
    constexpr const char* ACCOUNT_LOGGED_IN = "account:logged_in";
    constexpr const char* ACCOUNT_LOGGED_OUT = "account:logged_out";

    // 系统事件
    constexpr const char* THEME_CHANGED = "system:theme_changed";
    constexpr const char* LANGUAGE_CHANGED = "system:language_changed";
    constexpr const char* CONFIG_CHANGED = "system:config_changed";

    // 警告事件
    constexpr const char* ALERT_TRIGGERED = "alert:triggered";
    constexpr const char* PRICE_ALERT = "alert:price_alert";

    // 新闻事件
    constexpr const char* NEWS_RECEIVED = "news:received";
    constexpr const char* NEWS_IMPORTANT = "news:important";

    // AI 事件
    constexpr const char* AI_ANALYSIS_COMPLETE = "ai:analysis_complete";
    constexpr const char* AI_RECOMMENDATION = "ai:recommendation";
}

} // namespace WealthPilot

#endif // WEALTHPILOT_SHARED_UTILS_EVENTBUS_H
