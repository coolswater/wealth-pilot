#ifndef PAGENAVIGATORMANAGER_H
#define PAGENAVIGATORMANAGER_H

#include "ui/components/BasePage.h"
#include <QStackedWidget>
#include <QMap>
#include <QTimer>
#include <QDateTime>
#include <memory>

/**
 * @brief 页面缓存策略枚举
 * @details 三种策略分别适应不同场景：内存占用和响应速度权衡
 * - NoCache: 适用于频繁创建、内存占用大且不常用页面（如大数据报表）
 * - WeakCache: 默认策略，平衡内存占用和响应速度，当系统低内存时自动清理
 * - StrongCache: 适用于高频访问页面（如主页Dashboard），常驻内存确保零延迟
 */
enum class PageCachePolicy {
    NoCache,        ///< 无缓存，每次切换都重新创建，内存占用最小但响应最慢
    WeakCache,      ///< 弱引用缓存（可被Qt自动清理），平衡内存和响应
    StrongCache     ///< 强引用缓存（常驻内存），适合主页等高频页面
};

/**
 * @brief 导航历史记录项
 * @details 记录页面跳转历史，支持前进/后退和状态恢复
 * @note timestamp 字段用于 LRU 缓存淘汰策略，越早越容易被清理
 */
struct HistoryEntry {
    QString pageId;          ///< 页面唯一标识符
    QVariantMap params;      ///< 页面间传递的参数（如用户ID、可选项等）
    qint64 timestamp;        ///< 跳转时间（毫秒级），可用于分析或日志
};

/**
 * @class PageNavigatorManager
 * @brief 页面导航管理器（单例模式）
 * @details 核心职责：
 * 1. 页面生命周期管理（创建、初始化、激活、失活、销毁）
 * 2. 智能缓存系统：双级缓存（强引用+弱引用），实现内存优化和快速响应平衡
 * 3. 导航历史栈：支持复杂页面流程（如向导步骤），支持返回和条件判断后返回
 * 4. 安全导航：页面通过信号请求导航，不感知导航器存在，实现MVVM架构
 *
 * @warning 仅限主线程使用（QWidget限制），非线程安全
 */
class PageNavigatorManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例（线程安全，C++11静态初始化）
     * @param parent 父对象，建议传入 nullptr 以避免应用退出时自动删除
     * @return 导航器唯一实例
     */
    static PageNavigatorManager* instance(QObject *parent = nullptr);

    /**
     * @brief 初始化导航器，绑定到UI容器
     * @param container QStackedWidget 容器，用于实际页面堆叠显示
     * @warning 必须在任何导航操作前调用，且只能初始化一次
     * @note 初始化后容器会设置 WA_DeleteOnClose = false，防止意外自动删除
     */
    void initialize(QStackedWidget *container);

    /**
     * @brief 注册页面缓存策略
     * @param pageId 页面ID（必须与工厂注册ID一致）
     * @param policy 缓存策略枚举
     * @details 建议在页面注册后立即调用，可根据设备内存状态动态调整策略
     * @note 对于 StrongCache 策略，建议配合 preloadPage() 后台预加载
     */
    void registerCachePolicy(const QString &pageId, PageCachePolicy policy);

    /**
     * @brief 导航到指定页面（主入口接口）
     * @param pageId 目标页面ID
     * @param params 页面间传递的参数（支持任何Qt可序列化类型，目标页面通过 onPageActivated 接收）
     * @param replaceCurrent 是否替换当前历史记录（默认压栈）
     *   - true: 替换当前记录（如登录成功后替换登录页，防止用户返回到登录页）
     *   - false: 压入新记录（默认），支持返回操作
     * @details 执行流程：
     * 1. 历史栈处理（根据 replaceCurrent 决定压栈或替换）
     * 2. 获取或创建页面（getOrCreatePage，查缓存或调用工厂创建并初始化）
     * 3. 当前页面失活（deactivateCurrentPage，调用 onPageDeactivated，断开信号）
     * 4. 新页面激活（activatePage，调用 onPageActivated，连接信号）
     * 5. 发射 pageChanged 信号通知外部（如更新面包屑导航）
     */
    void navigateTo(const QString &pageId, const QVariantMap &params = {},
                    bool replaceCurrent = false);

    /**
     * @brief 返回上一页（历史栈弹出）
     * @return true 表示成功返回，false 表示历史栈为空（已在栈底）
     * @note 会恢复上一次导航时的参数（params），支持状态恢复
     */
    bool navigateBack();

    /**
     * @brief 获取当前页面实例指针
     * @return 当前活跃页面的 shared_ptr，无则返回 nullptr
     * @warning 不要在长期存储此指针，导航器可能自动销毁页面
     */
    std::shared_ptr<WealthPilot::BasePage> currentPage() const { return m_currentPage; }

    /**
     * @brief 获取当前页面ID（便捷方法）
     * @return 当前页面ID字符串，无活跃页面返回空字符串
     */
    QString currentPageId() const { return m_currentPage ? m_currentPage->pageId() : QString(); }

    /**
     * @brief 后台预加载指定页面
     * @param pageId 目标页面ID
     * @details 在后台创建并初始化页面并加入缓存，但不显示：
     * - 应用启动时预加载常用页面（提升用户感知速度）
     * - 用户在A页面时预加载可能跳转的B页面（预加载建议场景）
     * @note 不会触发 onPageActivated（页面未真正显示），但会执行 initializePage
     */
    void preloadPage(const QString &pageId);

    /**
     * @brief 清除指定或全部页面缓存
     * @param pageId 页面ID，为空字符串时清除全部
     * @details 强制释放内存，适用场景：
     * - 内存告急时响应系统的低内存通知
     * - 配置变更需要重建页面（如主题切换导致某些页面需重建）
     * @warning 若清除当前活跃页面的缓存，当前页面将继续显示直到下次导航时重建
     */
    void clearCache(const QString &pageId = QString());

    /**
     * @brief 获取历史栈深度
     * @return 当前可返回的步数，0 表示无历史记录
     */
    int historyDepth() const { return m_historyStack.size(); }

signals:
    /**
     * @brief 页面切换完成信号
     * @param pageId 新页面ID
     * @param params 传递给新页面的参数（外部UI同步、如更新标题栏）
     */
    void pageChanged(const QString &pageId, const QVariantMap &params);

    /**
     * @brief 历史栈变化信号
     * @param depth 当前栈深度，用于控制返回按钮的禁用状态
     */
    void historyStackChanged(int depth);

    /**
     * @brief 缓存状态变化信号
     * @param pageId 受影响的页面ID
     * @param cached true 表示加入缓存，false 表示被移除
     * @details 可用于调试监控和内存分析（如显示缓存统计）
     */
    void cacheStatusChanged(const QString &pageId, bool cached);

private slots:
    /**
     * @brief 处理页面内部发出的导航请求（解耦关键设计）
     * @param targetPageId 目标页面ID（页面通过 requestNavigation 信号发送）
     * @param params 页面传递的参数
     * @param replaceCurrent 是否替换当前历史记录
     * @details 这是核心解耦机制，此槽函数响应 BasePage::requestNavigation 信号的请求
     * 使用 Qt::UniqueConnection 确保同一页面多次激活不会建立重复连接
     * @note 由于这是槽函数，不能使用Lambda，必须使用 UniqueConnection 连接（Qt 元对象系统要求）
     */
    void onPageRequestNavigation(const QString &targetPageId,
                                 const QVariantMap &params = {},
                                 bool replaceCurrent = false);

private:
    /**
     * @brief 私有构造函数（单例模式）
     * @details 初始化定时器，每隔30秒扫描并删除已失效的弱引用缓存，防止内存碎片
     */
    explicit PageNavigatorManager(QObject *parent = nullptr);
    ~PageNavigatorManager();  ///< 私有析构，确保只能通过 instance() 销毁

    // 禁止复制和赋值（C++11特性）
    PageNavigatorManager(const PageNavigatorManager&) = delete;
    PageNavigatorManager& operator=(const PageNavigatorManager&) = delete;

    /**
     * @brief 获取或创建页面实例（核心逻辑）
     * @param pageId 页面ID
     * @return 页面实例指针（shared_ptr），失败返回 nullptr
     * @details 查找顺序：
     * 1. 强缓存（m_strongCache）：直接返回，最快
     * 2. 弱缓存（m_weakCache）：尝试 lock()，若成功则提升为强引用（命中）
     * 3. 工厂创建：调用 PageFactoryRegistry 创建实例，执行 initializePage()，加入缓存
     *
     * 性能优化：
     * - 使用 move 语义减少 shared_ptr 引用计数开销
     * - 缓存命中时直接返回，避免工厂创建开销
     */
    std::shared_ptr<WealthPilot::BasePage> getOrCreatePage(const QString &pageId);

    /**
     * @brief 激活指定页面（切换到前台）
     * @param page 页面实例指针（必须已初始化）
     * @param params 传递给页面的参数
     * @details 执行操作：
     * - 设置 QStackedWidget 当前部件（显示页面）
     * - 调用 onPageActivated(params)，页面可在此加载动态数据
     * - 连接 requestNavigation 信号到 onPageRequestNavigation（Qt::UniqueConnection 防止重复）
     */
    void activatePage(std::shared_ptr<WealthPilot::BasePage> page, const QVariantMap &params);

    /**
     * @brief 失活当前页面（切换到后台，不销毁）
     * @details 执行操作：
     * - 断开 requestNavigation 信号连接，防止后台页面意外触发导航
     * - 调用 onPageDeactivated()，页面可在此保存临时状态
     * - 若缓存策略为 NoCache，则从缓存移除并释放
     * @note 对于缓存页面，仅断开信号，实例保留以便下次快速激活
     */
    void deactivateCurrentPage();

    /**
     * @brief 清理过期缓存
     * @details 定时器回调，扫描 m_weakCache 中已失效（expired）的 weak_ptr 并删除
     * 防止大量失效指针堆积导致的内存泄漏
     */
    void cleanupExpiredCache();

    // ==================== 成员变量 ====================

    QStackedWidget *m_container = nullptr;  ///< UI容器，用于页面堆叠显示，不拥有页面所有权
    std::shared_ptr<WealthPilot::BasePage> m_currentPage;  ///< 当前活跃页面（强引用，确保不会被自动销毁）

    // 双级缓存系统说明：
    // m_strongCache: QMap<QString, shared_ptr> 强引用，手动控制生命周期，常驻内存直到 clearCache 或应用退出
    // m_weakCache: QMap<QString, weak_ptr> 弱引用，允许 Qt 自动清理，适合低频访问页面
    QMap<QString, std::weak_ptr<WealthPilot::BasePage>> m_weakCache;      ///< 弱引用缓存（LRU淘汰，Qt自动管理生命周期）
    QMap<QString, std::shared_ptr<WealthPilot::BasePage>> m_strongCache;  ///< 强引用缓存（手动控制生命周期）
    QMap<QString, PageCachePolicy> m_cachePolicies;          ///< 页面ID到缓存策略映射

    QList<HistoryEntry> m_historyStack;  ///< 导航历史栈，支持返回操作
    static constexpr int MaxHistorySize = 50;  ///< 历史栈最大深度，防止无限增长

    QTimer *m_cleanupTimer = nullptr;  ///< 缓存清理定时器，30秒触发一次
};



#endif // PAGENAVIGATORMANAGER_H