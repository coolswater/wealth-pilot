#include "DataHub.h"
#include "core/services/cache/CacheManager.h"
#include <QDebug>
#include <QDateTime>

namespace WealthPilot {
namespace DataHub {

// ========== 生命周期管理 ==========

void DataHub::shutdown()
{
    qDebug() << "[DataHub] Explicit shutdown called";
    
    // 停止调度器
    if (m_schedulerTimer) {
        m_schedulerTimer->stop();
    }
    
    // 清理所有订阅
    m_ownerSubscriptions.clear();
    m_patternSubscriptions.clear();
    
    // 通知 Producer 停止
    for (auto* producer : m_producers) {
        if (producer) {
            for (auto it = m_topics.begin(); it != m_topics.end(); ++it) {
                if (it->producer == producer) {
                    producer->onTopicIdle(it.key());
                }
            }
        }
    }
    
    m_topics.clear();
    m_producers.clear();
    
    qDebug() << "[DataHub] Shutdown complete";
}

// ========== 单例实现 ==========

DataHub& DataHub::instance()
{
    static DataHub instance;
    return instance;
}

DataHub::DataHub()
    : QObject(nullptr)
    , m_schedulerTimer(new QTimer(this))
{
    // 调度器每秒检查一次需要刷新的topic
    m_schedulerTimer->setInterval(1000);
    connect(m_schedulerTimer, &QTimer::timeout, 
            this, &DataHub::processScheduledRefresh);
    m_schedulerTimer->start();
    
    qDebug() << "[DataHub] Initialized";
}

DataHub::~DataHub()
{
    qDebug() << "[DataHub] Shutdown starting...";
    
    // 1. 先停止调度器（最重要！）
    if (m_schedulerTimer) {
        m_schedulerTimer->stop();
        // 确保定时器事件处理完毕
        m_schedulerTimer->deleteLater();
        m_schedulerTimer = nullptr;
    }
    
    // 2. 清理所有订阅（避免回调到已销毁的对象）
    m_ownerSubscriptions.clear();
    m_patternSubscriptions.clear();
    
    // 3. 通知所有 Producer 停止
    for (auto* producer : m_producers) {
        if (producer) {
            // 通知所有活跃 topic 变为空闲
            for (auto it = m_topics.begin(); it != m_topics.end(); ++it) {
                if (it->producer == producer && !it->subscribers.isEmpty()) {
                    producer->onTopicIdle(it.key());
                }
            }
        }
    }
    m_producers.clear();
    
    // 4. 清理 topic 状态
    m_topics.clear();
    
    qDebug() << "[DataHub] Shutdown complete";
}

// ========== 订阅实现 ==========

QMetaObject::Connection DataHub::subscribe(
    QObject* owner,
    const QString& topic,
    std::function<void(const QVariant&)> slot)
{
    if (!owner || topic.isEmpty()) {
        qWarning() << "[DataHub] Invalid subscribe parameters";
        return QMetaObject::Connection();
    }
    
    // 记录订阅
    Subscription sub;
    sub.owner = owner;
    sub.topic = topic;
    sub.slot = slot;
    sub.isPattern = false;
    m_ownerSubscriptions[owner].append(sub);
    
    // 确保topic状态存在
    auto& state = m_topics[topic];
    bool wasIdle = state.subscribers.isEmpty();
    state.subscribers.insert(owner);
    
    // 绑定owner销毁时自动取消订阅
    // 注意：不能使用 UniqueConnection 与 lambda，先断开再连接
    disconnect(owner, &QObject::destroyed, this, nullptr);
    connect(owner, &QObject::destroyed, 
            this, [this, owner]() { onOwnerDestroyed(owner); });
    
    // 如果之前空闲，触发激活
    if (wasIdle) {
        emit topicActive(topic);
        
        // 查找并通知Producer
        auto* producer = findProducerForTopic(topic);
        if (producer) {
            state.producer = producer;
            producer->onTopicActive(topic);
        }
    }
    
    // 如果有有效缓存，立即回调
    if (state.cachedValue.isValid() && !isTopicExpired(topic)) {
        slot(state.cachedValue);
    }
    
    qDebug() << "[DataHub] Subscribed:" << topic 
             << "owner:" << owner->objectName()
             << "subscribers:" << state.subscribers.size();
    
    return QMetaObject::Connection();
}

QMetaObject::Connection DataHub::subscribePattern(
    QObject* owner,
    const QString& pattern,
    [[maybe_unused]] std::function<void(const QString&, const QVariant&)> slot)
{
    if (!owner || pattern.isEmpty()) {
        return QMetaObject::Connection();
    }
    
    // 记录模式订阅
    m_patternSubscriptions[pattern].insert(owner);
    
    // 绑定销毁
    disconnect(owner, &QObject::destroyed, this, nullptr);
    connect(owner, &QObject::destroyed, 
            this, [this, owner]() { onOwnerDestroyed(owner); });
    
    qDebug() << "[DataHub] Subscribed pattern:" << pattern;
    
    return QMetaObject::Connection();
}

void DataHub::unsubscribe(QObject* owner)
{
    onOwnerDestroyed(owner);
}

void DataHub::unsubscribe(QObject* owner, const QString& topic)
{
    auto it = m_ownerSubscriptions.find(owner);
    if (it == m_ownerSubscriptions.end()) {
        return;
    }
    
    auto& subs = it.value();
    subs.erase(std::remove_if(subs.begin(), subs.end(),
        [&topic](const Subscription& s) { return s.topic == topic; }),
        subs.end());
    
    // 更新topic订阅者
    auto stateIt = m_topics.find(topic);
    if (stateIt != m_topics.end()) {
        stateIt->subscribers.remove(owner);
        
        if (stateIt->subscribers.isEmpty()) {
            emit topicIdle(topic);
            if (stateIt->producer) {
                stateIt->producer->onTopicIdle(topic);
            }
        }
    }
    
    if (subs.isEmpty()) {
        m_ownerSubscriptions.erase(it);
    }
}

// ========== 发布实现 ==========

void DataHub::publish(const QString& topic, const QVariant& value)
{
    publish(topic, value, std::chrono::milliseconds(0));
}

void DataHub::publish(const QString& topic, const QVariant& value, 
                      std::chrono::milliseconds ttl)
{
    auto& state = m_topics[topic];
    
    // 更新缓存
    state.cachedValue = value;
    state.lastPublishMs = QDateTime::currentMSecsSinceEpoch();
    state.totalPublishes++;
    state.inFlight = false;
    
    // 存入CacheManager（如果配置了TTL）
    qint64 cacheTtl = ttl.count() > 0 ? ttl.count() : state.policy.ttlMs;
    if (cacheTtl > 0) {
        // CacheManager::instance()->put(topic, value, cacheTtl);
    }
    
    // 广播信号
    emit topicUpdated(topic, value);
    
    // 通知直接订阅者
    for (auto* subscriber : state.subscribers) {
        auto it = m_ownerSubscriptions.find(subscriber);
        if (it != m_ownerSubscriptions.end()) {
            for (const auto& sub : it.value()) {
                if (sub.topic == topic && !sub.isPattern) {
                    sub.slot(value);
                }
            }
        }
    }
    
    // 通知模式订阅者
    for (auto patternIt = m_patternSubscriptions.begin(); 
         patternIt != m_patternSubscriptions.end(); ++patternIt) {
        if (matchesPattern(patternIt.key(), topic)) {
            for (auto* subscriber : patternIt.value()) {
                auto it = m_ownerSubscriptions.find(subscriber);
                if (it != m_ownerSubscriptions.end()) {
                    for (const auto& sub : it.value()) {
                        if (sub.isPattern && sub.pattern == patternIt.key()) {
                            // 模式订阅需要传递topic
                            sub.slot(value);
                        }
                    }
                }
            }
        }
    }
    
    qDebug() << "[DataHub] Published:" << topic 
             << "subscribers:" << state.subscribers.size();
}

// ========== Producer管理 ==========

void DataHub::registerProducer(IDataProducer* producer)
{
    if (!producer || m_producers.contains(producer)) {
        return;
    }
    
    m_producers.append(producer);
    
    // 绑定topic到producer
    for (const auto& pattern : producer->topicPatterns()) {
        for (auto it = m_topics.begin(); it != m_topics.end(); ++it) {
            if (matchesPattern(pattern, it.key())) {
                it->producer = producer;
            }
        }
    }
    
    qDebug() << "[DataHub] Registered producer:" << producer->metaObject()->className()
             << "patterns:" << producer->topicPatterns();
}

void DataHub::unregisterProducer(IDataProducer* producer)
{
    m_producers.removeOne(producer);
    
    // 清除绑定
    for (auto it = m_topics.begin(); it != m_topics.end(); ++it) {
        if (it->producer == producer) {
            it->producer = nullptr;
        }
    }
}

// ========== 策略管理 ==========

void DataHub::setPolicy(const QString& topic, const TopicPolicy& policy)
{
    m_topics[topic].policy = policy;
}

void DataHub::setPolicyPattern(const QString& pattern, const TopicPolicy& policy)
{
    for (auto it = m_topics.begin(); it != m_topics.end(); ++it) {
        if (matchesPattern(pattern, it.key())) {
            it->policy = policy;
        }
    }
}

// ========== 数据访问 ==========

QVariant DataHub::peek(const QString& topic) const
{
    auto it = m_topics.find(topic);
    if (it != m_topics.end()) {
        return it->cachedValue;
    }
    return QVariant();
}

void DataHub::request(const QString& topic, bool force)
{
    request(QStringList{topic}, force);
}

void DataHub::request(const QStringList& topics, bool force)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    // 按producer分组
    QHash<IDataProducer*, QStringList> grouped;
    
    for (const auto& topic : topics) {
        auto& state = m_topics[topic];
        
        // 跳过push-only topic
        if (state.policy.pushOnly) {
            continue;
        }
        
        // 检查是否需要刷新
        if (!force && !isTopicExpired(topic)) {
            continue;
        }
        
        // 检查最小间隔
        if (!force && (now - state.lastRefreshRequestMs) < state.policy.minIntervalMs) {
            continue;
        }
        
        // 检查是否已在请求中
        if (state.inFlight) {
            continue;
        }
        
        state.lastRefreshRequestMs = now;
        state.inFlight = true;
        
        auto* producer = findProducerForTopic(topic);
        if (producer) {
            grouped[producer].append(topic);
        }
    }
    
    // 调用各producer刷新
    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        it.key()->refresh(it.value());
    }
}

// ========== 统计信息 ==========

QVector<TopicStats> DataHub::stats() const
{
    QVector<TopicStats> result;
    result.reserve(m_topics.size());
    
    for (auto it = m_topics.begin(); it != m_topics.end(); ++it) {
        TopicStats s;
        s.topic = it.key();
        s.subscriberCount = it->subscribers.size();
        s.lastPublishMs = it->lastPublishMs;
        s.lastRefreshRequestMs = it->lastRefreshRequestMs;
        s.totalPublishes = it->totalPublishes;
        s.inFlight = it->inFlight;
        result.append(s);
    }
    
    return result;
}

// ========== 内部方法 ==========

void DataHub::onOwnerDestroyed(QObject* owner)
{
    auto it = m_ownerSubscriptions.find(owner);
    if (it == m_ownerSubscriptions.end()) {
        return;
    }
    
    // 清理所有订阅
    for (const auto& sub : it.value()) {
        auto stateIt = m_topics.find(sub.topic);
        if (stateIt != m_topics.end()) {
            stateIt->subscribers.remove(owner);
            
            // 检查是否变为空闲
            if (stateIt->subscribers.isEmpty()) {
                emit topicIdle(sub.topic);
                if (stateIt->producer) {
                    stateIt->producer->onTopicIdle(sub.topic);
                }
            }
        }
        
        // 清理模式订阅
        if (sub.isPattern) {
            auto patternIt = m_patternSubscriptions.find(sub.pattern);
            if (patternIt != m_patternSubscriptions.end()) {
                patternIt->remove(owner);
                if (patternIt->isEmpty()) {
                    m_patternSubscriptions.erase(patternIt);
                }
            }
        }
    }
    
    m_ownerSubscriptions.erase(it);
}

IDataProducer* DataHub::findProducerForTopic(const QString& topic) const
{
    for (auto* producer : m_producers) {
        for (const auto& pattern : producer->topicPatterns()) {
            if (matchesPattern(pattern, topic)) {
                return producer;
            }
        }
    }
    return nullptr;
}

bool DataHub::matchesPattern(const QString& pattern, const QString& topic) const
{
    // 简单的通配符匹配：只支持尾部*
    if (pattern.endsWith("*")) {
        QString prefix = pattern.left(pattern.length() - 1);
        return topic.startsWith(prefix);
    }
    return pattern == topic;
}

bool DataHub::isTopicExpired(const QString& topic) const
{
    auto it = m_topics.find(topic);
    if (it == m_topics.end()) {
        return true;
    }
    
    if (!it->cachedValue.isValid()) {
        return true;
    }
    
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    return (now - it->lastPublishMs) > it->policy.ttlMs;
}

void DataHub::processScheduledRefresh()
{
    QStringList toRefresh;
    
    // 收集需要刷新的topic
    for (auto it = m_topics.begin(); it != m_topics.end(); ++it) {
        // 跳过无订阅者的topic
        if (it->subscribers.isEmpty()) {
            continue;
        }
        
        // 跳过push-only
        if (it->policy.pushOnly) {
            continue;
        }
        
        // 跳过正在请求的
        if (it->inFlight) {
            continue;
        }
        
        // 检查是否过期
        if (isTopicExpired(it.key())) {
            toRefresh.append(it.key());
        }
    }
    
    // 批量请求
    if (!toRefresh.isEmpty()) {
        request(toRefresh, false);
    }
}

} // namespace DataHub
} // namespace WealthPilot