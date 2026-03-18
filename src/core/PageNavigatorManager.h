#ifndef PAGENAVIGATORMANAGER_H
#define PAGENAVIGATORMANAGER_H

#include "BasePage.h"
#include <QStackedWidget>
#include <QMap>
#include <QCache>
#include <QTimer>

/**
 * @brief 页面缓存策略枚举
 */
enum class CachePolicy {
    NoCache,        // 不缓存，每次切换重新创建（适用于内存敏感页面）
    WeakCache,      // 弱引用缓存（页面可被GC，但可复用）
    StrongCache     // 强引用缓存（常驻内存，适用于首页等高频页面）
};

/**
 * @brief 导航历史记录条目
 */
struct HistoryEntry {
    QString pageId;
    QVariantMap params;
};

/**
 * @brief 页面导航控制器（单例）
 * 管理页面切换、缓存、历史栈，提供MVVM级别的解耦
 */
class PageNavigatorManager : public QObject {
    Q_OBJECT

public:
    static PageNavigatorManager* instance(QObject *parent = nullptr);

    /**
     * @brief 初始化导航器，绑定到QStackedWidget
     * @param container 页面容器控件
     */
    void initialize(QStackedWidget *container);

    /**
     * @brief 注册页面并指定缓存策略
     */
    void registerPage(const QString &pageId, CachePolicy policy = CachePolicy::WeakCache);

    /**
     * @brief 导航到指定页面（主接口）
     * @param pageId 目标页面ID
     * @param params 传递参数（通过onActivate接收）
     * @param replaceCurrent 是否替换当前历史记录（默认false，即压栈）
     */
    void navigateTo(const QString &pageId, const QVariantMap &params = QVariantMap(),
                    bool replaceCurrent = false);

    /**
     * @brief 返回上一页
     * @return 是否成功返回（false表示历史栈为空）
     */
    bool navigateBack();

    /**
     * @brief 获取当前页面
     */
    std::shared_ptr<BasePage> currentPage() const { return m_currentPage; }

    /**
     * @brief 预加载页面到缓存（提升感知性能）
     */
    void preloadPage(const QString &pageId);

    /**
     * @brief 清空指定页面缓存（释放内存）
     */
    void clearCache(const QString &pageId = QString());

    /**
     * @brief 获取历史栈深度
     */
    int historyDepth() const { return m_historyStack.size(); }

signals:
    void pageChanged(const QString &pageId, const QVariantMap &params);
    void historyStackChanged(int depth);
    void cacheStatusChanged(const QString &pageId, bool cached);

private:
    explicit PageNavigatorManager(QObject *parent = nullptr);

    // 缓存管理
    std::shared_ptr<BasePage> getCachedPage(const QString &pageId);
    void putCachedPage(const QString &pageId, std::shared_ptr<BasePage> page, CachePolicy policy);

    // 页面切换核心逻辑
    void switchToPage(const QString &pageId, const QVariantMap &params);

    QStackedWidget *m_container = nullptr;
    std::shared_ptr<BasePage> m_currentPage;

    // 缓存系统：使用QCache自动LRU淘汰，结合weak_ptr防止内存泄漏
    QMap<QString, std::weak_ptr<BasePage>> m_weakCache;
    QMap<QString, std::shared_ptr<BasePage>> m_strongCache;
    QMap<QString, CachePolicy> m_cachePolicies;

    // 历史栈（支持复杂导航场景）
    QList<HistoryEntry> m_historyStack;
    int m_maxHistorySize = 50;  // 防止内存无限增长

    QTimer *m_cleanupTimer;     // 定期清理失效weak_ptr
};

#endif // PAGENAVIGATORMANAGER_H
