/**
 * @file AboutUSPage.cpp
 * @brief 关于页面实现
 */

#include "AboutUSPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>

#include <views/widgets/CardWidget.h>
#include <core/Tokens.h>

using namespace Tokens;

struct AboutUSPage::Impl {
    QVBoxLayout* mainLayout = nullptr;
    QLabel* m_logoLabel = nullptr;
};

AboutUSPage::AboutUSPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
}

AboutUSPage::~AboutUSPage()
{
}

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
    QLabel* nameLabel = new QLabel("WealthPilot 智能投资管理", this);
    nameLabel->setProperty("heading", true);
    nameLabel->setMargin(20);
    d->mainLayout->addWidget(nameLabel);

    // 简介
    QLabel* infoLabel = new QLabel(
        "简介: WealthPilot 是一个基于 Qt 框架开发的金融信息展示与分析软件，"
        "专为 PC 平台设计。该软件提供股票、期货等金融产品的实时数据展示、"
        "自选股管理、市场全景等功能，旨在为用户提供全面的金融市场信息。", this);
    infoLabel->setProperty("secondary", true);
    infoLabel->setWordWrap(true);
    d->mainLayout->addWidget(infoLabel);

    // 版本
    QLabel* versionLabel = new QLabel("版本: v1.0.0", this);
    versionLabel->setProperty("secondary", true);
    d->mainLayout->addWidget(versionLabel);

    // 技术栈
    QLabel* techLabel = new QLabel("技术栈: Qt 6.10.2 / C++17", this);
    techLabel->setProperty("secondary", true);
    d->mainLayout->addWidget(techLabel);

    // 开发者
    QLabel* devLabel = new QLabel("开发者: WealthPilot Team", this);
    devLabel->setProperty("secondary", true);
    d->mainLayout->addWidget(devLabel);

    d->mainLayout->addStretch();
}
