// TitleBarWidget.h
#ifndef TITLEBARWIDGET_H
#define TITLEBARWIDGET_H

#include <QWidget>
#include <memory>

class QLabel;
class QPushButton;
class ThemeToggleButton;

/**
 * @class TitleBarWidget
 * @brief 自定义标题栏组件，支持主题切换和窗口控制
 *
 * 特性：
 * - 响应ThemeManager主题变化，自动更新颜色
 * - 支持窗口拖动、双击最大化/还原
 * - 集成ThemeToggleButton循环切换主题
 * - 使用SvgColorIcon支持SVG图标主题色适配
 */
class TitleBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBarWidget(QWidget *parent = nullptr);
    ~TitleBarWidget() override;

    // 设置标题栏标题文本
    void setTitle(const QString& title);

    // 设置窗口图标（显示在左上角）
    void setWindowIcon(const QPixmap& icon);

    // 更新最大化按钮图标状态（最大化/还原）
    void updateMaximizeButton(bool isMaximized);

protected:
    // 鼠标事件处理 - 实现窗口拖动
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    // 事件过滤 - 监听父窗口状态变化
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    // 主题变更响应 - 更新标题栏配色
    void onThemeChanged();

    // 窗口控制槽函数
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();

private:
    // 初始化UI布局
    void setupUI();

    // 初始化信号槽连接
    void initConnections();

    // 应用当前主题样式到标题栏
    void applyThemeStyle();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // TITLEBARWIDGET_H
