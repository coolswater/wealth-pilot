/**
 * @file NetworkManager.h
 * @brief 网络管理器 - 统一网络请求管理
 *
 * @details 主要功能：
 * - HTTP 请求封装（GET/POST/PUT/DELETE）
 * - WebSocket 连接管理
 * - 请求队列和重试机制
 * - 网络状态监控
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
    QString url;                     ///< 请求URL
    QString method;                  ///< 请求方法（GET, POST, PUT, DELETE）
    QByteArray data;                 ///< 请求数据
    QMap<QString, QString> headers;  ///< 请求头
    int retryCount = 0;              ///< 当前重试次数
    int maxRetries = 3;              ///< 最大重试次数
    int timeout = 30000;             ///< 超时时间（毫秒）
};

/**
 * @brief 网络管理器类
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
     * @brief 关闭网络管理器
     */
    void shutdown();

    // ========== HTTP 请求 ==========

    /**
     * @brief GET 请求
     * @param url 请求URL
     * @param headers 请求头
     * @return QNetworkReply* 需要调用者管理生命周期
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
     * @brief 异步GET请求（自动处理响应）
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

    // ========== 批量请求 ==========

    /**
     * @brief 批量GET请求
     * @param urls 请求URL列表
     * @param callback 单个请求完成回调
     * @param allCompleteCallback 所有请求完成回调
     */
    void getBatchAsync(const QStringList& urls,
                       std::function<void(const QString & url, Result<QByteArray>)> callback,
                       std::function<void()> allCompleteCallback = nullptr,
                       const QMap<QString, QString>& headers = {});

    /**
     * @brief 批量GET请求（合并结果）
     * @param urls 请求URL列表
     * @param callback 所有请求完成后的合并结果回调
     */
    void getBatchMerged(const QStringList& urls,
                        std::function<void(QMap<QString, Result<QByteArray>>)> callback,
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
     * @brief WebSocket 是否已连接
     */
    bool isWebSocketConnected() const;

    /**
     * @brief 发送 WebSocket 消息
     */
    void sendWebSocketMessage(const QString& message);

    // ========== 网络状态 ==========

    /**
     * @brief 是否在线
     */
    bool isOnline() const;

    /**
     * @brief 设置网络状态
     */
    void setOnline(bool online);

    // ========== 配置 ==========

    /**
     * @brief 设置默认超时时间
     */
    void setDefaultTimeout(int ms);

    /**
     * @brief 设置最大重试次数
     */
    void setMaxRetries(int retries);

    // ========== 缓存配置 ==========

    /**
     * @brief 设置缓存是否启用
     */
    void setCacheEnabled(bool enabled);

    /**
     * @brief 设置缓存超时时间
     */
    void setCacheTimeout(int timeoutMs);

    /**
     * @brief 清空缓存
     */
    void clearCache();

signals:
    /**
     * @brief WebSocket 已连接
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
     * @brief 网络状态变化
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

    std::unique_ptr<QNetworkAccessManager> m_httpManager;  ///< HTTP管理器
    std::unique_ptr<QWebSocket> m_webSocket;               ///< WebSocket连接
    QQueue<NetworkRequest> m_requestQueue;                 ///< 请求队列
    QMap<QNetworkReply*, NetworkRequest> m_activeRequests; ///< 活动请求
    mutable QMutex m_mutex;                                ///< 线程安全锁
    bool m_online = true;                                  ///< 是否在线
    bool m_initialized = false;                            ///< 是否初始化
    int m_defaultTimeout = 30000;                          ///< 默认超时
    int m_maxRetries = 3;                                  ///< 最大重试次数

    // 缓存配置
    bool m_cacheEnabled = true;                            ///< 是否启用缓存
    int m_requestCacheTimeout = 300000;                    ///< 缓存超时（5分钟）

    struct RequestCacheEntry {
        QByteArray response;
        qint64 expiryTime;
    };

    QMap<QString, RequestCacheEntry> m_requestCache;       ///< 请求缓存

    // 内部方法
    void cleanupCache();
    QVariant getCachedResponse(const QString& key);
    void setCachedResponse(const QString& key, const QByteArray& response);
};

#endif // NETWORKMANAGER_H