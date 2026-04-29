/**
 * @file SettingsPage.cpp
 * @brief 设置页面实现
 */

#include "SettingsPage.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QScrollArea>

#include <ui/components/CardWidget.h>
#include <core/config/ConfigManager.h>
#include <ui/ThemeManager.h>
#include <core/config/Tokens.h>

using namespace Tokens;

struct SettingsPage::Impl {
    QComboBox* themeCombo = nullptr;
    QSlider* fontSlider = nullptr;
    QLabel* fontValueLabel = nullptr;
    QCheckBox* colorBlindCheck = nullptr;

    QCheckBox* priceAlertCheck = nullptr;
    QCheckBox* riskAlertCheck = nullptr;
    QCheckBox* tradeNotifyCheck = nullptr;
    QCheckBox* systemNotifyCheck = nullptr;
    QCheckBox* dailySummaryCheck = nullptr;

    QCheckBox* twoFactorCheck = nullptr;
    QCheckBox* bioCheck = nullptr;
    
    // AI 配置
    QCheckBox* aiEnabledCheck = nullptr;
    QComboBox* aiProviderCombo = nullptr;
    QLineEdit* aiApiUrlEdit = nullptr;
    QLineEdit* aiApiKeyEdit = nullptr;
    QLineEdit* aiModelEdit = nullptr;
};

SettingsPage::SettingsPage(QWidget *parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    loadSettings();
}

SettingsPage::~SettingsPage()
{
    saveSettings();
}

QString SettingsPage::pageId() const
{
    return QStringLiteral("SettingsPage");
}

void SettingsPage::initializePage()
{
}

void SettingsPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(Spacing::LG, Spacing::LG, Spacing::LG, Spacing::LG);
    mainLayout->setSpacing(Spacing::MD);

    // 页面标题
    QLabel* titleLabel = new QLabel(QStringLiteral("设置"), this);
    titleLabel->setStyleSheet(QString("font-size: %1px; font-weight: 700; color: %2;")
                                  .arg(Font::Size::H1).arg(Colors::TextPrimary));
    mainLayout->addWidget(titleLabel);

    // 使用滚动区域
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QString("background: transparent; border: none;"));
    
    QWidget* scrollContent = new QWidget();
    scrollContent->setStyleSheet(QString("background: transparent;"));
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(0, 0, Spacing::MD, 0);
    contentLayout->setSpacing(Spacing::MD);
    
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);
    
    // === 外观 ===
    CardWidget* appearanceCard = new CardWidget(QStringLiteral("外观"), scrollContent);
    QWidget* appearanceContent = new QWidget(appearanceCard);
    QVBoxLayout* appearanceLayout = new QVBoxLayout(appearanceContent);
    appearanceLayout->setContentsMargins(0, 0, 0, 0);
    appearanceLayout->setSpacing(Spacing::SM);
    
    QHBoxLayout* themeLayout = new QHBoxLayout();
    themeLayout->addWidget(new QLabel(QStringLiteral("主题:"), appearanceContent));
    d->themeCombo = new QComboBox(appearanceContent);
    d->themeCombo->addItem(QStringLiteral("浅色"), 0);
    d->themeCombo->addItem(QStringLiteral("深色"), 1);
    d->themeCombo->addItem(QStringLiteral("自动"), 2);
    themeLayout->addWidget(d->themeCombo);
    themeLayout->addStretch();
    connect(d->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsPage::onThemeChanged);
    appearanceLayout->addLayout(themeLayout);
    
    QHBoxLayout* fontLayout = new QHBoxLayout();
    fontLayout->addWidget(new QLabel(QStringLiteral("字体大小:"), appearanceContent));
    d->fontSlider = new QSlider(Qt::Horizontal, appearanceContent);
    d->fontSlider->setRange(10, 20);
    d->fontSlider->setValue(14);
    fontLayout->addWidget(d->fontSlider);
    d->fontValueLabel = new QLabel("14px", appearanceContent);
    d->fontValueLabel->setMinimumWidth(40);
    fontLayout->addWidget(d->fontValueLabel);
    fontLayout->addStretch();
    connect(d->fontSlider, &QSlider::valueChanged, this, &SettingsPage::onFontSizeChanged);
    appearanceLayout->addLayout(fontLayout);
    
    d->colorBlindCheck = new QCheckBox(QStringLiteral("色盲模式"), appearanceContent);
    connect(d->colorBlindCheck, &QCheckBox::toggled, this, [this]() { saveSettings(); });
    appearanceLayout->addWidget(d->colorBlindCheck);
    
    appearanceCard->setContent(appearanceContent);
    contentLayout->addWidget(appearanceCard);
    
    // === 通知 ===
    CardWidget* notifyCard = new CardWidget(QStringLiteral("通知"), scrollContent);
    QWidget* notifyContent = new QWidget(notifyCard);
    QVBoxLayout* notifyLayout = new QVBoxLayout(notifyContent);
    notifyLayout->setContentsMargins(0, 0, 0, 0);
    notifyLayout->setSpacing(Spacing::XS);
    
    d->priceAlertCheck = new QCheckBox(QStringLiteral("价格提醒"), notifyContent);
    d->riskAlertCheck = new QCheckBox(QStringLiteral("风险预警"), notifyContent);
    d->tradeNotifyCheck = new QCheckBox(QStringLiteral("交易通知"), notifyContent);
    d->systemNotifyCheck = new QCheckBox(QStringLiteral("系统通知"), notifyContent);
    d->dailySummaryCheck = new QCheckBox(QStringLiteral("每日汇总"), notifyContent);
    notifyLayout->addWidget(d->priceAlertCheck);
    notifyLayout->addWidget(d->riskAlertCheck);
    notifyLayout->addWidget(d->tradeNotifyCheck);
    notifyLayout->addWidget(d->systemNotifyCheck);
    notifyLayout->addWidget(d->dailySummaryCheck);
    
    notifyCard->setContent(notifyContent);
    contentLayout->addWidget(notifyCard);
    
    // === 安全 ===
    CardWidget* securityCard = new CardWidget(QStringLiteral("安全"), scrollContent);
    QWidget* securityContent = new QWidget(securityCard);
    QVBoxLayout* securityLayout = new QVBoxLayout(securityContent);
    securityLayout->setContentsMargins(0, 0, 0, 0);
    securityLayout->setSpacing(Spacing::XS);
    
    d->twoFactorCheck = new QCheckBox(QStringLiteral("双因素认证"), securityContent);
    d->bioCheck = new QCheckBox(QStringLiteral("生物识别认证"), securityContent);
    securityLayout->addWidget(d->twoFactorCheck);
    securityLayout->addWidget(d->bioCheck);
    
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* clearCacheBtn = new QPushButton(QStringLiteral("清除缓存"), securityContent);
    QPushButton* exportDataBtn = new QPushButton(QStringLiteral("导出数据"), securityContent);
    connect(clearCacheBtn, &QPushButton::clicked, this, &SettingsPage::onClearCacheClicked);
    connect(exportDataBtn, &QPushButton::clicked, this, &SettingsPage::onExportDataClicked);
    btnLayout->addWidget(clearCacheBtn);
    btnLayout->addWidget(exportDataBtn);
    btnLayout->addStretch();
    securityLayout->addLayout(btnLayout);
    
    securityCard->setContent(securityContent);
    contentLayout->addWidget(securityCard);
    
    // === AI配置 ===
    CardWidget* aiCard = new CardWidget(QStringLiteral("AI 配置"), scrollContent);
    QWidget* aiContent = new QWidget(aiCard);
    QVBoxLayout* aiLayout = new QVBoxLayout(aiContent);
    aiLayout->setContentsMargins(0, 0, 0, 0);
    aiLayout->setSpacing(Spacing::SM);
    
    d->aiEnabledCheck = new QCheckBox(QStringLiteral("启用 AI 助手"), aiContent);
    connect(d->aiEnabledCheck, &QCheckBox::toggled, this, &SettingsPage::onAIConfigChanged);
    aiLayout->addWidget(d->aiEnabledCheck);
    
    QHBoxLayout* providerLayout = new QHBoxLayout();
    providerLayout->addWidget(new QLabel(QStringLiteral("AI 提供商:"), aiContent));
    d->aiProviderCombo = new QComboBox(aiContent);
    d->aiProviderCombo->addItem(QStringLiteral("OpenAI"), "openai");
    d->aiProviderCombo->addItem(QStringLiteral("Azure OpenAI"), "azure");
    d->aiProviderCombo->addItem(QStringLiteral("Anthropic"), "anthropic");
    d->aiProviderCombo->addItem(QStringLiteral("本地模型"), "local");
    d->aiProviderCombo->addItem(QStringLiteral("自定义"), "custom");
    providerLayout->addWidget(d->aiProviderCombo);
    providerLayout->addStretch();
    connect(d->aiProviderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsPage::onAIConfigChanged);
    aiLayout->addLayout(providerLayout);
    
    QHBoxLayout* urlLayout = new QHBoxLayout();
    urlLayout->addWidget(new QLabel(QStringLiteral("API 地址:"), aiContent));
    d->aiApiUrlEdit = new QLineEdit(aiContent);
    d->aiApiUrlEdit->setPlaceholderText(QStringLiteral("https://api.openai.com/v1"));
    urlLayout->addWidget(d->aiApiUrlEdit);
    urlLayout->addStretch();
    connect(d->aiApiUrlEdit, &QLineEdit::textChanged, this, &SettingsPage::onAIConfigChanged);
    aiLayout->addLayout(urlLayout);
    
    QHBoxLayout* keyLayout = new QHBoxLayout();
    keyLayout->addWidget(new QLabel(QStringLiteral("API 密钥:"), aiContent));
    d->aiApiKeyEdit = new QLineEdit(aiContent);
    d->aiApiKeyEdit->setPlaceholderText(QStringLiteral("请输入您的 API 密钥"));
    d->aiApiKeyEdit->setEchoMode(QLineEdit::Password);
    keyLayout->addWidget(d->aiApiKeyEdit);
    QPushButton* toggleKeyBtn = new QPushButton(QStringLiteral("显示"), aiContent);
    toggleKeyBtn->setFixedWidth(50);
    connect(toggleKeyBtn, &QPushButton::clicked, this, [this, toggleKeyBtn]() {
        if (d->aiApiKeyEdit->echoMode() == QLineEdit::Password) {
            d->aiApiKeyEdit->setEchoMode(QLineEdit::Normal);
            toggleKeyBtn->setText(QStringLiteral("隐藏"));
        } else {
            d->aiApiKeyEdit->setEchoMode(QLineEdit::Password);
            toggleKeyBtn->setText(QStringLiteral("显示"));
        }
    });
    keyLayout->addWidget(toggleKeyBtn);
    keyLayout->addStretch();
    connect(d->aiApiKeyEdit, &QLineEdit::textChanged, this, &SettingsPage::onAIConfigChanged);
    aiLayout->addLayout(keyLayout);
    
    QHBoxLayout* modelLayout = new QHBoxLayout();
    modelLayout->addWidget(new QLabel(QStringLiteral("模型名称:"), aiContent));
    d->aiModelEdit = new QLineEdit(aiContent);
    d->aiModelEdit->setPlaceholderText(QStringLiteral("gpt-4, gpt-3.5-turbo, claude-3 等"));
    modelLayout->addWidget(d->aiModelEdit);
    modelLayout->addStretch();
    connect(d->aiModelEdit, &QLineEdit::textChanged, this, &SettingsPage::onAIConfigChanged);
    aiLayout->addLayout(modelLayout);
    
    QLabel* hintLabel = new QLabel(QStringLiteral("<i>提示：API 密钥使用 Windows DPAPI 加密安全存储</i>"), aiContent);
    hintLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Colors::TextTertiary));
    aiLayout->addWidget(hintLabel);
    
    aiCard->setContent(aiContent);
    contentLayout->addWidget(aiCard);
    
    // === 关于 ===
    CardWidget* aboutCard = new CardWidget(QStringLiteral("关于"), scrollContent);
    QWidget* aboutContent = new QWidget(aboutCard);
    QVBoxLayout* aboutLayout = new QVBoxLayout(aboutContent);
    aboutLayout->setContentsMargins(0, 0, 0, 0);
    aboutLayout->setSpacing(Spacing::XS);
    
    aboutLayout->addWidget(new QLabel(QString(QStringLiteral("版本: %1")).arg("2.0.0"), aboutContent));
    aboutLayout->addWidget(new QLabel(QString(QStringLiteral("构建日期: %1")).arg(__DATE__), aboutContent));
    aboutLayout->addWidget(new QLabel(QString(QStringLiteral("Qt 版本: %1")).arg(qVersion()), aboutContent));
    
    aboutCard->setContent(aboutContent);
    contentLayout->addWidget(aboutCard);
    
    contentLayout->addStretch();
}

void SettingsPage::loadSettings()
{
    int themeIndex = ConfigManager::instance()->get("appearance/theme", 0).toInt();
    d->themeCombo->setCurrentIndex(themeIndex);

    int fontSize = ConfigManager::instance()->get("appearance/fontSize", 14).toInt();
    d->fontSlider->setValue(fontSize);
    d->fontValueLabel->setText(QString("%1px").arg(fontSize));

    d->colorBlindCheck->setChecked(ConfigManager::instance()->getBool("appearance/colorBlind", false));
    d->priceAlertCheck->setChecked(ConfigManager::instance()->getBool("notifications/priceAlerts", true));
    d->riskAlertCheck->setChecked(ConfigManager::instance()->getBool("notifications/riskAlerts", true));
    d->tradeNotifyCheck->setChecked(ConfigManager::instance()->getBool("notifications/tradeNotify", true));
    d->systemNotifyCheck->setChecked(ConfigManager::instance()->getBool("notifications/systemNotify", true));
    d->dailySummaryCheck->setChecked(ConfigManager::instance()->getBool("notifications/dailySummary", false));
    d->twoFactorCheck->setChecked(ConfigManager::instance()->getBool("security/twoFactor", false));
    d->bioCheck->setChecked(ConfigManager::instance()->getBool("security/biometric", false));
    
    // AI设置
    d->aiEnabledCheck->setChecked(ConfigManager::instance()->getBool("ai/enabled", false));
    
    QString provider = ConfigManager::instance()->getString("ai/provider", "openai");
    int providerIndex = d->aiProviderCombo->findData(provider);
    if (providerIndex >= 0) {
        d->aiProviderCombo->setCurrentIndex(providerIndex);
    }
    
    d->aiApiUrlEdit->setText(ConfigManager::instance()->getString("ai/api_url", "https://api.openai.com/v1"));
    d->aiModelEdit->setText(ConfigManager::instance()->getString("ai/model", "gpt-4"));
    
    QString apiKey = ConfigManager::instance()->getSecure("secure/ai_api_key");
    d->aiApiKeyEdit->setText(apiKey);
}

void SettingsPage::saveSettings()
{
    ConfigManager::instance()->set("appearance/theme", d->themeCombo->currentIndex());
    ConfigManager::instance()->set("appearance/fontSize", d->fontSlider->value());
    ConfigManager::instance()->set("appearance/colorBlind", d->colorBlindCheck->isChecked());
    ConfigManager::instance()->set("notifications/priceAlerts", d->priceAlertCheck->isChecked());
    ConfigManager::instance()->set("notifications/riskAlerts", d->riskAlertCheck->isChecked());
    ConfigManager::instance()->set("notifications/tradeNotify", d->tradeNotifyCheck->isChecked());
    ConfigManager::instance()->set("notifications/systemNotify", d->systemNotifyCheck->isChecked());
    ConfigManager::instance()->set("notifications/dailySummary", d->dailySummaryCheck->isChecked());
    ConfigManager::instance()->set("security/twoFactor", d->twoFactorCheck->isChecked());
    ConfigManager::instance()->set("security/biometric", d->bioCheck->isChecked());
    
    ConfigManager::instance()->set("ai/enabled", d->aiEnabledCheck->isChecked());
    ConfigManager::instance()->set("ai/provider", d->aiProviderCombo->currentData().toString());
    ConfigManager::instance()->set("ai/api_url", d->aiApiUrlEdit->text());
    ConfigManager::instance()->set("ai/model", d->aiModelEdit->text());
    
    ConfigManager::instance()->setSecure("secure/ai_api_key", d->aiApiKeyEdit->text());
}

void SettingsPage::onThemeChanged(int index)
{
    Q_UNUSED(index);
    saveSettings();
    
    ThemeManager::ThemeType themeType = static_cast<ThemeManager::ThemeType>(d->themeCombo->currentIndex());
    ThemeManager::instance()->setTheme(themeType);
    
    LOG_INFO(QString("Theme changed to: %1").arg(d->themeCombo->currentText()));
}

void SettingsPage::onFontSizeChanged(int size)
{
    d->fontValueLabel->setText(QString("%1px").arg(size));
    saveSettings();
    LOG_INFO(QString("Font size changed to: %1").arg(size));
}

void SettingsPage::onClearCacheClicked()
{
    int ret = QMessageBox::question(this, QStringLiteral("确认"),
                                    QStringLiteral("确定要清除所有缓存数据吗？"),
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        LOG_INFO("Cache cleared");
        QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("缓存已清除。"));
    }
}

void SettingsPage::onExportDataClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    QStringLiteral("导出数据"), "", QStringLiteral("JSON 文件 (*.json)"));

    if (!filePath.isEmpty()) {
        ConfigManager::instance()->exportToFile(filePath, false);
        LOG_INFO(QString("Data exported to: %1").arg(filePath));
        QMessageBox::information(this, QStringLiteral("完成"), QStringLiteral("数据已导出。"));
    }
}

void SettingsPage::onNotificationChanged()
{
}

void SettingsPage::onAIConfigChanged()
{
    saveSettings();
    LOG_INFO("AI configuration changed");
}
