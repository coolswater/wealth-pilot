#ifndef WATCHLISTPAGE_H
#define WATCHLISTPAGE_H

#include <QObject>

#include <core/base/BasePage.h>
#include <memory>

/**
 * @brief 自选页面
 * @details 建议强引用缓存（StrongCache），因为主页常驻内存，避免重复创建
 */
class WatchListPage : public BasePage {
    Q_OBJECT
public:
    explicit WatchListPage(QWidget *parent = nullptr);
    ~WatchListPage() override;

    QString pageId() const override;
    void initializePage() override;

private:
    void setupUI();                ///< UI构建
    void setupAnimations();        ///< 动画效果设置
    void connectSignals();         ///< 内部信号槽连接

    struct Impl;                   ///< 前向声明实现结构体
    std::unique_ptr<Impl> d;       ///< Pimpl指针，隐藏实现细节
};

#endif // WATCHLISTPAGE_H