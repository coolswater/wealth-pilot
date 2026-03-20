#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QObject>

#include <core/BasePage.h>
#include <memory>

/**
 * @brief 仪表板页面 - 应用主入口页面
 * @details 采用强缓存策略（StrongCache），作为首页常驻内存避免重复创建
 */
class Dashboard : public BasePage {
    Q_OBJECT
public:
    explicit Dashboard(QWidget *parent = nullptr);
    ~Dashboard() override;

    QString pageId() const override;
    void initializePage() override;

private:
    struct Impl;  // 前置声明实现类
    std::unique_ptr<Impl> d;  // Pimpl指针，减少头文件依赖

    void setupUI();           // UI构建
    void setupAnimations();   // 动画效果配置
    void connectSignals();    // 内部信号连接
};

#endif // DASHBOARD_H
