/**
 * @file BasePage.h
 * @brief 页面基类 - 所有页面的统一基类
 *
 * @details 提供页面生命周期管理、初始化状态、导航参数等功能
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QWidget>
#include <QVariantMap>
#include <QString>
#include <memory>

// 前向声明
class PageNavigatorManager;
class MainWindow;

namespace WealthPilot {

/**
 * @brief 页面基类
 * @details 所有页面应继承此基类，统一管理页面生命周期
 */
class BasePage : public QWidget
{
    Q_OBJECT

    // 允许 PageNavigatorManager 和 MainWindow 访问 protected 方法
    friend class ::PageNavigatorManager;
    friend class ::MainWindow;

public:
    explicit BasePage(QWidget* parent = nullptr);
    virtual ~BasePage();

    /**
     * @brief 获取页面唯一标识
     */
    virtual QString pageId() const = 0;

    /**
     * @brief 获取页面名称
     */
    virtual QString pageName() const { return pageId(); }

    /**
     * @brief 页面是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief 设置初始化状态
     */
    void setInitialized(bool initialized) { m_initialized = initialized; }

    /**
     * @brief 获取导航参数
     */
    QVariantMap navigationParams() const { return m_params; }

    /**
     * @brief 设置导航参数
     */
    void setNavigationParams(const QVariantMap& params) { m_params = params; }

signals:
    /**
     * @brief 页面初始化完成信号
     */
    void initialized();

    /**
     * @brief 页面激活信号
     */
    void pageActivated();

    /**
     * @brief 页面 deactivated 信号
     */
    void pageDeactivated();

    /**
     * @brief 请求导航到其他页面
     */
    void navigateToRequested(const QString& pageId, const QVariantMap& params = QVariantMap());

protected:
    /**
     * @brief 初始化页面（子类实现）
     * @details 只在首次显示时调用一次
     */
    virtual void initializePage() {}

    /**
     * @brief 页面激活时调用
     */
    virtual void onPageActivated() {}

    /**
     * @brief 页面激活时调用（带参数）
     */
    virtual void onPageActivated(const QVariantMap& params) { Q_UNUSED(params); }

    /**
     * @brief 页面 deactivated 时调用
     */
    virtual void onPageDeactivated() {}

    /**
     * @brief 页面显示时调用
     */
    void showEvent(QShowEvent* event) override;

    /**
     * @brief 页面隐藏时调用
     */
    void hideEvent(QHideEvent* event) override;

private:
    bool m_initialized = false;
    QVariantMap m_params;
};

} // namespace WealthPilot

#endif // BASEPAGE_H