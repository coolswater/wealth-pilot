#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


/**
 * @file MainWindow.h
 * @brief 主窗口类
 *
 * @details 应用主界面，采用三栏布局：
 * - 左侧：可折叠的导航侧边栏
 * - 中间：内容区域（页面堆栈）
 * - 右侧：可滑出的AI助理面板
 *
 * @note 使用QStackedWidget管理多个页面
 * @note 支持响应式布局，自动适应窗口大小
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow();

    // 更新最大化按钮图标（全屏/正常状态切换时调用）
    void updateMaximizeButton(bool isMaximized);

protected:
    /**
     * @brief 窗口大小变化事件
     * @param event 大小变化事件
     *
     * @details 重新计算各区域尺寸
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief 窗口关闭事件
     * @param event 关闭事件
     *
     * @details 保存窗口状态，执行清理操作
     */
    void closeEvent(QCloseEvent *event) override;

private slots:

    /**
     * @brief 侧边栏项点击处理
     * @param id 点击的项ID
     */
    void onSidebarItemClicked(const QString& id);

private:
    /**
     * @brief 设置UI布局
     */
    void setupUI();

    /**
     * @brief 创建所有页面
     */
    void createPages();

    /**
     * @brief 连接信号槽
     */
    void connectSignals();

    /**
     * @brief 调整布局
     * @details 根据当前窗口大小重新计算各区域尺寸
     */
    void adjustLayout();

    // 懒加载页面
    QWidget* getPage(const QString& pageId);

    /**
     * @brief PIMPL实现结构体
     */
    struct Impl;
    std::unique_ptr<Impl> d;
};
#endif // MAINWINDOW_H
