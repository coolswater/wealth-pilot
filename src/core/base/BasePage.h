#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QWidget>
#include <QVariantMap>
#include <QString>

/**
 * @class BasePage
 * @brief 所有页面的抽象基类（接口规范）
 * @details 设计原则：
 * 1. 延迟初始化：构造函数只做轻量设置，UI构建延迟到 initializePage()
 * 2. 生命周期感知：通过 onPageActivated/onPageDeactivated 管理可见性状态
 * 3. 完全解耦：页面不直接依赖导航器，通过信号 requestNavigation 发起跳转
 * 4. Pimpl支持：派生类应使用 d-pointer 模式隐藏实现细节（编译防火墙）
 *
 * @note 所有虚函数都有默认空实现（除纯虚函数外），减少派生类样板代码
 */
class BasePage : public QWidget {
    Q_OBJECT
public:
    explicit BasePage(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual ~BasePage() = default;

    /**
     * @brief 页面唯一标识符（纯虚函数，必须实现）
     * @return 页面ID字符串，必须全局唯一（建议格式："module/feature"，如"settings/account"）
     * @warning 必须与 PageFactoryRegistry::registerPage() 中使用的ID完全一致，否则导航失败
     */
    virtual QString pageId() const = 0;

    /**
     * @brief 页面友好名称（用于UI显示，如标签页标题、面包屑导航）
     * @return 显示名称，默认实现返回 pageId()（建议派生类覆写提供本地化名称）
     */
    virtual QString pageName() const { return pageId(); }

    /**
     * @brief 页面初始化钩子（纯虚函数，必须实现）
     * @details 调用时机：页面首次被创建时（由 PageNavigatorManager::getOrCreatePage 触发）
     * 应完成所有UI构建和静态配置（如布局、样式、信号连接），但不应加载远程数据
     * @note 会被多次调用防护（isInitialized() 检查），但派生类实现仍应避免重复初始化
     */
    virtual void initializePage() = 0;

    /**
     * @brief 页面激活回调（每次显示时触发）
     * @param params 上游页面传递的参数（如用户ID、筛选条件、编辑模式标志等）
     * @details 调用时机：页面从后台切换到前台时（navigateTo 或返回操作）
     * 适合执行：
     * - 加载动态数据（网络请求、数据库查询）
     * - 根据参数刷新UI（如显示指定用户资料）
     * - 恢复上次离开时的状态（滚动位置、输入内容）
     *
     * @note 参数通过 QVariantMap 传递，支持任意Qt可拷贝类型（QString, int, bool, 自定义结构体等）
     * 使用前先检查 contains：if (params.contains("userId")) { int id = params["userId"].toInt(); }
     */
    virtual void onPageActivated(const QVariantMap &params = {}) {}

    /**
     * @brief 页面失活回调（每次隐藏时触发）
     * @details 调用时机：页面从前台切换到后台时（导航到其他页或返回）
     * 适合执行：
     * - 保存临时状态（未提交的表单数据、滚动位置）
     * - 停止后台任务（定时器、网络轮询、动画）
     * - 释放重量级资源（大图缓存、临时文件）
     *
     * @note 不保证一定被调用（如应用强制退出），关键数据应实时保存
     */
    virtual void onPageDeactivated() {}

    /**
     * @brief 是否允许多实例
     * @return false（默认）：单例模式，全局唯一实例，适合设置页、首页
     *          true：每次导航创建新实例，适合详情页（如不同商品详情并行打开）
     * @warning 多实例模式需谨慎使用，会增加内存占用，且历史栈管理更复杂（需通过参数区分实例）
     */
    virtual bool allowMultiInstance() const { return false; }

    // ==================== 初始化状态管理（供框架调用） ====================

    /** @brief 检查是否已完成初始化 */
    bool isInitialized() const { return m_initialized; }

    /** @brief 设置初始化标记（由 PageNavigatorManager 在 initializePage 后调用） */
    void setInitialized(bool init) { m_initialized = init; }

signals:
    /**
     * @brief 页面请求导航信号（实现与导航器的零耦合通信）
     * @param targetPageId 目标页面ID（必须已在工厂注册）
     * @param params 向下一个页面传递的参数（支持任意键值对）
     * @param replaceCurrent 是否替换当前历史记录（默认false）
     * @details 页面内部通过 emit requestNavigation("target", params) 发起跳转
     * 由 PageNavigatorManager 接收并执行，页面无需包含导航器头文件（解耦关键）
     *
     * 典型使用场景：
     * - 列表页点击详情：emit requestNavigation("productDetail", {{"id", productId}});
     * - 登录成功替换登录页：emit requestNavigation("dashboard", {}, true);
     */
    void requestNavigation(const QString &targetPageId,
                           const QVariantMap &params = {},
                           bool replaceCurrent = false);

    /**
     * @brief 页面状态变更通知（用于调试和状态栏显示）
     * @param status 状态描述（如"loading", "error", "initialized"）
     * @details 可选实现，外部可连接此信号显示加载状态（如标题栏转圈、日志记录）
     */
    void pageStatusChanged(const QString &status);

private:
    bool m_initialized = false;  ///< 初始化标记（防止重复初始化）
};

#endif // BASEPAGE_H
