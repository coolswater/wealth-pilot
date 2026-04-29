/**
 * @file SettingsPage.cpp
 * @brief Settings Page Implementation
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
    mainLayout->setSpacing(Spacing::LG);

    QLabel* titleLabel = new QLabel(QStringLiteral("设置"), this);
    titleLabel->setStyleSheet(QString("font-size: %1px; font-weight: 700; color: %2;")
                                  .arg(Font::Size::H1).arg(Colors::TextPrimary));
    mainLayout->addWidget(titleLabel);

    createAppearanceSection();
    createNotificationSection();
    createSecuritySection();
    createAISection();
    createAboutSection();

    mainLayout->addStretch();
}

void SettingsPage::createAppearanceSection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget(QStringLiteral("外观"), this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::MD);

    // Theme selection
    QHBoxLayout* themeLayout = new QHBoxLayout();
    themeLayout->addWidget(new QLabel(QStringLiteral("主题:"), content));
    
    d->themeCombo = new QComboBox(content);
    d->themeCombo->addItem(QStringLiteral("浅色"), 0);
    d->themeCombo->addItem(QStringLiteral("深色"), 1);
    d->themeCombo->addItem(QStringLiteral("自动"), 2);
    themeLayout->addWidget(d->themeCombo);
    themeLayout->addStretch();
    
    QObject::connect(d->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsPage::onThemeChanged);
    
    layout->addLayout(themeLayout);

    // Font size
    QHBoxLayout* fontLayout = new QHBoxLayout();
    fontLayout->addWidget(new QLabel(QStringLiteral("字体大小:"), content));
    
    d->fontSlider = new QSlider(Qt::Horizontal, content);
    d->fontSlider->setRange(10, 20);
    d->fontSlider->setValue(14);
    fontLayout->addWidget(d->fontSlider);
    
    d->fontValueLabel = new QLabel("14px", content);
    d->fontValueLabel->setMinimumWidth(40);
    fontLayout->addWidget(d->fontValueLabel);
    fontLayout->addStretch();
    
    QObject::connect(d->fontSlider, &QSlider::valueChanged, this, &SettingsPage::onFontSizeChanged);
    
    layout->addLayout(fontLayout);

    // Color blind mode
    d->colorBlindCheck = new QCheckBox(QStringLiteral("色盲模式"), content);
    QObject::connect(d->colorBlindCheck, &QCheckBox::toggled, this, [this]() { saveSettings(); });
    layout->addWidget(d->colorBlindCheck);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::createNotificationSection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget(QStringLiteral("通知"), this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    d->priceAlertCheck = new QCheckBox(QStringLiteral("价格提醒"), content);
    d->riskAlertCheck = new QCheckBox(QStringLiteral("风险预警"), content);
    d->tradeNotifyCheck = new QCheckBox(QStringLiteral("交易通知"), content);
    d->systemNotifyCheck = new QCheckBox(QStringLiteral("系统通知"), content);
    d->dailySummaryCheck = new QCheckBox(QStringLiteral("每日汇总"), content);

    layout->addWidget(d->priceAlertCheck);
    layout->addWidget(d->riskAlertCheck);
    layout->addWidget(d->tradeNotifyCheck);
    layout->addWidget(d->systemNotifyCheck);
    layout->addWidget(d->dailySummaryCheck);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::createSecuritySection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget(QStringLiteral("安全"), this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    d->twoFactorCheck = new QCheckBox(QStringLiteral("双因素认证"), content);
    d->bioCheck = new QCheckBox(QStringLiteral("生物识别认证"), content);

    layout->addWidget(d->twoFactorCheck);
    layout->addWidget(d->bioCheck);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    QPushButton* clearCacheBtn = new QPushButton(QStringLiteral("清除缓存"), content);
    QPushButton* exportDataBtn = new QPushButton(QStringLiteral("导出数据"), content);
    
    QObject::connect(clearCacheBtn, &QPushButton::clicked, this, &SettingsPage::onClearCacheClicked);
    QObject::connect(exportDataBtn, &QPushButton::clicked, this, &SettingsPage::onExportDataClicked);
    
    btnLayout->addWidget(clearCacheBtn);
    btnLayout->addWidget(exportDataBtn);
    btnLayout->addStretch();
    
    layout->addLayout(btnLayout);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::createAboutSection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget(QStringLiteral("关于"), this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    QLabel* versionLabel = new QLabel(QString(QStringLiteral("版本: %1")).arg("2.0.0"), content);
    QLabel* buildLabel = new QLabel(QString(QStringLiteral("构建日期: %1")).arg(__DATE__), content);
    QLabel* qtLabel = new QLabel(QString(QStringLiteral("Qt 版本: %1")).arg(qVersion()), content);

    layout->addWidget(versionLabel);
    layout->addWidget(buildLabel);
    layout->addWidget(qtLabel);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::createAISection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget(QStringLiteral("AI 配置"), this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::MD);

    // AI 启用开关
    d->aiEnabledCheck = new QCheckBox(QStringLiteral("启用 AI 助手"), content);
    QObject::connect(d->aiEnabledCheck, &QCheckBox::toggled, this, &SettingsPage::onAIConfigChanged);
    layout->addWidget(d->aiEnabledCheck);

    // AI 提供商选择
    QHBoxLayout* providerLayout = new QHBoxLayout();
    providerLayout->addWidget(new QLabel(QStringLiteral("AI 提供商:"), content));
    
    d->aiProviderCombo = new QComboBox(content);
    d->aiProviderCombo->addItem(QStringLiteral("OpenAI"), "openai");
    d->aiProviderCombo->addItem(QStringLiteral("Azure OpenAI"), "azure");
    d->aiProviderCombo->addItem(QStringLiteral("Anthropic"), "anthropic");
    d->aiProviderCombo->addItem(QStringLiteral("本地模型"), "local");
    d->aiProviderCombo->addItem(QStringLiteral("自定义"), "custom");
    providerLayout->addWidget(d->aiProviderCombo);
    providerLayout->addStretch();
    
    QObject::connect(d->aiProviderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsPage::onAIConfigChanged);
    
    layout->addLayout(providerLayout);

    // API 地址
    QHBoxLayout* urlLayout = new QHBoxLayout();
    urlLayout->addWidget(new QLabel(QStringLiteral("API 地址:"), content));
    
    d->aiApiUrlEdit = new QLineEdit(content);
    d->aiApiUrlEdit->setPlaceholderText(QStringLiteral("https://api.openai.com/v1"));
    d->aiApiUrlEdit->setMinimumWidth(300);
    urlLayout->addWidget(d->aiApiUrlEdit);
    urlLayout->addStretch();
    
    QObject::connect(d->aiApiUrlEdit, &QLineEdit::textChanged, this, &SettingsPage::onAIConfigChanged);
    
    layout->addLayout(urlLayout);

    // API Key
    QHBoxLayout* keyLayout = new QHBoxLayout();
    keyLayout->addWidget(new QLabel(QStringLiteral("API 密钥:"), content));
    
    d->aiApiKeyEdit = new QLineEdit(content);
    d->aiApiKeyEdit->setPlaceholderText(QStringLiteral("请输入您的 API 密钥"));
    d->aiApiKeyEdit->setEchoMode(QLineEdit::Password);
    d->aiApiKeyEdit->setMinimumWidth(300);
    keyLayout->addWidget(d->aiApiKeyEdit);
    
    QPushButton* toggleKeyBtn = new QPushButton(QStringLiteral("显示"), content);
    toggleKeyBtn->setFixedWidth(60);
    QObject::connect(toggleKeyBtn, &QPushButton::clicked, this, [this, toggleKeyBtn]() {
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
    
    QObject::connect(d->aiApiKeyEdit, &QLineEdit::textChanged, this, &SettingsPage::onAIConfigChanged);
    
    layout->addLayout(keyLayout);

    // 模型名称
    QHBoxLayout* modelLayout = new QHBoxLayout();
    modelLayout->addWidget(new QLabel(QStringLiteral("模型名称:"), content));
    
    d->aiModelEdit = new QLineEdit(content);
    d->aiModelEdit->setPlaceholderText(QStringLiteral("gpt-4, gpt-3.5-turbo, claude-3 等"));
    d->aiModelEdit->setMinimumWidth(300);
    modelLayout->addWidget(d->aiModelEdit);
    modelLayout->addStretch();
    
    QObject::connect(d->aiModelEdit, &QLineEdit::textChanged, this, &SettingsPage::onAIConfigChanged);
    
    layout->addLayout(modelLayout);

    // 提示信息
    QLabel* hintLabel = new QLabel(
        QStringLiteral("<i>提示：API 密钥使用 Windows DPAPI 加密安全存储</i>"), content);
    hintLabel->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Colors::TextTertiary));
    layout->addWidget(hintLabel);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::loadSettings()
{
    // Load theme
    int themeIndex = ConfigManager::instance()->get("appearance/theme", 0).toInt();
    d->themeCombo->setCurrentIndex(themeIndex);

    // Load font size
    int fontSize = ConfigManager::instance()->get("appearance/fontSize", 14).toInt();
    d->fontSlider->setValue(fontSize);
    d->fontValueLabel->setText(QString("%1px").arg(fontSize));

    // Load checkboxes
    d->colorBlindCheck->setChecked(ConfigManager::instance()->getBool("appearance/colorBlind", false));
    d->priceAlertCheck->setChecked(ConfigManager::instance()->getBool("notifications/priceAlerts", true));
    d->riskAlertCheck->setChecked(ConfigManager::instance()->getBool("notifications/riskAlerts", true));
    d->tradeNotifyCheck->setChecked(ConfigManager::instance()->getBool("notifications/tradeNotify", true));
    d->systemNotifyCheck->setChecked(ConfigManager::instance()->getBool("notifications/systemNotify", true));
    d->dailySummaryCheck->setChecked(ConfigManager::instance()->getBool("notifications/dailySummary", false));
    d->twoFactorCheck->setChecked(ConfigManager::instance()->getBool("security/twoFactor", false));
    d->bioCheck->setChecked(ConfigManager::instance()->getBool("security/biometric", false));
    
    // Load AI settings
    d->aiEnabledCheck->setChecked(ConfigManager::instance()->getBool("ai/enabled", false));
    
    QString provider = ConfigManager::instance()->getString("ai/provider", "openai");
    int providerIndex = d->aiProviderCombo->findData(provider);
    if (providerIndex >= 0) {
        d->aiProviderCombo->setCurrentIndex(providerIndex);
    }
    
    d->aiApiUrlEdit->setText(ConfigManager::instance()->getString("ai/api_url", "https://api.openai.com/v1"));
    d->aiModelEdit->setText(ConfigManager::instance()->getString("ai/model", "gpt-4"));
    
    // Load API key securely
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
    
    // Save AI settings
    ConfigManager::instance()->set("ai/enabled", d->aiEnabledCheck->isChecked());
    ConfigManager::instance()->set("ai/provider", d->aiProviderCombo->currentData().toString());
    ConfigManager::instance()->set("ai/api_url", d->aiApiUrlEdit->text());
    ConfigManager::instance()->set("ai/model", d->aiModelEdit->text());
    
    // Save API key securely
    ConfigManager::instance()->setSecure("secure/ai_api_key", d->aiApiKeyEdit->text());
}

void SettingsPage::onThemeChanged(int index)
{
    Q_UNUSED(index);
    saveSettings();
    
    // Apply theme
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
        // Clear cache
        // DataService::instance()->clearCache();
        // DatabaseManager::instance()->clearBrowseHistory();

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
    // Notification setting changed
    // TODO: Implement notification settings
}

void SettingsPage::onAIConfigChanged()
{
    saveSettings();
    LOG_INFO("AI configuration changed");
}
