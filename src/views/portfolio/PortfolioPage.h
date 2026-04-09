#ifndef PORTFOLIOPAGE_H
#define PORTFOLIOPAGE_H

#include <core/BasePage.h>
#include <memory>

// 前向声明减少编译依赖
class QLabel;
class QLineEdit;
class QPushButton;
/**
 * @brief 仪表板页面 - 应用主入口页面
 * @details 采用强缓存策略（StrongCache），作为首页常驻内存避免重复创建
 */
class PortfolioPage : public BasePage {
    Q_OBJECT
public:
    explicit PortfolioPage(QWidget *parent = nullptr);
    ~PortfolioPage() override;

    QString pageId() const override;
    void initializePage() override;

    // 刷新数据
    void refreshData();

    // 设置模拟数据（演示用）
    void setupDemoData() const;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onSearch(const QString& text);
    void onTimeRangeChanged(int index) const;
    void updateRealTimeData() const;

private:
    struct Impl;  // 前置声明实现类
    std::unique_ptr<Impl> d;  // Pimpl指针，减少头文件依赖

    void setupUI();           // UI构建
    void createHeader();
    void createCardsSection();
    void createChartsSection();
    void setupAnimations();   // 动画效果配置
    void connectSignals();    // 内部信号连接
    // 数据更新（异步）
    void fetchDataAsync();
};

#endif // DASHBOARDPAGE_H
