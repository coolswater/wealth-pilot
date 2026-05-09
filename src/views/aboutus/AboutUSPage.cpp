/**
 * @file AboutUSPage.cpp
 * @brief 关于页面实现 - 参考设置页面样式优化
 */

#include "AboutUSPage.h"
#include "ui/components/StyleHelper.h"
#include "ui/ThemeManager.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>

using namespace Tokens;

struct AboutUSPage::Impl {
    QLabel* logoLabel = nullptr;
    QLabel* appNameLabel = nullptr;
    QLabel* sloganLabel = nullptr;
    
    QPushButton* checkUpdateBtn = nullptr;
    QPushButton* visitWebsiteBtn = nullptr;
    QPushButton* viewLicenseBtn = nullptr;
};

AboutUSPage::AboutUSPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setObjectName("AboutUSPage");
    setupUI();
}

AboutUSPage::~AboutUSPage() = default;

QString AboutUSPage::pageId() const
{
    return QStringLiteral("AboutUS");
}

void AboutUSPage::initializePage()
{
}

void AboutUSPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(Spacing::MD);
    mainLayout->setContentsMargins(Spacing::LG, Spacing::MD, Spacing::LG, Spacing::MD);

    // ========== 页面标题栏 ==========
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QLabel* titleLabel = new QLabel(QStringLiteral("关于"), this);
    StyleHelper::setTitleLabel(titleLabel);
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);
    
    // ========== 应用信息卡片 ==========
    QWidget* appInfoWidget = new QWidget(this);
    appInfoWidget->setProperty("cardType", "transparent");

    QHBoxLayout* appInfoLayout = new QHBoxLayout(appInfoWidget);
    appInfoLayout->setContentsMargins(0, Spacing::MD, 0, Spacing::MD);
    appInfoLayout->setSpacing(Spacing::LG);

    // Logo
    d->logoLabel = new QLabel(this);
    d->logoLabel->setFixedSize(80, 80);
    d->logoLabel->setText(QStringLiteral("WP"));
    d->logoLabel->setAlignment(Qt::AlignCenter);
    d->logoLabel->setObjectName("appLogo");
    appInfoLayout->addWidget(d->logoLabel);
    
    // 应用名称和标语
    QVBoxLayout* nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(Spacing::XS);

    d->appNameLabel = new QLabel(QStringLiteral("WealthPilot"), this);
    d->appNameLabel->setObjectName("appName");
    d->appNameLabel->setProperty("dataType", "title");
    nameLayout->addWidget(d->appNameLabel);
    
    d->sloganLabel = new QLabel(QStringLiteral("智能金融信息分析平台"), this);
    d->sloganLabel->setProperty("dataType", "slogan");
    nameLayout->addWidget(d->sloganLabel);
    
    appInfoLayout->addLayout(nameLayout);
    appInfoLayout->addStretch();
    
    mainLayout->addWidget(appInfoWidget);
    
    // ========== 版本信息 ==========
    QGroupBox* versionGroup = new QGroupBox(QStringLiteral("版本信息"), this);
    // 全局样式自动生效
    QFormLayout* versionLayout = new QFormLayout(versionGroup);
    versionLayout->setSpacing(Spacing::SM);

    QLabel* versionValue = new QLabel(QStringLiteral("v2.0.0"), this);
    StyleHelper::setValueLabel(versionValue);
    versionLayout->addRow(QStringLiteral("当前版本:"), versionValue);
    
    QLabel* buildDateValue = new QLabel(QString(__DATE__) + " " + QString(__TIME__), this);
    StyleHelper::setLabelText(buildDateValue);
    versionLayout->addRow(QStringLiteral("构建日期:"), buildDateValue);
    
    QLabel* qtVersionValue = new QLabel(QString(qVersion()), this);
    StyleHelper::setLabelText(qtVersionValue);
    versionLayout->addRow(QStringLiteral("Qt 版本:"), qtVersionValue);
    
    QLabel* compilerValue = new QLabel(QStringLiteral("MinGW 13.1.0 (GCC)"), this);
    StyleHelper::setLabelText(compilerValue);
    versionLayout->addRow(QStringLiteral("编译器:"), compilerValue);
    
    QLabel* archValue = new QLabel(QStringLiteral("x86_64 (64-bit)"), this);
    StyleHelper::setLabelText(archValue);
    versionLayout->addRow(QStringLiteral("架构:"), archValue);
    
    mainLayout->addWidget(versionGroup);
    
    // ========== 产品介绍 ==========
    QGroupBox* introGroup = new QGroupBox(QStringLiteral("产品介绍"), this);
    // 全局样式自动生效
    QVBoxLayout* introLayout = new QVBoxLayout(introGroup);
    introLayout->setSpacing(Spacing::SM);

    QLabel* introLabel = new QLabel(
        QStringLiteral("WealthPilot 是一款专为 PC 用户设计的金融信息分析平台。"
        "基于 Qt 6 框架开发，提供实时股票期货行情追踪、自选股管理、"
        "市场全景分析、基金行情、AI 智能分析等功能，助您把握投资先机。"),
        this);
    introLabel->setWordWrap(true);
    introLabel->setProperty("dataType", "description");
    introLayout->addWidget(introLabel);
    
    // 功能特性
    QLabel* featuresTitle = new QLabel(QStringLiteral("主要功能："), this);
    featuresTitle->setProperty("dataType", "subtitle");
    introLayout->addWidget(featuresTitle);
    
    QStringList features = {
        QStringLiteral("• 实时行情：股票、期货、外汇、基金、数字货币"),
        QStringLiteral("• 自选管理：智能分组、快速搜索、实时监控"),
        QStringLiteral("• 市场全景：指数概览、板块热度、资金流向"),
        QStringLiteral("• K线分析：多周期图表、技术指标、分时图"),
        QStringLiteral("• 数据分析：智能选股、风险评估、投资建议"),
        QStringLiteral("• AI 助手：智能问答、投资分析、风险提示")
    };
    
    for (const QString& feature : features) {
        QLabel* featureLabel = new QLabel(feature, this);
        featureLabel->setProperty("dataType", "feature");
        introLayout->addWidget(featureLabel);
    }
    
    mainLayout->addWidget(introGroup);
    
    // ========== 开发团队 ==========
    QGroupBox* teamGroup = new QGroupBox(QStringLiteral("开发团队"), this);
    QFormLayout* teamLayout = new QFormLayout(teamGroup);
    teamLayout->setSpacing(Spacing::SM);

    QLabel* teamName = new QLabel(QStringLiteral("WealthPilot Team"), this);
    StyleHelper::setValueLabel(teamName);
    teamLayout->addRow(QStringLiteral("团队:"), teamName);
    
    QLabel* copyrightLabel = new QLabel(QStringLiteral("© 2024-2026 WealthPilot Team. All rights reserved."), this);
    StyleHelper::setLabelText(copyrightLabel);
    teamLayout->addRow(QStringLiteral("版权:"), copyrightLabel);
    
    QLabel* websiteLabel = new QLabel(QStringLiteral("https://github.com/openclaw/wealth-pilot"), this);
    websiteLabel->setObjectName("websiteLink");
    websiteLabel->setCursor(Qt::PointingHandCursor);
    connect(websiteLabel, &QLabel::linkActivated, this, [](const QString& link) {
        QDesktopServices::openUrl(QUrl(link));
    });
    teamLayout->addRow(QStringLiteral("官网:"), websiteLabel);
    
    mainLayout->addWidget(teamGroup);
    
    // ========== 技术支持 ==========
    QGroupBox* supportGroup = new QGroupBox(QStringLiteral("技术支持"), this);
    // 全局样式自动生效
    QVBoxLayout* supportLayout = new QVBoxLayout(supportGroup);
    supportLayout->setSpacing(Spacing::SM);

    // 按钮行
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(Spacing::SM);

    d->checkUpdateBtn = new QPushButton(QStringLiteral("检查更新"), this);
    StyleHelper::setPrimaryButton(d->checkUpdateBtn);
    d->checkUpdateBtn->setFixedWidth(100);
    connect(d->checkUpdateBtn, &QPushButton::clicked, this, &AboutUSPage::onCheckUpdateClicked);
    btnLayout->addWidget(d->checkUpdateBtn);
    
    d->visitWebsiteBtn = new QPushButton(QStringLiteral("访问官网"), this);
    StyleHelper::setSecondaryButton(d->visitWebsiteBtn);
    d->visitWebsiteBtn->setFixedWidth(100);
    connect(d->visitWebsiteBtn, &QPushButton::clicked, this, &AboutUSPage::onVisitWebsiteClicked);
    btnLayout->addWidget(d->visitWebsiteBtn);
    
    d->viewLicenseBtn = new QPushButton(QStringLiteral("查看许可"), this);
    StyleHelper::setSecondaryButton(d->viewLicenseBtn);
    d->viewLicenseBtn->setFixedWidth(100);
    connect(d->viewLicenseBtn, &QPushButton::clicked, this, &AboutUSPage::onViewLicenseClicked);
    btnLayout->addWidget(d->viewLicenseBtn);
    
    btnLayout->addStretch();
    supportLayout->addLayout(btnLayout);
    
    mainLayout->addWidget(supportGroup);
    
    mainLayout->addStretch();
}

void AboutUSPage::onCheckUpdateClicked()
{
    LOG_INFO("Checking for updates...");
    
    // 模拟检查更新
    QMessageBox::information(this, QStringLiteral("检查更新"),
                             QStringLiteral("当前已是最新版本 v2.0.0\n\n"
        "如有新版本发布，请访问官网下载。"));
}

void AboutUSPage::onVisitWebsiteClicked()
{
    QString url = QStringLiteral("https://github.com/openclaw/wealth-pilot");
    QDesktopServices::openUrl(QUrl(url));
    LOG_INFO(QString("Opening website: %1").arg(url));
}

void AboutUSPage::onViewLicenseClicked()
{
    QMessageBox::about(this, QStringLiteral("开源许可"), 
        QStringLiteral("<h3>开源许可协议</h3>"
        "<p>WealthPilot 使用以下开源项目：</p>"
        "<ul>"
        "<li><b>Qt Framework</b> - LGPL v3</li>"
        "<li><b>SQLite</b> - Public Domain</li>"
        "<li><b>OpenSSL</b> - Apache 2.0</li>"
        "</ul>"
        "<p>本项目采用 MIT 许可证开源。</p>"));
}
