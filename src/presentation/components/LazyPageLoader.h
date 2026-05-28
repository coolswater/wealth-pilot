/**
 * @file LazyPageLoader.h
 * @brief 懒加载页面管理器 - 按需创建和缓存页面组件
 *
 * @details 功能：
 * - 懒加载：页面首次访问时才创建
 * - 缓存管理：页面创建后缓存，避免重复创建
 * - 预加载：支持后台预加载即将访问的页面
 * - 内存管理：长时间未使用的页面可自动卸载
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef LAZYPAGELOADER_H
#define LAZYPAGELOADER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QStackedWidget>
#include <memory>
#include <functional>

namespace WealthPilot {

/**
 * @brief 页面工厂函数类型
 */
using PageFactory = std::function<QWidget*()>;

/**
 * @brief 页面配置
 */
struct PageConfig {
    QString id;                    ///< 页面唯一标识
    QString name;                  ///< 页面显示名称
    PageFactory factory;           ///< 页面创建工厂函数
    int priority = 0;             ///< 预加载优先级（越高越先预加载）
    int unloadTimeoutMs = 300000;  ///< 空闲超时时间（5分钟），0表示永不卸载
    bool preload = false;          ///< 是否在启动时预加载
};

/**
 * @brief 页面状态
 */
struct PageState {
    QWidget* widget = nullptr;      ///< 页面控件指针
    bool created = false;           ///< 是否已创建
    bool preloaded = false;          ///< 是否已预加载
    qint64 lastAccessTime = 0;      ///< 最后访问时间戳（毫秒）
    int accessCount = 0;            ///< 访问次数
};

/**
 * @brief 懒加载页面管理器
 * 
 * 使用示例：
 * @code
 * // 注册页面
 * loader->registerPage({
 *     "dashboard",
 *     "行情看板",
 *     []() { return new DashboardPage(); },
 *     100,  // 最高优先级
 *     0,    // 永不卸载
 *     true  // 启动时预加载
 * });
 * 
 * // 切换页面（按需创建）
 * loader->switchToPage("dashboard");
 * 
 * // 预加载页面
 * loader->preloadPage("quotes");
 * @endcode
 */
class LazyPageLoader : public QObject {
    Q_OBJECT

public:
    explicit LazyPageLoader(QStackedWidget* container, QObject* parent = nullptr);
    ~LazyPageLoader() override;

    // ========== 页面注册 ==========
    
    /**
     * @brief 注册页面
     */
    void registerPage(const PageConfig& config);
    
    /**
     * @brief 批量注册页面
     */
    void registerPages(const QVector<PageConfig>& configs);
    
    /**
     * @brief 注销页面
     */
    void unregisterPage(const QString& pageId);

    // ========== 页面访问 ==========
    
    /**
     * @brief 切换到指定页面
     * @param pageId 页面ID
     * @return 切换成功返回true
     */
    bool switchToPage(const QString& pageId);
    
    /**
     * @brief 获取当前页面ID
     */
    QString currentPageId() const;
    
    /**
     * @brief 获取当前页面控件
     */
    QWidget* currentPage() const;
    
    /**
     * @brief 获取页面控件（如果存在）
     */
    QWidget* getPage(const QString& pageId) const;
    
    /**
     * @brief 检查页面是否已创建
     */
    bool isPageCreated(const QString& pageId) const;

    // ========== 预加载管理 ==========
    
    /**
     * @brief 预加载页面（后台创建）
     */
    void preloadPage(const QString& pageId);
    
    /**
     * @brief 批量预加载
     */
    void preloadPages(const QStringList& pageIds);
    
    /**
     * @brief 启动预加载（按优先级）
     */
    void startPreloading();
    
    /**
     * @brief 取消预加载
     */
    void cancelPreloading();

    // ========== 内存管理 ==========
    
    /**
     * @brief 卸载页面（释放内存）
     */
    void unloadPage(const QString& pageId);
    
    /**
     * @brief 卸载所有空闲页面
     */
    void unloadIdlePages();
    
    /**
     * @brief 设置空闲超时时间
     */
    void setIdleTimeout(int milliseconds);

    // ========== 统计信息 ==========
    
    /**
     * @brief 获取已创建页面数
     */
    int createdPageCount() const;
    
    /**
     * @brief 获取已注册页面数
     */
    int registeredPageCount() const;
    
    /**
     * @brief 获取页面统计信息
     */
    QMap<QString, QVariant> getStatistics() const;

signals:
    /**
     * @brief 页面切换信号
     */
    void pageChanged(const QString& pageId);
    
    /**
     * @brief 页面创建完成信号
     */
    void pageCreated(const QString& pageId);
    
    /**
     * @brief 页面预加载完成信号
     */
    void pagePreloaded(const QString& pageId);
    
    /**
     * @brief 页面卸载信号
     */
    void pageUnloaded(const QString& pageId);

private slots:
    /**
     * @brief 检查并卸载空闲页面
     */
    void checkIdlePages();
    
    /**
     * @brief 执行下一个预加载任务
     */
    void processPreloadQueue();

private:
    /**
     * @brief 创建页面
     */
    QWidget* createPage(const QString& pageId);

private:
    QStackedWidget* m_container;                   ///< 页面容器
    QHash<QString, PageConfig> m_configs;           ///< 页面配置
    QHash<QString, PageState> m_states;             ///< 页面状态
    QString m_currentPageId;                        ///< 当前页面ID
    
    QTimer* m_idleCheckTimer;                       ///< 空闲检查定时器
    QTimer* m_preloadTimer;                         ///< 预加载定时器
    QStringList m_preloadQueue;                     ///< 预加载队列
    int m_idleTimeoutMs = 300000;                  ///< 空闲超时时间（5分钟）
    
    bool m_preloadingInProgress = false;           ///< 预加载是否进行中
};

} // namespace WealthPilot

#endif // LAZYPAGELOADER_H
