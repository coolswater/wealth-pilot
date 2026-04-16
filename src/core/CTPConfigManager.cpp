/**
 * @file CTPConfigManager.cpp
 * @brief CTP配置管理器实现
 */

#include "CTPConfigManager.h"
#include "ConfigManager.h"
#include "../utils/Logger.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QMutexLocker>
#include <QSettings>
#include <QDateTime>

namespace CTP {

// ========== PIMPL实现 ==========

struct CTPConfigManager::Impl {
    QMap<QString, CTPBrokerConfig> brokers;
    QString currentBrokerId;
    QString configPath;
    mutable QMutex mutex;
    
    Impl() {
        configPath = QCoreApplication::applicationDirPath() + "/config/ctp_brokers.json";
    }
};

// ========== 单例实现 ==========

CTPConfigManager* CTPConfigManager::instance()
{
    static CTPConfigManager instance;
    return &instance;
}

CTPConfigManager::CTPConfigManager()
    : d(std::make_unique<Impl>())
{
    LOG_DEBUG("CTPConfigManager created");
}

CTPConfigManager::~CTPConfigManager()
{
    saveConfig();
    LOG_DEBUG("CTPConfigManager destroyed");
}

// ========== 初始化 ==========

bool CTPConfigManager::initialize(const QString& configPath)
{
    QMutexLocker locker(&d->mutex);
    
    if (!configPath.isEmpty()) {
        d->configPath = configPath;
    }
    
    // 确保配置目录存在
    QFileInfo fileInfo(d->configPath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 加载预设配置
    loadPresetBrokers();
    
    // 尝试加载用户配置
    if (QFile::exists(d->configPath)) {
        QFile file(d->configPath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(data, &error);
            if (error.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject root = doc.object();
                
                // 加载服务商配置
                QJsonArray brokersArr = root["brokers"].toArray();
                for (const auto& item : brokersArr) {
                    CTPBrokerConfig config = CTPBrokerConfig::fromJson(item.toObject());
                    if (config.isValid()) {
                        d->brokers[config.id] = config;
                    }
                }
                
                // 加载当前服务商
                d->currentBrokerId = root["currentBrokerId"].toString();
                
                LOG_INFO(QString("CTPConfigManager loaded %1 brokers from %2")
                    .arg(d->brokers.size()).arg(d->configPath));
            }
        }
    }
    
    // 如果没有当前服务商，设置默认
    if (d->currentBrokerId.isEmpty() && !d->brokers.isEmpty()) {
        d->currentBrokerId = d->brokers.firstKey();
    }
    
    return true;
}

bool CTPConfigManager::saveConfig()
{
    QMutexLocker locker(&d->mutex);
    
    QJsonObject root;
    
    // 保存服务商配置
    QJsonArray brokersArr;
    for (const auto& config : d->brokers) {
        brokersArr.append(config.toJson());
    }
    root["brokers"] = brokersArr;
    root["currentBrokerId"] = d->currentBrokerId;
    
    // 写入文件
    QFile file(d->configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open config file for writing: %1").arg(d->configPath));
        return false;
    }
    
    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    LOG_DEBUG(QString("CTPConfigManager saved to %1").arg(d->configPath));
    return true;
}

bool CTPConfigManager::reloadConfig()
{
    return initialize(d->configPath);
}

// ========== 服务商管理 ==========

QList<CTPBrokerConfig> CTPConfigManager::getAllBrokers() const
{
    QMutexLocker locker(&d->mutex);
    return d->brokers.values();
}

QList<CTPBrokerConfig> CTPConfigManager::getEnabledBrokers() const
{
    QMutexLocker locker(&d->mutex);
    QList<CTPBrokerConfig> result;
    for (const auto& config : d->brokers) {
        if (config.isEnabled) {
            result.append(config);
        }
    }
    return result;
}

std::optional<CTPBrokerConfig> CTPConfigManager::getBroker(const QString& id) const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->brokers.find(id);
    if (it != d->brokers.end()) {
        return it.value();
    }
    return std::nullopt;
}

void CTPConfigManager::setBroker(const CTPBrokerConfig& config)
{
    if (!config.isValid()) {
        LOG_WARNING(QString("Invalid broker config: %1").arg(config.id));
        return;
    }
    
    QMutexLocker locker(&d->mutex);
    bool isNew = !d->brokers.contains(config.id);
    d->brokers[config.id] = config;
    locker.unlock();
    
    if (isNew) {
        emit brokerAdded(config.id);
    } else {
        emit brokerUpdated(config.id);
    }
    
    LOG_INFO(QString("Broker config saved: %1 (%2)").arg(config.id, config.name));
}

bool CTPConfigManager::removeBroker(const QString& id)
{
    QMutexLocker locker(&d->mutex);
    
    if (!d->brokers.contains(id)) {
        return false;
    }
    
    d->brokers.remove(id);
    
    // 如果删除的是当前服务商，切换到第一个可用的
    if (d->currentBrokerId == id) {
        d->currentBrokerId = d->brokers.isEmpty() ? QString() : d->brokers.firstKey();
    }
    
    locker.unlock();
    emit brokerRemoved(id);
    
    LOG_INFO(QString("Broker removed: %1").arg(id));
    return true;
}

bool CTPConfigManager::hasBroker(const QString& id) const
{
    QMutexLocker locker(&d->mutex);
    return d->brokers.contains(id);
}

// ========== 当前服务商 ==========

QString CTPConfigManager::currentBrokerId() const
{
    QMutexLocker locker(&d->mutex);
    return d->currentBrokerId;
}

bool CTPConfigManager::setCurrentBroker(const QString& brokerId)
{
    QMutexLocker locker(&d->mutex);
    
    if (!d->brokers.contains(brokerId)) {
        LOG_WARNING(QString("Broker not found: %1").arg(brokerId));
        return false;
    }
    
    if (d->currentBrokerId == brokerId) {
        return true;
    }
    
    d->currentBrokerId = brokerId;
    locker.unlock();
    
    emit currentBrokerChanged(brokerId);
    LOG_INFO(QString("Current broker changed to: %1").arg(brokerId));
    
    return true;
}

std::optional<CTPBrokerConfig> CTPConfigManager::currentBroker() const
{
    QMutexLocker locker(&d->mutex);
    
    if (d->currentBrokerId.isEmpty()) {
        return std::nullopt;
    }
    
    auto it = d->brokers.find(d->currentBrokerId);
    if (it != d->brokers.end()) {
        return it.value();
    }
    
    return std::nullopt;
}

// ========== 用户凭证管理 ==========

void CTPConfigManager::setUserCredentials(const QString& brokerId, const QString& userId, const QString& password)
{
    QMutexLocker locker(&d->mutex);
    
    auto it = d->brokers.find(brokerId);
    if (it == d->brokers.end()) {
        LOG_WARNING(QString("Broker not found: %1").arg(brokerId));
        return;
    }
    
    it->userId = userId;
    it->encryptedPassword = encryptPassword(password);
    
    locker.unlock();
    emit brokerUpdated(brokerId);
    
    LOG_INFO(QString("User credentials saved for broker: %1").arg(brokerId));
}

QString CTPConfigManager::getUserId(const QString& brokerId) const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->brokers.find(brokerId);
    if (it != d->brokers.end()) {
        return it->userId;
    }
    return QString();
}

QString CTPConfigManager::getPassword(const QString& brokerId) const
{
    QMutexLocker locker(&d->mutex);
    auto it = d->brokers.find(brokerId);
    if (it != d->brokers.end() && !it->encryptedPassword.isEmpty()) {
        return decryptPassword(it->encryptedPassword);
    }
    return QString();
}

void CTPConfigManager::clearUserCredentials(const QString& brokerId)
{
    QMutexLocker locker(&d->mutex);
    
    auto it = d->brokers.find(brokerId);
    if (it != d->brokers.end()) {
        it->userId.clear();
        it->encryptedPassword.clear();
    }
    
    locker.unlock();
    emit brokerUpdated(brokerId);
}

// ========== 加密/解密 ==========

QString CTPConfigManager::encryptPassword(const QString& password) const
{
    // 使用 ConfigManager 的安全存储功能
    // 生成一个临时的 key，然后获取加密后的值
    QString tempKey = QString("__temp_encrypt_%1").arg(QDateTime::currentMSecsSinceEpoch());
    ConfigManager::instance()->setSecure(tempKey, password);
    
    // 从 settings 中获取加密后的值
    QSettings settings;
    settings.beginGroup("__secure__");
    QString encrypted = settings.value(tempKey).toString();
    settings.remove(tempKey);
    settings.endGroup();
    
    return encrypted;
}

QString CTPConfigManager::decryptPassword(const QString& encrypted) const
{
    if (encrypted.isEmpty()) return QString();
    
    // 直接使用 ConfigManager 的解密功能
    // 将加密值临时存储，然后读取
    QString tempKey = QString("__temp_decrypt_%1").arg(QDateTime::currentMSecsSinceEpoch());
    QSettings settings;
    settings.beginGroup("__secure__");
    settings.setValue(tempKey, encrypted);
    settings.endGroup();
    
    QString decrypted = ConfigManager::instance()->getSecure(tempKey);
    
    settings.beginGroup("__secure__");
    settings.remove(tempKey);
    settings.endGroup();
    
    return decrypted;
}

// ========== 预设服务商 ==========

CTPBrokerConfig CTPConfigManager::getSimNow24Config()
{
    CTPBrokerConfig config;
    config.id = "simnow_24";
    config.name = "SimNow 7x24模拟";
    config.brokerId = "9999";
    config.description = "SimNow 7x24小时模拟环境，适合开发和测试";
    config.website = "https://www.simnow.com.cn";
    config.isSimulation = true;
    config.isEnabled = true;
    config.requireAuth = false;
    
    // 7x24环境前置地址
    config.marketFronts = {
        "tcp://180.168.146.187:10131",
        "tcp://180.168.146.187:10132",
        "tcp://218.202.237.33:10131",
        "tcp://218.202.237.33:10132"
    };
    
    config.tradingFronts = {
        "tcp://180.168.146.187:10101",
        "tcp://180.168.146.187:10102",
        "tcp://218.202.237.33:10101",
        "tcp://218.202.237.33:10102"
    };
    
    return config;
}

CTPBrokerConfig CTPConfigManager::getSimNowTelecomConfig()
{
    CTPBrokerConfig config;
    config.id = "simnow_telecom";
    config.name = "SimNow 电信模拟";
    config.brokerId = "9999";
    config.description = "SimNow 电信环境，交易时间与实盘一致";
    config.website = "https://www.simnow.com.cn";
    config.isSimulation = true;
    config.isEnabled = true;
    config.requireAuth = false;
    
    // 电信环境前置地址
    config.marketFronts = {
        "tcp://218.202.237.33:10212",
        "tcp://218.202.237.33:10213"
    };
    
    config.tradingFronts = {
        "tcp://218.202.237.33:10202",
        "tcp://218.202.237.33:10203"
    };
    
    return config;
}

CTPBrokerConfig CTPConfigManager::getSimNowMobileConfig()
{
    CTPBrokerConfig config;
    config.id = "simnow_mobile";
    config.name = "SimNow 移动模拟";
    config.brokerId = "9999";
    config.description = "SimNow 移动环境，交易时间与实盘一致";
    config.website = "https://www.simnow.com.cn";
    config.isSimulation = true;
    config.isEnabled = true;
    config.requireAuth = false;
    
    // 移动环境前置地址
    config.marketFronts = {
        "tcp://218.202.237.33:10222",
        "tcp://218.202.237.33:10223"
    };
    
    config.tradingFronts = {
        "tcp://218.202.237.33:10212",
        "tcp://218.202.237.33:10213"
    };
    
    return config;
}

void CTPConfigManager::loadPresetBrokers()
{
    // 添加预设服务商（不覆盖已有配置）
    auto addPreset = [this](const CTPBrokerConfig& config) {
        if (!d->brokers.contains(config.id)) {
            d->brokers[config.id] = config;
        }
    };
    
    // SimNow 环境
    addPreset(getSimNow24Config());
    addPreset(getSimNowTelecomConfig());
    addPreset(getSimNowMobileConfig());
    
    // 常用期货公司预设（需要用户自行填写账号密码）
    // 国泰君安
    CTPBrokerConfig gtja;
    gtja.id = "gtja";
    gtja.name = "国泰君安期货";
    gtja.brokerId = "1001";  // 示例，实际需要查询
    gtja.description = "国泰君安期货有限公司";
    gtja.website = "https://www.gtjaqh.com";
    gtja.isSimulation = false;
    gtja.isEnabled = false;  // 默认禁用，需要用户配置
    gtja.requireAuth = true;
    // 用户需要自行填写前置地址
    d->brokers[gtja.id] = gtja;
    
    // 银河期货
    CTPBrokerConfig yh;
    yh.id = "yh";
    yh.name = "银河期货";
    yh.brokerId = "1002";  // 示例，实际需要查询
    yh.description = "银河期货有限公司";
    yh.website = "https://www.yhqh.com.cn";
    yh.isSimulation = false;
    yh.isEnabled = false;
    yh.requireAuth = true;
    d->brokers[yh.id] = yh;
    
    // 中信期货
    CTPBrokerConfig zx;
    zx.id = "zx";
    zx.name = "中信期货";
    zx.brokerId = "1003";  // 示例，实际需要查询
    zx.description = "中信期货有限公司";
    zx.website = "https://www.citicsf.com";
    zx.isSimulation = false;
    zx.isEnabled = false;
    zx.requireAuth = true;
    d->brokers[zx.id] = zx;
    
    // 永安期货
    CTPBrokerConfig ya;
    ya.id = "ya";
    ya.name = "永安期货";
    ya.brokerId = "1004";  // 示例，实际需要查询
    ya.description = "永安期货股份有限公司";
    ya.website = "https://www.yafco.com";
    ya.isSimulation = false;
    ya.isEnabled = false;
    ya.requireAuth = true;
    d->brokers[ya.id] = ya;
    
    LOG_DEBUG(QString("Loaded %1 preset brokers").arg(d->brokers.size()));
}

void CTPConfigManager::resetToDefaults()
{
    QMutexLocker locker(&d->mutex);
    d->brokers.clear();
    d->currentBrokerId.clear();
    locker.unlock();
    
    loadPresetBrokers();
    
    // 设置默认当前服务商
    if (!d->brokers.isEmpty()) {
        d->currentBrokerId = "simnow_24";
    }
    
    LOG_INFO("CTPConfigManager reset to defaults");
}

} // namespace CTP
