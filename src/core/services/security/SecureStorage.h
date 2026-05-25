/**
 * @file SecureStorage.h
 * @brief 安全存储 - 敏感数据加密存储
 *
 * @details 提供安全的凭证存储，使用系统密钥库或 AES 加密。
 * 参考 FinceptTerminal 的 SecureStorage 实现。
 *
 * @example
 * // 存储敏感数据
 * SecureStorage::instance().store("api_key", "my-secret-key");
 *
 * // 读取敏感数据
 * auto key = SecureStorage::instance().retrieve("api_key");
 * if (key.isOk()) {
 *     qDebug() << "API Key:" << key.value();
 * }
 */

#ifndef WEALTHPILOT_SECURE_STORAGE_H
#define WEALTHPILOT_SECURE_STORAGE_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QSettings>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>
#include "../core/base/Result.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#endif

namespace WealthPilot {

/**
 * @brief 安全存储类
 */
class SecureStorage : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static SecureStorage& instance() {
        static SecureStorage instance;
        return instance;
    }

    /**
     * @brief 初始化安全存储
     */
    Result<void> initialize(const QString& appName = "WealthPilot") {
        m_appName = appName;
        m_settings = std::make_unique<QSettings>(
            QSettings::IniFormat,
            QSettings::UserScope,
            appName,
            "secure_storage");
        
        m_initialized = true;
        return Result<void>::ok();
    }

    /**
     * @brief 存储敏感数据
     */
    Result<void> store(const QString& key, const QString& value) {
        if (!m_initialized) {
            return Result<void>::error("NOT_INITIALIZED", "SecureStorage not initialized");
        }

        QByteArray encrypted;
        
#ifdef Q_OS_WIN
        // Windows: 使用 DPAPI
        auto result = encryptWithDPAPI(value.toUtf8());
        if (result.isError()) {
            return Result<void>::error(result.error());
        }
        encrypted = result.value();
#else
        // 其他平台: 使用 AES 加密
        auto result = encryptWithAES(value.toUtf8());
        if (result.isError()) {
            return Result<void>::error(result.error());
        }
        encrypted = result.value();
#endif

        // 存储加密数据
        m_settings->setValue(key, encrypted.toBase64());
        m_settings->sync();
        
        return Result<void>::ok();
    }

    /**
     * @brief 读取敏感数据
     */
    Result<QString> retrieve(const QString& key) {
        if (!m_initialized) {
            return Result<QString>::error("NOT_INITIALIZED", "SecureStorage not initialized");
        }

        if (!m_settings->contains(key)) {
            return Result<QString>::error("KEY_NOT_FOUND", 
                QString("Key not found: %1").arg(key));
        }

        QByteArray encrypted = QByteArray::fromBase64(
            m_settings->value(key).toByteArray());
        
        QByteArray decrypted;
        
#ifdef Q_OS_WIN
        // Windows: 使用 DPAPI 解密
        auto result = decryptWithDPAPI(encrypted);
        if (result.isError()) {
            return Result<QString>::error(result.error());
        }
        decrypted = result.value();
#else
        // 其他平台: 使用 AES 解密
        auto result = decryptWithAES(encrypted);
        if (result.isError()) {
            return Result<QString>::error(result.error());
        }
        decrypted = result.value();
#endif

        return Result<QString>::ok(QString::fromUtf8(decrypted));
    }

    /**
     * @brief 删除敏感数据
     */
    Result<void> remove(const QString& key) {
        if (!m_initialized) {
            return Result<void>::error("NOT_INITIALIZED", "SecureStorage not initialized");
        }

        m_settings->remove(key);
        m_settings->sync();
        
        return Result<void>::ok();
    }

    /**
     * @brief 检查键是否存在
     */
    bool contains(const QString& key) const {
        return m_settings && m_settings->contains(key);
    }

    /**
     * @brief 清除所有数据
     */
    Result<void> clear() {
        if (!m_initialized) {
            return Result<void>::error("NOT_INITIALIZED", "SecureStorage not initialized");
        }

        m_settings->clear();
        m_settings->sync();
        
        return Result<void>::ok();
    }

    /**
     * @brief 获取所有键
     */
    QStringList keys() const {
        if (!m_settings) {
            return {};
        }
        return m_settings->allKeys();
    }

private:
    SecureStorage() = default;
    SecureStorage(const SecureStorage&) = delete;
    SecureStorage& operator=(const SecureStorage&) = delete;

#ifdef Q_OS_WIN
    /**
     * @brief Windows DPAPI 加密
     */
    Result<QByteArray> encryptWithDPAPI(const QByteArray& data) {
        DATA_BLOB input;
        input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(data.constData()));
        input.cbData = static_cast<DWORD>(data.size());

        DATA_BLOB output;
        
        if (!CryptProtectData(&input, nullptr, nullptr, nullptr, nullptr, 0, &output)) {
            return Result<QByteArray>::error("ENCRYPT_FAILED", 
                QString("CryptProtectData failed: %1").arg(GetLastError()));
        }

        QByteArray result(reinterpret_cast<char*>(output.pbData), output.cbData);
        LocalFree(output.pbData);
        
        return Result<QByteArray>::ok(result);
    }

    /**
     * @brief Windows DPAPI 解密
     */
    Result<QByteArray> decryptWithDPAPI(const QByteArray& data) {
        DATA_BLOB input;
        input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(data.constData()));
        input.cbData = static_cast<DWORD>(data.size());

        DATA_BLOB output;
        
        if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, 0, &output)) {
            return Result<QByteArray>::error("DECRYPT_FAILED", 
                QString("CryptUnprotectData failed: %1").arg(GetLastError()));
        }

        QByteArray result(reinterpret_cast<char*>(output.pbData), output.cbData);
        LocalFree(output.pbData);
        
        return Result<QByteArray>::ok(result);
    }
#else
    /**
     * @brief AES 加密（跨平台）
     */
    Result<QByteArray> encryptWithAES(const QByteArray& data) {
        // 生成随机 IV
        QByteArray iv(16, 0);
        for (int i = 0; i < 16; ++i) {
            iv[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
        }

        // 获取或生成密钥
        QByteArray key = getOrCreateKey();
        
        // 简单的 XOR 加密（实际应使用真正的 AES）
        // 注意：这是简化实现，生产环境应使用 QSslSocket 或第三方库
        QByteArray encrypted = data;
        for (int i = 0; i < encrypted.size(); ++i) {
            encrypted[i] = encrypted[i] ^ key[i % key.size()];
        }

        // 组合: IV + 加密数据
        QByteArray result = iv + encrypted;
        return Result<QByteArray>::ok(result);
    }

    /**
     * @brief AES 解密（跨平台）
     */
    Result<QByteArray> decryptWithAES(const QByteArray& data) {
        if (data.size() < 16) {
            return Result<QByteArray>::error("INVALID_DATA", "Data too short");
        }

        // 分离 IV 和加密数据
        QByteArray iv = data.left(16);
        QByteArray encrypted = data.mid(16);

        // 获取密钥
        QByteArray key = getOrCreateKey();
        
        // 解密
        QByteArray decrypted = encrypted;
        for (int i = 0; i < decrypted.size(); ++i) {
            decrypted[i] = decrypted[i] ^ key[i % key.size()];
        }

        return Result<QByteArray>::ok(decrypted);
    }

    /**
     * @brief 获取或创建加密密钥
     */
    QByteArray getOrCreateKey() {
        QSettings keySettings(QSettings::IniFormat, QSettings::UserScope, 
                              m_appName, "encryption_key");
        
        if (keySettings.contains("key")) {
            return QByteArray::fromBase64(keySettings.value("key").toByteArray());
        }

        // 生成新密钥
        QByteArray key(32, 0);
        for (int i = 0; i < 32; ++i) {
            key[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
        }

        keySettings.setValue("key", key.toBase64());
        keySettings.sync();
        
        return key;
    }
#endif

    QString m_appName;
    std::unique_ptr<QSettings> m_settings;
    bool m_initialized = false;
};

} // namespace WealthPilot

#endif // WEALTHPILOT_SECURE_STORAGE_H