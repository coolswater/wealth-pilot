#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include "BaseWidget.h"
#include <QMap>

class QPushButton;
class QVBoxLayout;

/**
 * @brief 侧边栏控件
 *
 * 可折叠的导航侧边栏
 */
class SidebarWidget : public BaseWidget
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget *parent = nullptr);
    ~SidebarWidget();

    // 添加导航项
    // 重载 addItem，支持 QIcon（用于 SvgColorIcon）
    void addItem(const QString& id, const QString& text, const QIcon& icon);

    // 保留旧接口用于兼容（内部转换为 QIcon）
    void addItem(const QString& id, const QString& text);

    // 新增：设置折叠按钮图标（动态主题切换）
    void setCollapseIcons(const QIcon& left, const QIcon& right) const;

    // 设置当前选中项
    void setCurrentItem(const QString& id) const;
    QString currentItem() const;
    
    // 清除选中状态
    void clearSelection();

    // 折叠/展开
    void setCollapsed(bool collapsed);
    bool isCollapsed() const;
    void toggle();

    // 设置宽度
    void setExpandedWidth(int width);
    void setCollapsedWidth(int width);

signals:
    void itemClicked(const QString& id);
    void collapsedChanged(bool collapsed);

private slots:
    void onItemClicked();

private:
    void setupUI();
    void updateLayout();
    void animateCollapse(bool collapse);
    void updateToggleButtonIcon() const;
    void updateTheme();  // 主题更新方法

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // SIDEBARWIDGET_H
