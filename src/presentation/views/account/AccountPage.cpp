/**
 * @file AccountPage.cpp
 * @brief 账户页面实现 - 账户信息展示
 *
 * @author WealthPilot Team
 * @version 2.0.0
 */

#include "AccountPage.h"
#include "core/config/Tokens.h"
#include "presentation/components/StyleHelper.h"
#include "shared/utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDateTime>

using namespace Tokens;

namespace WealthPilot
{
    struct AccountPage::Impl
    {
        // 预留扩展
    };

    AccountPage::AccountPage(QWidget* parent)
        : DataHubPageBase(parent)
          , d(std::make_unique<Impl>())
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

    // ============================================================
    // 设置 DataHub 订阅
    // ============================================================
    setupDataHubSubscriptions();

    setInitialized(true);
    LOG_INFO("AccountPage initialized with DataHub");
}

void AccountPage::setupDataHubSubscriptions()
{
    // 订阅账户余额
    dataHub().subscribe(this, "account:balance",
        [this](const QVariant& value) {
            Q_UNUSED(value)
            // 更新账户余额显示
        });
    
    // 订阅账户设置
    dataHub().subscribe(this, "account:settings",
        [this](const QVariant& value) {
            Q_UNUSED(value)
            // 更新账户设置显示
        });
    
    // 订阅账户交易统计
    dataHub().subscribe(this, "account:stats",
        [this](const QVariant& value) {
            Q_UNUSED(value)
            // 更新交易统计
        });
    
    LOG_INFO("[AccountPage] DataHub subscriptions setup complete");
}

void AccountPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 页面头部
    auto* header = StyleHelper::createPageHeader(this, QStringLiteral("账户信息"));
    mainLayout->addWidget(header);

    // 主内容区域
    auto* contentWidget = new QWidget(this);
    contentWidget->setStyleSheet(QString("background-color: %1;").arg(Colors::BgBase));
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(16);

    // 账户信息卡片
    auto* infoCard = createAccountInfoCard();
    contentLayout->addWidget(infoCard);

    contentLayout->addStretch();
    mainLayout->addWidget(contentWidget, 1);

    // 状态栏
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    auto* statusBar = StyleHelper::createPageStatusBar(this, QStringLiteral("账户状态: 正常"), timeStr);
    mainLayout->addWidget(statusBar);
}

QFrame* AccountPage::createAccountInfoCard()
{
    auto* card = new QFrame(this);
    card->setStyleSheet(QString(R"(
        QFrame {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 8px;
        }
    )").arg(Colors::BgElevated, Colors::Border));

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);

    // 标题
    auto* titleLabel = new QLabel(QStringLiteral("账户概览"), card);
    titleLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;")
        .arg(Colors::TextPrimary));
    layout->addWidget(titleLabel);

    // 分隔线
    auto* separator = new QFrame(card);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(QString("background-color: %1;").arg(Colors::Border));
    separator->setFixedHeight(1);
    layout->addWidget(separator);

    // 提示信息
    auto* infoLabel = new QLabel(QStringLiteral("账户信息功能开发中，敬请期待..."), card);
    infoLabel->setStyleSheet(QString("color: %1; font-size: 14px;")
        .arg(Colors::TextSecondary));
    infoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel);

    return card;
}

void AccountPage::setupConnections()
{
    // 暂无信号连接
}

} // namespace WealthPilot
