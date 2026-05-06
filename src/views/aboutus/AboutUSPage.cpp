/**
 * @file AboutUSPage.cpp
 * @brief 关于页面实现
 */

#include "AboutUSPage.h"
#include "core/config/Tokens.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QMessageBox>

using namespace Tokens;

struct AboutUSPage::Impl {
    QLabel* logoLabel = nullptr;
    QLabel* titleLabel = nullptr;
    QLabel* versionLabel = nullptr;
    QLabel* infoLabel = nullptr;
    QLabel* devLabel = nullptr;
    QLabel* qtLabel = nullptr;
    QPushButton* checkUpdateBtn = nullptr;
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
    return "AboutUS";
}

void AboutUSPage::initializePage()
{
}

void AboutUSPage::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(Spacing::MD);
    mainLayout->setContentsMargins(Spacing::XL, Spacing::XL, Spacing::XL, Spacing::XL);

    // Logo
    d->logoLabel = new QLabel(this);
    d->logoLabel->setFixedSize(100, 100);
    d->logoLabel->setText(QStringLiteral("WP"));
    d->logoLabel->setAlignment(Qt::AlignCenter);
    d->logoLabel->setStyleSheet(QString(
        "QLabel {"
        "  font-size: 40px; font-weight: bold; color: white;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "    stop:0 %1, stop:2 %2);"
        "  border-radius: 20px;"
        "}")
        .arg(Colors::Primary)
        .arg(Colors::PrimaryDark));
    mainLayout->addWidget(d->logoLabel, 0, Qt::AlignLeft);

    // 标题
    d->titleLabel = new QLabel(QStringLiteral("WealthPilot"), this);
    d->titleLabel->setStyleSheet(QString("font-size: 28px; font-weight: bold; color: %1;")
        .arg(Colors::TextPrimary));
    mainLayout->addWidget(d->titleLabel, 0, Qt::AlignLeft);

    // 版本
    d->versionLabel = new QLabel(QStringLiteral("版本 2.0.0"), this);
    d->versionLabel->setStyleSheet(QString("font-size: 14px; color: %1;")
        .arg(Colors::TextSecondary));
    mainLayout->addWidget(d->versionLabel, 0, Qt::AlignLeft);

    // 简介
    d->infoLabel = new QLabel(
        QStringLiteral("WealthPilot 是一款专为 PC 用户设计的金融信息分析平台。"
        "基于 Qt 框架开发，提供实时股票期货行情追踪、自选股管理、"
        "市场全景分析等功能，助您把握投资先机。"),
        this);
    d->infoLabel->setWordWrap(true);
    d->infoLabel->setAlignment(Qt::AlignLeft);
    d->infoLabel->setMaximumWidth(500);
    d->infoLabel->setStyleSheet(QString("font-size: 13px; color: %1; line-height: 1.6;")
        .arg(Colors::TextSecondary));
    mainLayout->addWidget(d->infoLabel, 0, Qt::AlignLeft);

    // Qt版本
    d->qtLabel = new QLabel(QString(QStringLiteral("基于 Qt %1 构建")).arg(qVersion()), this);
    d->qtLabel->setStyleSheet(QString("font-size: 12px; color: %1;")
        .arg(Colors::TextTertiary));
    mainLayout->addWidget(d->qtLabel, 0, Qt::AlignLeft);

    // 开发团队
    d->devLabel = new QLabel(QStringLiteral("WealthPilot 团队出品"), this);
    d->devLabel->setStyleSheet(QString("font-size: 12px; color: %1;")
        .arg(Colors::TextTertiary));
    mainLayout->addWidget(d->devLabel, 0, Qt::AlignLeft);

    // 检查更新按钮
    d->checkUpdateBtn = new QPushButton(QStringLiteral("检查更新"), this);
    d->checkUpdateBtn->setFixedWidth(120);
    d->checkUpdateBtn->setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: none;"
        "  border-radius: %3px;"
        "  padding: 8px 16px;"
        "  font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "  background-color: %4;"
        "}")
        .arg(Colors::Primary)
        .arg(Colors::TextPrimary)
        .arg(Radius::SM)
        .arg(Colors::PrimaryHover));
    QObject::connect(d->checkUpdateBtn, &QPushButton::clicked, this, [this]() {
        // 检查更新（模拟）
        QMessageBox::information(this, QStringLiteral("检查更新"), 
            QStringLiteral("当前已是最新版本 v1.0.0\n\n如有新版本发布，请访问官网下载。"));
    });
    mainLayout->addSpacing(Spacing::MD);
    mainLayout->addWidget(d->checkUpdateBtn, 0, Qt::AlignLeft);

    mainLayout->addStretch();
}
