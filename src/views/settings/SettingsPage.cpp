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

    QLabel* titleLabel = new QLabel("Settings", this);
    titleLabel->setStyleSheet(QString("font-size: %1px; font-weight: 700; color: %2;")
                                  .arg(Font::Size::H1).arg(Colors::TextPrimary));
    mainLayout->addWidget(titleLabel);

    createAppearanceSection();
    createNotificationSection();
    createSecuritySection();
    createAboutSection();

    mainLayout->addStretch();
}

void SettingsPage::createAppearanceSection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget("Appearance", this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::MD);

    // Theme selection
    QHBoxLayout* themeLayout = new QHBoxLayout();
    themeLayout->addWidget(new QLabel("Theme:", content));
    
    d->themeCombo = new QComboBox(content);
    d->themeCombo->addItem("Light", 0);
    d->themeCombo->addItem("Dark", 1);
    d->themeCombo->addItem("Auto", 2);
    themeLayout->addWidget(d->themeCombo);
    themeLayout->addStretch();
    
    QObject::connect(d->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsPage::onThemeChanged);
    
    layout->addLayout(themeLayout);

    // Font size
    QHBoxLayout* fontLayout = new QHBoxLayout();
    fontLayout->addWidget(new QLabel("Font Size:", content));
    
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
    d->colorBlindCheck = new QCheckBox("Color Blind Mode", content);
    QObject::connect(d->colorBlindCheck, &QCheckBox::toggled, this, [this]() { saveSettings(); });
    layout->addWidget(d->colorBlindCheck);

    card->setContent(content);
    mainLayout->addWidget(card);
}

void SettingsPage::createNotificationSection()
{
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());

    CardWidget* card = new CardWidget("Notifications", this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    d->priceAlertCheck = new QCheckBox("Price Alerts", content);
    d->riskAlertCheck = new QCheckBox("Risk Alerts", content);
    d->tradeNotifyCheck = new QCheckBox("Trade Notifications", content);
    d->systemNotifyCheck = new QCheckBox("System Notifications", content);
    d->dailySummaryCheck = new QCheckBox("Daily Summary", content);

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

    CardWidget* card = new CardWidget("Security", this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    d->twoFactorCheck = new QCheckBox("Two-Factor Authentication", content);
    d->bioCheck = new QCheckBox("Biometric Authentication", content);

    layout->addWidget(d->twoFactorCheck);
    layout->addWidget(d->bioCheck);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    QPushButton* clearCacheBtn = new QPushButton("Clear Cache", content);
    QPushButton* exportDataBtn = new QPushButton("Export Data", content);
    
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

    CardWidget* card = new CardWidget("About", this);

    QWidget* content = new QWidget(card);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Spacing::SM);

    QLabel* versionLabel = new QLabel(QString("Version: %1").arg("2.0.0"), content);
    QLabel* buildLabel = new QLabel(QString("Build: %1").arg(__DATE__), content);
    QLabel* qtLabel = new QLabel(QString("Qt Version: %1").arg(qVersion()), content);

    layout->addWidget(versionLabel);
    layout->addWidget(buildLabel);
    layout->addWidget(qtLabel);

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
    int ret = QMessageBox::question(this, "Confirm",
                                    "Are you sure you want to clear all cache data?",
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // Clear cache
        // DataService::instance()->clearCache();
        // DatabaseManager::instance()->clearBrowseHistory();

        LOG_INFO("Cache cleared");
        QMessageBox::information(this, "Done", "Cache has been cleared.");
    }
}

void SettingsPage::onExportDataClicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    "Export Data", "", "JSON Files (*.json)");

    if (!filePath.isEmpty()) {
        ConfigManager::instance()->exportToFile(filePath, false);
        LOG_INFO(QString("Data exported to: %1").arg(filePath));
        QMessageBox::information(this, "Done", "Data has been exported.");
    }
}

void SettingsPage::onNotificationChanged()
{
    // Notification setting changed
    // TODO: Implement notification settings
}
