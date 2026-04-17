/**
 * @file ComponentFactory.h
 * @brief 组件工厂 - 统一创建和管理UI组件
 *
 * @details 功能：
 * - 统一的组件创建接口
 * - 组件缓存和复用
 * - 主题样式自动应用
 * - 性能优化：延迟创建
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef COMPONENTFACTORY_H
#define COMPONENTFACTORY_H

#include <QObject>
#include <QWidget>
#include <QHash>
#include <QString>
#include <functional>
#include <memory>

#include "ThemeColors.h"

/**
 * @brief 组件工厂类
 *
 * @details 提供统一的组件创建和管理功能：
 * - 注册组件创建函数
 * - 按需创建组件
 * - 自动应用主题样式
 * - 组件缓存复用
 *
 * @example
 * @code
 * // 注册组件
 * ComponentFactory::instance().registerComponent("KLineChart", []() {
 *     return new KLineChart();
 * });
 *
 * // 创建组件
 * KLineChart* chart = ComponentFactory::instance().create<KLineChart>("KLineChart");
 * @endcode
 */
class ComponentFactory : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 组件创建函数类型
     */
    using CreatorFunc = std::function<QWidget*()>;

    /**
     * @brief 获取单例实例
     */
    static ComponentFactory& instance();

    // ========== 组件注册 ==========

    /**
     * @brief 注册组件创建函数
     * @param name 组件名称
     * @param creator 创建函数
     */
    void registerComponent(const QString& name, CreatorFunc creator);

    /**
     * @brief 注销组件
     * @param name 组件名称
     */
    void unregisterComponent(const QString& name);

    /**
     * @brief 检查组件是否已注册
     * @param name 组件名称
     */
    bool hasComponent(const QString& name) const;

    // ========== 组件创建 ==========

    /**
     * @brief 创建组件
     * @param name 组件名称
     * @param parent 父控件
     * @return 组件实例（如果未注册返回nullptr）
     */
    QWidget* create(const QString& name, QWidget* parent = nullptr);

    /**
     * @brief 创建组件（模板版本）
     * @tparam T 组件类型
     * @param name 组件名称
     * @param parent 父控件
     * @return 类型化的组件实例
     */
    template<typename T>
    T* create(const QString& name, QWidget* parent = nullptr)
    {
        QWidget* widget = create(name, parent);
        return qobject_cast<T*>(widget);
    }

    // ========== 组件缓存 ==========

    /**
     * @brief 获取缓存的组件
     * @param name 组件名称
     * @return 缓存的组件（如果不存在则创建）
     */
    QWidget* getOrCreate(const QString& name, QWidget* parent = nullptr);

    /**
     * @brief 清空组件缓存
     */
    void clearCache();

    /**
     * @brief 获取缓存大小
     */
    int cacheSize() const;

    // ========== 样式应用 ==========

    /**
     * @brief 设置是否自动应用样式
     * @param autoApply 是否自动应用
     */
    void setAutoApplyStyle(bool autoApply) { m_autoApplyStyle = autoApply; }

    /**
     * @brief 是否自动应用样式
     */
    bool autoApplyStyle() const { return m_autoApplyStyle; }

signals:
    /**
     * @brief 组件创建信号
     */
    void componentCreated(const QString& name, QWidget* widget);

    /**
     * @brief 组件注册信号
     */
    void componentRegistered(const QString& name);

private:
    ComponentFactory();
    ~ComponentFactory() = default;
    ComponentFactory(const ComponentFactory&) = delete;
    ComponentFactory& operator=(const ComponentFactory&) = delete;

    /**
     * @brief 应用默认样式
     */
    void applyDefaultStyle(QWidget* widget);

    // 组件创建函数映射
    QHash<QString, CreatorFunc> m_creators;

    // 组件缓存
    QHash<QString, QWidget*> m_cache;

    // 是否自动应用样式
    bool m_autoApplyStyle = true;
};

// ============================================================================
// 便捷宏定义
// ============================================================================

/**
 * @brief 注册组件的便捷宏
 */
#define REGISTER_COMPONENT(name, className) \
    ComponentFactory::instance().registerComponent(name, []() -> QWidget* { \
        return new className(); \
    })

/**
 * @brief 创建组件的便捷宏
 */
#define CREATE_COMPONENT(name, className) \
    ComponentFactory::instance().create<className>(name)

#endif // COMPONENTFACTORY_H
