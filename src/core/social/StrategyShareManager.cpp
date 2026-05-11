/**
 * @file StrategyShareManager.cpp
 * @brief 策略分享管理器实现
 */

#include "StrategyShareManager.h"
#include "../utils/Logger.h"
#include <QUuid>
#include <QSettings>
#include <algorithm>

StrategyShareManager* StrategyShareManager::instance()
{
    static StrategyShareManager* inst = new StrategyShareManager();
    return inst;
}

StrategyShareManager::StrategyShareManager(QObject* parent)
    : QObject(parent)
{
    // 加载用户ID
    QSettings settings("WealthPilot", "User");
    m_currentUserId = settings.value("userId", QUuid::createUuid().toString()).toString();
    settings.setValue("userId", m_currentUserId);

    LOG_INFO(QString("StrategyShareManager initialized, userId: %1").arg(m_currentUserId));
}

// ========== 策略发布 ==========

bool StrategyShareManager::publishStrategy(const SharedStrategy& strategy)
{
    SharedStrategy newStrategy = strategy;
    newStrategy.id = QUuid::createUuid().toString();
    newStrategy.authorId = m_currentUserId;
    newStrategy.publishTime = QDateTime::currentDateTime();

    m_strategies[newStrategy.id] = newStrategy;

    emit strategyPublished(newStrategy);
    LOG_INFO(QString("Strategy published: %1 (%2)").arg(newStrategy.name).arg(newStrategy.id));

    return true;
}

bool StrategyShareManager::updateStrategy(const SharedStrategy& strategy)
{
    if (!m_strategies.contains(strategy.id)) {
        LOG_WARNING(QString("Strategy not found: %1").arg(strategy.id));
        return false;
    }

    // 检查权限
    if (m_strategies[strategy.id].authorId != m_currentUserId) {
        LOG_WARNING(QString("No permission to update strategy: %1").arg(strategy.id));
        return false;
    }

    m_strategies[strategy.id] = strategy;
    LOG_INFO(QString("Strategy updated: %1").arg(strategy.id));

    return true;
}

bool StrategyShareManager::deleteStrategy(const QString& strategyId)
{
    if (!m_strategies.contains(strategyId)) {
        return false;
    }

    // 检查权限
    if (m_strategies[strategyId].authorId != m_currentUserId) {
        LOG_WARNING(QString("No permission to delete strategy: %1").arg(strategyId));
        return false;
    }

    m_strategies.remove(strategyId);
    m_subscribers.remove(strategyId);
    m_ratings.remove(strategyId);

    LOG_INFO(QString("Strategy deleted: %1").arg(strategyId));
    return true;
}

SharedStrategy StrategyShareManager::getStrategy(const QString& strategyId) const
{
    return m_strategies.value(strategyId);
}

QVector<SharedStrategy> StrategyShareManager::getHotStrategies(int limit) const
{
    QVector<SharedStrategy> result;
    for (const auto& strategy : m_strategies) {
        if (strategy.isPublic) {
            result.append(strategy);
        }
    }

    // 按订阅数排序
    std::sort(result.begin(), result.end(),
              [](const SharedStrategy& a, const SharedStrategy& b) {
                  return a.subscribers > b.subscribers;
              });

    if (result.size() > limit) {
        result = result.mid(0, limit);
    }

    return result;
}

QVector<SharedStrategy> StrategyShareManager::getLatestStrategies(int limit) const
{
    QVector<SharedStrategy> result;
    for (const auto& strategy : m_strategies) {
        if (strategy.isPublic) {
            result.append(strategy);
        }
    }

    // 按发布时间排序
    std::sort(result.begin(), result.end(),
              [](const SharedStrategy& a, const SharedStrategy& b) {
                  return a.publishTime > b.publishTime;
              });

    if (result.size() > limit) {
        result = result.mid(0, limit);
    }

    return result;
}

QVector<SharedStrategy> StrategyShareManager::searchStrategies(const QString& keyword,
                                                               const QStringList& tags) const
{
    QVector<SharedStrategy> result;

    for (const auto& strategy : m_strategies) {
        if (!strategy.isPublic) continue;

        // 关键词匹配
        bool keywordMatch = keyword.isEmpty() ||
            strategy.name.contains(keyword, Qt::CaseInsensitive) ||
            strategy.description.contains(keyword, Qt::CaseInsensitive);

        // 标签匹配
        bool tagsMatch = tags.isEmpty();
        if (!tagsMatch) {
            for (const QString& tag : tags) {
                if (strategy.tags.contains(tag, Qt::CaseInsensitive)) {
                    tagsMatch = true;
                    break;
                }
            }
        }

        if (keywordMatch && tagsMatch) {
            result.append(strategy);
        }
    }

    return result;
}

// ========== 订阅管理 ==========

bool StrategyShareManager::subscribeStrategy(const QString& strategyId,
                                             bool autoFollow,
                                             double followRatio)
{
    if (!m_strategies.contains(strategyId)) {
        LOG_WARNING(QString("Strategy not found: %1").arg(strategyId));
        return false;
    }

    // 检查是否已订阅
    for (const Subscription& sub : m_mySubscriptions) {
        if (sub.strategyId == strategyId) {
            LOG_WARNING(QString("Already subscribed: %1").arg(strategyId));
            return false;
        }
    }

    Subscription sub;
    sub.id = QUuid::createUuid().toString();
    sub.strategyId = strategyId;
    sub.userId = m_currentUserId;
    sub.subscribeTime = QDateTime::currentDateTime();
    sub.autoFollow = autoFollow;
    sub.followRatio = followRatio;

    m_mySubscriptions.append(sub);
    m_subscribers[strategyId].append(sub);

    // 更新订阅数
    m_strategies[strategyId].subscribers++;

    emit subscriptionChanged(strategyId, true);
    LOG_INFO(QString("Subscribed to strategy: %1").arg(strategyId));

    return true;
}

bool StrategyShareManager::unsubscribeStrategy(const QString& strategyId)
{
    // 从我的订阅中移除
    for (int i = 0; i < m_mySubscriptions.size(); ++i) {
        if (m_mySubscriptions[i].strategyId == strategyId) {
            m_mySubscriptions.removeAt(i);
            break;
        }
    }

    // 从策略订阅者中移除
    if (m_subscribers.contains(strategyId)) {
        for (int i = 0; i < m_subscribers[strategyId].size(); ++i) {
            if (m_subscribers[strategyId][i].userId == m_currentUserId) {
                m_subscribers[strategyId].removeAt(i);
                break;
            }
        }
    }

    // 更新订阅数
    if (m_strategies.contains(strategyId)) {
        m_strategies[strategyId].subscribers--;
    }

    emit subscriptionChanged(strategyId, false);
    LOG_INFO(QString("Unsubscribed from strategy: %1").arg(strategyId));

    return true;
}

QVector<Subscription> StrategyShareManager::getSubscriptions() const
{
    return m_mySubscriptions;
}

QVector<Subscription> StrategyShareManager::getSubscribers(const QString& strategyId) const
{
    return m_subscribers.value(strategyId);
}

// ========== 评分系统 ==========

bool StrategyShareManager::rateStrategy(const QString& strategyId,
                                        double rating,
                                        const QString& comment)
{
    if (!m_strategies.contains(strategyId)) {
        return false;
    }

    // 检查是否已评分
    StrategyRating newRating;
    newRating.strategyId = strategyId;
    newRating.userId = m_currentUserId;
    newRating.rating = qBound(1.0, rating, 5.0);
    newRating.comment = comment;
    newRating.time = QDateTime::currentDateTime();

    // 更新或添加评分
    bool found = false;
    QVector<StrategyRating>& ratings = m_ratings[strategyId];
    for (int i = 0; i < ratings.size(); ++i) {
        if (ratings[i].userId == m_currentUserId) {
            ratings[i] = newRating;
            found = true;
            break;
        }
    }
    if (!found) {
        ratings.append(newRating);
    }

    // 更新策略评分
    updateStrategyRating(strategyId);

    emit ratingUpdated(strategyId, m_strategies[strategyId].rating);
    LOG_INFO(QString("Strategy rated: %1 -> %2").arg(strategyId).arg(rating));

    return true;
}

QVector<StrategyRating> StrategyShareManager::getRatings(const QString& strategyId) const
{
    return m_ratings.value(strategyId);
}

StrategyRating StrategyShareManager::getUserRating(const QString& strategyId,
                                                   const QString& userId) const
{
    QVector<StrategyRating> ratings = m_ratings.value(strategyId);
    for (const StrategyRating& rating : ratings) {
        if (rating.userId == userId) {
            return rating;
        }
    }
    return StrategyRating();
}

// ========== 跟单交易 ==========

void StrategyShareManager::setFollowConfig(const QString& strategyId,
                                           bool enabled,
                                           double ratio,
                                           double maxAmount)
{
    for (Subscription& sub : m_mySubscriptions) {
        if (sub.strategyId == strategyId) {
            sub.autoFollow = enabled;
            sub.followRatio = ratio;
            // maxAmount 暂不使用
            break;
        }
    }

    LOG_INFO(QString("Follow config updated: %1, enabled=%2, ratio=%3")
        .arg(strategyId).arg(enabled).arg(ratio));
}

void StrategyShareManager::processSignal(const StrategyTradeSignal& signal)
{
    emit signalReceived(signal);

    // 处理跟单
    if (m_subscribers.contains(signal.strategyId)) {
        for (const Subscription& sub : m_subscribers[signal.strategyId]) {
            if (sub.autoFollow && sub.userId != m_currentUserId) {
                executeFollowTrade(signal, sub);
            }
        }
    }

    LOG_DEBUG(QString("Signal processed: %1 %2 %3@%4")
        .arg(signal.strategyId).arg(signal.symbol).arg(signal.action).arg(signal.price));
}

QVector<StrategyTradeSignal> StrategyShareManager::getFollowRecords(const QString& strategyId) const
{
    if (strategyId.isEmpty()) {
        return m_followRecords;
    }

    QVector<StrategyTradeSignal> result;
    for (const auto& record : m_followRecords) {
        if (record.strategyId == strategyId) {
            result.append(record);
        }
    }
    return result;
}

void StrategyShareManager::updateStrategyRating(const QString& strategyId)
{
    QVector<StrategyRating> ratings = m_ratings.value(strategyId);
    if (ratings.isEmpty()) return;

    double total = 0;
    for (const StrategyRating& r : ratings) {
        total += r.rating;
    }

    m_strategies[strategyId].rating = total / ratings.size();
    m_strategies[strategyId].ratingCount = ratings.size();
}

void StrategyShareManager::executeFollowTrade(const StrategyTradeSignal& signal,
                                               const Subscription& sub)
{
    // 创建跟单信号
    StrategyTradeSignal followSignal = signal;
    followSignal.quantity = static_cast<int>(signal.quantity * sub.followRatio);
    followSignal.reason = QString("跟单策略 %1").arg(signal.strategyId);

    m_followRecords.append(followSignal);

    LOG_INFO(QString("Follow trade executed: %1 %2 %3@%4 for user %5")
        .arg(sub.strategyId).arg(signal.symbol).arg(followSignal.action)
        .arg(followSignal.price).arg(sub.userId));
}