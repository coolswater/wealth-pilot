/**
 * @file FuturesPageIntegration.h
 * @brief 期货页面集成 - 将行情列表页和K线详情页集成
 *
 * @details 功能：
 * - 连接FuturesQuotesPage和FuturesKLinePage
 * - 处理页面跳转
 * - 管理实时行情订阅
 * - 统一CTP连接管理
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef FUTURES_PAGE_INTEGRATION_H
#define FUTURES_PAGE_INTEGRATION_H

#include <QObject>
#include <QStackedWidget>
#include <memory>

// 前向声明
namespace WealthPilot {
class FuturesQuotesPage;
}

class FuturesKLinePage;
class PageNavigator;

/**
 * @brief 期货页面集成管理器
 * 
 * @details 负责管理期货行情列表页和K线详情页的集成，包括：
 * - 页面切换
 * - 参数传递
 * - 实时行情订阅管理
 * - CTP连接共享
 */
class FuturesPageIntegration : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static FuturesPageIntegration& instance();

    /**
     * @brief 初始化集成
     * @param stackedWidget 页面容器
     */
    void initialize(QStackedWidget* stackedWidget);

    /**
     * @brief 显示行情列表页
     */
    void showQuotesPage();

    /**
     * @brief 显示K线详情页
     * @param instrumentId 合约代码
     */
    void showKLinePage(const QString& instrumentId);

    /**
     * @brief 返回行情列表页
     */
    void goBack();

    /**
     * @brief 获取行情列表页
     */
    WealthPilot::FuturesQuotesPage* quotesPage() const;

    /**
     * @brief 获取K线详情页
     */
    FuturesKLinePage* klinePage() const;

signals:
    /**
     * @brief 页面切换信号
     */
    void pageChanged(const QString& pageId);

    /**
     * @brief 合约选择信号
     */
    void instrumentSelected(const QString& instrumentId);

private slots:
    /**
     * @brief 处理导航到K线页面的请求
     */
    void onNavigateToKLinePage(const QString& instrumentId, const QVariantMap& params);

    /**
     * @brief 处理导航完成事件
     */
    void onNavigated(const QString& pageId, const QVariantMap& params);

private:
    FuturesPageIntegration();
    ~FuturesPageIntegration();
    Q_DISABLE_COPY(FuturesPageIntegration)

    void setupConnections();

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FUTURES_PAGE_INTEGRATION_H
