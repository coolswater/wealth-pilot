#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QTimer>
#include <QApplication>
#include <QKeyEvent>

#include "core/base/BasePage.h"
#include "market/QuoteDataManager.h"
#include "views/widgets/TreeMapWidget.h"

// 如果你的项目�?BasePage，取消注释下面这行并确保 BasePage 继承�?QWidget
// #include "core/base/BasePage.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;

/**
 * @brief 行情仪表盘页�?
 * 如果项目使用 PageFactoryRegistry 要求继承 BasePage，则改为�?
 * class DashboardPage : public BasePage
 */
class DashboardPage : public BasePage
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget* parent = nullptr);
    ~DashboardPage() override;

    [[nodiscard]] QString pageId() const override;
    void initializePage() override;


    // 公共接口
    void refreshData() const; // 手动刷新
    void setMarket(const QString& market) const; // 设置当前市场
    void setFullScreen(bool fullscreen) const; // 全屏模式切换

    // 处理键盘导航（供 TreeMapWidget 回调或外部调用）
    void handleKeyNavigation(int key);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onMarketChanged(int index) const;
    void onIndustryChanged(int index) const;
    void onSearchTextChanged(const QString& text) const;
    void onRefreshClicked() const;
    void onReviewModeToggled(bool enabled);
    void onAutoPlay() const;
    void onDataUpdated() const;
    void onItemDoubleClicked(const StockQuoteItem& item);
    void onItemHovered(const StockQuoteItem& item) const;
    void onStatsChanged(int up, int flat, int down, double turnover) const;
    void updateTimeDisplay() const;
    void showHelp();

private:
    void initUI();
    void initToolBar();
    void initStatusBar();
    void connectSignals();
    void initData();
    void loadMarketData(const QString& market) const;
    void updateIndustryCombo(const QVector<StockQuoteItem>& items) const;

    // 复盘控制
    void nextReviewFrame() const;
    void prevReviewFrame() const;
    void toggleAutoPlay() const;

    struct Impl; // 前置声明实现�?
    std::unique_ptr<Impl> d; // Pimpl指针，减少头文件依赖
};

#endif // DASHBOARDPAGE_H
