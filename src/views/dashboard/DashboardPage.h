#ifndef DASHBOARDPAGE_H
#define DASHBOARDPAGE_H

#include <QWidget>
#include <QTimer>
#include <QApplication>
#include <QKeyEvent>

#include "core/base/BasePage.h"
#include "market/QuoteDataManager.h"
#include "views/widgets/TreeMapWidget.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;

/**
 * @brief 仪表盘页面类
 * 
 * 使用 PageFactoryRegistry 需要继承 BasePage
 * 展示市场全景图，包括板块涨跌幅、个股表现等
 */
class DashboardPage : public BasePage
{
    Q_OBJECT

public:
    explicit DashboardPage(QWidget* parent = nullptr);
    ~DashboardPage() override;

    /**
     * @brief 获取页面ID
     */
    [[nodiscard]] QString pageId() const override;
    
    /**
     * @brief 初始化页面
     */
    void initializePage() override;

    // 公共接口
    void refreshData() const;           ///< 手动刷新
    void setMarket(const QString& market) const;  ///< 设置当前市场
    void setFullScreen(bool fullscreen) const;    ///< 全屏模式切换

    // 键盘导航接口（供外部调用）
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
    void initUI();          ///< 初始化UI
    void initToolBar();     ///< 初始化工具栏
    void initStatusBar();   ///< 初始化状态栏
    void connectSignals();  ///< 连接信号槽
    void initData();        ///< 初始化数据
    void loadMarketData(const QString& market) const;       ///< 加载市场数据
    void updateIndustryCombo(const QVector<StockQuoteItem>& items) const;  ///< 更新行业下拉框

    // 复盘控制
    void nextReviewFrame() const;   ///< 下一帧
    void prevReviewFrame() const;   ///< 上一帧
    void toggleAutoPlay() const;    ///< 切换自动播放

    struct Impl;                    ///< 前向声明实现结构体
    std::unique_ptr<Impl> d;        ///< Pimpl指针，隐藏实现细节
};

#endif // DASHBOARDPAGE_H