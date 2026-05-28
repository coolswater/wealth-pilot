/**
 * @file AlertSettingsWidget.cpp
 * @brief 智能预警设置面板实现
 */

#include "AlertSettingsWidget.h"
#include "services/alert/AlertNotificationService.h"
#include "presentation/components/StyleHelper.h"
#include "presentation/styles/ButtonStyles.h"
#include "infrastructure/config/ConfigManager.h"
#include "shared/utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QGroupBox>
#include <QMessageBox>

using namespace WealthPilot;

struct AlertSettingsWidget::Impl {
    // 微信推送
    QCheckBox* wechatEnabled = nullptr;
    QLineEdit* wechatSendKey = nullptr;
    QPushButton* wechatTestBtn = nullptr;

    // 企业微信
    QCheckBox* wechatWorkEnabled = nullptr;
    QLineEdit* wechatWorkWebhook = nullptr;
    QPushButton* wechatWorkTestBtn = nullptr;

    // 邮件
    QCheckBox* emailEnabled = nullptr;
    QLineEdit* smtpHost = nullptr;
    QSpinBox* smtpPort = nullptr;
    QLineEdit* smtpUser = nullptr;
    QLineEdit* smtpPassword = nullptr;
    QLineEdit* emailFrom = nullptr;
    QLineEdit* emailTo = nullptr;
    QPushButton* emailTestBtn = nullptr;

    // 钉钉
    QCheckBox* dingTalkEnabled = nullptr;
    QLineEdit* dingTalkWebhook = nullptr;
    QLineEdit* dingTalkSecret = nullptr;
    QPushButton* dingTalkTestBtn = nullptr;

    // 预警规则
    QSpinBox* minPriority = nullptr;
    QCheckBox* priceAlertCheck = nullptr;
    QCheckBox* riskAlertCheck = nullptr;
    QCheckBox* tradeNotifyCheck = nullptr;
};

AlertSettingsWidget::AlertSettingsWidget(QWidget* parent)
    : QWidget(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    loadConfig();
}

AlertSettingsWidget::~AlertSettingsWidget() = default;

void AlertSettingsWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 微信推送
    mainLayout->addWidget(createWeChatGroup());

    // 企业微信
    mainLayout->addWidget(createWeChatWorkGroup());

    // 邮件
    mainLayout->addWidget(createEmailGroup());

    // 钉钉
    mainLayout->addWidget(createDingTalkGroup());

    // 预警规则
    mainLayout->addWidget(createRuleGroup());

    mainLayout->addStretch();

    setupConnections();
}

QGroupBox* AlertSettingsWidget::createWeChatGroup()
{
    auto* group = new QGroupBox(QStringLiteral("微信推送（Server酱）"));
    auto* layout = new QFormLayout(group);
    layout->setSpacing(8);

    d->wechatEnabled = new QCheckBox(QStringLiteral("启用"));
    layout->addRow(d->wechatEnabled);

    d->wechatSendKey = new QLineEdit();
    d->wechatSendKey->setPlaceholderText(QStringLiteral("输入 SendKey"));
    d->wechatSendKey->setEchoMode(QLineEdit::Password);
    layout->addRow(QStringLiteral("SendKey:"), d->wechatSendKey);

    d->wechatTestBtn = new QPushButton(QStringLiteral("测试连接"));
    ButtonStyles::setSecondary(d->wechatTestBtn);
    layout->addRow(QString(), d->wechatTestBtn);

    return group;
}

QGroupBox* AlertSettingsWidget::createWeChatWorkGroup()
{
    auto* group = new QGroupBox(QStringLiteral("企业微信机器人"));
    auto* layout = new QFormLayout(group);
    layout->setSpacing(8);

    d->wechatWorkEnabled = new QCheckBox(QStringLiteral("启用"));
    layout->addRow(d->wechatWorkEnabled);

    d->wechatWorkWebhook = new QLineEdit();
    d->wechatWorkWebhook->setPlaceholderText(QStringLiteral("https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=xxx"));
    layout->addRow(QStringLiteral("Webhook:"), d->wechatWorkWebhook);

    d->wechatWorkTestBtn = new QPushButton(QStringLiteral("测试连接"));
    ButtonStyles::setSecondary(d->wechatWorkTestBtn);
    layout->addRow(QString(), d->wechatWorkTestBtn);

    return group;
}

QGroupBox* AlertSettingsWidget::createEmailGroup()
{
    auto* group = new QGroupBox(QStringLiteral("邮件推送"));
    auto* layout = new QFormLayout(group);
    layout->setSpacing(8);

    d->emailEnabled = new QCheckBox(QStringLiteral("启用"));
    layout->addRow(d->emailEnabled);

    d->smtpHost = new QLineEdit();
    d->smtpHost->setPlaceholderText(QStringLiteral("smtp.example.com"));
    layout->addRow(QStringLiteral("SMTP 服务器:"), d->smtpHost);

    d->smtpPort = new QSpinBox();
    d->smtpPort->setRange(1, 65535);
    d->smtpPort->setValue(465);
    layout->addRow(QStringLiteral("端口:"), d->smtpPort);

    d->smtpUser = new QLineEdit();
    layout->addRow(QStringLiteral("用户名:"), d->smtpUser);

    d->smtpPassword = new QLineEdit();
    d->smtpPassword->setEchoMode(QLineEdit::Password);
    layout->addRow(QStringLiteral("密码:"), d->smtpPassword);

    d->emailFrom = new QLineEdit();
    layout->addRow(QStringLiteral("发件人:"), d->emailFrom);

    d->emailTo = new QLineEdit();
    d->emailTo->setPlaceholderText(QStringLiteral("多个收件人用逗号分隔"));
    layout->addRow(QStringLiteral("收件人:"), d->emailTo);

    d->emailTestBtn = new QPushButton(QStringLiteral("发送测试邮件"));
    ButtonStyles::setSecondary(d->emailTestBtn);
    layout->addRow(QString(), d->emailTestBtn);

    return group;
}

QGroupBox* AlertSettingsWidget::createDingTalkGroup()
{
    auto* group = new QGroupBox(QStringLiteral("钉钉机器人"));
    auto* layout = new QFormLayout(group);
    layout->setSpacing(8);

    d->dingTalkEnabled = new QCheckBox(QStringLiteral("启用"));
    layout->addRow(d->dingTalkEnabled);

    d->dingTalkWebhook = new QLineEdit();
    d->dingTalkWebhook->setPlaceholderText(QStringLiteral("https://oapi.dingtalk.com/robot/send?access_token=xxx"));
    layout->addRow(QStringLiteral("Webhook:"), d->dingTalkWebhook);

    d->dingTalkSecret = new QLineEdit();
    d->dingTalkSecret->setPlaceholderText(QStringLiteral("加签密钥（可选）"));
    d->dingTalkSecret->setEchoMode(QLineEdit::Password);
    layout->addRow(QStringLiteral("Secret:"), d->dingTalkSecret);

    d->dingTalkTestBtn = new QPushButton(QStringLiteral("测试连接"));
    ButtonStyles::setSecondary(d->dingTalkTestBtn);
    layout->addRow(QString(), d->dingTalkTestBtn);

    return group;
}

QGroupBox* AlertSettingsWidget::createRuleGroup()
{
    auto* group = new QGroupBox(QStringLiteral("预警规则"));
    auto* layout = new QFormLayout(group);
    layout->setSpacing(8);

    d->minPriority = new QSpinBox();
    d->minPriority->setRange(0, 9);
    d->minPriority->setValue(3);
    d->minPriority->setToolTip(QStringLiteral("仅推送优先级 >= 设定值的预警（0-9，越高越紧急）"));
    layout->addRow(QStringLiteral("最低优先级:"), d->minPriority);

    d->priceAlertCheck = new QCheckBox(QStringLiteral("价格预警"));
    d->priceAlertCheck->setChecked(true);
    layout->addRow(d->priceAlertCheck);

    d->riskAlertCheck = new QCheckBox(QStringLiteral("风险预警"));
    d->riskAlertCheck->setChecked(true);
    layout->addRow(d->riskAlertCheck);

    d->tradeNotifyCheck = new QCheckBox(QStringLiteral("交易通知"));
    d->tradeNotifyCheck->setChecked(true);
    layout->addRow(d->tradeNotifyCheck);

    return group;
}

void AlertSettingsWidget::setupConnections()
{
    connect(d->wechatTestBtn, &QPushButton::clicked, this, &AlertSettingsWidget::onTestWeChat);
    connect(d->wechatWorkTestBtn, &QPushButton::clicked, this, &AlertSettingsWidget::onTestWeChatWork);
    connect(d->emailTestBtn, &QPushButton::clicked, this, &AlertSettingsWidget::onTestEmail);
    connect(d->dingTalkTestBtn, &QPushButton::clicked, this, &AlertSettingsWidget::onTestDingTalk);
}

void AlertSettingsWidget::loadConfig()
{
    auto* config = ConfigManager::instance();

    // 微信
    d->wechatEnabled->setChecked(config->getBool("alert/wechat_enabled", false));
    d->wechatSendKey->setText(config->getSecure("alert/wechat_sendkey"));

    // 企业微信
    d->wechatWorkEnabled->setChecked(config->getBool("alert/wechatwork_enabled", false));
    d->wechatWorkWebhook->setText(config->getString("alert/wechatwork_webhook"));

    // 邮件
    d->emailEnabled->setChecked(config->getBool("alert/email_enabled", false));
    d->smtpHost->setText(config->getString("alert/smtp_host"));
    d->smtpPort->setValue(config->get("alert/smtp_port", 465).toInt());
    d->smtpUser->setText(config->getString("alert/smtp_user"));
    d->smtpPassword->setText(config->getSecure("alert/smtp_password"));
    d->emailFrom->setText(config->getString("alert/email_from"));
    d->emailTo->setText(config->getString("alert/email_to"));

    // 钉钉
    d->dingTalkEnabled->setChecked(config->getBool("alert/dingtalk_enabled", false));
    d->dingTalkWebhook->setText(config->getString("alert/dingtalk_webhook"));
    d->dingTalkSecret->setText(config->getSecure("alert/dingtalk_secret"));

    // 规则
    d->minPriority->setValue(config->get("alert/min_priority", 3).toInt());
    d->priceAlertCheck->setChecked(config->getBool("alert/price_alert", true));
    d->riskAlertCheck->setChecked(config->getBool("alert/risk_alert", true));
    d->tradeNotifyCheck->setChecked(config->getBool("alert/trade_notify", true));
}

void AlertSettingsWidget::saveConfig()
{
    auto* config = ConfigManager::instance();

    // 微信
    config->set("alert/wechat_enabled", d->wechatEnabled->isChecked());
    config->setSecure("alert/wechat_sendkey", d->wechatSendKey->text());

    // 企业微信
    config->set("alert/wechatwork_enabled", d->wechatWorkEnabled->isChecked());
    config->set("alert/wechatwork_webhook", d->wechatWorkWebhook->text());

    // 邮件
    config->set("alert/email_enabled", d->emailEnabled->isChecked());
    config->set("alert/smtp_host", d->smtpHost->text());
    config->set("alert/smtp_port", d->smtpPort->value());
    config->set("alert/smtp_user", d->smtpUser->text());
    config->setSecure("alert/smtp_password", d->smtpPassword->text());
    config->set("alert/email_from", d->emailFrom->text());
    config->set("alert/email_to", d->emailTo->text());

    // 钉钉
    config->set("alert/dingtalk_enabled", d->dingTalkEnabled->isChecked());
    config->set("alert/dingtalk_webhook", d->dingTalkWebhook->text());
    config->setSecure("alert/dingtalk_secret", d->dingTalkSecret->text());

    // 规则
    config->set("alert/min_priority", d->minPriority->value());
    config->set("alert/price_alert", d->priceAlertCheck->isChecked());
    config->set("alert/risk_alert", d->riskAlertCheck->isChecked());
    config->set("alert/trade_notify", d->tradeNotifyCheck->isChecked());

    // 同步到 AlertNotificationService
    syncToService();

    LOG_INFO("[AlertSettingsWidget] Config saved");
    emit configChanged();
}

void AlertSettingsWidget::syncToService()
{
    auto* service = AlertNotificationService::instance();

    // 微信
    if (d->wechatEnabled->isChecked()) {
        ChannelConfig config;
        config.channel = AlertChannel::WeChat;
        config.enabled = true;
        config.wechatSendKey = d->wechatSendKey->text();
        config.minPriority = d->minPriority->value();
        service->configureChannel(config);
    }

    // 企业微信
    if (d->wechatWorkEnabled->isChecked()) {
        ChannelConfig config;
        config.channel = AlertChannel::WeChatWork;
        config.enabled = true;
        config.wechatWorkWebhook = d->wechatWorkWebhook->text();
        config.minPriority = d->minPriority->value();
        service->configureChannel(config);
    }

    // 邮件
    if (d->emailEnabled->isChecked()) {
        ChannelConfig config;
        config.channel = AlertChannel::Email;
        config.enabled = true;
        config.smtpHost = d->smtpHost->text();
        config.smtpPort = d->smtpPort->value();
        config.smtpUser = d->smtpUser->text();
        config.smtpPassword = d->smtpPassword->text();
        config.emailFrom = d->emailFrom->text();
        config.emailTo = d->emailTo->text().split(',', Qt::SkipEmptyParts);
        config.minPriority = d->minPriority->value();
        service->configureChannel(config);
    }

    // 钉钉
    if (d->dingTalkEnabled->isChecked()) {
        ChannelConfig config;
        config.channel = AlertChannel::DingTalk;
        config.enabled = true;
        config.dingTalkWebhook = d->dingTalkWebhook->text();
        config.dingTalkSecret = d->dingTalkSecret->text();
        config.minPriority = d->minPriority->value();
        service->configureChannel(config);
    }
}

void AlertSettingsWidget::onTestWeChat()
{
    if (d->wechatSendKey->text().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请先输入 SendKey"));
        return;
    }

    AlertMessage msg;
    msg.title = QStringLiteral("WealthPilot 测试消息");
    msg.content = QStringLiteral("这是一条测试消息，如果您收到此消息，说明微信推送配置成功！");
    msg.priority = 9;

    ChannelConfig config;
    config.channel = AlertChannel::WeChat;
    config.wechatSendKey = d->wechatSendKey->text();

    bool ok = AlertNotificationService::instance()->testChannel(AlertChannel::WeChat);
    if (ok) {
        QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("测试消息已发送"));
    } else {
        QMessageBox::warning(this, QStringLiteral("失败"), QStringLiteral("发送失败，请检查配置"));
    }
}

void AlertSettingsWidget::onTestWeChatWork()
{
    if (d->wechatWorkWebhook->text().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请先输入 Webhook 地址"));
        return;
    }

    bool ok = AlertNotificationService::instance()->testChannel(AlertChannel::WeChatWork);
    if (ok) {
        QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("测试消息已发送"));
    } else {
        QMessageBox::warning(this, QStringLiteral("失败"), QStringLiteral("发送失败，请检查配置"));
    }
}

void AlertSettingsWidget::onTestEmail()
{
    if (d->smtpHost->text().isEmpty() || d->emailTo->text().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请先填写 SMTP 服务器和收件人"));
        return;
    }

    bool ok = AlertNotificationService::instance()->testChannel(AlertChannel::Email);
    if (ok) {
        QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("测试邮件已发送"));
    } else {
        QMessageBox::warning(this, QStringLiteral("失败"), QStringLiteral("发送失败，请检查配置"));
    }
}

void AlertSettingsWidget::onTestDingTalk()
{
    if (d->dingTalkWebhook->text().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("错误"), QStringLiteral("请先输入 Webhook 地址"));
        return;
    }

    bool ok = AlertNotificationService::instance()->testChannel(AlertChannel::DingTalk);
    if (ok) {
        QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("测试消息已发送"));
    } else {
        QMessageBox::warning(this, QStringLiteral("失败"), QStringLiteral("发送失败，请检查配置"));
    }
}
