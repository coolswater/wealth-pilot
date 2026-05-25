/**
 * @file AIConversationManager.h
 * @brief AI 对话历史管理器
 *
 * @details 功能：
 * - 保存/加载对话历史
 * - 对话历史搜索
 * - 对话导出
 * - 对话统计分析
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef AICONVERSATIONMANAGER_H
#define AICONVERSATIONMANAGER_H

#include "infrastructure/ai/service/AIService.h"
#include "shared/utils/Logger.h"

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>

namespace WealthPilot
{
    /**
 * @brief 对话会话结构
 */
    struct ConversationSession
    {
        QString id; ///< 会话 ID
        QString title; ///< 会话标题
        QDateTime createdAt; ///< 创建时间
        QDateTime updatedAt; ///< 更新时间
        QList<AIMessage> messages; ///< 消息列表
        int messageCount; ///< 消息数量
        QString summary; ///< 会话摘要

        /**
     * @brief 从 JSON 解析
     */
        static ConversationSession fromJson(const QJsonObject& json);

        /**
     * @brief 转换为 JSON
     */
        QJsonObject toJson() const;

        /**
     * @brief 生成自动标题
     */
        QString generateTitle() const;
    };

    /**
 * @brief AI 对话历史管理器
 *
 * @details 管理所有 AI 对话历史，支持：
 * - 保存/加载对话
 * - 搜索对话
 * - 导出对话
 * - 统计分析
 */
    class AIConversationManager : public QObject
    {
        Q_OBJECT

    public:
        /**
     * @brief 构造函数
     * @param parent 父对象
     */
        explicit AIConversationManager(QObject* parent = nullptr);

        /**
     * @brief 析构函数
     */
        ~AIConversationManager() override;

        /**
     * @brief 初始化管理器
     * @param storagePath 存储路径
     * @return 是否成功
     */
        bool initialize(const QString& storagePath = "data/conversations");

        // ========== 会话管理 ==========

        /**
     * @brief 创建新会话
     * @return 会话 ID
     */
        QString createSession();

        /**
     * @brief 获取会话
     * @param sessionId 会话 ID
     * @return 会话数据
     */
        ConversationSession getSession(const QString& sessionId) const;

        /**
     * @brief 保存会话
     * @param session 会话数据
     * @return 是否成功
     */
        bool saveSession(const ConversationSession& session);

        /**
     * @brief 删除会话
     * @param sessionId 会话 ID
     * @return 是否成功
     */
        bool deleteSession(const QString& sessionId);

        /**
     * @brief 获取所有会话列表
     * @return 会话列表
     */
        QList<ConversationSession> listSessions() const;

        /**
     * @brief 获取最近会话
     * @param count 数量
     * @return 会话列表
     */
        QList<ConversationSession> getRecentSessions(int count = 10) const;

        // ========== 消息管理 ==========

        /**
     * @brief 添加消息到会话
     * @param sessionId 会话 ID
     * @param message 消息
     * @return 是否成功
     */
        bool addMessage(const QString& sessionId, const AIMessage& message);

        /**
     * @brief 获取会话消息
     * @param sessionId 会话 ID
     * @return 消息列表
     */
        QList<AIMessage> getMessages(const QString& sessionId) const;

        /**
     * @brief 清空会话消息
     * @param sessionId 会话 ID
     * @return 是否成功
     */
        bool clearMessages(const QString& sessionId);

        // ========== 搜索功能 ==========

        /**
     * @brief 搜索会话
     * @param keyword 关键词
     * @return 匹配的会话列表
     */
        QList<ConversationSession> searchSessions(const QString& keyword) const;

        /**
     * @brief 搜索消息
     * @param sessionId 会话 ID
     * @param keyword 关键词
     * @return 匹配的消息列表
     */
        QList<AIMessage> searchMessages(const QString& sessionId, const QString& keyword) const;

        // ========== 导出功能 ==========

        /**
     * @brief 导出会话为文本
     * @param sessionId 会话 ID
     * @param format 格式 (txt, markdown, json)
     * @return 导出内容
     */
        QString exportSession(const QString& sessionId, const QString& format = "markdown") const;

        /**
     * @brief 导出会话到文件
     * @param sessionId 会话 ID
     * @param filePath 文件路径
     * @param format 格式
     * @return 是否成功
     */
        bool exportSessionToFile(const QString& sessionId, const QString& filePath,
                                 const QString& format = "markdown") const;

        /**
     * @brief 导出所有会话
     * @param filePath 文件路径
     * @return 是否成功
     */
        bool exportAllSessions(const QString& filePath) const;

        // ========== 统计功能 ==========

        /**
     * @brief 获取统计信息
     * @return 统计 JSON
     */
        QJsonObject getStatistics() const;

        /**
     * @brief 获取会话数量
     * @return 数量
     */
        int getSessionCount() const;

        /**
     * @brief 获取总消息数量
     * @return 数量
     */
        int getTotalMessageCount() const;

        // ========== 配置 ==========

        /**
     * @brief 设置最大会话数量
     * @param max 最大数量
     */
        void setMaxSessions(int max);

        /**
     * @brief 设置自动保存
     * @param enabled 是否启用
     */
        void setAutoSave(bool enabled);

        signals :
        /**
     * @brief 会话创建信号
     */

        void sessionCreated(const QString& sessionId);

        /**
     * @brief 会话更新信号
     */
        void sessionUpdated(const QString& sessionId);

        /**
     * @brief 会话删除信号
     */
        void sessionDeleted(const QString& sessionId);

        /**
     * @brief 消息添加信号
     */
        void messageAdded(const QString& sessionId, const AIMessage& message);

        /**
     * @brief 错误信号
     */
        void errorOccurred(const QString& error);

    private:
        /**
     * @brief 加载所有会话
     */
        void loadAllSessions();

        /**
     * @brief 保存会话到文件
     * @param session 会话
     * @return 是否成功
     */
        bool saveSessionToFile(const ConversationSession& session);

        /**
     * @brief 删除会话文件
     * @param sessionId 会话 ID
     * @return 是否成功
     */
        bool deleteSessionFile(const QString& sessionId);

        /**
     * @brief 清理旧会话
     */
        void cleanupOldSessions();

        /**
     * @brief 生成会话 ID
     * @return ID
     */
        QString generateSessionId() const;

    private:
        QString m_storagePath; ///< 存储路径
        QMap<QString, ConversationSession> m_sessions; ///< 会话缓存
        int m_maxSessions = 100; ///< 最大会话数
        bool m_autoSave = true; ///< 自动保存
        bool m_initialized = false; ///< 是否已初始化
    };
} // namespace WealthPilot

#endif // AICONVERSATIONMANAGER_H