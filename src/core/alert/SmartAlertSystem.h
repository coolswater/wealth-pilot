/**
 * @file SmartAlertSystem.h
 * @brief 智能预警系统 - 自定义条件预警
 *
 * @details 提供智能预警功能：
 * - 价格突破预警
 * - 均线金叉/死叉预警
 * - 成交量异动预警
 * - RSI超买超卖预警
 * - 多种推送方式（桌面/邮件/Webhook）
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef SMARTALERTSYSTEM_H
#define SMARTALERTSYSTEM_H

#include <QObject>
#include <QTimer>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <functional>

/**
 * @brief 预警类型
 */
enum class SmartAlertType
{
    PriceBreakUp,       ///< 价格突破上限
    PriceBreakDown,     ///< 价格突破下限
    MaGoldenCross,      ///< 均线金叉
    MaDeathCross,       ///< 均线死叉
    VolumeSpike,        ///< 成交量异动
    RsiOverbought,      ///< RSI超买
    RsiOversold,        ///< RSI超卖
    Custom              ///< 自定义条件
};

/**
 * @brief 推送方式
 */
enum class PushMethod {
    Desktop = 1,        ///< 桌面弹窗
    Email = 2,          ///< 邮件
    Webhook = 4,        ///< Webhook（钉钉/微信）
    All = 7             ///< 全部
};

Q_DECLARE_FLAGS(PushMethods, PushMethod)
Q_DECLARE_OPERATORS_FOR_FLAGS(PushMethods)

/**
 * @brief 预警条件
 */
struct AlertCondition {
    QString id;                     ///< 条件ID
    QString symbol;                 ///< 股票代码
    SmartAlertType type; ///< 预警类型
    double threshold = 0.0;         ///< 阈值
    QString groupName;              ///< 分组名称
    bool enabled = true;            ///< 是否启用
    QDateTime createTime;           ///< 创建时间
    QDateTime lastTriggerTime;      ///< 最后触发时间
    int triggerCount = 0;           ///< 触发次数
    PushMethods pushMethods = PushMethod::Desktop; ///< 推送方式

    // 条件参数
    QVariantMap params;             ///< 额外参数
};

/**
 * @brief 预警触发记录
 */
struct AlertTrigger {
    QString id;                     ///< 记录ID
    QString conditionId;            ///< 条件ID
    QString symbol;                 ///< 股票代码
    SmartAlertType type; ///< 预警类型
    double triggerValue = 0.0;      ///< 触发值
    double threshold = 0.0;         ///< 阈值
    QDateTime triggerTime;          ///< 触发时间
    QString message;                ///< 预警消息
    bool acknowledged = false;      ///< 是否已确认
};

/**
 * @brief Webhook配置
 */
struct WebhookConfig {
    QString name;                   ///< 名称
    QString url;                    ///< URL
    QString method = "POST";        ///< HTTP方法
    QString contentType = "application/json"; ///< 内容类型
    QString template_;              ///< 消息模板
    QMap<QString, QString> headers; ///< 自定义请求头
};

/**
 * @brief 智能预警系统
 */
class SmartAlertSystem : public QObject
{
    Q_OBJECT

public:
    static SmartAlertSystem* instance();

    /**
     * @brief 初始化系统
     */
    bool initialize();

    /**
     * @brief 添加预警条件
     */
    QString addAlertCondition(const AlertCondition& condition);

    /**
     * @brief 更新预警条件
     */
    void updateAlertCondition(const QString& conditionId, const AlertCondition& condition);

    /**
     * @brief 删除预警条件
     */
    void removeAlertCondition(const QString& conditionId);

    /**
     * @brief 获取预警条件
     */
    QVector<AlertCondition> getAlertConditions(const QString& symbol = QString()) const;

    /**
     * @brief 启用/禁用预警条件
     */
    void setAlertEnabled(const QString& conditionId, bool enabled);

    /**
     * @brief 设置Webhook配置
     */
    void setWebhookConfig(const QString& name, const WebhookConfig& config);

    /**
     * @brief 获取Webhook配置
     */
    WebhookConfig getWebhookConfig(const QString& name) const;

    /**
     * @brief 设置邮件配置
     */
    void setEmailConfig(const QString& smtpServer, int port,
                       const QString& username, const QString& password);

    /**
     * @brief 获取预警记录
     */
    QVector<AlertTrigger> getAlertTriggers(const QString& symbol = QString()) const;

    /**
     * @brief 确认预警
     */
    void acknowledgeAlert(const QString& triggerId);

    /**
     * @brief 更新行情数据
     */
    void updateMarketData(const QString& symbol, double price, qint64 volume);

signals:
    /**
     * @brief 预警触发信号
     */
    void alertTriggered(const AlertTrigger& trigger);

    /**
     * @brief 预警条件变化信号
     */
    void alertConditionsChanged();

private slots:
    void onPeriodicCheck();

private:
    explicit SmartAlertSystem(QObject* parent = nullptr);
    ~SmartAlertSystem() override;

    // 预警检查
    void checkPriceAlert(const QString& symbol, double price);
    void checkMaAlert(const QString& symbol);
    void checkVolumeAlert(const QString& symbol, qint64 volume);
    void checkRsiAlert(const QString& symbol);

    // 推送通知
    void pushDesktopNotification(const AlertTrigger& trigger);
    void pushEmailNotification(const AlertTrigger& trigger);
    void pushWebhookNotification(const AlertTrigger& trigger);

    // 辅助方法
    QString generateConditionId() const;
    QString alertTypeToString(SmartAlertType type) const;
    QString generateAlertMessage(const AlertTrigger& trigger) const;

    // 数据成员
    QMap<QString, AlertCondition> m_conditions;     // 条件ID -> 条件
    QMap<QString, QVector<AlertTrigger>> m_triggers; // 股票代码 -> 触发记录
    QMap<QString, WebhookConfig> m_webhooks;        // Webhook名称 -> 配置

    // 行情数据缓存
    QMap<QString, double> m_latestPrices;
    QMap<QString, qint64> m_latestVolumes;
    QMap<QString, QVector<double>> m_priceHistory;

    // 邮件配置
    QString m_smtpServer;
    int m_smtpPort = 587;
    QString m_emailUsername;
    QString m_emailPassword;

    QTimer* m_checkTimer = nullptr;
    bool m_initialized = false;
};

#endif // SMARTALERTSYSTEM_H
