/**
 * @file AccountPage.cpp
 * @brief 账户页面实现 - 账户信息展示
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "AccountPage.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QLabel>

namespace WealthPilot
{
    AccountPage::AccountPage(QWidget* parent)
        : BasePage(parent)
{
    setupUI();
    setupConnections();
    LOG_DEBUG("AccountPage created");
}

AccountPage::~AccountPage()
{
    LOG_DEBUG("AccountPage destroyed");
}

void AccountPage::initializePage()
{
    if (isInitialized())
    {
        return;
    }

    setInitialized(true);
    LOG_INFO("AccountPage initialized");
}

void AccountPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建账户信息页面
    auto* accountInfoPage = createAccountInfoPage();
    mainLayout->addWidget(accountInfoPage);
}

QWidget* AccountPage::createAccountInfoPage()
{
    // 创建账户信息页面
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    auto* label = new QLabel(QStringLiteral("账户信息页面 - 待实现"), page);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    return page;
}

void AccountPage::setupConnections()
{
    // 暂无信号连接
}

} // namespace WealthPilot
