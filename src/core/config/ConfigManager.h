/**
 * @file ConfigManager.h
 * @brief 配置管理�?- 统一管理应用配置和安全存�?
 *
 * @details 功能�?
 * - 普通配置：使用 QSettings 持久�?
 * - 安全存储：使�?Windows DPAPI 加密（API密钥等敏感信息）
 * - 内存缓存：提高读取性能
 */

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "../base/Singleton.h"
#include <QString>
#include <QVariant>
#include <QSettings>
#include <QMap>
#include <QMutex>

// ========== 预定义配置键 ==========
namespace ConfigKeys {
// 外观
constexpr auto Theme = "appearance/theme";
constexpr auto Language = "appearance/language";
constexpr auto WindowGeometry = "window/geometry";
constexpr auto WindowState = "window/state";

// 网络
constexpr auto ApiBaseUrl = "network/api_base_url";
constexpr auto RequestTimeout = "network/request_timeout";

// CTP
constexpr auto CtpBrokerId = "ctp/broker_id";
constexpr auto CtpMarketFront = "ctp/market_front";
constexpr auto CtpTradeFront = "ctp/trade_front";

// AI
constexpr auto AiProvider = "ai/provider";
constexpr auto AiModel = "ai/model";

// 安全存储键（加密�?
constexpr auto SecureCtpUserId = "secure/ctp_user_id";
constexpr auto SecureCtpPassword = "secure/ctp_password";
constexpr auto SecureApiKey = "secure/api_key";
}

/**
 * @brief 配置管理�?
 */
class ConfigManager : public Singleton<ConfigManager>
{
    friend class Singleton<ConfigManager>;

public:
    /**
     * @brief 初始化配置管理器
     */
    bool initialize(const QString& orgName = "Hexd", const QString& appName = "WealthPilot");

    // ========== 普通配�?==========

    QVariant get(const QString& key, const QVariant& defaultValue = QVariant());
    void set(const QString& key, const QVariant& value);
    bool contains(const QString& key) const;
    void remove(const QString& key);
    void clear();

    // ========== 便捷方法 ==========

    QString getString(const QString& key, const QString& defaultValue = QString());
    int getInt(const QString& key, int defaultValue = 0);
    bool getBool(const QString& key, bool defaultValue = false);
    double getDouble(const QString& key, double defaultValue = 0.0);

    // ========== 安全存储（加密） ==========

    bool setSecure(const QString& key, const QString& value);
    QString getSecure(const QString& key);
    bool containsSecure(const QString& key) const;
    void removeSecure(const QString& key);

    // ========== 配置导入导出 ==========

    bool exportToFile(const QString& filePath, bool includeSecure = false);
    bool importFromFile(const QString& filePath);

private:
    ConfigManager();
    ~ConfigManager();

    QString encrypt(const QString& plainText);
    QString decrypt(const QString& cipherText);

    std::unique_ptr<QSettings> m_settings;
    QMap<QString, QVariant> m_cache;
    mutable QMutex m_mutex;
    bool m_initialized = false;
};

#endif // CONFIGMANAGER_H
