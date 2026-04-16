/**
 * @file CTPConfigManager.h
 * @brief CTP配置管理器 - 支持多服务商配置和动态切换
 *
 * @details 功能：
 * - 多服务商配置管理（SimNow、各期货公司等）
 * - 配置持久化存储
 * - 动态切换服务商
 * - 支持不同的认证方式
 * - 环境区分（生产/模拟/测试）
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef CTPCONFIGMANAGER_H
#define CTPCONFIGMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QSettings>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

namespace CTP {

/**
 * @brief CTP服务商配置
 */
struct CTPBrokerConfig {
    QString id;                     // 服务商唯一标识（如 "simnow", "gtja", "yh"）
    QString name;                   // 服务商名称（如 "SimNow模拟", "国泰君安", "银河期货"）
    QString brokerId;               // 经纪公司代码
    
    // 行情前置地址列表（支持多个，自动切换）
    QStringList marketFronts;
    
    // 交易前置地址列表（支持多个，自动切换）
    QStringList tradingFronts;
    
    // 认证信息
    bool requireAuth = false;       // 是否需要认证（CTP 6.6.1+）
    QString defaultAppId;           // 默认AppID
    QString defaultAuthCode;        // 默认认证码
    
    // 其他配置
    QString description;            // 描述信息
    QString website;                // 官网地址
    bool isSimulation = true;       // 是否为模拟环境
    bool isEnabled = true;          // 是否启用
    
    // 用户凭证（加密存储）
    QString userId;
    QString encryptedPassword;      // 加密后的密码
    
    /**
     * @brief 从JSON对象加载
     */
    static CTPBrokerConfig fromJson(const QJsonObject& json) {
        CTPBrokerConfig config;
        config.id = json["id"].toString();
        config.name = json["name"].toString();
        config.brokerId = json["brokerId"].toString();
        
        // 解析行情前置
        QJsonArray marketArr = json["marketFronts"].toArray();
        for (const auto& addr : marketArr) {
            config.marketFronts.append(addr.toString());
        }
        
        // 解析交易前置
        QJsonArray tradingArr = json["tradingFronts"].toArray();
        for (const auto& addr : tradingArr) {
            config.tradingFronts.append(addr.toString());
        }
        
        config.requireAuth = json["requireAuth"].toBool(false);
        config.defaultAppId = json["defaultAppId"].toString();
        config.defaultAuthCode = json["defaultAuthCode"].toString();
        config.description = json["description"].toString();
        config.website = json["website"].toString();
        config.isSimulation = json["isSimulation"].toBool(true);
        config.isEnabled = json["isEnabled"].toBool(true);
        
        return config;
    }
    
    /**
     * @brief 转换为JSON对象
     */
    QJsonObject toJson() const {
        QJsonObject json;
        json["id"] = id;
        json["name"] = name;
        json["brokerId"] = brokerId;
        
        QJsonArray marketArr;
        for (const auto& addr : marketFronts) {
            marketArr.append(addr);
        }
        json["marketFronts"] = marketArr;
        
        QJsonArray tradingArr;
        for (const auto& addr : tradingFronts) {
            tradingArr.append(addr);
        }
        json["tradingFronts"] = tradingArr;
        
        json["requireAuth"] = requireAuth;
        json["defaultAppId"] = defaultAppId;
        json["defaultAuthCode"] = defaultAuthCode;
        json["description"] = description;
        json["website"] = website;
        json["isSimulation"] = isSimulation;
        json["isEnabled"] = isEnabled;
        
        return json;
    }
    
    /**
     * @brief 获取第一个可用的行情前置
     */
    QString getFirstMarketFront() const {
        return marketFronts.isEmpty() ? QString() : marketFronts.first();
    }
    
    /**
     * @brief 获取第一个可用的交易前置
     */
    QString getFirstTradingFront() const {
        return tradingFronts.isEmpty() ? QString() : tradingFronts.first();
    }
    
    /**
     * @brief 验证配置是否有效
     */
    bool isValid() const {
        return !id.isEmpty() && 
               !brokerId.isEmpty() && 
               !marketFronts.isEmpty() && 
               !tradingFronts.isEmpty();
    }
};

/**
 * @brief CTP配置管理器
 * @details 单例模式，管理所有CTP服务商配置
 */
class CTPConfigManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static CTPConfigManager* instance();

    /**
     * @brief 初始化配置管理器
     * @param configPath 配置文件路径，为空则使用默认路径
     */
    bool initialize(const QString& configPath = QString());

    /**
     * @brief 保存配置到文件
     */
    bool saveConfig();

    /**
     * @brief 重新加载配置
     */
    bool reloadConfig();

    /////////////////////////////////////////////////////////////////////////
    /// 服务商管理
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 获取所有服务商配置
     */
    QList<CTPBrokerConfig> getAllBrokers() const;

    /**
     * @brief 获取启用的服务商列表
     */
    QList<CTPBrokerConfig> getEnabledBrokers() const;

    /**
     * @brief 根据ID获取服务商配置
     */
    std::optional<CTPBrokerConfig> getBroker(const QString& id) const;

    /**
     * @brief 添加或更新服务商配置
     */
    void setBroker(const CTPBrokerConfig& config);

    /**
     * @brief 删除服务商配置
     */
    bool removeBroker(const QString& id);

    /**
     * @brief 检查服务商是否存在
     */
    bool hasBroker(const QString& id) const;

    /////////////////////////////////////////////////////////////////////////
    /// 当前服务商
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 获取当前选中的服务商ID
     */
    QString currentBrokerId() const;

    /**
     * @brief 设置当前服务商
     */
    bool setCurrentBroker(const QString& brokerId);

    /**
     * @brief 获取当前服务商配置
     */
    std::optional<CTPBrokerConfig> currentBroker() const;

    /////////////////////////////////////////////////////////////////////////
    /// 用户凭证管理
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 设置用户凭证
     * @param brokerId 服务商ID
     * @param userId 用户ID
     * @param password 密码（明文，内部加密存储）
     */
    void setUserCredentials(const QString& brokerId, const QString& userId, const QString& password);

    /**
     * @brief 获取用户ID
     */
    QString getUserId(const QString& brokerId) const;

    /**
     * @brief 获取解密后的密码
     */
    QString getPassword(const QString& brokerId) const;

    /**
     * @brief 清除用户凭证
     */
    void clearUserCredentials(const QString& brokerId);

    /////////////////////////////////////////////////////////////////////////
    /// 预设服务商
    /////////////////////////////////////////////////////////////////////////

    /**
     * @brief 获取SimNow配置（7x24环境）
     */
    static CTPBrokerConfig getSimNow24Config();

    /**
     * @brief 获取SimNow配置（电信环境）
     */
    static CTPBrokerConfig getSimNowTelecomConfig();

    /**
     * @brief 获取SimNow配置（移动环境）
     */
    static CTPBrokerConfig getSimNowMobileConfig();

    /**
     * @brief 加载预设服务商配置
     */
    void loadPresetBrokers();

    /**
     * @brief 重置为默认配置
     */
    void resetToDefaults();

signals:
    /**
     * @brief 当前服务商变更
     */
    void currentBrokerChanged(const QString& brokerId);

    /**
     * @brief 服务商配置更新
     */
    void brokerUpdated(const QString& brokerId);

    /**
     * @brief 服务商添加
     */
    void brokerAdded(const QString& brokerId);

    /**
     * @brief 服务商删除
     */
    void brokerRemoved(const QString& brokerId);

private:
    CTPConfigManager();
    ~CTPConfigManager();
    CTPConfigManager(const CTPConfigManager&) = delete;
    CTPConfigManager& operator=(const CTPConfigManager&) = delete;

    // 加密/解密密码
    QString encryptPassword(const QString& password) const;
    QString decryptPassword(const QString& encrypted) const;

    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace CTP

#endif // CTPCONFIGMANAGER_H
