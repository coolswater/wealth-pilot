/**
 * @file UIComponents.h
 * @brief UI组件库 - 统一管理所有可复用UI组件
 *
 * @details 包含：
 * - 响应式布局组件（ResponsiveLayout）
 * - 基础组件（Button, Input, Table等）
 * - 高级组件（Chart, Form等）
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef UICOMPONENTS_H
#define UICOMPONENTS_H

#include <QHash>
#include <QMap>
#include <QColor>
#include <QFont>
#include <QSize>
#include <QVariant>

// 前置声明
class ResponsiveLayout;
class Button;
class InputField;
class DataTable;
class ChartWidget;

/**
 * @brief UI组件库命名空间
 */
namespace UIComponents {

// ========== 主题配置 ==========

/**
 * @brief 主题配置类
 */
struct ThemeConfig {
    QString name;                    // 主题名称
    QColor primaryColor;             // 主色
    QColor secondaryColor;           // 辅助色
    QColor backgroundColor;          // 背景色
    QColor surfaceColor;             // 表面色
    QColor onPrimaryColor;           // 主色文字色
    QColor onSecondaryColor;         // 辅助色文字色
    QColor onBackgroundColor;        // 背景色文字色
    QColor onSurfaceColor;           // 表面色文字色
    QFont baseFont;                 // 基础字体
    QFont titleFont;                // 标题字体
    QFont bodyFont;                 // 正文字体
    int borderRadius;               // 圆角半径
    int spacing;                    // 间距
    int padding;                    // 内边距
    qreal elevation;                // 阴影高度
};

// ========== 响应式断点 ==========

/**
 * @brief 响应式断点配置
 */
struct BreakpointConfig {
    QString name;                   // 断点名称
    int width;                      // 宽度阈值（px）
    qreal scale;                    // 缩放因子
};

// ========== 组件注册 ==========

/**
 * @brief 组件注册器
 */
class ComponentRegistry {
public:
    static ComponentRegistry& instance();

    // 注册组件类型
    template<typename T>
    void registerComponent(const QString& name);

    // 获取组件类型
    template<typename T>
    T* getComponent(const QString& name);

private:
    ComponentRegistry() = default;
    ~ComponentRegistry() = default;
    ComponentRegistry(const ComponentRegistry&) = delete;
    ComponentRegistry& operator=(const ComponentRegistry&) = delete;

    QHash<QString, QVariant> m_components;
};

} // namespace UIComponents

#endif // UICOMPONENTS_H