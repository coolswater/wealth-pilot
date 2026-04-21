/**
 * @file NavigationManager.h
 * @brief 导航管理器 - 页面路由和导航控制
 *
 * @details 功能：
 * - 页面注册与管理
 * - 页面跳转控制
 * - 导航历史记录
 * - 页面参数传递
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef NAVIGATIONMANAGER_H
#define NAVIGATIONMANAGER_H

#include <QObject>
#include <QHash>
#include <QStack>
#include <QVariantMap>
#include <memory>

class BasePage;

/**
 * @brief 导航记录
 */
struct NavigationRecord {
    QString pageId;
    QVariantMap params;
    QDateTime timestamp;
};

/**
 * @brief 导航管理器
 */
class NavigationManager : public QObject
{
    Q_OBJECT

public:
    static NavigationManager& instance();

    /**
     * @brief 初始化导航管理器
     */
    bool initialize();

    /**
     * @brief 注册页面
     */
    void registerPage(const QString& pageId, BasePage* page);

    /**
     * @brief 注销页面
     */
    void unregisterPage(const QString& pageId);

    /**
     * @brief 获取页面
     */
    BasePage* getPage(const QString& pageId) const;

    /**
     * @brief 导航到指定页面
     */
    bool navigateTo(const QString& pageId, const QVariantMap& params = QVariantMap());

    /**
     * @brief 返回上一页
     */
    bool goBack();

    /**
     * @brief 返回首页
     */
    bool goHome();

    /**
     * @brief 获取当前页面
     */
    BasePage* currentPage() const;

    /**
     * @brief 获取当前页面ID
     */
    QString currentPageId() const;

    /**
     * @brief 是否可以返回
     */
    bool canGoBack() const;

    /**
     * @brief 清空导航历史
     */
    void clearHistory();

    /**
     * @brief 获取导航历史
     */
    QVector<NavigationRecord> getHistory() const;

signals:
    void pageChanged(const QString& pageId, const QVariantMap& params);
    void pageNotFound(const QString& pageId);
    void navigationError(const QString& error);

private:
    NavigationManager(QObject* parent = nullptr);
    ~NavigationManager() override;
    Q_DISABLE_COPY(NavigationManager)

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // NAVIGATIONMANAGER_H
