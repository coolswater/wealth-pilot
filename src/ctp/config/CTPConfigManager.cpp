/**
 * @file CTPConfigManager.cpp
 * @brief CTP Configuration Manager Implementation
 */

#include "CTPConfigManager.h"
#include "../../core/config/ConfigManager.h"
#include "../../utils/Logger.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QMutexLocker>
#include <QSettings>
#include <QDateTime>

namespace CTP {

// ========== PIMPL Implementation ==========

struct CTPConfigManager::Impl {
    QMap<QString, CTPBrokerConfig> brokers;
    QString currentBrokerId;
    QString configPath;
    mutable QMutex mutex;
    
    Impl() {
        configPath = QCoreApplication::applicationDirPath() + "/config/ctp_brokers.json";
    }
};

// ========== Singleton Implementation ==========

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

// ========== Initialization ==========

bool CTPConfigManager::initialize(const QString& configPath)
{
    QMutexLocker locker(&d->mutex);
    
    if (!configPath.isEmpty()) {
        d->configPath = configPath;
    }
    
    // Ensure config directory exists
    QFileInfo fileInfo(d->configPath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // Load preset configs
    loadPresetBrokers();
    
    // Try to load user config
    if (QFile::exists(d->configPath)) {
        QFile file(d->configPath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            
            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(data, &error);
            if (error.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject root = doc.object();
                
                // Load broker configs
                QJsonArray brokersArr = root["brokers"].toArray();
                for (const auto& item : brokersArr) {
                    CTPBrokerConfig config = CTPBrokerConfig::fromJson(item.toObject());
                    if (config.isValid()) {
                        d->brokers[config.id] = config;
                    }
                }
                
                // Load current broker
                d->currentBrokerId = root["currentBrokerId"].toString();
                
                LOG_INFO(QString("CTPConfigManager loaded %1 brokers from %2")
                    .arg(d->brokers.size()).arg(d->configPath));
            }
        }
    }
    
    // If no current broker, set default
    if (d->currentBrokerId.isEmpty() && !d->brokers.isEmpty()) {
        d->currentBrokerId = d->brokers.firstKey();
    }
    
    return true;
}

bool CTPConfigManager::saveConfig()
{
    QMutexLocker locker(&d->mutex);
    
    QJsonObject root;
    
    // Save broker configs
    QJsonArray brokersArr;
    for (const auto& config : d->brokers) {
        brokersArr.append(config.toJson());
    }
    root["brokers"] = brokersArr;
    root["currentBrokerId"] = d->currentBrokerId;
    
    // Write to file
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

// ========== Broker Management ==========

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
    
    // If removed current broker, switch to first available
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

// ========== Current Broker ==========

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

// ========== User Credentials Management ==========

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

// ========== Encryption/Decryption ==========

QString CTPConfigManager::encryptPassword(const QString& password) const
{
    // Use ConfigManager's secure storage
    QString tempKey = QString("__temp_encrypt_%1").arg(QDateTime::currentMSecsSinceEpoch());
    ConfigManager::instance()->setSecure(tempKey, password);
    
    // Get encrypted value from settings
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
    
    // Use ConfigManager's secure storage
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

// ========== Preset Brokers ==========

CTPBrokerConfig CTPConfigManager::getSimNow24Config()
{
    CTPBrokerConfig config;
    config.id = "simnow_24";
    config.name = "SimNow 7x24 Simulation";
    config.brokerId = "9999";
    config.description = "SimNow 7x24 simulation environment for development and testing";
    config.website = "https://www.simnow.com.cn";
    config.isSimulation = true;
    config.isEnabled = true;
    config.requireAuth = false;
    
    // 7x24 environment front addresses
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
    config.name = "SimNow Telecom Simulation";
    config.brokerId = "9999";
    config.description = "SimNow telecom environment, trading time matches real market";
    config.website = "https://www.simnow.com.cn";
    config.isSimulation = true;
    config.isEnabled = true;
    config.requireAuth = false;
    
    // Telecom environment front addresses
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
    config.name = "SimNow Mobile Simulation";
    config.brokerId = "9999";
    config.description = "SimNow mobile environment, trading time matches real market";
    config.website = "https://www.simnow.com.cn";
    config.isSimulation = true;
    config.isEnabled = true;
    config.requireAuth = false;
    
    // Mobile environment front addresses
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
    // Add preset brokers (don't overwrite existing)
    auto addPreset = [this](const CTPBrokerConfig& config) {
        if (!d->brokers.contains(config.id)) {
            d->brokers[config.id] = config;
        }
    };
    
    // SimNow environments
    addPreset(getSimNow24Config());
    addPreset(getSimNowTelecomConfig());
    addPreset(getSimNowMobileConfig());
    
    // Common futures companies (users need to fill in credentials)
    // Guotai Junan
    CTPBrokerConfig gtja;
    gtja.id = "gtja";
    gtja.name = "Guotai Junan Futures";
    gtja.brokerId = "1001";  // Example, actual needs to be verified
    gtja.description = "Guotai Junan Futures Co., Ltd.";
    gtja.website = "https://www.gtjaqh.com";
    gtja.isSimulation = false;
    gtja.isEnabled = false;  // Disabled by default, needs user config
    gtja.requireAuth = true;
    d->brokers[gtja.id] = gtja;
    
    // Yinhe Futures
    CTPBrokerConfig yh;
    yh.id = "yh";
    yh.name = "Yinhe Futures";
    yh.brokerId = "1002";  // Example, actual needs to be verified
    yh.description = "Yinhe Futures Co., Ltd.";
    yh.website = "https://www.yhqh.com.cn";
    yh.isSimulation = false;
    yh.isEnabled = false;
    yh.requireAuth = true;
    d->brokers[yh.id] = yh;
    
    // CITIC Futures
    CTPBrokerConfig zx;
    zx.id = "zx";
    zx.name = "CITIC Futures";
    zx.brokerId = "1003";  // Example, actual needs to be verified
    zx.description = "CITIC Futures Co., Ltd.";
    zx.website = "https://www.citicsf.com";
    zx.isSimulation = false;
    zx.isEnabled = false;
    zx.requireAuth = true;
    d->brokers[zx.id] = zx;
    
    // Yongan Futures
    CTPBrokerConfig ya;
    ya.id = "ya";
    ya.name = "Yongan Futures";
    ya.brokerId = "1004";  // Example, actual needs to be verified
    ya.description = "Yongan Futures Co., Ltd.";
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
    
    // Set default current broker
    if (!d->brokers.isEmpty()) {
        d->currentBrokerId = "simnow_24";
    }
    
    LOG_INFO("CTPConfigManager reset to defaults");
}

} // namespace CTP
