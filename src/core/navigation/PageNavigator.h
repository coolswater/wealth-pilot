/**
 * @file PageNavigator.h
 * @brief 页面导航管理器 - 统一管理页面跳转和参数传递
 *
 * @details 功能：
 * - 页面跳转管理
 * - 参数传递
 * - 历史记录
 * - 返回导航
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef PAGENAVIGATOR_H
#define PAGENAVIGATOR_H

#include <QObject>
#include <QVariantMap>
#include <QStack>
#include <QMap>
#include <QDateTime>
#include <functional>

// 前向声明
class BasePage;

/**
 * @brief 导航参数键定义
 */
namespace NavParam {
    // 合约相关
    constexpr const char* INSTRUMENT_ID = "instrumentId";
    constexpr const char* INSTRUMENT_NAME = "instrumentName";
    constexpr const char* EXCHANGE_ID = "exchangeId";
    
    // K线相关
    constexpr const char* PERIOD = "period";
    constexpr const char* SHOW_INDICATORS = "showIndicators";
    
    // 交易相关
    constexpr const char* DIRECTION = "direction";
    constexpr const char* PRICE = "price";
    constexpr const char* VOLUME = "volume";
    
    // 来源页面
    constexpr const char* SOURCE_PAGE = "sourcePage";
}

/**
 * @brief 导航历史记录项
 */
struct NavigationHistory {
    QString pageId;             // 页面ID
    QVariantMap params;         // 页面参数
    QDateTime timestamp;        // 导航时间
};

/**
 * @brief 页面导航管理器
 */
class PageNavigator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     */
    static PageNavigator& instance();

    /**
     * @brief 导航到指定页面
     * @param pageId 目标页面ID
     * @param params 导航参数
     * @param addToHistory 是否添加到历史记录
     */
    void navigateTo(const QString& pageId, 
                   const QVariantMap& params = QVariantMap(),
                   bool addToHistory = true);

    /**
     * @brief 返回上一页
     * @return 是否成功返回
     */
    bool goBack();

    /**
     * @brief 返回到指定页面
     * @param pageId 目标页面ID
     * @return 是否成功返回
     */
    bool goBackTo(const QString& pageId);

    /**
     * @brief 清空历史记录
     */
    void clearHistory();

    /**
     * @brief 获取当前页面ID
     */
    QString currentPageId() const;

    /**
     * @brief 获取当前页面参数
     */
    QVariantMap currentParams() const;

    /**
     * @brief 获取历史记录数量
     */
    int historyCount() const;

    /**
     * @brief 是否可以返回
     */
    bool canGoBack() const;

    /**
     * @brief 注册页面创建函数
     * @param pageId 页面ID
     * @param creator 创建函数
     */
    void registerPage(const QString& pageId,
                     std::function<BasePage*()> creator);

    /**
     * @brief 注销页面
     * @param pageId 页面ID
     */
    void unregisterPage(const QString& pageId);

    /**
     * @brief 创建页面实例
     * @param pageId 页面ID
     * @return 页面实例（如果已注册）
     */
    BasePage* createPage(const QString& pageId);

signals:
    /**
     * @brief 导航前信号
     */
    void navigating(const QString& pageId, const QVariantMap& params);

    /**
     * @brief 导航完成信号
     */
    void navigated(const QString& pageId, const QVariantMap& params);

    /**
     * @brief 返回导航信号
     */
    void goingBack(const QString& fromPageId, const QString& toPageId);

private:
    PageNavigator();
    ~PageNavigator();
    Q_DISABLE_COPY(PageNavigator)

    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // PAGENAVIGATOR_H
