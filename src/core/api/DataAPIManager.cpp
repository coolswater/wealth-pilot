/**
 * @file DataAPIManager.cpp
 * @brief 数据 API 管理器实现
 */

#include "DataAPIManager.h"
#include "../utils/Logger.h"
#include <QUuid>
#include <QFile>
#include <QTextStream>
#include <QCryptographicHash>

DataAPIManager* DataAPIManager::instance()
{
    static DataAPIManager* inst = new DataAPIManager();
    return inst;
}

DataAPIManager::DataAPIManager(QObject* parent)
    : QObject(parent)
{
    initializeDefaultEndpoints();
    LOG_INFO("DataAPIManager initialized");
}

void DataAPIManager::initializeDefaultEndpoints()
{
    // 市场数据端点
    APIEndpoint marketQuote;
    marketQuote.path = "/api/v1/market/quote";
    marketQuote.method = "GET";
    marketQuote.description = "获取股票行情";
    marketQuote.requiredParams = {"symbol"};
    marketQuote.optionalParams = {"fields"};
    marketQuote.responseType = "json";
    marketQuote.rateLimit = 1000;
    m_endpoints[marketQuote.path] = marketQuote;

    // K线数据端点
    APIEndpoint kline;
    kline.path = "/api/v1/market/kline";
    kline.method = "GET";
    kline.description = "获取K线数据";
    kline.requiredParams = {"symbol"};
    kline.optionalParams = {"period", "start", "end", "limit"};
    kline.responseType = "json";
    kline.rateLimit = 500;
    m_endpoints[kline.path] = kline;

    // 账户信息端点
    APIEndpoint account;
    account.path = "/api/v1/account/info";
    account.method = "GET";
    account.description = "获取账户信息";
    account.requiredParams = {};
    account.optionalParams = {};
    account.responseType = "json";
    account.rateLimit = 100;
    m_endpoints[account.path] = account;

    // 持仓端点
    APIEndpoint positions;
    positions.path = "/api/v1/account/positions";
    positions.method = "GET";
    positions.description = "获取持仓信息";
    positions.requiredParams = {};
    positions.optionalParams = {};
    positions.responseType = "json";
    positions.rateLimit = 100;
    m_endpoints[positions.path] = positions;

    // 下单端点
    APIEndpoint order;
    order.path = "/api/v1/trading/order";
    order.method = "POST";
    order.description = "下单";
    order.requiredParams = {"symbol", "direction", "quantity"};
    order.optionalParams = {"price", "type"};
    order.responseType = "json";
    order.rateLimit = 50;
    m_endpoints[order.path] = order;

    LOG_DEBUG(QString("Default endpoints initialized: %1").arg(m_endpoints.size()));
}

void DataAPIManager::registerEndpoint(const APIEndpoint& endpoint)
{
    m_endpoints[endpoint.path] = endpoint;
    LOG_INFO(QString("Endpoint registered: %1").arg(endpoint.path));
}

APIEndpoint DataAPIManager::getEndpoint(const QString& path) const
{
    return m_endpoints.value(path);
}

QVector<APIEndpoint> DataAPIManager::getAllEndpoints() const
{
    return m_endpoints.values();
}

APIResponse DataAPIManager::handleRequest(const QString& apiKey, const QString& endpoint,
                                          const QString& method, const QVariantMap& params)
{
    APIResponse response;
    response.timestamp = QDateTime::currentDateTime();

    // 记录请求开始时间
    QElapsedTimer timer;
    timer.start();

    APIRequest request;
    request.requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    request.apiKey = apiKey;
    request.endpoint = endpoint;
    request.method = method;
    request.params = params;
    request.time = response.timestamp;

    // 验证密钥
    if (!validateKey(apiKey)) {
        response.success = false;
        response.statusCode = 401;
        response.error = "Invalid API key";
        request.success = false;
        request.statusCode = 401;
        recordRequest(request);
        return response;
    }

    // 检查端点是否存在
    if (!m_endpoints.contains(endpoint)) {
        response.success = false;
        response.statusCode = 404;
        response.error = "Endpoint not found";
        request.success = false;
        request.statusCode = 404;
        recordRequest(request);
        return response;
    }

    // 检查访问权限
    if (!checkAccess(apiKey, endpoint)) {
        response.success = false;
        response.statusCode = 403;
        response.error = "Access denied";
        request.success = false;
        request.statusCode = 403;
        recordRequest(request);
        return response;
    }

    // 检查速率限制
    if (!checkRateLimit(apiKey)) {
        response.success = false;
        response.statusCode = 429;
        response.error = "Rate limit exceeded";
        request.success = false;
        request.statusCode = 429;
        recordRequest(request);
        emit rateLimitExceeded(apiKey);
        return response;
    }

    // 验证参数
    const APIEndpoint& ep = m_endpoints[endpoint];
    for (const QString& param : ep.requiredParams) {
        if (!params.contains(param)) {
            response.success = false;
            response.statusCode = 400;
            response.error = QString("Missing required parameter: %1").arg(param);
            request.success = false;
            request.statusCode = 400;
            recordRequest(request);
            return response;
        }
    }

    // 处理请求
    response.data = fetchData(endpoint, params);
    response.success = true;
    response.statusCode = 200;
    response.message = "Success";

    // 记录请求
    request.success = true;
    request.statusCode = 200;
    request.responseTime = timer.elapsed();
    recordRequest(request);

    emit requestProcessed(request);

    LOG_DEBUG(QString("Request processed: %1 %2 (%3ms)")
        .arg(method).arg(endpoint).arg(request.responseTime));

    return response;
}

APIKey DataAPIManager::createKey(const QString& userId, const QString& name,
                                 int rateLimit, const QDateTime& expireTime)
{
    APIKey key;
    key.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    key.key = QCryptographicHash::hash(
        QUuid::createUuid().toString().toUtf8(),
        QCryptographicHash::Sha256).toHex();
    key.userId = userId;
    key.name = name;
    key.createTime = QDateTime::currentDateTime();
    key.expireTime = expireTime;
    key.rateLimit = rateLimit;
    key.enabled = true;

    // 默认允许所有端点
    for (const QString& path : m_endpoints.keys()) {
        key.allowedEndpoints.append(path);
    }

    m_keys[key.id] = key;

    emit keyCreated(key);
    LOG_INFO(QString("API key created: %1 for user %2").arg(key.id).arg(userId));

    return key;
}

bool DataAPIManager::deleteKey(const QString& keyId)
{
    if (!m_keys.contains(keyId)) {
        return false;
    }

    m_keys.remove(keyId);
    m_recentRequests.remove(keyId);

    emit keyDeleted(keyId);
    LOG_INFO(QString("API key deleted: %1").arg(keyId));

    return true;
}

bool DataAPIManager::setKeyEnabled(const QString& keyId, bool enabled)
{
    if (!m_keys.contains(keyId)) {
        return false;
    }

    m_keys[keyId].enabled = enabled;
    LOG_INFO(QString("API key %1 %2").arg(keyId).arg(enabled ? "enabled" : "disabled"));

    return true;
}

APIKey DataAPIManager::getKey(const QString& keyId) const
{
    return m_keys.value(keyId);
}

bool DataAPIManager::validateKey(const QString& apiKey) const
{
    for (const APIKey& key : m_keys) {
        if (key.key == apiKey && key.enabled) {
            // 检查过期时间
            if (key.expireTime.isValid() && key.expireTime < QDateTime::currentDateTime()) {
                return false;
            }
            return true;
        }
    }
    return false;
}

QVector<APIKey> DataAPIManager::getUserKeys(const QString& userId) const
{
    QVector<APIKey> result;
    for (const APIKey& key : m_keys) {
        if (key.userId == userId) {
            result.append(key);
        }
    }
    return result;
}

bool DataAPIManager::checkAccess(const QString& apiKey, const QString& endpoint)
{
    for (const APIKey& key : m_keys) {
        if (key.key == apiKey) {
            return key.allowedEndpoints.contains(endpoint);
        }
    }
    return false;
}

bool DataAPIManager::checkRateLimit(const QString& apiKey)
{
    // 找到对应的密钥ID
    QString keyId;
    int rateLimit = 100;

    for (const APIKey& key : m_keys) {
        if (key.key == apiKey) {
            keyId = key.id;
            rateLimit = key.rateLimit;
            break;
        }
    }

    if (keyId.isEmpty()) {
        return false;
    }

    // 清理超过1分钟的请求记录
    QDateTime oneMinuteAgo = QDateTime::currentDateTime().addSecs(-60);
    QVector<QDateTime>& recent = m_recentRequests[keyId];
    for (int i = recent.size() - 1; i >= 0; --i) {
        if (recent[i] < oneMinuteAgo) {
            recent.removeAt(i);
        }
    }

    // 检查是否超过限制
    if (recent.size() >= rateLimit) {
        return false;
    }

    // 记录本次请求
    recent.append(QDateTime::currentDateTime());

    return true;
}

void DataAPIManager::setEndpointPermission(const QString& keyId,
                                           const QString& endpoint, bool allowed)
{
    if (!m_keys.contains(keyId)) {
        return;
    }

    APIKey& key = m_keys[keyId];
    if (allowed) {
        if (!key.allowedEndpoints.contains(endpoint)) {
            key.allowedEndpoints.append(endpoint);
        }
    } else {
        key.allowedEndpoints.removeAll(endpoint);
    }

    LOG_INFO(QString("Endpoint permission set: key=%1, endpoint=%2, allowed=%3")
        .arg(keyId).arg(endpoint).arg(allowed));
}

QVector<APIRequest> DataAPIManager::getRequestHistory(const QString& apiKey,
                                                       int limit) const
{
    QVector<APIRequest> result;

    for (int i = m_requestHistory.size() - 1; i >= 0 && result.size() < limit; --i) {
        if (apiKey.isEmpty() || m_requestHistory[i].apiKey == apiKey) {
            result.append(m_requestHistory[i]);
        }
    }

    return result;
}

DataAPIManager::UsageStats DataAPIManager::getUsageStats(const QString& apiKey) const
{
    UsageStats stats;

    for (const APIRequest& req : m_requestHistory) {
        if (apiKey.isEmpty() || req.apiKey == apiKey) {
            stats.totalRequests++;
            stats.avgResponseTime += req.responseTime;

            if (req.success) {
                stats.successRequests++;
            } else {
                stats.failedRequests++;
            }

            stats.endpointCounts[req.endpoint]++;
        }
    }

    if (stats.totalRequests > 0) {
        stats.avgResponseTime /= stats.totalRequests;
    }

    return stats;
}

QString DataAPIManager::generateAPIDocumentation() const
{
    QString doc;
    doc += "# WealthPilot API 文档\n\n";
    doc += "## 概述\n\n";
    doc += "WealthPilot 提供RESTful API接口，用于获取市场数据、账户信息和执行交易。\n\n";
    doc += "## 认证\n\n";
    doc += "所有API请求需要在Header中携带API密钥：\n\n";
    doc += "```\nAuthorization: Bearer YOUR_API_KEY\n```\n\n";
    doc += "## 端点列表\n\n";

    for (const APIEndpoint& ep : m_endpoints) {
        doc += QString("### %1\n\n").arg(ep.path);
        doc += QString("**方法**: %1\n\n").arg(ep.method);
        doc += QString("**描述**: %1\n\n").arg(ep.description);

        if (!ep.requiredParams.isEmpty()) {
            doc += "**必需参数**:\n";
            for (const QString& p : ep.requiredParams) {
                doc += QString("- `%1`\n").arg(p);
            }
            doc += "\n";
        }

        if (!ep.optionalParams.isEmpty()) {
            doc += "**可选参数**:\n";
            for (const QString& p : ep.optionalParams) {
                doc += QString("- `%1`\n").arg(p);
            }
            doc += "\n";
        }

        doc += QString("**速率限制**: %1 次/分钟\n\n").arg(ep.rateLimit);
    }

    return doc;
}

bool DataAPIManager::exportDocumentation(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    out << generateAPIDocumentation();
    file.close();

    LOG_INFO(QString("API documentation exported: %1").arg(filePath));
    return true;
}

QVariant DataAPIManager::fetchData(const QString& endpoint, const QVariantMap& params)
{
    // TODO: 从实际数据源获取数据
    // 这里返回示例数据

    if (endpoint == "/api/v1/market/quote") {
        QVariantMap quote;
        quote["symbol"] = params["symbol"];
        quote["price"] = 100.0;
        quote["change"] = 2.5;
        quote["changePercent"] = 2.5;
        quote["volume"] = 1234567;
        quote["time"] = QDateTime::currentDateTime().toString();
        return quote;
    }

    if (endpoint == "/api/v1/market/kline") {
        QVariantList klines;
        for (int i = 0; i < 10; ++i) {
            QVariantMap bar;
            bar["time"] = QDateTime::currentDateTime().addDays(-i).toString();
            bar["open"] = 100.0 + i;
            bar["high"] = 105.0 + i;
            bar["low"] = 95.0 + i;
            bar["close"] = 102.0 + i;
            bar["volume"] = 1000000;
            klines.append(bar);
        }
        return klines;
    }

    if (endpoint == "/api/v1/account/info") {
        QVariantMap account;
        account["balance"] = 1000000.0;
        account["available"] = 500000.0;
        account["marketValue"] = 500000.0;
        account["totalAsset"] = 1000000.0;
        return account;
    }

    if (endpoint == "/api/v1/account/positions") {
        QVariantList positions;
        QVariantMap pos;
        pos["symbol"] = "600519";
        pos["quantity"] = 100;
        pos["cost"] = 1000.0;
        pos["marketValue"] = 1100.0;
        pos["profit"] = 100.0;
        positions.append(pos);
        return positions;
    }

    if (endpoint == "/api/v1/trading/order") {
        QVariantMap order;
        order["orderId"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        order["symbol"] = params["symbol"];
        order["direction"] = params["direction"];
        order["quantity"] = params["quantity"];
        order["price"] = params.value("price", 0.0);
        order["status"] = "pending";
        order["time"] = QDateTime::currentDateTime().toString();
        return order;
    }

    return QVariant();
}

void DataAPIManager::recordRequest(const APIRequest& request)
{
    m_requestHistory.append(request);

    // 更新密钥请求计数
    for (APIKey& key : m_keys) {
        if (key.key == request.apiKey) {
            key.requestCount++;
            break;
        }
    }

    // 保持历史记录不超过10000条
    if (m_requestHistory.size() > 10000) {
        m_requestHistory.removeFirst();
    }
}