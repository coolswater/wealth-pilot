/**
 * @file MainWindow.h
 * @brief 主窗口类 - 重构版本，集成新架构
 *
 * @details 功能：
 * - 三栏布局（侧边栏、内容区、AI助理面板）
 * - 集成ApplicationInitializer
 * - 集成ServiceLocator依赖注入
 * - 集成ThemeEngine主题系统
 * - 性能优化：懒加载、异步初始化
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <memory>

// 前向声明
class ThemeEngine;
class ApplicationInitializer;

/**
 * @brief 主窗口类
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow() override;

    /**
     * @brief 更新最大化按钮图标
     */
    void updateMaximizeButton(bool isMaximized);

protected:
    /**
     * @brief 窗口大小变化事件
     */
    void resizeEvent(QResizeEvent *event) override;

    /**
     * @brief 窗口关闭事件
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /**
     * @brief 侧边栏项点击处理
     */
    void onSidebarItemClicked(const QString& id);

    /**
     * @brief 主题切换处理
     */
    void onThemeChanged(const QString& themeName);

    /**
     * @brief 初始化进度更新
     */
    void onInitializationProgress(int current, int total, const QString& currentModule);

    /**
     * @brief 初始化完成
     */
    void onInitializationComplete(bool success);

private:
    /**
     * @brief 设置UI布局
     */
    void setupUI();

    /**
     * @brief 创建页面
     */
    void createPages();

    /**
     * @brief 连接信号槽
     */
    void connectSignals();

    /**
     * @brief 调整布局
     */
    void adjustLayout();

    /**
     * @brief 懒加载页面
     */
    QWidget* getPage(const QString& pageId);

    /**
     * @brief 初始化应用
     */
    bool initializeApplication();

    /**
     * @brief 加载设置
     */
    void loadSettings();

    /**
     * @brief 保存设置
     */
    void saveSettings();

    /**
     * @brief 应用主题
     */
    void applyTheme();

    /**
     * @brief 显示启动画面
     */
    void showSplashScreen();

    /**
     * @brief 隐藏启动画面
     */
    void hideSplashScreen();

    // PIMPL实现
    struct Impl;
    std::unique_ptr<Impl> d;
    
    // 初始化状态
    bool m_initialized;
    
    // 启动画面标签
    QLabel* m_splashLabel;
};

#endif // MAINWINDOW_H
