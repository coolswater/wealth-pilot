/**
 * @file ConfigManager.cpp
 * @brief 配置管理器实现
 */

#include "ConfigManager.h"
#include "../utils/Logger.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QCoreApplication>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#endif

ConfigManager::ConfigManager()
    : m_settings(std::make_unique<QSettings>(
          QSettings::IniFormat,
          QSettings::UserScope,
          QCoreApplication::organizationName().isEmpty() ? "Hexd" : QCoreApplication::organizationName(),
          QCoreApplication::applicationName().isEmpty() ? "WealthPilot" : QCoreApplication::applicationName()
          ))
{
}

ConfigManager::~ConfigManager()
{
    if (m_settings) {
        m_settings->sync();
    }
}

bool ConfigManager::initialize(const QString& orgName, const QString& appName)
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        return m_initialized;
    }

    if (!orgName.isEmpty() && !appName.isEmpty()) {
        m_settings = std::make_unique<QSettings>(
            QSettings::IniFormat,
            QSettings::UserScope,
            orgName,
            appName
            );
    }

    static const QStringList preloadKeys = {
        ConfigKeys::Theme,
        ConfigKeys::Language,
        ConfigKeys::ApiBaseUrl,
        ConfigKeys::AiProvider,
    };

    for (const QString& key : preloadKeys) {
        if (m_settings->contains(key)) {
            m_cache[key] = m_settings->value(key);
        }
    }

    m_initialized = true;
    LOG_INFO("ConfigManager initialized");

    return m_initialized;
}

QVariant ConfigManager::get(const QString& key, const QVariant& defaultValue)
{
    QMutexLocker locker(&m_mutex);

    if (m_cache.contains(key)) {
        return m_cache[key];
    }

    QVariant value = m_settings->value(key, defaultValue);

    if (value.isValid()) {
        m_cache[key] = value;
    }

    return value;
}

void ConfigManager::set(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&m_mutex);

    m_settings->setValue(key, value);
    m_cache[key] = value;

    LOG_DEBUG(QString("Config set: %1 = %2").arg(key, value.toString()));
}

bool ConfigManager::contains(const QString& key) const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.contains(key) || m_settings->contains(key);
}

void ConfigManager::remove(const QString& key)
{
    QMutexLocker locker(&m_mutex);

    m_settings->remove(key);
    m_cache.remove(key);
}

void ConfigManager::clear()
{
    QMutexLocker locker(&m_mutex);

    m_settings->clear();
    m_cache.clear();

    LOG_INFO("All config cleared");
}

QString ConfigManager::getString(const QString& key, const QString& defaultValue)
{
    return get(key, defaultValue).toString();
}

int ConfigManager::getInt(const QString& key, int defaultValue)
{
    return get(key, defaultValue).toInt();
}

bool ConfigManager::getBool(const QString& key, bool defaultValue)
{
    return get(key, defaultValue).toBool();
}

double ConfigManager::getDouble(const QString& key, double defaultValue)
{
    return get(key, defaultValue).toDouble();
}

bool ConfigManager::setSecure(const QString& key, const QString& value)
{
    QString encrypted = encrypt(value);
    if (encrypted.isEmpty()) {
        LOG_ERROR(QString("Failed to encrypt value for key: %1").arg(key));
        return false;
    }

    QMutexLocker locker(&m_mutex);
    m_settings->beginGroup("__secure__");
    m_settings->setValue(key, encrypted);
    m_settings->endGroup();

    LOG_INFO(QString("Secure value set for key: %1").arg(key));
    return true;
}

QString ConfigManager::getSecure(const QString& key)
{
    QMutexLocker locker(&m_mutex);

    m_settings->beginGroup("__secure__");
    QString encrypted = m_settings->value(key).toString();
    m_settings->endGroup();

    if (encrypted.isEmpty()) {
        return QString();
    }

    return decrypt(encrypted);
}

bool ConfigManager::containsSecure(const QString& key) const
{
    QMutexLocker locker(&m_mutex);

    m_settings->beginGroup("__secure__");
    bool result = m_settings->contains(key);
    m_settings->endGroup();

    return result;
}

void ConfigManager::removeSecure(const QString& key)
{
    QMutexLocker locker(&m_mutex);

    m_settings->beginGroup("__secure__");
    m_settings->remove(key);
    m_settings->endGroup();
}

QString ConfigManager::encrypt(const QString& plainText)
{
#ifdef Q_OS_WIN
    QByteArray plainData = plainText.toUtf8();
    DATA_BLOB inputBlob;
    inputBlob.pbData = reinterpret_cast<BYTE*>(plainData.data());
    inputBlob.cbData = plainData.size();

    DATA_BLOB outputBlob;
    ZeroMemory(&outputBlob, sizeof(outputBlob));

    if (CryptProtectData(&inputBlob, nullptr, nullptr, nullptr, nullptr, 0, &outputBlob)) {
        QByteArray result(reinterpret_cast<char*>(outputBlob.pbData), outputBlob.cbData);
        LocalFree(outputBlob.pbData);
        return result.toBase64();
    }

    LOG_ERROR("CryptProtectData failed");
    return QString();
#else
    LOG_WARNING("Secure storage not fully implemented on non-Windows platform");
    return plainText.toUtf8().toBase64();
#endif
}

QString ConfigManager::decrypt(const QString& cipherText)
{
#ifdef Q_OS_WIN
    QByteArray cipherData = QByteArray::fromBase64(cipherText.toUtf8());
    DATA_BLOB inputBlob;
    inputBlob.pbData = reinterpret_cast<BYTE*>(cipherData.data());
    inputBlob.cbData = cipherData.size();

    DATA_BLOB outputBlob;
    ZeroMemory(&outputBlob, sizeof(outputBlob));

    if (CryptUnprotectData(&inputBlob, nullptr, nullptr, nullptr, nullptr, 0, &outputBlob)) {
        QString result = QString::fromUtf8(reinterpret_cast<char*>(outputBlob.pbData), outputBlob.cbData);
        LocalFree(outputBlob.pbData);
        return result;
    }

    LOG_ERROR("CryptUnprotectData failed");
    return QString();
#else
    return QString::fromUtf8(QByteArray::fromBase64(cipherText.toUtf8()));
#endif
}

bool ConfigManager::exportToFile(const QString& filePath, bool includeSecure)
{
    QMutexLocker locker(&m_mutex);

    QJsonObject root;

    m_settings->beginGroup(QString());
    for (const QString& key : m_settings->allKeys()) {
        if (key.startsWith("__secure__")) continue;
        root[key] = QJsonValue::fromVariant(m_settings->value(key));
    }
    m_settings->endGroup();

    if (includeSecure) {
        m_settings->beginGroup("__secure__");
        QJsonObject secureObj;
        for (const QString& key : m_settings->childKeys()) {
            secureObj[key] = m_settings->value(key).toString();
        }
        root["__secure__"] = secureObj;
        m_settings->endGroup();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        LOG_ERROR(QString("Failed to open file for export: %1").arg(filePath));
        return false;
    }

    file.write(QJsonDocument(root).toJson());
    file.close();

    LOG_INFO(QString("Config exported to: %1").arg(filePath));
    return true;
}

bool ConfigManager::importFromFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(QString("Failed to open file for import: %1").arg(filePath));
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        LOG_ERROR("Invalid config file format");
        return false;
    }

    QMutexLocker locker(&m_mutex);

    QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (it.key() == "__secure__") {
            m_settings->beginGroup("__secure__");
            QJsonObject secureObj = it.value().toObject();
            for (auto sit = secureObj.begin(); sit != secureObj.end(); ++sit) {
                m_settings->setValue(sit.key(), sit.value().toString());
            }
            m_settings->endGroup();
        } else {
            m_settings->setValue(it.key(), it.value().toVariant());
            m_cache[it.key()] = it.value().toVariant();
        }
    }

    LOG_INFO(QString("Config imported from: %1").arg(filePath));
    return true;
}
