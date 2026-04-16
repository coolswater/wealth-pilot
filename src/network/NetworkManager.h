/**
 * @file NetworkManager.h
 * @brief 网络管理�?- 处理所有网络请�?
 *
 * @details 功能�?
 * - HTTP 请求封装（GET/POST/PUT/DELETE�?
 * - WebSocket 连接管理
 * - 请求队列和重试机�?
 * - 网络状态监�?
 */

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "../core/base/Singleton.h"
#include "../utils/Result.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWebSocket>
#include <QQueue>
#include <QMutex>
#include <QMap>
#include <memory>

/**
 * @brief 网络请求结构
 */
struct NetworkRequest {
    QString id;                      ///< 请求ID
    QString url;
    QString method;                  // GET, POST, PUT, DELETE
    QByteArray data;
    QMap<QString, QString> headers;
    int retryCount = 0;
    int maxRetries = 3;
    int timeout = 30000;             ///< 超时时间(ms)
};

/**
 * @brief 网络管理�?
 */
class NetworkManager : public QObject, public Singleton<NetworkManager>
{
    Q_OBJECT
    friend class Singleton<NetworkManager>;

public:
    /**
     * @brief 初始化网络管理器
     */
    bool initialize();

    /**
     * @brief 关闭网络管理�?
     */
    void shutdown();

    // ========== HTTP 请求 ==========

    /**
     * @brief GET 请求
     * @param url 请求URL
     * @param headers 请求�?
     * @return QNetworkReply* 需要调用者管理生命周�?
     */
    QNetworkReply* get(const QString& url, const QMap<QString, QString>& headers = {});

    /**
     * @brief POST 请求
     */
    QNetworkReply* post(const QString& url, const QByteArray& data,
                        const QMap<QString, QString>& headers = {});

    /**
     * @brief PUT 请求
     */
    QNetworkReply* put(const QString& url, const QByteArray& data,
                       const QMap<QString, QString>& headers = {});

    /**
     * @brief DELETE 请求
     */
    QNetworkReply* del(const QString& url, const QMap<QString, QString>& headers = {});

    /**
     * @brief 异步GET请求（自动管理响应）
     * @param callback 完成回调
     */
    void getAsync(const QString& url,
                  std::function<void(Result<QByteArray>)> callback,
                  const QMap<QString, QString>& headers = {});

    /**
     * @brief 异步POST请求
     */
    void postAsync(const QString& url, const QByteArray& data,
                   std::function<void(Result<QByteArray>)> callback,
                   const QMap<QString, QString>& headers = {});

    // ========== WebSocket ==========

    /**
     * @brief 连接 WebSocket
     */
    void connectWebSocket(const QString& url);

    /**
     * @brief 断开 WebSocket
     */
    void disconnectWebSocket();

    /**
     * @brief WebSocket 是否已连�?
     */
    bool isWebSocketConnected() const;

    /**
     * @brief 发�?WebSocket 消息
     */
    void sendWebSocketMessage(const QString& message);

    // ========== 网络状�?==========

    /**
     * @brief 是否在线
     */
    bool isOnline() const;

    /**
     * @brief 设置在线状�?
     */
    void setOnline(bool online);

    // ========== 配置 ==========

    /**
     * @brief 设置请求超时时间
     */
    void setDefaultTimeout(int ms);

    /**
     * @brief 设置重试次数
     */
    void setMaxRetries(int retries);

    // ========== 缓存管理 ==========

    /**
     * @brief 设置缓存是否启用
     */
    void setCacheEnabled(bool enabled);

    /**
     * @brief 设置缓存超时时间
     */
    void setCacheTimeout(int timeoutMs);

    /**
     * @brief 清除缓存
     */
    void clearCache();

signals:
    /**
     * @brief WebSocket 已连�?
     */
    void webSocketConnected();

    /**
     * @brief WebSocket 已断开
     */
    void webSocketDisconnected();

    /**
     * @brief WebSocket 收到消息
     */
    void webSocketMessageReceived(const QString& message);

    /**
     * @brief 网络状态变�?
     */
    void networkStateChanged(bool online);

    /**
     * @brief 请求失败
     */
    void requestFailed(const QString& requestId, const QString& error);

private slots:
    void onNetworkReplyFinished(QNetworkReply* reply);
    void onWebSocketConnected();
    void onWebSocketDisconnected();
    void onWebSocketMessage(const QString& message);
    void onNetworkInformationChanged();
    void processQueue();

private:
    explicit NetworkManager(QObject* parent = nullptr);
    ~NetworkManager();

    QNetworkReply* sendRequest(const NetworkRequest& request);
    void retryRequest(const NetworkRequest& request);
    void enqueueRequest(const NetworkRequest& request);

    std::unique_ptr<QNetworkAccessManager> m_httpManager;
    std::unique_ptr<QWebSocket> m_webSocket;
    QQueue<NetworkRequest> m_requestQueue;
    QMap<QNetworkReply*, NetworkRequest> m_activeRequests;
    mutable QMutex m_mutex;
    bool m_online = true;
    bool m_initialized = false;
    int m_defaultTimeout = 30000;
    int m_maxRetries = 3;

    // 缓存相关
    bool m_cacheEnabled = true;
    int m_requestCacheTimeout = 300000; // 5分钟

    struct RequestCacheEntry {
        QByteArray response;
        qint64 expiryTime;
    };

    QMap<QString, RequestCacheEntry> m_requestCache;

    // 缓存管理
    void cleanupCache();
    QVariant getCachedResponse(const QString& key);
    void setCachedResponse(const QString& key, const QByteArray& response);
};

#endif // NETWORKMANAGER_H
