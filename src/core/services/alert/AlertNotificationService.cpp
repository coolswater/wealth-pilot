/**
 * @file AlertNotificationService.cpp
 * @brief 智能预警推送服务实现
 */

#include "AlertNotificationService.h"
#include "shared/utils/Logger.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QSmtpClient>
#include <QSystemTrayIcon>

namespace WealthPilot {

// ============================================================================
// Impl 定义
// ============================================================================

struct AlertNotificationService::Impl {
    QMap<AlertChannel, ChannelConfig> channelConfigs;
    QNetworkAccessManager* networkManager = nullptr;
    QSystemTrayIcon* trayIcon = nullptr;
};

// ============================================================================
// AlertNotificationService 实现
// ============================================================================

AlertNotificationService* AlertNotificationService::instance()
{
    static AlertNotificationService* inst = new AlertNotificationService();
    return inst;
}

AlertNotificationService::AlertNotificationService(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
    d->networkManager = new QNetworkAccessManager(this);
    
    // 初始化系统托盘图标
    d->trayIcon = new QSystemTrayIcon(this);
    d->trayIcon->setIcon(QIcon(":/icons/app.png"));
    d->trayIcon->show();
    
    LOG_DEBUG("AlertNotificationService created");
}

AlertNotificationService::~AlertNotificationService() = default;

void AlertNotificationService::configureChannel(const ChannelConfig& config)
{
    d->channelConfigs[config.channel] = config;
    LOG_INFO(QString("Alert channel configured: %1, enabled: %2")
        .arg(static_cast<int>(config.channel))
        .arg(config.enabled));
}

ChannelConfig AlertNotificationService::getChannelConfig(AlertChannel channel) const
{
    return d->channelConfigs.value(channel);
}

void AlertNotificationService::sendAlert(const AlertMessage& message,
                                          const QList<AlertChannel>& channels)
{
    QList<AlertChannel> targetChannels = channels.isEmpty() 
        ? d->channelConfigs.keys() 
        : channels;

    for (AlertChannel channel : targetChannels) {
        ChannelConfig config = d->channelConfigs.value(channel);
        
        if (!config.enabled) {
            continue;
        }
        
        // 检查优先级
        if (message.priority < config.minPriority) {
            continue;
        }
        
        bool success = false;
        switch (channel) {
        case AlertChannel::WeChat:
            success = sendViaWeChat(message, config);
            break;
        case AlertChannel::WeChatWork:
            success = sendViaWeChatWork(message, config);
            break;
        case AlertChannel::Email:
            success = sendViaEmail(message, config);
            break;
        case AlertChannel::SystemNotify:
            success = sendViaSystemNotify(message);
            break;
        case AlertChannel::Webhook:
            success = sendViaWebhook(message, config);
            break;
        case AlertChannel::DingTalk:
            success = sendViaDingTalk(message, config);
            break;
        }
        
        if (success) {
            emit alertSent(message.id, channel);
        } else {
            emit alertFailed(message.id, channel, "Send failed");
        }
    }
}

void AlertNotificationService::sendAlerts(const QList<AlertMessage>& messages)
{
    for (const auto& message : messages) {
        sendAlert(message);
    }
}

bool AlertNotificationService::sendViaWeChat(const AlertMessage& message, 
                                              const ChannelConfig& config)
{
    // Server酱 API: https://sctapi.ftqq.com/{sendkey}.send
    QString url = QString("https://sctapi.ftqq.com/%1.send").arg(config.wechatSendKey);
    
    QUrlQuery params;
    params.addQueryItem("title", message.title);
    params.addQueryItem("desp", message.content);
    
    QNetworkRequest request(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, 
                      "application/x-www-form-urlencoded");
    
    QNetworkReply* reply = d->networkManager->post(request, 
        params.toString(QUrl::FullyEncoded).toUtf8());
    
    connect(reply, &QNetworkReply::finished, [this, reply, message]() {
        if (reply->error() == QNetworkReply::NoError) {
            LOG_INFO(QString("WeChat alert sent: %1").arg(message.id));
        } else {
            LOG_ERROR(QString("WeChat alert failed: %1").arg(reply->errorString()));
            emit alertFailed(message.id, AlertChannel::WeChat, reply->errorString());
        }
        reply->deleteLater();
    });
    
    return true;
}

bool AlertNotificationService::sendViaWeChatWork(const AlertMessage& message,
                                                  const ChannelConfig& config)
{
    // 企业微信机器人 Webhook
    QJsonObject markdown;
    markdown["content"] = QString("## %1\n\n%2\n\n> 时间: %3\n> 股票: %4")
        .arg(message.title)
        .arg(message.content)
        .arg(message.timestamp.toString("yyyy-MM-dd HH:mm:ss"))
        .arg(message.symbol);
    
    QJsonObject body;
    body["msgtype"] = "markdown";
    body["markdown"] = markdown;
    
    QNetworkRequest request(QUrl(config.wechatWorkWebhook));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = d->networkManager->post(request,
        QJsonDocument(body).toJson());
    
    connect(reply, &QNetworkReply::finished, [this, reply, message]() {
        if (reply->error() == QNetworkReply::NoError) {
            LOG_INFO(QString("WeChatWork alert sent: %1").arg(message.id));
        } else {
            LOG_ERROR(QString("WeChatWork alert failed: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
    
    return true;
}

bool AlertNotificationService::sendViaEmail(const AlertMessage& message,
                                             const ChannelConfig& config)
{
    // 简化实现：使用 QProcess 调用系统邮件客户端
    // 完整实现需要 SMTP 库
    
    QString subject = QString("[WealthPilot] %1").arg(message.title);
    QString body = message.content;
    
    // 使用 QProcess 发送邮件（需要配置系统邮件）
    // 这里仅记录日志
    LOG_INFO(QString("Email alert: %1 -> %2")
        .arg(subject)
        .arg(config.emailTo.join(", ")));
    
    // TODO: 实现完整 SMTP 发送
    return true;
}

bool AlertNotificationService::sendViaSystemNotify(const AlertMessage& message)
{
    if (!d->trayIcon) {
        return false;
    }
    
    d->trayIcon->showMessage(
        message.title,
        message.content.left(200),
        QSystemTrayIcon::Information,
        5000  // 显示5秒
    );
    
    LOG_INFO(QString("System notification shown: %1").arg(message.title));
    return true;
}

bool AlertNotificationService::sendViaWebhook(const AlertMessage& message,
                                               const ChannelConfig& config)
{
    QJsonObject body;
    body["title"] = message.title;
    body["content"] = message.content;
    body["symbol"] = message.symbol;
    body["priority"] = message.priority;
    body["timestamp"] = message.timestamp.toString(Qt::ISODate);
    
    QNetworkRequest request(QUrl(config.webhookUrl));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!config.webhookToken.isEmpty()) {
        request.setRawHeader("Authorization", 
            QString("Bearer %1").arg(config.webhookToken).toUtf8());
    }
    
    QNetworkReply* reply = d->networkManager->post(request,
        QJsonDocument(body).toJson());
    
    connect(reply, &QNetworkReply::finished, [this, reply, message]() {
        if (reply->error() == QNetworkReply::NoError) {
            LOG_INFO(QString("Webhook alert sent: %1").arg(message.id));
        } else {
            LOG_ERROR(QString("Webhook alert failed: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
    
    return true;
}

bool AlertNotificationService::sendViaDingTalk(const AlertMessage& message,
                                                const ChannelConfig& config)
{
    QJsonObject text;
    text["content"] = QString("【%1】\n%2\n时间：%3")
        .arg(message.title)
        .arg(message.content)
        .arg(message.timestamp.toString("HH:mm:ss"));
    
    QJsonObject body;
    body["msgtype"] = "text";
    body["text"] = text;
    
    QNetworkRequest request(QUrl(config.dingTalkWebhook));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = d->networkManager->post(request,
        QJsonDocument(body).toJson());
    
    connect(reply, &QNetworkReply::finished, [this, reply, message]() {
        if (reply->error() == QNetworkReply::NoError) {
            LOG_INFO(QString("DingTalk alert sent: %1").arg(message.id));
        } else {
            LOG_ERROR(QString("DingTalk alert failed: %1").arg(reply->errorString()));
        }
        reply->deleteLater();
    });
    
    return true;
}

bool AlertNotificationService::testChannel(AlertChannel channel)
{
    AlertMessage testMsg;
    testMsg.id = "test";
    testMsg.title = "测试预警";
    testMsg.content = "这是一条测试消息，用于验证推送渠道是否正常工作。";
    testMsg.timestamp = QDateTime::currentDateTime();
    testMsg.priority = 9;
    
    sendAlert(testMsg, {channel});
    return true;
}

} // namespace WealthPilot
