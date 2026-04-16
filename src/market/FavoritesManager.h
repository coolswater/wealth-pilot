/**
* @file favoritesmanager.h
 * @brief 自选管理器
 * @details 管理用户的自选交易对列表，支持本地持久化存储
 */

#ifndef FAVORITESMANAGER_H
#define FAVORITESMANAGER_H

#include <QObject>
#include <QStringList>
#include <QSettings>
#include <QMutex>

class FavoritesManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例（线程安全）
     */
    static FavoritesManager* instance();

    /**
     * @brief 添加自选交易对
     * @param symbol 交易对符号，如 BTC-USDT
     * @return 是否添加成功（重复添加返回false）
     */
    bool addFavorite(const QString &symbol);

    /**
     * @brief 移除自选交易对
     * @param symbol 交易对符号
     */
    void removeFavorite(const QString &symbol);

    /**
     * @brief 检查是否为自选
     */
    bool isFavorite(const QString &symbol);

    /**
     * @brief 获取所有自选列表
     */
    QStringList getFavorites();

    /**
     * @brief 清空所有自选
     */
    void clearFavorites();

    signals:
        /**
         * @brief 自选列表变更信号
         */
        void favoritesChanged();

private:
    explicit FavoritesManager(QObject *parent = nullptr);
    ~FavoritesManager();

    void loadFavorites();  ///< 从配置文件加载
    void saveFavorites();  ///< 保存到配置文件

    mutable QMutex m_mutex;           ///< 线程安全锁
    QStringList m_favorites;          ///< 自选列表
    QSettings *m_settings = nullptr;  ///< 配置存储
    static FavoritesManager *s_instance;
};

#endif // FAVORITESMANAGER_H