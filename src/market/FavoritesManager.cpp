/**
 * @file favoritesmanager.cpp
 * @brief 自选管理器实现
 */

#include "favoritesmanager.h"
#include <QStandardPaths>
#include <QDir>

FavoritesManager* FavoritesManager::s_instance = nullptr;

FavoritesManager* FavoritesManager::instance()
{
    // 双重检查锁定模式实现线程安全单例
    if (!s_instance) {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!s_instance) {
            s_instance = new FavoritesManager();
        }
    }
    return s_instance;
}

FavoritesManager::FavoritesManager(QObject *parent)
    : QObject(parent)
{
    // 初始化配置存储路径
    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    m_settings = new QSettings(
        QDir(configPath).filePath("favorites.ini"),
        QSettings::IniFormat,
        this
    );

    loadFavorites();
}

FavoritesManager::~FavoritesManager()
{
    saveFavorites();
}

bool FavoritesManager::addFavorite(const QString &symbol)
{
    QMutexLocker locker(&m_mutex);

    QString upperSymbol = symbol.toUpper().trimmed();
    if (upperSymbol.isEmpty() || m_favorites.contains(upperSymbol)) {
        return false;
    }

    m_favorites.append(upperSymbol);
    saveFavorites();

    emit favoritesChanged();
    return true;
}

void FavoritesManager::removeFavorite(const QString &symbol)
{
    QMutexLocker locker(&m_mutex);

    QString upperSymbol = symbol.toUpper().trimmed();
    if (m_favorites.removeOne(upperSymbol)) {
        saveFavorites();
        emit favoritesChanged();
    }
}

bool FavoritesManager::isFavorite(const QString &symbol)
{
    QMutexLocker locker(&m_mutex);
    return m_favorites.contains(symbol.toUpper().trimmed());
}

QStringList FavoritesManager::getFavorites()
{
    QMutexLocker locker(&m_mutex);
    return m_favorites;
}

void FavoritesManager::clearFavorites()
{
    QMutexLocker locker(&m_mutex);
    m_favorites.clear();
    saveFavorites();
    emit favoritesChanged();
}

void FavoritesManager::loadFavorites()
{
    m_favorites = m_settings->value("favorites/list").toStringList();

    // 去重和格式化
    QStringList uniqueList;
    for (const QString &sym : m_favorites) {
        QString upper = sym.toUpper().trimmed();
        if (!upper.isEmpty() && !uniqueList.contains(upper)) {
            uniqueList.append(upper);
        }
    }
    m_favorites = uniqueList;
}

void FavoritesManager::saveFavorites()
{
    m_settings->setValue("favorites/list", m_favorites);
    m_settings->sync();
}