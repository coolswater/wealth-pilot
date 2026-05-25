/**
 * @file ComponentFactory.cpp
 * @brief 组件工厂实现
 */

#include "ComponentFactory.h"
#include <QMutexLocker>

// ============================================================================
// 单例实现
// ============================================================================

ComponentFactory& ComponentFactory::instance()
{
    static ComponentFactory instance;
    return instance;
}

// ============================================================================
// 构造函数
// ============================================================================

ComponentFactory::ComponentFactory()
    : QObject(nullptr)
{
}

// ============================================================================
// 组件注册
// ============================================================================

void ComponentFactory::registerComponent(const QString& name, CreatorFunc creator)
{
    if (name.isEmpty() || !creator) {
        return;
    }

    m_creators[name] = creator;
    emit componentRegistered(name);
}

void ComponentFactory::unregisterComponent(const QString& name)
{
    m_creators.remove(name);
    m_cache.remove(name);
}

bool ComponentFactory::hasComponent(const QString& name) const
{
    return m_creators.contains(name);
}

// ============================================================================
// 组件创建
// ============================================================================

QWidget* ComponentFactory::create(const QString& name, QWidget* parent)
{
    if (!m_creators.contains(name)) {
        return nullptr;
    }

    // 调用创建函数
    QWidget* widget = m_creators[name]();

    if (!widget) {
        return nullptr;
    }

    // 设置父控件
    if (parent) {
        widget->setParent(parent);
    }

    // 自动应用样式
    if (m_autoApplyStyle) {
        applyDefaultStyle(widget);
    }

    emit componentCreated(name, widget);
    return widget;
}

// ============================================================================
// 组件缓存
// ============================================================================

QWidget* ComponentFactory::getOrCreate(const QString& name, QWidget* parent)
{
    // 检查缓存
    if (m_cache.contains(name)) {
        QWidget* cached = m_cache[name];
        if (cached) {
            return cached;
        }
    }

    // 创建新组件
    QWidget* widget = create(name, parent);
    if (widget) {
        m_cache[name] = widget;
    }

    return widget;
}

void ComponentFactory::clearCache()
{
    // 删除缓存中的组件
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.value()) {
            it.value()->deleteLater();
        }
    }
    m_cache.clear();
}

int ComponentFactory::cacheSize() const
{
    return m_cache.size();
}

// ============================================================================
// 私有方法
// ============================================================================

void ComponentFactory::applyDefaultStyle(QWidget* widget)
{
    if (!widget) {
        return;
    }

    // 应用深色主题背景
    widget->setAutoFillBackground(true);
    QPalette pal = widget->palette();
    pal.setColor(QPalette::Window, QColor(Tokens::Colors::BgBase));
    pal.setColor(QPalette::WindowText, QColor(Tokens::Colors::TextPrimary));
    pal.setColor(QPalette::Text, QColor(Tokens::Colors::TextPrimary));
    widget->setPalette(pal);
}
