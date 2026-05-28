/**
 * @file BackpressurePolicy.cpp
 * @brief DataHub 背压策略实现
 */

#include "BackpressurePolicy.h"
#include "shared/utils/Logger.h"
#include <QMutexLocker>
#include <algorithm>

namespace WealthPilot {
namespace DataHub {

// ========== BackpressureController ==========

BackpressureController::BackpressureController(const BackpressureConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
    LOG_DEBUG(QString("[Backpressure] Created controller with mode=%1, queueSize=%2")
              .arg(static_cast<int>(config.type))
              .arg(config.maxQueueSize));
}

BackpressureController::~BackpressureController()
{
    LOG_DEBUG(QString("[Backpressure] Destroyed controller, stats: received=%1, dropped=%2, delivered=%3")
              .arg(m_stats.totalReceived)
              .arg(m_stats.totalDropped)
              .arg(m_stats.totalDelivered));
}

bool BackpressureController::processMessage(const QString& topic, const QVariant& value, int priority)
{
    QMutexLocker locker(&m_mutex);
    
    m_stats.totalReceived++;
    
    // 构建消息项
    MessageItem item;
    item.topic = topic;
    item.value = value;
    item.timestamp = QDateTime::currentMSecsSinceEpoch();
    item.priority = priority;
    
    bool accepted = false;
    
    switch (m_config.type) {
    case BackpressureType::None:
        // 直通模式：直接投递
        deliverMessage(item);
        accepted = true;
        break;
        
    case BackpressureType::Buffer:
        accepted = handleBufferMode(item);
        break;
        
    case BackpressureType::DropOldest:
    case BackpressureType::DropNewest:
        accepted = handleDropMode(item);
        break;
        
    case BackpressureType::Sample:
        // 采样模式：先检查采样条件
        if (handleSampleMode(topic)) {
            deliverMessage(item);
            accepted = true;
        } else {
            m_stats.totalDropped++;
            emit messageDropped(topic);
        }
        break;
        
    case BackpressureType::Throttle:
        // 限流模式：检查速率限制
        if (handleThrottleMode(topic)) {
            deliverMessage(item);
            accepted = true;
        } else {
            m_stats.totalDropped++;
            emit messageDropped(topic);
        }
        break;
    }
    
    // 更新统计
    m_stats.currentQueueSize = m_queue.size();
    if (m_queue.size() > m_stats.maxQueueSizeReached) {
        m_stats.maxQueueSizeReached = m_queue.size();
    }
    
    if (m_stats.totalReceived > 0) {
        m_stats.dropRate = static_cast<double>(m_stats.totalDropped) / m_stats.totalReceived;
    }
    
    return accepted;
}

bool BackpressureController::handleBufferMode(const MessageItem& item)
{
    // 队列满时等待（通过定时器分批投递）
    if (m_queue.size() >= m_config.maxQueueSize) {
        // 突发保护：队列满时立即投递一部分
        if (m_config.enableBurstProtection) {
            int burstDrop = std::min(m_config.burstThreshold, m_queue.size());
            for (int i = 0; i < burstDrop; ++i) {
                deliverMessage(m_queue.dequeue());
            }
            emit queueOverflow(item.topic, m_queue.size());
        }
    }
    
    // 添加到队列
    m_queue.enqueue(item);
    
    // 定时投递（每次投递一批）
    int batchSize = std::min(10, m_queue.size());
    for (int i = 0; i < batchSize && !m_queue.isEmpty(); ++i) {
        deliverMessage(m_queue.dequeue());
    }
    
    return true;
}

bool BackpressureController::handleDropMode(const MessageItem& item)
{
    bool accepted = true;
    
    if (m_queue.size() >= m_config.maxQueueSize) {
        // 队列满时丢弃
        m_stats.totalDropped++;
        
        if (m_config.type == BackpressureType::DropOldest) {
            // 丢弃最旧的（队列头部）
            m_queue.dequeue();
            m_queue.enqueue(item);
            emit messageDropped(item.topic);
        } else {
            // 丢弃最新的（不添加）
            accepted = false;
            emit messageDropped(item.topic);
        }
        
        emit queueOverflow(item.topic, m_queue.size());
    } else {
        m_queue.enqueue(item);
    }
    
    // 投递队列中的消息
    if (!m_queue.isEmpty()) {
        deliverMessage(m_queue.dequeue());
    }
    
    return accepted;
}

bool BackpressureController::handleSampleMode(const QString& topic)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 lastTime = m_lastSampleTime.value(topic, 0);
    
    // 按时间间隔采样
    if (now - lastTime >= m_config.sampleIntervalMs) {
        m_lastSampleTime[topic] = now;
        return true;
    }
    
    // 次数采样（每隔N个消息采样一次）
    int count = m_sampleCounter.value(topic, 0) + 1;
    m_sampleCounter[topic] = count;
    
    if (count % 10 == 0) {  // 每10个消息采样一次
        return true;
    }
    
    return false;
}

bool BackpressureController::handleThrottleMode(const QString& topic)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 lastTime = m_lastDeliverTime.value(topic, 0);
    
    // 计算最小间隔
    int minIntervalMs = 1000 / m_config.throttleRatePerSecond;
    
    if (now - lastTime >= minIntervalMs) {
        m_lastDeliverTime[topic] = now;
        m_deliverCount[topic] = 1;
        return true;
    }
    
    return false;
}

void BackpressureController::deliverMessage(const MessageItem& item)
{
    m_stats.totalDelivered++;
    emit messageReady(item.topic, item.value);
}

void BackpressureController::setConfig(const BackpressureConfig& config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;
    m_queue.clear();
    m_lastSampleTime.clear();
    m_sampleCounter.clear();
}

BackpressureConfig BackpressureController::config() const
{
    QMutexLocker locker(&m_mutex);
    return m_config;
}

int BackpressureController::queueSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_queue.size();
}

void BackpressureController::clear()
{
    QMutexLocker locker(&m_mutex);
    m_queue.clear();
}

BackpressureController::Statistics BackpressureController::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats;
}

// ========== BackpressureManager ==========

BackpressureManager& BackpressureManager::instance()
{
    static BackpressureManager instance;
    return instance;
}

BackpressureManager::BackpressureManager()
    : QObject(nullptr)
{
    // 默认配置：缓冲模式，队列大小100
    m_defaultConfig.type = BackpressureType::Buffer;
    m_defaultConfig.maxQueueSize = 100;
    m_defaultConfig.enableBurstProtection = true;
    
    LOG_DEBUG("[BackpressureManager] Initialized with default buffer mode");
}

BackpressureManager::~BackpressureManager()
{
    LOG_DEBUG("[BackpressureManager] Destroyed");
}

void BackpressureManager::setPolicy(const QString& topic, const BackpressureConfig& config)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_controllers.contains(topic)) {
        m_controllers[topic] = std::make_unique<BackpressureController>(config);
        connect(m_controllers[topic].get(), &BackpressureController::messageReady,
                this, &BackpressureManager::messageReady);
        connect(m_controllers[topic].get(), &BackpressureController::messageDropped,
                this, &BackpressureManager::messageDropped);
    } else {
        m_controllers[topic]->setConfig(config);
    }
    
    LOG_DEBUG(QString("[BackpressureManager] Set policy for topic: %1 (type=%2)")
              .arg(topic)
              .arg(static_cast<int>(config.type)));
}

void BackpressureManager::setPatternPolicy(const QString& pattern, const BackpressureConfig& config)
{
    QMutexLocker locker(&m_mutex);
    m_patternPolicies[pattern] = config;
    
    LOG_DEBUG(QString("[BackpressureManager] Set pattern policy: %1").arg(pattern));
}

bool BackpressureManager::processMessage(const QString& topic, const QVariant& value, int priority)
{
    auto* controller = getController(topic);
    if (controller) {
        return controller->processMessage(topic, value, priority);
    }
    
    // 使用默认控制器
    QMutexLocker locker(&m_mutex);
    if (!m_controllers.contains("_default_")) {
        m_controllers["_default_"] = std::make_unique<BackpressureController>(m_defaultConfig);
        connect(m_controllers["_default_"].get(), &BackpressureController::messageReady,
                this, &BackpressureManager::messageReady);
    }
    
    return m_controllers["_default_"]->processMessage(topic, value, priority);
}

BackpressureController* BackpressureManager::getController(const QString& topic)
{
    QMutexLocker locker(&m_mutex);
    
    // 直接匹配
    if (m_controllers.contains(topic)) {
        return m_controllers[topic].get();
    }
    
    // 模式匹配
    for (auto it = m_patternPolicies.begin(); it != m_patternPolicies.end(); ++it) {
        QString pattern = it.key();
        
        // 简单通配符匹配
        if (pattern.endsWith("*")) {
            QString prefix = pattern.left(pattern.length() - 1);
            if (topic.startsWith(prefix)) {
                // 创建 topic 专属控制器
                m_controllers[topic] = std::make_unique<BackpressureController>(it.value());
                connect(m_controllers[topic].get(), &BackpressureController::messageReady,
                        this, &BackpressureManager::messageReady);
                connect(m_controllers[topic].get(), &BackpressureController::messageDropped,
                        this, &BackpressureManager::messageDropped);
                return m_controllers[topic].get();
            }
        }
    }
    
    return nullptr;
}

} // namespace DataHub
} // namespace WealthPilot