#ifndef WATCHLISTPAGE_H
#define WATCHLISTPAGE_H

#include <QObject>

#include <core/BasePage.h>
#include <memory>

/**
 * @brief 自选页�?
 * @details 采用强缓存策略（StrongCache），作为首页常驻内存避免重复创建
 */
class WatchListPage : public BasePage {
    Q_OBJECT
public:
    explicit WatchListPage(QWidget *parent = nullptr);
    ~WatchListPage() override;

    QString pageId() const override;
    void initializePage() override;

private:
    void setupUI();           // UI构建
    void setupAnimations();   // 动画效果配置
    void connectSignals();    // 内部信号连接

    struct Impl;  // 前置声明实现�?
    std::unique_ptr<Impl> d;  // Pimpl指针，减少头文件依赖
};

#endif // WATCHLISTPAGE_H
