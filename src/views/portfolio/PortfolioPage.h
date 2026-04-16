#ifndef PORTFOLIOPAGE_H
#define PORTFOLIOPAGE_H

#include <core/base/BasePage.h>
#include <memory>

// 前向声明，减少头文件依赖
class QLabel;
class QLineEdit;
class QPushButton;

/**
 * @brief 投资组合页面 - 应配置为强缓存页面
 * @details 建议强引用缓存（StrongCache），因为主页常驻内存，避免重复创建
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

    // 设置演示模式数据（用于演示）
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
    struct Impl;                   ///< 前向声明实现结构体
    std::unique_ptr<Impl> d;       ///< Pimpl指针，隐藏实现细节

    void setupUI();                ///< UI构建
    void createHeader();           ///< 创建头部
    void createCardsSection();     ///< 创建卡片区域
    void createChartsSection();    ///< 创建图表区域
    void setupAnimations();        ///< 动画效果设置
    void connectSignals();         ///< 内部信号槽连接
    void fetchDataAsync();         ///< 数据更新（异步）
};

#endif // PORTFOLIOPAGE_H