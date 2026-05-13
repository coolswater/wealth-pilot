/**
 * @file AccountPage.h
 * @brief 账户页面 - 账户信息展示
 *
 * @details 功能：
 * - 账户基本信息
 * - 资金状况
 * - 账户设置
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef ACCOUNTPAGE_H
#define ACCOUNTPAGE_H

#include "ui/components/BasePage.h"
#include <QMap>

namespace WealthPilot
{
    /**
 * @brief 账户页面 - 账户信息展示
 */
class AccountPage : public BasePage
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
     */
    void initializePage() override;

private:
    /**
     * @brief 设置 UI
     */
    void setupUI();

    /**
     * @brief 创建账户信息页
     */
    QWidget* createAccountInfoPage();

    /**
     * @brief 设置信号连接
     */
    void setupConnections();
};

} // namespace WealthPilot

#endif // ACCOUNTPAGE_H
