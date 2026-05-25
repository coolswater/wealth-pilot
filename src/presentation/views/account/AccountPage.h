/**
 * @file AccountPage.h
 * @brief 账户页面 - 使用 DataHub 数据中心
 *
 * @details 功能：
 * - 账户基本信息
 * - 资金状况
 * - 账户设置
 *
 * DataHub 集成：
 * - 通过 DataHub 订阅账户数据
 * - 自动生命周期管理
 * - 实时资金更新
 *
 * @author WealthPilot Team
 * @version 3.0.0
 */

#ifndef ACCOUNTPAGE_H
#define ACCOUNTPAGE_H

#include "presentation/components/DataHubPageBase.h"
#include <QMap>
#include <memory>

QT_BEGIN_NAMESPACE
class QFrame;
QT_END_NAMESPACE

namespace WealthPilot
{

/**
 * @brief 账户页面 - 账户信息展示
 *
 * @details 继承 DataHubPageBase，自动管理数据订阅：
 * - 订阅账户资金数据（account:balance）
 * - 订阅账户设置（account:settings）
 * - 页面销毁时自动取消订阅
 */
class AccountPage : public DataHubPageBase
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit AccountPage(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~AccountPage() override;

    // ========== 页面信息 ==========

    /**
     * @brief 获取页面ID
     */
    QString pageId() const override { return QStringLiteral("account"); }

    /**
     * @brief 获取页面名称
     */
    QString pageName() const override { return QStringLiteral("账户"); }

    /**
     * @brief 初始化页面
     *
     * @details 初始化流程：
     * 1. 设置 UI 组件
     * 2. 订阅 DataHub 账户数据
     * 3. 加载初始数据
     */
    void initializePage() override;

private:
    // ========== UI 初始化 ==========

    /**
     * @brief 设置 UI
     */
    void setupUI();

    /**
     * @brief 创建账户信息卡片
     */
    QFrame* createAccountInfoCard();

    /**
     * @brief 设置信号连接
     */
    void setupConnections();

    // ========== DataHub 数据订阅 ==========

    /**
     * @brief 设置 DataHub 数据订阅
     *
     * @details 订阅流程：
     * 1. 订阅账户余额（account:balance）
     * 2. 订阅账户设置（account:settings）
     * 3. 回调函数中更新显示
     */
    void setupDataHubSubscriptions();

    // ========== 私有实现类（PIMPL） ==========
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // ACCOUNTPAGE_H