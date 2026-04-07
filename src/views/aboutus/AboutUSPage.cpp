/**
 * @file AboutUSPage.cpp
 * @brief 关于页面实现
 */

#include "AboutUSPage.h"

#include <QPixmap>

#include <views/widgets/CardWidget.h>
#include <core/Tokens.h>

using namespace Tokens;

struct AboutUSPage::Impl
{
    QVBoxLayout* mainLayout = nullptr;
    QLabel* m_logoLabel = nullptr;
};

AboutUSPage::AboutUSPage(QWidget* parent)
    : BasePage(parent)
      , d(std::make_unique<Impl>())
{
    setupUI();
}

AboutUSPage::~AboutUSPage() = default;

QString AboutUSPage::pageId() const
{
    return QStringLiteral("AboutUSPage");
}

void AboutUSPage::initializePage()
{
}

void AboutUSPage::setupUI()
{
    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setContentsMargins(20, 20, 20, 20);
    d->mainLayout->setSpacing(Spacing::SM);

    // Logo
    d->m_logoLabel = new QLabel(this);
    d->m_logoLabel->setFixedSize(200, 50);
    d->m_logoLabel->setScaledContents(true);
    d->m_logoLabel->setPixmap(QPixmap(":/images/app_vertical_logo.png"));
    d->mainLayout->addWidget(d->m_logoLabel);

    // 应用名称 - 样式由QSS管理
    auto* nameLabel = new QLabel("WealthPilot 智能投资管理", this);
    nameLabel->setProperty("heading", true);
    nameLabel->setMargin(20);
    d->mainLayout->addWidget(nameLabel);

    // 简介
    auto* infoLabel = new QLabel(
        "WealthPilot 是专为 PC 用户打造的金融信息分析平台。基于 Qt 框架开发，它不仅为您实时追踪股票、期货行情，更提供便捷的自选股管理与宏观市场全景分析，助您一站式洞悉市场，高效决策", this);
    infoLabel->setProperty("secondary", true);
    infoLabel->setWordWrap(true);
    d->mainLayout->addWidget(infoLabel);

    // 版本
    auto* versionLabel = new QLabel("版本: v1.0.0", this);
    versionLabel->setProperty("secondary", true);
    d->mainLayout->addWidget(versionLabel);

    // 技术栈
    auto* techLabel = new QLabel("技术栈: Qt 6.10.2 / C++17", this);
    techLabel->setProperty("secondary", true);
    d->mainLayout->addWidget(techLabel);

    // 开发者
    auto* devLabel = new QLabel("开发者: WealthPilot Team", this);
    devLabel->setProperty("secondary", true);
    d->mainLayout->addWidget(devLabel);

    d->mainLayout->addStretch();
}
