#include "PageNavigatorManager.h"
#include "PageFactoryRegistry.h"
#include "utils/Logger.h"
#include <QDebug>
#include <QDateTime>

/**
 * @brief 构造函数：初始化缓存清理定时器
 * @details 定时器每30秒执行一次 cleanupExpiredCache()，清理已失效的弱引用
 * 这是内存管理的关键机制，防止长期运行后 m_weakCache 堆积大量无效条目
 */
PageNavigatorManager::PageNavigatorManager(QObject *parent)
    : QObject(parent)
{
    m_cleanupTimer = new QTimer(this);
    // 使用成员函数指针连接，确保类型安全（Lambda无法用于UniqueConnection）
    connect(m_cleanupTimer, &QTimer::timeout, this, &PageNavigatorManager::cleanupExpiredCache);
    m_cleanupTimer->start(30000);  // 30秒 = 30000毫秒
}

PageNavigatorManager::~PageNavigatorManager() = default;

/**
 * @brief 单例获取（Meyer's Implementation变种）
 * @details 使用静态指针而非静态局部对象，确保：
 * 1. 支持显式传入 parent 对象（QObject树集成，自动内存管理）
 * 2. C++11保证静态初始化线程安全（GCC/Clang/MSVC均支持）
 * 3. 延迟初始化（首次调用时构造）
 *
 * @warning 禁止在非主线程调用（QWidget限制）
 */
PageNavigatorManager* PageNavigatorManager::instance(QObject *parent) {
    static PageNavigatorManager *instance = new PageNavigatorManager(parent);
    return instance;
}

/**
 * @brief 初始化：绑定到UI容器
 * @details 必须首先调用此方法，设置 QStackedWidget 容器
 * 设置 WA_DeleteOnClose = false 防止容器被页面关闭事件误删（关键防御性编程）
 */
void PageNavigatorManager::initialize(QStackedWidget *container) {
    if (!container) {
        qCritical() << "PageNavigatorManager: Container cannot be null!";
        return;
    }
    m_container = container;
    // 防御性设置：确保容器不会因页面关闭而被删除（页面由智能指针管理生命周期）
    m_container->setAttribute(Qt::WA_DeleteOnClose, false);
}

/**
 * @brief 注册缓存策略
 * @details 策略与页面注册分离，允许：
 * - 不同设备使用不同策略（如手机用WeakCache，PC用StrongCache）
 * - 运行时动态调整（如内存不足时降级为NoCache）
 */
void PageNavigatorManager::registerCachePolicy(const QString &pageId, PageCachePolicy policy) {
    m_cachePolicies[pageId] = policy;
    // 强缓存策略立即后台预加载（热启动优化，用户感知零延迟）
    if (policy == PageCachePolicy::StrongCache) {
        preloadPage(pageId);
    }
}

/**
 * @brief 核心导航逻辑：页面切换主入口
 * @details 执行时序：
 * 1. 前置检查（初始化状态、工厂注册）
 * 2. 历史栈管理（保存当前位置以支持回退）
 * 3. 获取目标页面（查缓存/创建/初始化）
 * 4. 切换上下文（失活旧页面 → 激活新页面）
 * 5. 发射全局通知信号
 */
void PageNavigatorManager::navigateTo(const QString &pageId, const QVariantMap &params, bool replaceCurrent) {
    // ========== 前置条件检查 ==========
    if (!m_container) {
        qWarning() << "PageNavigatorManager: Not initialized! Call initialize() first.";
        return;
    }

    if (!PageFactoryRegistry::instance()->hasPage(pageId)) {
        qWarning() << "PageNavigatorManager: Page" << pageId << "not registered in factory!";
        return;
    }

    // ========== 历史栈管理 ==========
    // 若不需要替换当前记录，将当前页面压栈（支持后续返回）
    if (!replaceCurrent && m_currentPage) {
        HistoryEntry entry;
        entry.pageId = m_currentPage->pageId();
        entry.params = params;  // 保存参数以支持状态恢复
        entry.timestamp = QDateTime::currentMSecsSinceEpoch();
        m_historyStack.append(entry);

        // LRU淘汰：限制历史栈大小防止内存无限增长（如极端情况下的循环导航）
        if (m_historyStack.size() > MaxHistorySize) {
            m_historyStack.removeFirst();  // 移除最旧记录
        }
        emit historyStackChanged(m_historyStack.size());
    }

    // ========== 页面获取/创建 ==========
    auto page = getOrCreatePage(pageId);
    if (!page) {
        qCritical() << "PageNavigatorManager: Failed to obtain page" << pageId;
        return;
    }

    // ========== 执行切换 ==========
    deactivateCurrentPage();  // 旧页面失活（保存状态、断开信号）
    activatePage(page, params); // 新页面激活（加载数据、连接信号）
    emit pageChanged(pageId, params);  // 通知外部UI更新（如标题栏、面包屑）
}

/**
 * @brief 历史回退功能
 * @details 从栈顶弹出历史记录，恢复上一个页面及其参数
 * 适用于：返回按钮、取消操作、流程回退场景
 */
bool PageNavigatorManager::navigateBack() {
    if (m_historyStack.isEmpty()) return false;  // 栈空检查

    HistoryEntry entry = m_historyStack.takeLast();  // 弹出最近记录
    emit historyStackChanged(m_historyStack.size());  // 通知栈深度变化

    // 获取历史页面（会复用缓存或重新创建）
    auto page = getOrCreatePage(entry.pageId);
    if (page) {
        deactivateCurrentPage();
        activatePage(page, entry.params);  // 使用历史参数恢复状态
        emit pageChanged(entry.pageId, entry.params);
        return true;
    }
    return false;
}

/**
 * @brief 页面获取/创建核心逻辑（缓存系统实现）
 * @details 三层查找策略（性能从快到慢）：
 * 1. 强缓存命中：O(1) 直接返回，无创建开销
 * 2. 弱缓存命中：O(1) lock() 提升为强引用，轻微开销
 * 3. 工厂创建：涉及内存分配、UI构建（最慢，但会缓存结果供下次使用）
 *
 * @note 首次创建的页面会自动调用 initializePage() 完成UI构建
 * @warning 此函数是性能关键点，应避免在主线程执行耗时操作（已在锁外执行工厂创建）
 */
std::shared_ptr<BasePage> PageNavigatorManager::getOrCreatePage(const QString &pageId) {
    // ----- 第1层：强缓存检查 -----
    auto strongIt = m_strongCache.find(pageId);
    if (strongIt != m_strongCache.end()) {
        return strongIt.value();  // 直接返回强引用，引用计数+1
    }

    // ----- 第2层：弱缓存检查 -----
    auto weakIt = m_weakCache.find(pageId);
    if (weakIt != m_weakCache.end()) {
        // 尝试提升弱引用为强引用（检查对象是否仍存活）
        if (auto shared = weakIt.value().lock()) {
            return shared;  // 复用成功，避免重新创建
        }
        // 对象已被Qt对象树回收，清理无效条目
        m_weakCache.erase(weakIt);
    }

    // ----- 第3层：工厂创建 -----
    // 调用工厂创建新实例（工厂内部可能加锁，但此处无锁，避免死锁）
    auto page = PageFactoryRegistry::instance()->createPage(pageId, m_container);
    if (!page) return nullptr;  // 工厂创建失败（如构造异常）

    // 【关键步骤】执行页面初始化（构建UI、连接内部信号）
    // 延迟初始化设计：首次使用时才构建UI，减少启动时间和内存占用
    if (!page->isInitialized()) {
        page->initializePage();  // 虚函数多态调用具体页面实现
        page->setInitialized(true);  // 标记已完成初始化
    }

    // ----- 按策略缓存 -----
    PageCachePolicy policy = m_cachePolicies.value(pageId, PageCachePolicy::WeakCache);

    if (policy == PageCachePolicy::StrongCache) {
        // 强缓存：保持 shared_ptr，阻止任何回收，直到显式 clearCache
        m_strongCache[pageId] = page;
        emit cacheStatusChanged(pageId, true);
    } else if (policy == PageCachePolicy::WeakCache) {
        // 弱缓存：不阻止Qt对象树回收，但保留复用可能
        m_weakCache[pageId] = page;
    }
    // NoCache策略：不加入任何缓存，下次访问重新创建

    // ----- 加入UI容器 -----
    // 确保页面Widget已添加到 QStackedWidget（但不一定是当前显示）
    QWidget *widget = dynamic_cast<QWidget*>(page.get());
    if (widget && m_container->indexOf(widget) < 0) {
        m_container->addWidget(widget);  // 加入堆栈，索引自动管理
    }

    return page;
}

/**
 * @brief 页面激活（切入前台）
 * @details 职责：
 * 1. 更新当前页面指针（保持强引用防止被回收）
 * 2. 切换 QStackedWidget 当前索引（显示页面）
 * 3. 触发页面生命周期回调 onPageActivated（加载动态数据）
 * 4. 连接导航请求信号（使用 UniqueConnection 防止重复连接）
 *
 * 【重要修复】使用成员函数指针 &PageNavigatorManager::onPageRequestNavigation
 * 而非Lambda，以满足 Qt::UniqueConnection 的类型要求（槽必须是成员函数）
 */
void PageNavigatorManager::activatePage(std::shared_ptr<BasePage> page, const QVariantMap &params) {
    if (!page) return;

    m_currentPage = page;  // 保持强引用（即使弱缓存策略，当前页也强制驻留内存）

    // 切换到目标页面Widget
    QWidget *widget = dynamic_cast<QWidget*>(page.get());
    if (widget) {
        m_container->setCurrentWidget(widget);  // 自动处理页面显示
        widget->setVisible(true);               // 确保可见性（防御性编程）
    }

    // 触发页面激活回调（页面应在此加载数据，而非构造函数中）
    page->onPageActivated(params);

    // 【关键修复】使用成员函数指针实现 UniqueConnection，确保同一页面多次激活不会重复连接
    connect(
        page.get(),                                    // 信号发射者（页面）
        &BasePage::requestNavigation,                   // 信号（页面请求导航）
        this,                                          // 接收者（导航器）
        &PageNavigatorManager::onPageRequestNavigation, // 槽（必须是成员函数）
        Qt::UniqueConnection                            // 连接类型：若已连接则跳过
        );
}

/**
 * @brief 页面失活（切出前台或销毁）
 * @details 职责：
 * 1. 断开导航信号（防止后台页面发起导航请求，造成竞态条件）
 * 2. 触发失活回调 onPageDeactivated（保存临时状态、停止定时器等）
 * 3. 根据缓存策略决定是否立即销毁：
 *    - NoCache: 立即从容器移除并释放（节省内存）
 *    - Weak/Strong: 保留实例，仅断开信号，下次激活更快
 */
void PageNavigatorManager::deactivateCurrentPage() {
    if (!m_currentPage) return;

    // 【对应activatePage的connect】使用相同成员函数指针断开连接
    // 确保完全断开，防止残留连接导致重复响应
    disconnect(m_currentPage.get(), &BasePage::requestNavigation,
               this, &PageNavigatorManager::onPageRequestNavigation);

    // 触发页面失活生命周期（页面应在此保存未提交数据或停止后台任务）
    m_currentPage->onPageDeactivated();

    // NoCache策略：立即释放资源（适用于内存敏感型页面）
    if (m_cachePolicies.value(m_currentPage->pageId()) == PageCachePolicy::NoCache) {
        QWidget *widget = dynamic_cast<QWidget*>(m_currentPage.get());
        if (widget) {
            m_container->removeWidget(widget);  // 从UI容器移除（但不delete，智能指针管理）
        }
        m_currentPage.reset();  // 释放强引用（若无其他引用则立即析构）
    }
    // 其他策略：保持 m_currentPage 强引用直到下次导航，确保当前页内存安全
}

/**
 * @brief 页面内部导航请求的槽实现
 * @details 页面通过 emit requestNavigation(...) 发起跳转，此槽接收并执行
 * 简单的委托模式：转发到 navigateTo 主逻辑
 * @note 必须是 public slots 或 private slots 才能被 connect 使用 UniqueConnection
 */
void PageNavigatorManager::onPageRequestNavigation(const QString &targetPageId,
                                                   const QVariantMap &params,
                                                   bool replaceCurrent) {
    navigateTo(targetPageId, params, replaceCurrent);
}

/**
 * @brief 后台预加载
 * @details 静默创建页面并加入缓存，但不显示（不触发 onPageActivated）
 * 适用场景：
 * - 应用启动时预加载首页（消除首屏白屏时间）
 * - 预测性加载（如用户浏览商品列表时预加载详情页）
 */
void PageNavigatorManager::preloadPage(const QString &pageId) {
    // 检查是否已缓存（避免重复创建）
    if (m_weakCache.contains(pageId) || m_strongCache.contains(pageId)) return;

    auto page = getOrCreatePage(pageId);
    if (page) {
        LOG_INFO(QString("PageNavigatorManager Preloaded page: %1 (cached in %2 cache)")
                     .arg(pageId)
                     .arg(m_cachePolicies.value(pageId) == PageCachePolicy::StrongCache ? "strong" : "weak"));
    }
}

/**
 * @brief 清理缓存
 * @param pageId 指定页面ID（空字符串表示清空全部）
 * @details 强制立即释放内存，下次访问将重新创建页面
 */
void PageNavigatorManager::clearCache(const QString &pageId) {
    if (pageId.isEmpty()) {
        // 清空全部缓存（适用于内存警告响应）
        m_weakCache.clear();
        m_strongCache.clear();
    } else {
        // 清空指定页面
        m_weakCache.remove(pageId);
        m_strongCache.remove(pageId);
        emit cacheStatusChanged(pageId, false);
    }
}

/**
 * @brief 定时清理任务（30秒间隔）
 * @details 扫描弱缓存池，删除已失效（对应QWidget已被销毁）的weak_ptr
 * 防止长期运行下无效指针堆积（虽然不影响功能，但浪费内存和遍历时间）
 */
void PageNavigatorManager::cleanupExpiredCache() {
    int cleanedCount = 0;
    for (auto it = m_weakCache.begin(); it != m_weakCache.end();) {
        if (it.value().expired()) {
            // 弱引用已失效（页面被Qt对象树回收），删除条目
            emit cacheStatusChanged(it.key(), false);
            it = m_weakCache.erase(it);
            ++cleanedCount;
        } else {
            ++it;
        }
    }

    if (cleanedCount > 0) {
        LOG_INFO(QString("PageNavigatorManager Cleaned page: %1 expired weak cache entries")
                     .arg(cleanedCount));
    }
}
