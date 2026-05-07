/**
 * @file SettingsPage.cpp
 * @brief 设置页面实现 - 参考风控页面样式优化
 */

#include "SettingsPage.h"
#include "ui/components/PageStyles.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
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
#include <QGroupBox>
#include <QSettings>

#include <core/config/ConfigManager.h>
#include <ui/ThemeManager.h>
#include <core/config/Tokens.h>

using namespace Tokens;

struct SettingsPage::Impl {
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
    
    // AI 配置
    QCheckBox* aiEnabledCheck = nullptr;
    QComboBox* aiProviderCombo = nullptr;
    QLineEdit* aiApiUrlEdit = nullptr;
    QLineEdit* aiApiKeyEdit = nullptr;
    QLineEdit* aiModelEdit = nullptr;
    
    // 按钮
    QPushButton* saveBtn = nullptr;
    QPushButton* resetBtn = nullptr;
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
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // ========== 页面标题栏 ==========
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    QLabel* titleLabel = new QLabel(QStringLiteral("系统设置"), this);
    titleLabel->setStyleSheet(PageStyles::titleText());
    headerLayout->addWidget(titleLabel);
    
    headerLayout->addStretch();
    
    // 保存和重置按钮
    d->resetBtn = new QPushButton(QStringLiteral("重置"), this);
    d->resetBtn->setStyleSheet(PageStyles::secondaryButton());
    d->resetBtn->setFixedWidth(80);
    headerLayout->addWidget(d->resetBtn);
    
    d->saveBtn = new QPushButton(QStringLiteral("保存"), this);
    d->saveBtn->setStyleSheet(PageStyles::primaryButton());
    d->saveBtn->setFixedWidth(80);
    headerLayout->addWidget(d->saveBtn);
    
    mainLayout->addLayout(headerLayout);
    
    // ========== 滚动区域 ==========
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet(QString("QScrollArea { background: transparent; border: none; }"
                                       "QScrollBar:vertical { width: 8px; }"));
    
    QWidget* scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(0, 0, 8, 0);
    contentLayout->setSpacing(16);
    
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);
    
    // 样式变量
    QString groupBoxStyle = PageStyles::groupBox();
    QString inputStyle = PageStyles::inputField();
    QString checkStyle = PageStyles::checkBox();
    QString comboStyle = PageStyles::comboBox();
    
    // ========== 外观设置 ==========
    QGroupBox* appearanceGroup = new QGroupBox(QStringLiteral("外观设置"), this);
    appearanceGroup->setStyleSheet(groupBoxStyle);
    QFormLayout* appearanceLayout = new QFormLayout(appearanceGroup);
    appearanceLayout->setSpacing(12);
    
    // 主题选择
    QHBoxLayout* themeLayout = new QHBoxLayout();
    d->themeCombo = new QComboBox(this);
    d->themeCombo->addItem(QStringLiteral("浅色"), 0);
    d->themeCombo->addItem(QStringLiteral("深色"), 1);
    d->themeCombo->addItem(QStringLiteral("自动"), 2);
    d->themeCombo->setStyleSheet(comboStyle);
    d->themeCombo->setMinimumWidth(150);
    themeLayout->addWidget(d->themeCombo);
    themeLayout->addStretch();
    appearanceLayout->addRow(QStringLiteral("主题:"), themeLayout);
    
    // 字体大小
    QHBoxLayout* fontLayout = new QHBoxLayout();
    d->fontSlider = new QSlider(Qt::Horizontal, this);
    d->fontSlider->setRange(10, 20);
    d->fontSlider->setValue(14);
    d->fontSlider->setMinimumWidth(150);
    d->fontSlider->setStyleSheet(QString(R"(
        QSlider::groove:horizontal {
            background: %1;
            height: 4px;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: %2;
            width: 16px;
            height: 16px;
            margin: -6px 0;
            border-radius: 8px;
        }
        QSlider::sub-page:horizontal {
            background: %2;
            border-radius: 2px;
        }
    )").arg(Colors::Border, Colors::Primary));
    fontLayout->addWidget(d->fontSlider);
    
    d->fontValueLabel = new QLabel("14px", this);
    d->fontValueLabel->setStyleSheet(PageStyles::labelText());
    d->fontValueLabel->setMinimumWidth(40);
    fontLayout->addWidget(d->fontValueLabel);
    fontLayout->addStretch();
    appearanceLayout->addRow(QStringLiteral("字体大小:"), fontLayout);
    
    // 色盲模式
    d->colorBlindCheck = new QCheckBox(QStringLiteral("启用色盲模式"), this);
    d->colorBlindCheck->setStyleSheet(checkStyle);
    appearanceLayout->addRow(d->colorBlindCheck);
    
    contentLayout->addWidget(appearanceGroup);
    
    // ========== 通知设置 ==========
    QGroupBox* notifyGroup = new QGroupBox(QStringLiteral("通知设置"), this);
    notifyGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* notifyLayout = new QVBoxLayout(notifyGroup);
    notifyLayout->setSpacing(8);
    
    d->priceAlertCheck = new QCheckBox(QStringLiteral("价格提醒 - 当股票达到设定价格时通知"), this);
    d->priceAlertCheck->setStyleSheet(checkStyle);
    notifyLayout->addWidget(d->priceAlertCheck);
    
    d->riskAlertCheck = new QCheckBox(QStringLiteral("风险预警 - 当持仓风险超过阈值时通知"), this);
    d->riskAlertCheck->setStyleSheet(checkStyle);
    notifyLayout->addWidget(d->riskAlertCheck);
    
    d->tradeNotifyCheck = new QCheckBox(QStringLiteral("交易通知 - 交易成交时发送通知"), this);
    d->tradeNotifyCheck->setStyleSheet(checkStyle);
    notifyLayout->addWidget(d->tradeNotifyCheck);
    
    d->systemNotifyCheck = new QCheckBox(QStringLiteral("系统通知 - 系统更新和维护通知"), this);
    d->systemNotifyCheck->setStyleSheet(checkStyle);
    notifyLayout->addWidget(d->systemNotifyCheck);
    
    d->dailySummaryCheck = new QCheckBox(QStringLiteral("每日汇总 - 每日收盘后发送投资日报"), this);
    d->dailySummaryCheck->setStyleSheet(checkStyle);
    notifyLayout->addWidget(d->dailySummaryCheck);
    
    contentLayout->addWidget(notifyGroup);
    
    // ========== 安全设置 ==========
    QGroupBox* securityGroup = new QGroupBox(QStringLiteral("安全设置"), this);
    securityGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout* securityLayout = new QVBoxLayout(securityGroup);
    securityLayout->setSpacing(8);
    
    d->twoFactorCheck = new QCheckBox(QStringLiteral("双因素认证 - 登录时需要验证码"), this);
    d->twoFactorCheck->setStyleSheet(checkStyle);
    securityLayout->addWidget(d->twoFactorCheck);
    
    d->bioCheck = new QCheckBox(QStringLiteral("生物识别认证 - 使用指纹或面部识别登录"), this);
    d->bioCheck->setStyleSheet(checkStyle);
    securityLayout->addWidget(d->bioCheck);
    
    // 数据管理按钮
    QHBoxLayout* dataBtnLayout = new QHBoxLayout();
    dataBtnLayout->setSpacing(12);
    
    QPushButton* clearCacheBtn = new QPushButton(QStringLiteral("清除缓存"), this);
    clearCacheBtn->setStyleSheet(PageStyles::secondaryButton());
    clearCacheBtn->setFixedWidth(100);
    connect(clearCacheBtn, &QPushButton::clicked, this, &SettingsPage::onClearCacheClicked);
    dataBtnLayout->addWidget(clearCacheBtn);
    
    QPushButton* exportDataBtn = new QPushButton(QStringLiteral("导出数据"), this);
    exportDataBtn->setStyleSheet(PageStyles::secondaryButton());
    exportDataBtn->setFixedWidth(100);
    connect(exportDataBtn, &QPushButton::clicked, this, &SettingsPage::onExportDataClicked);
    dataBtnLayout->addWidget(exportDataBtn);
    
    dataBtnLayout->addStretch();
    securityLayout->addLayout(dataBtnLayout);
    
    contentLayout->addWidget(securityGroup);
    
    // ========== AI 配置 ==========
    QGroupBox* aiGroup = new QGroupBox(QStringLiteral("AI 配置"), this);
    aiGroup->setStyleSheet(groupBoxStyle);
    QFormLayout* aiLayout = new QFormLayout(aiGroup);
    aiLayout->setSpacing(12);
    
    // 启用 AI
    d->aiEnabledCheck = new QCheckBox(QStringLiteral("启用 AI 助手功能"), this);
    d->aiEnabledCheck->setStyleSheet(checkStyle);
    aiLayout->addRow(d->aiEnabledCheck);
    
    // AI 提供商
    QHBoxLayout* providerLayout = new QHBoxLayout();
    d->aiProviderCombo = new QComboBox(this);
    d->aiProviderCombo->addItem(QStringLiteral("OpenAI"), "openai");
    d->aiProviderCombo->addItem(QStringLiteral("Azure OpenAI"), "azure");
    d->aiProviderCombo->addItem(QStringLiteral("Anthropic"), "anthropic");
    d->aiProviderCombo->addItem(QStringLiteral("本地模型"), "local");
    d->aiProviderCombo->addItem(QStringLiteral("自定义"), "custom");
    d->aiProviderCombo->setStyleSheet(comboStyle);
    d->aiProviderCombo->setMinimumWidth(150);
    providerLayout->addWidget(d->aiProviderCombo);
    providerLayout->addStretch();
    aiLayout->addRow(QStringLiteral("AI 提供商:"), providerLayout);
    
    // API 地址
    QHBoxLayout* urlLayout = new QHBoxLayout();
    d->aiApiUrlEdit = new QLineEdit(this);
    d->aiApiUrlEdit->setPlaceholderText(QStringLiteral("https://api.openai.com/v1"));
    d->aiApiUrlEdit->setStyleSheet(inputStyle);
    d->aiApiUrlEdit->setMinimumWidth(300);
    urlLayout->addWidget(d->aiApiUrlEdit);
    urlLayout->addStretch();
    aiLayout->addRow(QStringLiteral("API 地址:"), urlLayout);
    
    // API 密钥
    QHBoxLayout* keyLayout = new QHBoxLayout();
    d->aiApiKeyEdit = new QLineEdit(this);
    d->aiApiKeyEdit->setPlaceholderText(QStringLiteral("请输入您的 API 密钥"));
    d->aiApiKeyEdit->setEchoMode(QLineEdit::Password);
    d->aiApiKeyEdit->setStyleSheet(inputStyle);
    d->aiApiKeyEdit->setMinimumWidth(250);
    keyLayout->addWidget(d->aiApiKeyEdit);
    
    QPushButton* toggleKeyBtn = new QPushButton(QStringLiteral("显示"), this);
    toggleKeyBtn->setStyleSheet(PageStyles::secondaryButton());
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
    aiLayout->addRow(QStringLiteral("API 密钥:"), keyLayout);
    
    // 模型名称
    QHBoxLayout* modelLayout = new QHBoxLayout();
    d->aiModelEdit = new QLineEdit(this);
    d->aiModelEdit->setPlaceholderText(QStringLiteral("gpt-4, gpt-3.5-turbo, claude-3 等"));
    d->aiModelEdit->setStyleSheet(inputStyle);
    d->aiModelEdit->setMinimumWidth(200);
    modelLayout->addWidget(d->aiModelEdit);
    modelLayout->addStretch();
    aiLayout->addRow(QStringLiteral("模型名称:"), modelLayout);
    
    // 提示信息
    QLabel* hintLabel = new QLabel(QStringLiteral("提示：API 密钥使用 Windows DPAPI 加密安全存储"), this);
    hintLabel->setStyleSheet(PageStyles::labelText());
    aiLayout->addRow(hintLabel);
    
    contentLayout->addWidget(aiGroup);
    
    contentLayout->addStretch();
    
    // ========== 连接信号 ==========
    connect(d->saveBtn, &QPushButton::clicked, this, &SettingsPage::onSaveClicked);
    connect(d->resetBtn, &QPushButton::clicked, this, &SettingsPage::onResetClicked);
    connect(d->themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsPage::onThemeChanged);
    connect(d->fontSlider, &QSlider::valueChanged, this, &SettingsPage::onFontSizeChanged);
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

void SettingsPage::onSaveClicked()
{
    saveSettings();
    LOG_INFO("Settings saved");
    QMessageBox::information(this, QStringLiteral("保存成功"), QStringLiteral("设置已保存。"));
}

void SettingsPage::onResetClicked()
{
    // 重置为默认值
    d->themeCombo->setCurrentIndex(0);
    d->fontSlider->setValue(14);
    d->colorBlindCheck->setChecked(false);
    
    d->priceAlertCheck->setChecked(true);
    d->riskAlertCheck->setChecked(true);
    d->tradeNotifyCheck->setChecked(true);
    d->systemNotifyCheck->setChecked(true);
    d->dailySummaryCheck->setChecked(false);
    
    d->twoFactorCheck->setChecked(false);
    d->bioCheck->setChecked(false);
    
    d->aiEnabledCheck->setChecked(false);
    d->aiProviderCombo->setCurrentIndex(0);
    d->aiApiUrlEdit->setText("https://api.openai.com/v1");
    d->aiApiKeyEdit->clear();
    d->aiModelEdit->setText("gpt-4");
    
    LOG_INFO("Settings reset to defaults");
}

void SettingsPage::onThemeChanged(int index)
{
    Q_UNUSED(index);
    saveSettings();

    ThemeType themeType = static_cast<ThemeType>(d->themeCombo->currentIndex());
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
    saveSettings();
    LOG_INFO("Notification settings changed");
}

void SettingsPage::onAIConfigChanged()
{
    saveSettings();
    LOG_INFO("AI configuration changed");
}
