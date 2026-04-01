/**
 * @file AboutUSPage.cpp
 * @brief 设置页面实现
 */

#include "AboutUSPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>

#include <views/widgets/CardWidget.h>
#include <core/ConfigManager.h>
#include <core/ThemeManager.h>
#include <core/Tokens.h>

using namespace Tokens;

struct AboutUSPage::Impl {
    // 外观设置
    QComboBox* themeCombo = nullptr;
    QSlider* fontSlider = nullptr;
    QLabel* fontValueLabel = nullptr;
    QCheckBox* colorBlindCheck = nullptr;

    // 通知设置
    QCheckBox* priceAlertCheck = nullptr;
    QCheckBox* riskAlertCheck = nullptr;
    QCheckBox* tradeNotifyCheck = nullptr;
    QCheckBox* systemNotifyCheck = nullptr;
    QCheckBox* dailySummaryCheck = nullptr;

    // 安全设置
    QCheckBox* twoFactorCheck = nullptr;
    QCheckBox* bioCheck = nullptr;
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
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget("关于", this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    QString labelStyle = QString("color: %1; font-size: %2px;")
                             .arg(Colors::TextSecondary).arg(Font::Size::Small);

    QLabel* nameLabel = new QLabel("WealthPilot 智能投资管理", content);
    nameLabel->setStyleSheet(QString("color: %1; font-size: %2px; font-weight: 600;")
                                 .arg(Colors::TextPrimary).arg(Font::Size::Body));
    layout->addWidget(nameLabel);

    QLabel* versionLabel = new QLabel("版本: 1.1.0", content);
    versionLabel->setStyleSheet(labelStyle);
    layout->addWidget(versionLabel);

    QLabel* techLabel = new QLabel("技术栈: Qt 6.10.2 / C++17", content);
    techLabel->setStyleSheet(labelStyle);
    layout->addWidget(techLabel);

    QLabel* devLabel = new QLabel("开发者: WealthPilot Team", content);
    devLabel->setStyleSheet(labelStyle);
    layout->addWidget(devLabel);

    // 导出数据按钮
    QHBoxLayout* btnRow = new QHBoxLayout();
    QPushButton* exportBtn = new QPushButton("导出数据", content);
    exportBtn->setFixedHeight(Size::ButtonHeightMD);
    exportBtn->setProperty("secondary", true);
    // connect(exportBtn, &QPushButton::clicked, this, &AboutUSPage::onExportDataClicked);
    btnRow->addWidget(exportBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    card->setContent(content);
    mainLayout->addWidget(card);
}
