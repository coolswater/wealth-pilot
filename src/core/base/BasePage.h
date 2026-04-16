#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QWidget>
#include <QVariantMap>
#include <QString>

/**
 * @class BasePage
 * @brief 所有页面的基类（接口规范）
 * @details 设计原则：
 * 1. 延迟初始化：构造函数只做基础设置，UI构建延迟到 initializePage()
 * 2. 状态感知：通过 onPageActivated/onPageDeactivated 感知可见状态
 * 3. 安全导航：页面不直接创建其他页面，通过信号 requestNavigation 请求跳转
 * 4. Pimpl支持：子类应使用 d-pointer 模式隐藏实现细节（非强制但建议）
 *
 * @note 所有虚函数都有默认空实现（除纯虚函数外），子类按需重写即可
 */
class BasePage : public QWidget {
    Q_OBJECT
public:
    explicit BasePage(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual ~BasePage() = default;

    /**
     * @brief 页面唯一标识符（纯虚函数，必须实现）
     * @return 页面ID字符串，建议全局唯一，推荐格式："module/feature"，如"settings/account"
     * @warning 必须与 PageFactoryRegistry::registerPage() 使用的ID完全一致，否则导航失败
     */
    virtual QString pageId() const = 0;

    /**
     * @brief 页面友好名称，用于UI显示（如标签页标题、面包屑导航等）
     * @return 显示名称，默认实现返回 pageId()，子类可重写提供本地化名称
     */
    virtual QString pageName() const { return pageId(); }

    /**
     * @brief 页面初始化钩子（纯虚函数，必须实现）
     * @details 调用时机：页面首次被创建时，由 PageNavigatorManager::getOrCreatePage 调用
     * 应在此处：创建UI组件、建立布局、连接信号、初始化模型等
     * @note 可能被多次调用（有 isInitialized() 检查），但实现应保证不重复初始化
     */
    virtual void initializePage() = 0;

    /**
     * @brief 页面激活回调：每次显示时触发
     * @param params 从其他页面传递的参数（如用户ID、可选项、编辑模式标志等）
     * @details 调用时机：页面从后台切换到前台时（navigateTo 或返回操作触发）
     * 适合执行：
     * - 加载动态数据（如刷新列表、查询数据库等）
     * - 根据参数刷新UI（如显示指定用户信息）
     * - 恢复上次离开时的状态（如滚动位置、表单数据）
     *
     * @note 参数通过 QVariantMap 传递，支持任何Qt可序列化类型（QString, int, bool, 自定义结构等）
     * 使用前建议检查 contains：if (params.contains("userId")) { int id = params["userId"].toInt(); }
     */
    virtual void onPageActivated(const QVariantMap &params = {}) {}

    /**
     * @brief 页面失活回调：每次隐藏时触发
     * @details 调用时机：页面从前台切换到后台时（导航到其他页面或返回）
     * 适合执行：
     * - 保存临时状态（未提交的表单数据、滚动位置等）
     * - 停止后台任务（定时器、轮询查询、动画等）
     * - 释放不必要的资源（图片缓存、临时文件等）
     *
     * @note 此方法保证一定会调用，子类应实时保存关键数据
     */
    virtual void onPageDeactivated() {}

    /**
     * @brief 是否允许多实例
     * @return false（默认）：单例模式，全局唯一实例，适合大多数页面（如设置页）
     *          true：每次导航创建新实例，适合多标签页（如不同产品详情页并排打开）
     * @warning 多实例模式谨慎使用，可能导致内存占用过高，历史栈管理复杂，通常不建议使用
     */
    virtual bool allowMultiInstance() const { return false; }

    // ==================== 初始化状态（供框架调用） ====================

    /** @brief 检查是否已完成初始化 */
    bool isInitialized() const { return m_initialized; }

    /** @brief 设置初始化标志（由 PageNavigatorManager 的 initializePage 调用） */
    void setInitialized(bool init) { m_initialized = init; }

signals:
    /**
     * @brief 页面请求导航信号（实现与导航系统解耦通信）
     * @param targetPageId 目标页面的ID（必须已在工厂注册）
     * @param params 想要传递给下一页面的参数，支持任意键值对
     * @param replaceCurrent 是否替换当前历史记录（默认false，即压栈）
     * @details 页面内部通过 emit requestNavigation("target", params) 请求跳转
     * 由 PageNavigatorManager 接收并执行，页面本身不持有导航器引用，完全解耦
     *
     * 典型使用场景：
     * - 列表页打开详情：emit requestNavigation("productDetail", {{"id", productId}});
     * - 登录成功替换登录页：emit requestNavigation("dashboard", {}, true);
     */
    void requestNavigation(const QString &targetPageId,
                           const QVariantMap &params = {},
                           bool replaceCurrent = false);

    /**
     * @brief 页面状态变更通知，用于调试和状态可视化显示
     * @param status 状态名称，如 "loading", "error", "initialized"
     * @details 可选实现，外部监听者可由此信号显示加载状态（如进度条、转圈动画、日志记录等）
     */
    void pageStatusChanged(const QString &status);

private:
    bool m_initialized = false;  ///< 初始化标志，防止重复初始化
};

#endif // BASEPAGE_H