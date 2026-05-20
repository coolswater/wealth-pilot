/**
 * @file FeedbackSystem.cpp
 * @brief 用户反馈系统实现
 */

#include "FeedbackSystem.h"
#include "utils/Logger.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QRandomGenerator>

namespace WealthPilot {

FeedbackSystem& FeedbackSystem::instance()
{
    static FeedbackSystem inst;
    return inst;
}

FeedbackSystem::FeedbackSystem() = default;

FeedbackSystem::~FeedbackSystem()
{
    if (m_initialized) {
        saveFeedbacks();
    }
}

void FeedbackSystem::initialize(const QString& storagePath)
{
    m_storagePath = storagePath;

    // 确保目录存在
    QDir dir(storagePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    loadFeedbacks();
    m_initialized = true;
    LOG_INFO(QString("FeedbackSystem initialized, path: %1").arg(storagePath));
}

QString FeedbackSystem::submitFeedback(const FeedbackInfo& feedback)
{
    FeedbackInfo newFeedback = feedback;
    newFeedback.id = generateId();
    newFeedback.createTime = QDateTime::currentDateTime();
    newFeedback.updateTime = newFeedback.createTime;
    newFeedback.status = FeedbackStatus::New;

    m_feedbacks.append(newFeedback);
    saveFeedbacks();

    LOG_INFO(QString("Feedback submitted: %1 - %2").arg(newFeedback.id).arg(newFeedback.title));
    emit feedbackSubmitted(newFeedback.id);

    return newFeedback.id;
}

bool FeedbackSystem::updateFeedback(const FeedbackInfo& feedback)
{
    for (int i = 0; i < m_feedbacks.size(); ++i) {
        if (m_feedbacks[i].id == feedback.id) {
            FeedbackStatus oldStatus = m_feedbacks[i].status;
            m_feedbacks[i] = feedback;
            m_feedbacks[i].updateTime = QDateTime::currentDateTime();

            if (feedback.status == FeedbackStatus::Resolved && oldStatus != FeedbackStatus::Resolved) {
                m_feedbacks[i].resolveTime = QDateTime::currentDateTime();
            }

            saveFeedbacks();

            LOG_INFO(QString("Feedback updated: %1").arg(feedback.id));
            emit feedbackUpdated(feedback.id);

            if (oldStatus != feedback.status) {
                emit feedbackStatusChanged(feedback.id, feedback.status);
            }

            return true;
        }
    }

    LOG_WARNING(QString("Feedback not found: %1").arg(feedback.id));
    return false;
}

FeedbackInfo FeedbackSystem::getFeedback(const QString& feedbackId) const
{
    for (const auto& feedback : m_feedbacks) {
        if (feedback.id == feedbackId) {
            return feedback;
        }
    }
    return FeedbackInfo();
}

QVector<FeedbackInfo> FeedbackSystem::getAllFeedbacks() const
{
    return m_feedbacks;
}

QVector<FeedbackInfo> FeedbackSystem::getFeedbacksByStatus(FeedbackStatus status) const
{
    QVector<FeedbackInfo> result;
    for (const auto& feedback : m_feedbacks) {
        if (feedback.status == status) {
            result.append(feedback);
        }
    }
    return result;
}

QVector<FeedbackInfo> FeedbackSystem::getFeedbacksByType(FeedbackCategoryType type) const
{
    QVector<FeedbackInfo> result;
    for (const auto& feedback : m_feedbacks) {
        if (feedback.type == type) {
            result.append(feedback);
        }
    }
    return result;
}

QVector<FeedbackInfo> FeedbackSystem::searchFeedbacks(const QString& keyword) const
{
    QVector<FeedbackInfo> result;
    for (const auto& feedback : m_feedbacks) {
        if (feedback.title.contains(keyword, Qt::CaseInsensitive) ||
            feedback.content.contains(keyword, Qt::CaseInsensitive)) {
            result.append(feedback);
        }
    }
    return result;
}

bool FeedbackSystem::deleteFeedback(const QString& feedbackId)
{
    for (int i = 0; i < m_feedbacks.size(); ++i) {
        if (m_feedbacks[i].id == feedbackId) {
            m_feedbacks.remove(i);
            saveFeedbacks();
            LOG_INFO(QString("Feedback deleted: %1").arg(feedbackId));
            return true;
        }
    }
    return false;
}

FeedbackStats FeedbackSystem::getStats() const
{
    FeedbackStats stats;
    stats.totalCount = m_feedbacks.size();

    for (const auto& feedback : m_feedbacks) {
        switch (feedback.status) {
            case FeedbackStatus::New: stats.newCount++; break;
            case FeedbackStatus::InProgress: stats.inProgressCount++; break;
            case FeedbackStatus::Resolved: stats.resolvedCount++; break;
            case FeedbackStatus::Closed: stats.closedCount++; break;
            default: break;
        }

        switch (feedback.type) {
            case FeedbackCategoryType::BugReport: stats.bugCount++; break;
            case FeedbackCategoryType::FeatureRequest: stats.featureCount++; break;
            case FeedbackCategoryType::Improvement: stats.improvementCount++; break;
            case FeedbackCategoryType::Question: stats.questionCount++; break;
            default: break;
        }

        // 计算平均解决时间
        if (feedback.status == FeedbackStatus::Resolved && feedback.resolveTime.isValid()) {
            qint64 hours = feedback.createTime.secsTo(feedback.resolveTime) / 3600;
            stats.avgResolutionTime += hours;
        }
    }

    if (stats.resolvedCount > 0) {
        stats.avgResolutionTime /= stats.resolvedCount;
    }

    return stats;
}

bool FeedbackSystem::exportFeedbacks(const QString& filePath, const QString& format)
{
    if (format == "json") {
        QJsonArray array;
        for (const auto& feedback : m_feedbacks) {
            QJsonObject obj;
            obj["id"] = feedback.id;
            obj["type"] = static_cast<int>(feedback.type);
            obj["priority"] = static_cast<int>(feedback.priority);
            obj["status"] = static_cast<int>(feedback.status);
            obj["title"] = feedback.title;
            obj["content"] = feedback.content;
            obj["category"] = feedback.category;
            obj["userId"] = feedback.userId;
            obj["userEmail"] = feedback.userEmail;
            obj["userVersion"] = feedback.userVersion;
            obj["userPlatform"] = feedback.userPlatform;
            obj["createTime"] = feedback.createTime.toString(Qt::ISODate);
            obj["updateTime"] = feedback.updateTime.toString(Qt::ISODate);
            if (feedback.resolveTime.isValid()) {
                obj["resolveTime"] = feedback.resolveTime.toString(Qt::ISODate);
            }
            obj["votes"] = feedback.votes;
            obj["tags"] = QJsonArray::fromStringList(feedback.tags);
            array.append(obj);
        }

        QJsonDocument doc(array);
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
            LOG_INFO(QString("Feedbacks exported to JSON: %1").arg(filePath));
            return true;
        }
    }

    return false;
}

void FeedbackSystem::saveFeedbacks()
{
    QString filePath = m_storagePath + "/feedbacks.json";
    exportFeedbacks(filePath, "json");
}

void FeedbackSystem::loadFeedbacks()
{
    QString filePath = m_storagePath + "/feedbacks.json";
    QFile file(filePath);

    if (!file.exists()) {
        return;
    }

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);

        if (error.error == QJsonParseError::NoError) {
            QJsonArray array = doc.array();
            for (const auto& item : array) {
                QJsonObject obj = item.toObject();
                FeedbackInfo feedback;
                feedback.id = obj["id"].toString();
                feedback.type = static_cast<FeedbackCategoryType>(obj["type"].toInt());
                feedback.priority = static_cast<FeedbackPriority>(obj["priority"].toInt());
                feedback.status = static_cast<FeedbackStatus>(obj["status"].toInt());
                feedback.title = obj["title"].toString();
                feedback.content = obj["content"].toString();
                feedback.category = obj["category"].toString();
                feedback.userId = obj["userId"].toString();
                feedback.userEmail = obj["userEmail"].toString();
                feedback.userVersion = obj["userVersion"].toString();
                feedback.userPlatform = obj["userPlatform"].toString();
                feedback.createTime = QDateTime::fromString(obj["createTime"].toString(), Qt::ISODate);
                feedback.updateTime = QDateTime::fromString(obj["updateTime"].toString(), Qt::ISODate);
                if (obj.contains("resolveTime")) {
                    feedback.resolveTime = QDateTime::fromString(obj["resolveTime"].toString(), Qt::ISODate);
                }
                feedback.votes = obj["votes"].toInt();

                QJsonArray tagsArray = obj["tags"].toArray();
                for (const auto& tag : tagsArray) {
                    feedback.tags.append(tag.toString());
                }

                m_feedbacks.append(feedback);
            }

            LOG_INFO(QString("Loaded %1 feedbacks from %2").arg(m_feedbacks.size()).arg(filePath));
        }
    }
}

QString FeedbackSystem::generateId()
{
    return QString("FB-%1-%2")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd"))
        .arg(QRandomGenerator::global()->bounded(10000, 99999));
}

bool FeedbackSystem::voteFeedback(const QString& feedbackId)
{
    for (int i = 0; i < m_feedbacks.size(); ++i) {
        if (m_feedbacks[i].id == feedbackId) {
            m_feedbacks[i].votes++;
            m_feedbacks[i].updateTime = QDateTime::currentDateTime();
            saveFeedbacks();
            LOG_DEBUG(QString("Feedback voted: %1, votes: %2").arg(feedbackId).arg(m_feedbacks[i].votes));
            return true;
        }
    }
    return false;
}

} // namespace WealthPilot