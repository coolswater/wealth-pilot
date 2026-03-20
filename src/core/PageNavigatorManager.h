#ifndef PAGENAVIGATORMANAGER_H
#define PAGENAVIGATORMANAGER_H

#include "BasePage.h"
#include <QStackedWidget>
#include <QMap>
#include <QTimer>
#include <QDateTime>
#include <memory>

/**
 * @brief 页面缓存策略枚举
 * @details 三种策略分别对应不同的内存管理和性能权衡：
 * - NoCache: 适用于低频访问且内存占用大的页面（如大型数据报表）
 * - WeakCache: 默认策略，平衡内存占用和响应速度，允许系统在内存紧张时自动回收
 * - StrongCache: 适用于高频核心页面（如首页Dashboard），常驻内存确保零延迟切换
 */
enum class CachePolicy {
    NoCache,        ///< 不缓存，每次切换重新创建（内存敏感型页面）
    WeakCache,      ///< 弱引用缓存（可被垃圾回收，但复用率高）
    StrongCache     ///< 强引用缓存（常驻内存，适用于首页等高频页面）
};

/**
 * @brief 导航历史记录条目
 * @details 保存页面跳转历史，支持前进后退导航和状态恢复
 * @note timestamp 用于 LRU（最近最少使用）分析和调试追踪
 */
struct HistoryEntry {
    QString pageId;          ///< 页面唯一标识符
    QVariantMap params;      ///< 页面间传递的参数（如用户ID、筛选条件）
    qint64 timestamp;        ///< 跳转时间戳（毫秒级，用于性能分析和日志）
};

/**
 * @class PageNavigatorManager
 * @brief 页面导航控制器（单例模式）
 * @details 核心职责：
 * 1. 页面生命周期管理：创建 → 初始化 → 激活 → 失活 → 销毁/缓存
 * 2. 智能缓存系统：双层缓存（强引用+弱引用）实现内存优化与快速响应的平衡
 * 3. 导航历史栈：支持复杂的页面回退场景（如表单填写中断后返回）
 * 4. 完全解耦：页面通过信号请求导航，无需知晓导航器存在（MVVM架构）
 *
 * @warning 必须在主线程使用（QWidget限制），非线程安全设计
 */
class PageNavigatorManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例（线程安全，C++11静态初始化）
     * @param parent 父对象，建议传入主窗口指针以便随应用析构
     * @return 导航器唯一实例
     */
    static PageNavigatorManager* instance(QObject *parent = nullptr);

    /**
     * @brief 初始化导航器，绑定到UI容器
     * @param container QStackedWidget 容器，负责实际页面堆叠显示
     * @warning 必须在任何导航操作前调用，且只能初始化一次
     * @note 初始化后会设置容器属性 WA_DeleteOnClose = false，防止容器被误删
     */
    void initialize(QStackedWidget *container);

    /**
     * @brief 注册页面缓存策略
     * @param pageId 页面ID（必须与工厂注册ID一致）
     * @param policy 缓存策略枚举值
     * @details 策略与页面注册分离设计，允许运行时动态调整策略（如根据设备内存状况切换）
     * @note 对于 StrongCache 策略，会立即触发 preloadPage() 后台预加载
     */
    void registerCachePolicy(const QString &pageId, CachePolicy policy);

    /**
     * @brief 导航到指定页面（核心接口）
     * @param pageId 目标页面ID
     * @param params 页面间传递的参数（任意Qt可拷贝类型），目标页面通过 onPageActivated 接收
     * @param replaceCurrent 是否替换当前历史记录（而非压栈）
     *   - true: 替换当前记录（适用于登录成功后替换登录页，防止回退到登录页）
     *   - false: 压入新记录（默认，支持返回操作）
     * @details 执行流程：
     * 1. 历史栈管理（根据 replaceCurrent 决定压栈或替换）
     * 2. 获取或创建页面（getOrCreatePage：查缓存→工厂创建→初始化→加入容器）
     * 3. 当前页面失活（deactivateCurrentPage：触发 onPageDeactivated，断开信号）
     * 4. 新页面激活（activatePage：触发 onPageActivated，连接信号）
     * 5. 发射 pageChanged 信号通知外部（如更新标题栏）
     */
    void navigateTo(const QString &pageId, const QVariantMap &params = {},
                    bool replaceCurrent = false);

    /**
     * @brief 返回上一页（历史栈回退）
     * @return true 表示成功返回，false 表示历史栈为空（已在最底层）
     * @note 会恢复上一次跳转时保存的参数（params），支持状态恢复
     */
    bool navigateBack();

    /**
     * @brief 获取当前页面智能指针
     * @return 当前活动页面的 shared_ptr，若无则返回 nullptr
     * @warning 不要长期持有此指针，避免干扰缓存自动回收机制
     */
    std::shared_ptr<BasePage> currentPage() const { return m_currentPage; }

    /**
     * @brief 获取当前页面ID（便捷方法）
     * @return 当前页面ID字符串，若无活动页面返回空字符串
     */
    QString currentPageId() const { return m_currentPage ? m_currentPage->pageId() : QString(); }

    /**
     * @brief 后台预加载指定页面
     * @param pageId 目标页面ID
     * @details 在后台静默创建页面并加入缓存，但不显示。适用于：
     * - 应用启动时预加载首页（提升首屏感知速度）
     * - 用户浏览A页面时预加载可能跳转的B页面（预测性加载）
     * @note 不会触发 onPageActivated（因页面未真正显示），仅执行 initializePage
     */
    void preloadPage(const QString &pageId);

    /**
     * @brief 清空指定或全部页面缓存
     * @param pageId 页面ID，为空字符串时清空所有缓存
     * @details 强制立即释放内存，适用于：
     * - 内存警告响应（收到系统低内存通知时）
     * - 配置变更后需要重建页面（如语言切换、主题切换）
     * @warning 若清空当前活动页面的缓存，当前页面将保持显示直至下次导航，但下次访问会重新创建
     */
    void clearCache(const QString &pageId = QString());

    /**
     * @brief 获取历史栈深度
     * @return 当前可回退的步数（0 表示无历史记录）
     */
    int historyDepth() const { return m_historyStack.size(); }

signals:
    /**
     * @brief 页面切换完成信号
     * @param pageId 新页面ID
     * @param params 传递给新页面的参数（用于外部UI同步，如更新面包屑导航）
     */
    void pageChanged(const QString &pageId, const QVariantMap &params);

    /**
     * @brief 历史栈变更信号
     * @param depth 当前栈深度（可用于控制返回按钮可用状态）
     */
    void historyStackChanged(int depth);

    /**
     * @brief 缓存状态变更信号
     * @param pageId 受影响的页面ID
     * @param cached true 表示被加入缓存，false 表示被移出缓存
     * @details 用于调试监控和内存分析（如显示缓存统计面板）
     */
    void cacheStatusChanged(const QString &pageId, bool cached);

private slots:
    /**
     * @brief 处理页面内部发起的导航请求（私有槽）
     * @param targetPageId 目标页面ID（由页面通过 requestNavigation 信号发射）
     * @param params 页面传递的参数
     * @param replaceCurrent 是否替换当前历史记录
     * @details 【关键设计】此槽函数是 BasePage::requestNavigation 信号的接收者
     * 使用 Qt::UniqueConnection 确保同一页面多次激活不会建立重复连接
     * @note 必须是槽函数（非Lambda）才能使用 UniqueConnection，这是 Qt 元对象系统限制
     */
    void onPageRequestNavigation(const QString &targetPageId,
                                 const QVariantMap &params = {},
                                 bool replaceCurrent = false);

private:
    /**
     * @brief 私有构造函数（单例模式）
     * @details 初始化定时清理器，每30秒扫描并删除已失效的弱引用（防止内存碎片）
     */
    explicit PageNavigatorManager(QObject *parent = nullptr);
    ~PageNavigatorManager();  ///< 私有析构确保通过 instance() 管理生命周期

    // 禁用拷贝构造和赋值（C++11风格）
    PageNavigatorManager(const PageNavigatorManager&) = delete;
    PageNavigatorManager& operator=(const PageNavigatorManager&) = delete;

    /**
     * @brief 获取或创建页面实例（缓存核心逻辑）
     * @param pageId 页面ID
     * @return 页面智能指针（shared_ptr），失败返回 nullptr
     * @details 查找顺序：
     * 1. 强缓存（m_strongCache）：立即返回，零开销
     * 2. 弱缓存（m_weakCache）：尝试提升（lock()）为强引用，成功则复用
     * 3. 工厂创建：调用 PageFactoryRegistry 创建新实例 → 执行 initializePage() → 按策略缓存
     *
     * 性能优化：
     * - 使用 move 语义减少 shared_ptr 引用计数操作
     * - 工厂创建在锁外执行（若工厂使用锁，已在工厂内部处理）
     */
    std::shared_ptr<BasePage> getOrCreatePage(const QString &pageId);

    /**
     * @brief 激活指定页面（切换到前台）
     * @param page 页面智能指针（必须已完成初始化）
     * @param params 传递给页面的参数
     * @details 执行操作：
     * - 设置 QStackedWidget 当前索引（显示页面）
     * - 触发 onPageActivated(params)（页面可在此加载动态数据）
     * - 连接 requestNavigation 信号到 onPageRequestNavigation（Qt::UniqueConnection 防重连）
     */
    void activatePage(std::shared_ptr<BasePage> page, const QVariantMap &params);

    /**
     * @brief 失活当前页面（切换到后台或销毁）
     * @details 执行操作：
     * - 断开 requestNavigation 信号连接（防止后台页面干扰导航）
     * - 触发 onPageDeactivated()（页面可在此保存临时状态）
     * - 若策略为 NoCache，立即从容器移除并释放资源
     * @note 对于缓存策略页面，仅断开信号，保持实例存活以加速下次访问
     */
    void deactivateCurrentPage();

    /**
     * @brief 清理过期弱缓存
     * @details 定时器回调，扫描 m_weakCache 中已失效（expired）的 weak_ptr 并删除
     * 防止长期运行下无效指针堆积造成的内存碎片
     */
    void cleanupExpiredCache();

    // ==================== 成员变量 ====================

    QStackedWidget *m_container = nullptr;  ///< UI容器，负责页面堆叠显示（不拥有页面所有权）
    std::shared_ptr<BasePage> m_currentPage;  ///< 当前活动页面的强引用（确保不被意外回收）

    // 双层缓存系统说明：
    // m_strongCache: QMap<QString, shared_ptr> → 手动管理生命周期，常驻内存直到 clearCache 或应用退出
    // m_weakCache: QMap<QString, weak_ptr> → 允许 Qt 对象树自动回收，但保留复用可能
    QMap<QString, std::weak_ptr<BasePage>> m_weakCache;      ///< 弱引用缓存池（LRU淘汰由Qt对象树触发）
    QMap<QString, std::shared_ptr<BasePage>> m_strongCache;  ///< 强引用缓存池（手动控制生命周期）
    QMap<QString, CachePolicy> m_cachePolicies;              ///< 页面ID→缓存策略映射表

    QList<HistoryEntry> m_historyStack;  ///< 导航历史栈（支持返回操作）
    static constexpr int MaxHistorySize = 50;  ///< 历史栈最大深度（防止无限内存增长）

    QTimer *m_cleanupTimer = nullptr;  ///< 弱缓存清理定时器（30秒间隔）
};

#endif // PAGENAVIGATORMANAGER_H
