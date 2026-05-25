/**
 * @file WebSocketManager.h
 * @brief WebSocket 连接管理器 - 支持断线重连
 *
 * @details 功能：
 * - 自动断线重连
 * - 心跳检测
 * - 连接状态管理
 * - 消息队列缓存
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef WEBSOCKETMANAGER_H
#define WEBSOCKETMANAGER_H

#include <QObject>
#include <QWebSocket>
#include <QTimer>
#include <QQueue>
#include <QMutex>

/**
 * @brief WebSocket 连接状态
 */
enum class WSConnectionState {
    Disconnected,       ///< 已断开
    Connecting,         ///< 连接中
    Connected,          ///< 已连接
    Reconnecting,       ///< 重连中
    Error               ///< 错误
};

/**
 * @brief WebSocket 管理器
 *
 * 提供可靠的 WebSocket 连接：
 * - 自动重连机制
 * - 心跳保活
 * - 消息缓存和重发
 */
class WebSocketManager : public QObject {
    Q_OBJECT

public:
    static WebSocketManager* instance();

    /**
     * @brief 连接到服务器
     * @param url WebSocket URL
     */
    void connectToServer(const QString& url);

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 发送消息
     * @param message 消息内容
     * @param cacheIfDisconnected 断开时是否缓存
     */
    void sendMessage(const QString& message, bool cacheIfDisconnected = true);

    /**
     * @brief 获取连接状态
     */
    WSConnectionState state() const { return m_state; }

    /**
     * @brief 是否已连接
     */
    bool isConnected() const { return m_state == WSConnectionState::Connected; }

    /**
     * @brief 配置重连参数
     * @param maxRetries 最大重试次数（-1 表示无限）
     * @param retryInterval 重试间隔（毫秒）
     */
    void setReconnectConfig(int maxRetries, int retryInterval);

    /**
     * @brief 配置心跳
     * @param interval 心跳间隔（毫秒，0 表示禁用）
     * @param pingMessage 心跳消息
     */
    void setHeartbeat(int interval, const QString& pingMessage = "{}");

signals:
    /**
     * @brief 连接状态变化
     */
    void stateChanged(WSConnectionState state);

    /**
     * @brief 收到消息
     */
    void messageReceived(const QString& message);

    /**
     * @brief 连接错误
     */
    void errorOccurred(const QString& error);

    /**
     * @brief 重连尝试
     */
    void reconnectAttempt(int attempt, int maxAttempts);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);
    void onHeartbeat();
    void onReconnect();

private:
    explicit WebSocketManager(QObject* parent = nullptr);
    ~WebSocketManager() override;

    void setState(WSConnectionState state);
    void startReconnect();
    void stopReconnect();
    void flushMessageQueue();

    QWebSocket* m_webSocket = nullptr;
    QString m_serverUrl;
    WSConnectionState m_state = WSConnectionState::Disconnected;

    // 重连配置
    int m_maxRetries = -1;          ///< 最大重试次数
    int m_retryInterval = 3000;     ///< 重试间隔
    int m_currentRetry = 0;         ///< 当前重试次数
    QTimer* m_reconnectTimer = nullptr;

    // 心跳配置
    QTimer* m_heartbeatTimer = nullptr;
    QString m_pingMessage;
    bool m_heartbeatPending = false;

    // 消息队列
    QQueue<QString> m_messageQueue;
    QMutex m_queueMutex;
    int m_maxQueueSize = 1000;
};

#endif // WEBSOCKETMANAGER_H
