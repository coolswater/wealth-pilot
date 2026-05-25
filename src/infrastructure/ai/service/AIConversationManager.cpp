/**
 * @file AIConversationManager.cpp
 * @brief AI 对话历史管理器实现
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "AIConversationManager.h"
#include <QJsonDocument>
#include <QStandardPaths>
#include <QUuid>
#include <QRegularExpression>

namespace WealthPilot
{
    // ============================================================================
    // ConversationSession 实现
    // ============================================================================

    ConversationSession ConversationSession::fromJson(const QJsonObject& json)
    {
        ConversationSession session;
        session.id = json["id"].toString();
        session.title = json["title"].toString();
        session.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
        session.updatedAt = QDateTime::fromString(json["updatedAt"].toString(), Qt::ISODate);
        session.messageCount = json["messageCount"].toInt();
        session.summary = json["summary"].toString();

        // 解析消息
        QJsonArray messagesArray = json["messages"].toArray();
        for (const auto& msgValue : messagesArray)
        {
            QJsonObject msgObj = msgValue.toObject();
            AIMessage msg;
            msg.content = msgObj["content"].toString();
            msg.role = static_cast<AIRole>(msgObj["role"].toInt());
            msg.timestamp = QDateTime::fromString(msgObj["timestamp"].toString(), Qt::ISODate);
            session.messages.append(msg);
        }

        return session;
    }

    QJsonObject ConversationSession::toJson() const
    {
        QJsonObject json;
        json["id"] = id;
        json["title"] = title;
        json["createdAt"] = createdAt.toString(Qt::ISODate);
        json["updatedAt"] = updatedAt.toString(Qt::ISODate);
        json["messageCount"] = messageCount;
        json["summary"] = summary;

        // 序列化消息
        QJsonArray messagesArray;
        for (const auto& msg : messages)
        {
            QJsonObject msgObj;
            msgObj["content"] = msg.content;
            msgObj["role"] = static_cast<int>(msg.role);
            msgObj["timestamp"] = msg.timestamp.toString(Qt::ISODate);
            messagesArray.append(msgObj);
        }
        json["messages"] = messagesArray;

        return json;
    }

    QString ConversationSession::generateTitle() const
    {
        if (messages.isEmpty())
        {
            return QStringLiteral("新对话");
        }

        // 使用第一条用户消息作为标题
        for (const auto& msg : messages)
        {
            if (msg.role == AIRole::User)
            {
                QString title = msg.content.left(50);
                if (msg.content.length() > 50)
                {
                    title += "...";
                }
                return title;
            }
        }

        return QStringLiteral("AI 对话");
    }

    // ============================================================================
    // AIConversationManager 实现
    // ============================================================================

    AIConversationManager::AIConversationManager(QObject* parent)
        : QObject(parent)
    {
    }

    AIConversationManager::~AIConversationManager()
    {
        if (m_autoSave)
        {
            // 保存所有会话
            for (const auto& session : m_sessions)
            {
                saveSessionToFile(session);
            }
        }
    }

    bool AIConversationManager::initialize(const QString& storagePath)
    {
        if (m_initialized)
        {
            return true;
        }

        // 设置存储路径
        if (storagePath.startsWith("data/"))
        {
            // 相对路径，使用应用数据目录
            QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            m_storagePath = appDataPath + "/" + storagePath;
        }
        else
        {
            m_storagePath = storagePath;
        }

        // 创建目录
        QDir dir(m_storagePath);
        if (!dir.exists())
        {
            if (!dir.mkpath("."))
            {
                LOG_ERROR("Failed to create conversation storage directory: " + m_storagePath);
                return false;
            }
        }

        // 加载所有会话
        loadAllSessions();

        m_initialized = true;
        LOG_INFO("AIConversationManager initialized with " + QString::number(m_sessions.size()) + " sessions");

        return true;
    }

    QString AIConversationManager::createSession()
    {
        QString sessionId = generateSessionId();

        ConversationSession session;
        session.id = sessionId;
        session.createdAt = QDateTime::currentDateTime();
        session.updatedAt = session.createdAt;
        session.messageCount = 0;
        session.title = QStringLiteral("新对话");

        m_sessions[sessionId] = session;
        saveSessionToFile(session);

        emit sessionCreated(sessionId);
        LOG_DEBUG("Created new conversation session: " + sessionId);

        return sessionId;
    }

    ConversationSession AIConversationManager::getSession(const QString& sessionId) const
    {
        return m_sessions.value(sessionId);
    }

    bool AIConversationManager::saveSession(const ConversationSession& session)
    {
        m_sessions[session.id] = session;
        return saveSessionToFile(session);
    }

    bool AIConversationManager::deleteSession(const QString& sessionId)
    {
        if (!m_sessions.contains(sessionId))
        {
            return false;
        }

        m_sessions.remove(sessionId);
        deleteSessionFile(sessionId);

        emit sessionDeleted(sessionId);
        LOG_DEBUG("Deleted conversation session: " + sessionId);

        return true;
    }

    QList<ConversationSession> AIConversationManager::listSessions() const
    {
        return m_sessions.values();
    }

    QList<ConversationSession> AIConversationManager::getRecentSessions(int count) const
    {
        QList<ConversationSession> sessions = m_sessions.values();

        // 按更新时间排序
        std::sort(sessions.begin(), sessions.end(),
                  [](const ConversationSession& a, const ConversationSession& b)
                  {
                      return a.updatedAt > b.updatedAt;
                  });

        return sessions.mid(0, qMin(count, sessions.size()));
    }

    bool AIConversationManager::addMessage(const QString& sessionId, const AIMessage& message)
    {
        if (!m_sessions.contains(sessionId))
        {
            LOG_WARNING("Session not found: " + sessionId);
            return false;
        }

        ConversationSession& session = m_sessions[sessionId];
        session.messages.append(message);
        session.messageCount = session.messages.size();
        session.updatedAt = QDateTime::currentDateTime();

        // 自动生成标题
        if (session.title == QStringLiteral("新对话") && message.role == AIRole::User)
        {
            session.title = session.generateTitle();
        }

        if (m_autoSave)
        {
            saveSessionToFile(session);
        }

        emit messageAdded(sessionId, message);
        emit sessionUpdated(sessionId);

        return true;
    }

    QList<AIMessage> AIConversationManager::getMessages(const QString& sessionId) const
    {
        return m_sessions.value(sessionId).messages;
    }

    bool AIConversationManager::clearMessages(const QString& sessionId)
    {
        if (!m_sessions.contains(sessionId))
        {
            return false;
        }

        ConversationSession& session = m_sessions[sessionId];
        session.messages.clear();
        session.messageCount = 0;
        session.updatedAt = QDateTime::currentDateTime();

        if (m_autoSave)
        {
            saveSessionToFile(session);
        }

        emit sessionUpdated(sessionId);
        return true;
    }

    QList<ConversationSession> AIConversationManager::searchSessions(const QString& keyword) const
    {
        QList<ConversationSession> results;

        for (const auto& session : m_sessions)
        {
            // 搜索标题
            if (session.title.contains(keyword, Qt::CaseInsensitive))
            {
                results.append(session);
                continue;
            }

            // 搜索消息内容
            for (const auto& msg : session.messages)
            {
                if (msg.content.contains(keyword, Qt::CaseInsensitive))
                {
                    results.append(session);
                    break;
                }
            }
        }

        return results;
    }

    QList<AIMessage> AIConversationManager::searchMessages(const QString& sessionId, const QString& keyword) const
    {
        QList<AIMessage> results;

        if (!m_sessions.contains(sessionId))
        {
            return results;
        }

        const auto& session = m_sessions[sessionId];
        for (const auto& msg : session.messages)
        {
            if (msg.content.contains(keyword, Qt::CaseInsensitive))
            {
                results.append(msg);
            }
        }

        return results;
    }

    QString AIConversationManager::exportSession(const QString& sessionId, const QString& format) const
    {
        if (!m_sessions.contains(sessionId))
        {
            return QString();
        }

        const auto& session = m_sessions[sessionId];

        if (format == "json")
        {
            QJsonDocument doc(session.toJson());
            return doc.toJson();
        }

        QString output;

        if (format == "markdown")
        {
            output += "# " + session.title + "\n\n";
            output += "**创建时间**: " + session.createdAt.toString("yyyy-MM-dd hh:mm:ss") + "\n";
            output += "**更新时间**: " + session.updatedAt.toString("yyyy-MM-dd hh:mm:ss") + "\n\n";
            output += "---\n\n";

            for (const auto& msg : session.messages)
            {
                QString role;
                switch (msg.role)
                {
                case AIRole::User:
                    role = "👤 **用户**";
                    break;
                case AIRole::Assistant:
                    role = "🤖 **AI 助手**";
                    break;
                case AIRole::System:
                    role = "⚙️ **系统**";
                    break;
                }

                output += role + " (" + msg.timestamp.toString("hh:mm:ss") + ")\n\n";
                output += msg.content + "\n\n";
                output += "---\n\n";
            }
        }
        else
        {
            // 纯文本格式
            output += "对话标题: " + session.title + "\n";
            output += "创建时间: " + session.createdAt.toString("yyyy-MM-dd hh:mm:ss") + "\n";
            output += "更新时间: " + session.updatedAt.toString("yyyy-MM-dd hh:mm:ss") + "\n";
            output += "========================================\n\n";

            for (const auto& msg : session.messages)
            {
                QString role;
                switch (msg.role)
                {
                case AIRole::User:
                    role = "[用户]";
                    break;
                case AIRole::Assistant:
                    role = "[AI]";
                    break;
                case AIRole::System:
                    role = "[系统]";
                    break;
                }

                output += role + " " + msg.timestamp.toString("hh:mm:ss") + "\n";
                output += msg.content + "\n\n";
            }
        }

        return output;
    }

    bool AIConversationManager::exportSessionToFile(const QString& sessionId, const QString& filePath,
                                                    const QString& format) const
    {
        QString content = exportSession(sessionId, format);
        if (content.isEmpty())
        {
            return false;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            LOG_ERROR("Failed to open file for writing: " + filePath);
            return false;
        }

        file.write(content.toUtf8());
        file.close();

        LOG_INFO("Exported conversation to: " + filePath);
        return true;
    }

    bool AIConversationManager::exportAllSessions(const QString& filePath) const
    {
        QJsonArray sessionsArray;
        for (const auto& session : m_sessions)
        {
            sessionsArray.append(session.toJson());
        }

        QJsonDocument doc(sessionsArray);

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
        {
            LOG_ERROR("Failed to open file for writing: " + filePath);
            return false;
        }

        file.write(doc.toJson());
        file.close();

        LOG_INFO("Exported all conversations to: " + filePath);
        return true;
    }

    QJsonObject AIConversationManager::getStatistics() const
    {
        QJsonObject stats;
        stats["totalSessions"] = m_sessions.size();
        stats["totalMessages"] = getTotalMessageCount();

        // 计算平均消息数
        double avgMessages = m_sessions.isEmpty() ? 0 : static_cast<double>(getTotalMessageCount()) / m_sessions.size();
        stats["averageMessages"] = avgMessages;

        // 计算今日会话数
        int todayCount = 0;
        QDateTime today = QDateTime::currentDateTime();
        for (const auto& session : m_sessions)
        {
            if (session.createdAt.date() == today.date())
            {
                todayCount++;
            }
        }
        stats["todaySessions"] = todayCount;

        return stats;
    }

    int AIConversationManager::getSessionCount() const
    {
        return m_sessions.size();
    }

    int AIConversationManager::getTotalMessageCount() const
    {
        int total = 0;
        for (const auto& session : m_sessions)
        {
            total += session.messageCount;
        }
        return total;
    }

    void AIConversationManager::setMaxSessions(int max)
    {
        m_maxSessions = max;
        cleanupOldSessions();
    }

    void AIConversationManager::setAutoSave(bool enabled)
    {
        m_autoSave = enabled;
    }

    void AIConversationManager::loadAllSessions()
    {
        QDir dir(m_storagePath);
        QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);

        for (const QString& file : files)
        {
            QString filePath = m_storagePath + "/" + file;
            QFile f(filePath);

            if (!f.open(QIODevice::ReadOnly))
            {
                LOG_WARNING("Failed to load conversation file: " + filePath);
                continue;
            }

            QByteArray data = f.readAll();
            f.close();

            QJsonDocument doc = QJsonDocument::fromJson(data);
            ConversationSession session = ConversationSession::fromJson(doc.object());

            m_sessions[session.id] = session;
        }

        LOG_DEBUG("Loaded " + QString::number(m_sessions.size()) + " conversation sessions");
    }

    bool AIConversationManager::saveSessionToFile(const ConversationSession& session)
    {
        QString filePath = m_storagePath + "/" + session.id + ".json";

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly))
        {
            LOG_ERROR("Failed to save conversation to: " + filePath);
            return false;
        }

        QJsonDocument doc(session.toJson());
        file.write(doc.toJson());
        file.close();

        return true;
    }

    bool AIConversationManager::deleteSessionFile(const QString& sessionId)
    {
        QString filePath = m_storagePath + "/" + sessionId + ".json";
        return QFile::remove(filePath);
    }

    void AIConversationManager::cleanupOldSessions()
    {
        if (m_sessions.size() <= m_maxSessions)
        {
            return;
        }

        // 按更新时间排序
        QList<ConversationSession> sessions = m_sessions.values();
        std::sort(sessions.begin(), sessions.end(),
                  [](const ConversationSession& a, const ConversationSession& b)
                  {
                      return a.updatedAt < b.updatedAt;
                  });

        // 删除最旧的会话
        int toDelete = m_sessions.size() - m_maxSessions;
        for (int i = 0; i < toDelete; ++i)
        {
            deleteSession(sessions[i].id);
        }

        LOG_INFO("Cleaned up " + QString::number(toDelete) + " old conversation sessions");
    }

    QString AIConversationManager::generateSessionId() const
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
} // namespace WealthPilot