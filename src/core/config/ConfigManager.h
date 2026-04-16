/**
 * @file ConfigManager.h
 * @brief 配置管理器 - 统一管理应用配置和安全存储
 *
 * @details 主要功能：
 * - 普通配置：使用 QSettings 持久化
 * - 安全存储：使用 Windows DPAPI 加密（API密钥、密码等敏感信息）
 * - 内存缓存：提高读取速度
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
constexpr auto Theme = "appearance/theme";              ///< 主题
constexpr auto Language = "appearance/language";        ///< 语言
constexpr auto WindowGeometry = "window/geometry";      ///< 窗口几何
constexpr auto WindowState = "window/state";            ///< 窗口状态

// 网络
constexpr auto ApiBaseUrl = "network/api_base_url";     ///< API基础URL
constexpr auto RequestTimeout = "network/request_timeout"; ///< 请求超时

// CTP
constexpr auto CtpBrokerId = "ctp/broker_id";           ///< 经纪商ID
constexpr auto CtpMarketFront = "ctp/market_front";     ///< 行情前置
constexpr auto CtpTradeFront = "ctp/trade_front";       ///< 交易前置

// AI
constexpr auto AiProvider = "ai/provider";              ///< AI提供商
constexpr auto AiModel = "ai/model";                    ///< AI模型

// 安全存储（加密保存）
constexpr auto SecureCtpUserId = "secure/ctp_user_id";   ///< CTP用户ID
constexpr auto SecureCtpPassword = "secure/ctp_password"; ///< CTP密码
constexpr auto SecureApiKey = "secure/api_key";          ///< API密钥
}

/**
 * @brief 配置管理器类
 */
class ConfigManager : public Singleton<ConfigManager>
{
    friend class Singleton<ConfigManager>;

public:
    /**
     * @brief 初始化配置管理器
     * @param orgName 组织名称
     * @param appName 应用名称
     * @return 是否初始化成功
     */
    bool initialize(const QString& orgName = "Hexd", const QString& appName = "WealthPilot");

    // ========== 普通配置 ==========

    /**
     * @brief 获取配置值
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值
     */
    QVariant get(const QString& key, const QVariant& defaultValue = QVariant());
    
    /**
     * @brief 设置配置值
     * @param key 配置键
     * @param value 配置值
     */
    void set(const QString& key, const QVariant& value);
    
    /**
     * @brief 检查配置是否存在
     */
    bool contains(const QString& key) const;
    
    /**
     * @brief 删除配置
     */
    void remove(const QString& key);
    
    /**
     * @brief 清空所有配置
     */
    void clear();

    // ========== 类型访问 ==========

    QString getString(const QString& key, const QString& defaultValue = QString());
    int getInt(const QString& key, int defaultValue = 0);
    bool getBool(const QString& key, bool defaultValue = false);
    double getDouble(const QString& key, double defaultValue = 0.0);

    // ========== 安全存储（加密功能） ==========

    /**
     * @brief 设置安全配置（加密存储）
     */
    bool setSecure(const QString& key, const QString& value);
    
    /**
     * @brief 获取安全配置（解密）
     */
    QString getSecure(const QString& key);
    
    /**
     * @brief 检查安全配置是否存在
     */
    bool containsSecure(const QString& key) const;
    
    /**
     * @brief 删除安全配置
     */
    void removeSecure(const QString& key);

    // ========== 配置导入导出 ==========

    bool exportToFile(const QString& filePath, bool includeSecure = false);
    bool importFromFile(const QString& filePath);

private:
    ConfigManager();
    ~ConfigManager();

    QString encrypt(const QString& plainText);
    QString decrypt(const QString& cipherText);

    std::unique_ptr<QSettings> m_settings;   ///< QSettings实例
    QMap<QString, QVariant> m_cache;         ///< 内存缓存
    mutable QMutex m_mutex;                  ///< 线程安全锁
    bool m_initialized = false;              ///< 初始化标志
};

#endif // CONFIGMANAGER_H