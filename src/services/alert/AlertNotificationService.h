/**
 * @file AlertNotificationService.h
 * @brief 智能预警推送服务
 *
 * @details 支持多种推送渠道：
 * - 微信推送（Server酱/企业微信机器人）
 * - 邮件推送（SMTP）
 * - 系统通知
 * - Webhook
 */

#ifndef ALERTNOTIFICATIONSERVICE_H
#define ALERTNOTIFICATIONSERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMap>
#include <memory>

namespace WealthPilot {

/**
 * @brief 推送渠道类型
 */
enum class AlertChannel {
    WeChat,         ///< 微信（Server酱）
    WeChatWork,     ///< 企业微信机器人
    Email,          ///< 邮件
    SystemNotify,   ///< 系统通知
    Webhook,        ///< 自定义 Webhook
    DingTalk        ///< 钉钉机器人
};

/**
 * @brief 预警消息
 */
struct AlertMessage {
    QString id;                 ///< 消息ID
    QString title;              ///< 标题
    QString content;            ///< 内容
    QString symbol;             ///< 相关股票代码
    QString alertType;          ///< 预警类型（price/volume/indicator）
    int priority = 0;           ///< 优先级（0-9，越高越紧急）
    QDateTime timestamp;        ///< 时间戳
    
    QString toString() const {
        return QString("[%1] %2: %3").arg(
            timestamp.toString("HH:mm:ss"),
            title,
            content.left(100)
        );
    }
};

/**
 * @brief 渠道配置
 */
struct ChannelConfig {
    AlertChannel channel;
    bool enabled = true;
    int minPriority = 0;        ///< 最低推送优先级
    
    // 微信配置
    QString wechatSendKey;      ///< Server酱 SendKey
    
    // 企业微信配置
    QString wechatWorkWebhook;  ///< 企业微信机器人 Webhook
    
    // 邮件配置
    QString smtpHost;
    int smtpPort = 465;
    QString smtpUser;
    QString smtpPassword;
    QString emailFrom;
    QStringList emailTo;
    
    // Webhook 配置
    QString webhookUrl;
    QString webhookToken;
    
    // 钉钉配置
    QString dingTalkWebhook;
    QString dingTalkSecret;
};

/**
 * @brief 智能预警推送服务
 */
class AlertNotificationService : public QObject {
    Q_OBJECT

public:
    static AlertNotificationService* instance();

    /**
     * @brief 配置推送渠道
     */
    void configureChannel(const ChannelConfig& config);
    
    /**
     * @brief 获取渠道配置
     */
    ChannelConfig getChannelConfig(AlertChannel channel) const;

    /**
     * @brief 发送预警消息
     * @param message 预警消息
     * @param channels 指定渠道（为空则使用所有已启用渠道）
     */
    void sendAlert(const AlertMessage& message, 
                   const QList<AlertChannel>& channels = {});

    /**
     * @brief 批量发送预警
     */
    void sendAlerts(const QList<AlertMessage>& messages);

    /**
     * @brief 测试渠道连接
     */
    bool testChannel(AlertChannel channel);

signals:
    /**
     * @brief 发送成功信号
     */
    void alertSent(const QString& messageId, AlertChannel channel);
    
    /**
     * @brief 发送失败信号
     */
    void alertFailed(const QString& messageId, AlertChannel channel, const QString& error);

private:
    explicit AlertNotificationService(QObject* parent = nullptr);
    ~AlertNotificationService() override;
    
    // 各渠道发送方法
    bool sendViaWeChat(const AlertMessage& message, const ChannelConfig& config);
    bool sendViaWeChatWork(const AlertMessage& message, const ChannelConfig& config);
    bool sendViaEmail(const AlertMessage& message, const ChannelConfig& config);
    bool sendViaSystemNotify(const AlertMessage& message);
    bool sendViaWebhook(const AlertMessage& message, const ChannelConfig& config);
    bool sendViaDingTalk(const AlertMessage& message, const ChannelConfig& config);
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot

#endif // ALERTNOTIFICATIONSERVICE_H
