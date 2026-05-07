/**
 * @file SmartAlertSystem.cpp
 * @brief 智能预警系统实现
 */

#include "SmartAlertSystem.h"
#include "utils/Logger.h"
#include <QUuid>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QApplication>
#include <QMessageBox>

SmartAlertSystem* SmartAlertSystem::instance()
{
    static SmartAlertSystem* inst = new SmartAlertSystem();
    return inst;
}

SmartAlertSystem::SmartAlertSystem(QObject* parent)
    : QObject(parent)
    , m_checkTimer(new QTimer(this))
{
    connect(m_checkTimer, &QTimer::timeout, this, &SmartAlertSystem::onPeriodicCheck);
}

SmartAlertSystem::~SmartAlertSystem()
{
    m_checkTimer->stop();
}

bool SmartAlertSystem::initialize()
{
    if (m_initialized) return true;

    LOG_INFO("Initializing Smart Alert System");

    // 启动定期检查（每10秒）
    m_checkTimer->start(10000);

    m_initialized = true;
    LOG_INFO("Smart Alert System initialized");
    return true;
}

QString SmartAlertSystem::addAlertCondition(const AlertCondition& condition)
{
    AlertCondition newCondition = condition;
    newCondition.id = generateConditionId();
    newCondition.createTime = QDateTime::currentDateTime();

    m_conditions[newCondition.id] = newCondition;

    LOG_INFO(QString("Alert condition added: %1 - %2")
        .arg(newCondition.symbol, alertTypeToString(newCondition.type)));

    emit alertConditionsChanged();
    return newCondition.id;
}

void SmartAlertSystem::updateAlertCondition(const QString& conditionId, const AlertCondition& condition)
{
    if (m_conditions.contains(conditionId)) {
        m_conditions[conditionId] = condition;
        emit alertConditionsChanged();
    }
}

void SmartAlertSystem::removeAlertCondition(const QString& conditionId)
{
    if (m_conditions.remove(conditionId) > 0) {
        LOG_INFO(QString("Alert condition removed: %1").arg(conditionId));
        emit alertConditionsChanged();
    }
}

QVector<AlertCondition> SmartAlertSystem::getAlertConditions(const QString& symbol) const
{
    QVector<AlertCondition> result;

    for (const auto& condition : m_conditions) {
        if (symbol.isEmpty() || condition.symbol == symbol) {
            result.append(condition);
        }
    }

    return result;
}

void SmartAlertSystem::setAlertEnabled(const QString& conditionId, bool enabled)
{
    if (m_conditions.contains(conditionId)) {
        m_conditions[conditionId].enabled = enabled;
        emit alertConditionsChanged();
    }
}

void SmartAlertSystem::setWebhookConfig(const QString& name, const WebhookConfig& config)
{
    m_webhooks[name] = config;
    LOG_INFO(QString("Webhook config set: %1").arg(name));
}

WebhookConfig SmartAlertSystem::getWebhookConfig(const QString& name) const
{
    return m_webhooks.value(name);
}

void SmartAlertSystem::setEmailConfig(const QString& smtpServer, int port,
                                      const QString& username, const QString& password)
{
    m_smtpServer = smtpServer;
    m_smtpPort = port;
    m_emailUsername = username;
    m_emailPassword = password;

    LOG_INFO(QString("Email config set: %1:%2").arg(smtpServer).arg(port));
}

QVector<AlertTrigger> SmartAlertSystem::getAlertTriggers(const QString& symbol) const
{
    if (symbol.isEmpty()) {
        QVector<AlertTrigger> all;
        for (const auto& triggers : m_triggers) {
            all.append(triggers);
        }
        return all;
    }
    return m_triggers.value(symbol);
}

void SmartAlertSystem::acknowledgeAlert(const QString& triggerId)
{
    for (auto& triggers : m_triggers) {
        for (auto& trigger : triggers) {
            if (trigger.id == triggerId) {
                trigger.acknowledged = true;
                return;
            }
        }
    }
}

void SmartAlertSystem::updateMarketData(const QString& symbol, double price, qint64 volume)
{
    m_latestPrices[symbol] = price;
    m_latestVolumes[symbol] = volume;

    // 更新价格历史
    if (!m_priceHistory.contains(symbol)) {
        m_priceHistory[symbol] = QVector<double>();
    }
    m_priceHistory[symbol].append(price);

    // 保留最近100个价格
    if (m_priceHistory[symbol].size() > 100) {
        m_priceHistory[symbol].removeFirst();
    }

    // 检查预警条件
    checkPriceAlert(symbol, price);
    checkVolumeAlert(symbol, volume);
}

void SmartAlertSystem::onPeriodicCheck()
{
    // 定期检查所有预警条件
    for (const QString& symbol : m_latestPrices.keys()) {
        checkMaAlert(symbol);
        checkRsiAlert(symbol);
    }
}

void SmartAlertSystem::checkPriceAlert(const QString& symbol, double price)
{
    for (auto& condition : m_conditions) {
        if (!condition.enabled || condition.symbol != symbol) {
            continue;
        }

        bool triggered = false;

        if (condition.type == AlertType::PriceBreakUp) {
            triggered = price >= condition.threshold;
        } else if (condition.type == AlertType::PriceBreakDown) {
            triggered = price <= condition.threshold;
        }

        if (triggered) {
            AlertTrigger trigger;
            trigger.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            trigger.conditionId = condition.id;
            trigger.symbol = symbol;
            trigger.type = condition.type;
            trigger.triggerValue = price;
            trigger.threshold = condition.threshold;
            trigger.triggerTime = QDateTime::currentDateTime();
            trigger.message = generateAlertMessage(trigger);

            m_triggers[symbol].append(trigger);
            condition.lastTriggerTime = trigger.triggerTime;
            condition.triggerCount++;

            emit alertTriggered(trigger);

            // 推送通知
            if (condition.pushMethods & PushMethod::Desktop) {
                pushDesktopNotification(trigger);
            }
            if (condition.pushMethods & PushMethod::Webhook) {
                pushWebhookNotification(trigger);
            }
            if (condition.pushMethods & PushMethod::Email) {
                pushEmailNotification(trigger);
            }
        }
    }
}

void SmartAlertSystem::checkMaAlert(const QString& symbol)
{
    if (!m_priceHistory.contains(symbol) || m_priceHistory[symbol].size() < 20) {
        return;
    }

    const auto& prices = m_priceHistory[symbol];
    int n = prices.size();

    // 计算MA5和MA10
    double ma5 = 0.0, ma10 = 0.0;
    for (int i = n - 5; i < n; ++i) {
        ma5 += prices[i];
    }
    ma5 /= 5;

    for (int i = n - 10; i < n; ++i) {
        ma10 += prices[i];
    }
    ma10 /= 10;

    // 检查金叉/死叉
    for (auto& condition : m_conditions) {
        if (!condition.enabled || condition.symbol != symbol) {
            continue;
        }

        if (condition.type == AlertType::MaGoldenCross) {
            // 简化的金叉判断：MA5 > MA10
            if (ma5 > ma10) {
                AlertTrigger trigger;
                trigger.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
                trigger.conditionId = condition.id;
                trigger.symbol = symbol;
                trigger.type = AlertType::MaGoldenCross;
                trigger.triggerValue = ma5;
                trigger.threshold = ma10;
                trigger.triggerTime = QDateTime::currentDateTime();
                trigger.message = generateAlertMessage(trigger);

                m_triggers[symbol].append(trigger);
                condition.lastTriggerTime = trigger.triggerTime;
                condition.triggerCount++;

                emit alertTriggered(trigger);
                pushDesktopNotification(trigger);
            }
        } else if (condition.type == AlertType::MaDeathCross) {
            // 死叉判断：MA5 < MA10
            if (ma5 < ma10) {
                AlertTrigger trigger;
                trigger.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
                trigger.conditionId = condition.id;
                trigger.symbol = symbol;
                trigger.type = AlertType::MaDeathCross;
                trigger.triggerValue = ma5;
                trigger.threshold = ma10;
                trigger.triggerTime = QDateTime::currentDateTime();
                trigger.message = generateAlertMessage(trigger);

                m_triggers[symbol].append(trigger);
                condition.lastTriggerTime = trigger.triggerTime;
                condition.triggerCount++;

                emit alertTriggered(trigger);
                pushDesktopNotification(trigger);
            }
        }
    }
}

void SmartAlertSystem::checkVolumeAlert(const QString& symbol, qint64 volume)
{
    for (auto& condition : m_conditions) {
        if (!condition.enabled || condition.symbol != symbol) {
            continue;
        }

        if (condition.type == AlertType::VolumeSpike) {
            // 成交量异动：超过阈值倍数
            qint64 avgVolume = condition.params[QStringLiteral("avgVolume")].toLongLong();
            double multiplier = condition.params[QStringLiteral("multiplier")].toDouble(3.0);

            if (avgVolume > 0 && volume > avgVolume * multiplier) {
                AlertTrigger trigger;
                trigger.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
                trigger.conditionId = condition.id;
                trigger.symbol = symbol;
                trigger.type = AlertType::VolumeSpike;
                trigger.triggerValue = volume;
                trigger.threshold = avgVolume * multiplier;
                trigger.triggerTime = QDateTime::currentDateTime();
                trigger.message = generateAlertMessage(trigger);

                m_triggers[symbol].append(trigger);
                condition.lastTriggerTime = trigger.triggerTime;
                condition.triggerCount++;

                emit alertTriggered(trigger);
                pushDesktopNotification(trigger);
            }
        }
    }
}

void SmartAlertSystem::checkRsiAlert(const QString& symbol)
{
    // TODO: 实现RSI计算和预警
    Q_UNUSED(symbol);
}

void SmartAlertSystem::pushDesktopNotification(const AlertTrigger& trigger)
{
    // 桌面弹窗通知
    QMessageBox::information(nullptr,
        QStringLiteral("预警提醒"),
        trigger.message);
}

void SmartAlertSystem::pushEmailNotification(const AlertTrigger& trigger)
{
    // TODO: 实现邮件发送
    LOG_INFO(QString("Email notification: %1").arg(trigger.message));
}

void SmartAlertSystem::pushWebhookNotification(const AlertTrigger& trigger)
{
    // 查找对应的Webhook配置
    QString webhookName = QStringLiteral("default");
    if (!m_webhooks.contains(webhookName)) {
        return;
    }

    const WebhookConfig& config = m_webhooks[webhookName];

    // 构建JSON消息
    QJsonObject json;
    json[QStringLiteral("symbol")] = trigger.symbol;
    json[QStringLiteral("type")] = alertTypeToString(trigger.type);
    json[QStringLiteral("value")] = trigger.triggerValue;
    json[QStringLiteral("threshold")] = trigger.threshold;
    json[QStringLiteral("time")] = trigger.triggerTime.toString(Qt::ISODate);
    json[QStringLiteral("message")] = trigger.message;

    // 发送HTTP请求
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request(QUrl(config.url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, config.contentType);

    for (auto it = config.headers.begin(); it != config.headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply* reply = manager->post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, [reply, manager]() {
        reply->deleteLater();
        manager->deleteLater();
    });

    LOG_INFO(QString("Webhook notification sent: %1").arg(config.url));
}

QString SmartAlertSystem::generateConditionId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString SmartAlertSystem::alertTypeToString(AlertType type) const
{
    switch (type) {
    case AlertType::PriceBreakUp: return QStringLiteral("价格突破上限");
    case AlertType::PriceBreakDown: return QStringLiteral("价格突破下限");
    case AlertType::MaGoldenCross: return QStringLiteral("均线金叉");
    case AlertType::MaDeathCross: return QStringLiteral("均线死叉");
    case AlertType::VolumeSpike: return QStringLiteral("成交量异动");
    case AlertType::RsiOverbought: return QStringLiteral("RSI超买");
    case AlertType::RsiOversold: return QStringLiteral("RSI超卖");
    case AlertType::Custom: return QStringLiteral("自定义条件");
    default: return QStringLiteral("未知");
    }
}

QString SmartAlertSystem::generateAlertMessage(const AlertTrigger& trigger) const
{
    QString typeStr = alertTypeToString(trigger.type);

    return QString(QStringLiteral("【%1】%2 %3，当前值：%4，阈值：%5"))
        .arg(typeStr)
        .arg(trigger.symbol)
        .arg(trigger.triggerValue >= trigger.threshold ? QStringLiteral("触发") : QStringLiteral("触发"))
        .arg(trigger.triggerValue, 0, 'f', 2)
        .arg(trigger.threshold, 0, 'f', 2);
}
