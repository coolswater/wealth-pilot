/**
 * @file DataAPIManager.h
 * @brief 数据 API 管理器 - RESTful API 开放平台
 *
 * @details 功能：
 * - API 接口管理
 * - 访问控制
 * - 数据导出
 * - API 文档
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef DATAAPIMANAGER_H
#define DATAAPIMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QVariant>

/**
 * @brief API 端点
 */
struct APIEndpoint {
    QString path;               ///< 路径
    QString method;             ///< 方法 (GET/POST/PUT/DELETE)
    QString description;        ///< 描述
    QStringList requiredParams; ///< 必需参数
    QStringList optionalParams; ///< 可选参数
    QString responseType;       ///< 响应类型
    bool requiresAuth = true;   ///< 是否需要认证
    int rateLimit = 100;        ///< 每分钟请求限制
};

/**
 * @brief API 密钥
 */
struct APIKey {
    QString id;                 ///< 密钥ID
    QString key;                ///< 密钥值
    QString userId;             ///< 用户ID
    QString name;               ///< 名称
    QDateTime createTime;       ///< 创建时间
    QDateTime expireTime;       ///< 过期时间
    int requestCount = 0;       ///< 请求次数
    int rateLimit = 100;        ///< 速率限制
    bool enabled = true;        ///< 是否启用
    QStringList allowedEndpoints; ///< 允许的端点
};

/**
 * @brief API 请求
 */
struct APIRequest {
    QString requestId;          ///< 请求ID
    QString apiKey;             ///< API密钥
    QString endpoint;           ///< 端点
    QString method;             ///< 方法
    QVariantMap params;         ///< 参数
    QDateTime time;             ///< 时间
    QString ipAddress;          ///< IP地址
    int responseTime = 0;       ///< 响应时间(ms)
    bool success = true;        ///< 是否成功
    int statusCode = 200;       ///< 状态码
};

/**
 * @brief API 响应
 */
struct APIResponse {
    bool success = true;
    int statusCode = 200;
    QString message;
    QVariant data;
    QString error;
    QDateTime timestamp;
};

/**
 * @brief 数据 API 管理器
 *
 * 提供开放数据 API：
 * - API 端点管理
 * - 密钥管理
 * - 访问控制
 * - 使用统计
 */
class DataAPIManager : public QObject {
    Q_OBJECT

public:
    static DataAPIManager* instance();

    // ========== 端点管理 ==========

    /**
     * @brief 注册端点
     */
    void registerEndpoint(const APIEndpoint& endpoint);

    /**
     * @brief 获取端点
     */
    APIEndpoint getEndpoint(const QString& path) const;

    /**
     * @brief 获取所有端点
     */
    QVector<APIEndpoint> getAllEndpoints() const;

    /**
     * @brief 处理请求
     */
    APIResponse handleRequest(const QString& apiKey, const QString& endpoint,
                              const QString& method, const QVariantMap& params);

    // ========== 密钥管理 ==========

    /**
     * @brief 创建密钥
     */
    APIKey createKey(const QString& userId, const QString& name,
                    int rateLimit = 100, const QDateTime& expireTime = QDateTime());

    /**
     * @brief 删除密钥
     */
    bool deleteKey(const QString& keyId);

    /**
     * @brief 启用/禁用密钥
     */
    bool setKeyEnabled(const QString& keyId, bool enabled);

    /**
     * @brief 获取密钥
     */
    APIKey getKey(const QString& keyId) const;

    /**
     * @brief 验证密钥
     */
    bool validateKey(const QString& apiKey) const;

    /**
     * @brief 获取用户密钥
     */
    QVector<APIKey> getUserKeys(const QString& userId) const;

    // ========== 访问控制 ==========

    /**
     * @brief 检查访问权限
     */
    bool checkAccess(const QString& apiKey, const QString& endpoint);

    /**
     * @brief 检查速率限制
     */
    bool checkRateLimit(const QString& apiKey);

    /**
     * @brief 设置端点权限
     */
    void setEndpointPermission(const QString& keyId, const QString& endpoint, bool allowed);

    // ========== 使用统计 ==========

    /**
     * @brief 获取请求历史
     */
    QVector<APIRequest> getRequestHistory(const QString& apiKey = QString(),
                                          int limit = 100) const;

    /**
     * @brief 获取使用统计
     */
    struct UsageStats {
        int totalRequests = 0;
        int successRequests = 0;
        int failedRequests = 0;
        double avgResponseTime = 0.0;
        QHash<QString, int> endpointCounts;
    };
    UsageStats getUsageStats(const QString& apiKey) const;

    // ========== API 文档 ==========

    /**
     * @brief 生成 API 文档
     */
    QString generateAPIDocumentation() const;

    /**
     * @brief 导出 API 文档
     */
    bool exportDocumentation(const QString& filePath) const;

signals:
    /**
     * @brief 请求处理完成
     */
    void requestProcessed(const APIRequest& request);

    /**
     * @brief 密钥创建
     */
    void keyCreated(const APIKey& key);

    /**
     * @brief 密钥删除
     */
    void keyDeleted(const QString& keyId);

    /**
     * @brief 速率限制触发
     */
    void rateLimitExceeded(const QString& apiKey);

private:
    explicit DataAPIManager(QObject* parent = nullptr);
    ~DataAPIManager() override = default;

    void initializeDefaultEndpoints();
    QVariant fetchData(const QString& endpoint, const QVariantMap& params);
    void recordRequest(const APIRequest& request);

    QHash<QString, APIEndpoint> m_endpoints;
    QHash<QString, APIKey> m_keys;
    QVector<APIRequest> m_requestHistory;

    // 速率限制追踪
    QHash<QString, QVector<QDateTime>> m_recentRequests;
};

#endif // DATAAPIMANAGER_H